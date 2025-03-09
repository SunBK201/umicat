/*
 * Copyright (C) SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

static uct_int_t
init_srv_timer(uct_upstream_srv_t *srv, uct_cycle_t *cycle) {
    struct itimerspec its;

    srv->timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (srv->timerfd < 0) {
        uct_log(cycle->log, UCT_LOG_ERROR, "timerfd_create error: %s",
            strerror(errno));
        return UCT_ERROR;
    }
    its.it_value.tv_sec = srv->check_interval;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = srv->check_interval;
    its.it_interval.tv_nsec = 0;
    if (timerfd_settime(srv->timerfd, 0, &its, NULL) < 0) {
        uct_log(cycle->log, UCT_LOG_ERROR, "timerfd_settime error: %s",
            strerror(errno));
        return UCT_ERROR;
    }
    return UCT_OK;
}

static int 
uct_connect_nonblocking(uct_socket_t sockfd, struct sockaddr *serv_addr,
    int addrlen, uct_cycle_t *cycle)
{
    int rc;
    uct_nonblocking(sockfd);
    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0) {
        if (errno == EINPROGRESS)
            return UCT_CHECK_TCP_CONNECTING;
        uct_log(cycle->log, UCT_LOG_ERROR, "Check Connect error: %s",
            strerror(errno));
        return UCT_CHECK_TCP_DISCONNECTED;
    }
    return UCT_CHECK_TCP_CONNECTED;
}

static uct_int_t
uct_check_tcp(char *hostname, uct_uint_t port, uct_cycle_t *cycle) {
    int clientfd;
    struct addrinfo hints, *listp, *p;
    uct_int_t rc;

    char *port_str = uct_pnalloc(cycle->pool, UCT_INET_PORTSTRLEN);
    uct_itoa(port, port_str, 10);
    /* Get a list of potential server addresses */
    uct_memzero(&hints, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV; /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG; /* Recommended for connections */
    uct_getaddrinfo(hostname, port_str, &hints, &listp, cycle);

    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((clientfd = uct_socket(p->ai_family, p->ai_socktype, p->ai_protocol,
                 cycle)) < 0)
            continue; /* Socket failed, try the next */
        rc = uct_connect_nonblocking(clientfd, p->ai_addr, p->ai_addrlen, cycle);
        switch (rc)
        {
        case UCT_CHECK_TCP_CONNECTING:
            return clientfd;
        case UCT_CHECK_TCP_CONNECTED:
            uct_close_socket(clientfd);
            return UCT_CHECK_TCP_CONNECTED;
        case UCT_CHECK_TCP_DISCONNECTED:
            uct_close_socket(clientfd);
            return UCT_CHECK_TCP_DISCONNECTED;
        }
    }
    return UCT_CHECK_TCP_DISCONNECTED;
}

static void
uct_check_tcp_cycle(uct_cycle_t *cycle) {
    int rc;
    uct_upstream_srv_t *srv;
    uct_int_t check_poll;
    struct epoll_event timer_ev;
    struct epoll_event ev;
    struct epoll_event *events;
    struct uct_check_data *check_data;
    uint64_t expirations;

    check_poll = epoll_create(cycle->srvs_n << 1);
    if (check_poll < 0) {
        uct_log(cycle->log, UCT_LOG_ERROR, "check epoll create error: %s",
            strerror(errno));
        return;
    }
    
    for (uct_uint_t i = 0; i < cycle->srvs_n; i++) {
        srv = (uct_upstream_srv_t *)uct_array_loc(cycle->srvs, i);
        srv->check_fails = 0;
        srv->check_successes = 0;
        rc = init_srv_timer(srv, cycle);
        if (rc != UCT_OK) {
            uct_log(cycle->log, UCT_LOG_ERROR, "init_timer error");
            return;
        }
        check_data = malloc(sizeof(struct uct_check_data));
        timer_ev.events = EPOLLIN;
        check_data->srv = srv;
        check_data->fd = srv->timerfd;
        timer_ev.data.ptr = check_data;
        if (epoll_ctl(check_poll, EPOLL_CTL_ADD, srv->timerfd, &timer_ev) < 0) {
            uct_log(cycle->log, UCT_LOG_ERROR, "check epoll ctl error: %s",
                strerror(errno));
            return;
        }
    }

    uct_log(cycle->log, UCT_LOG_INFO, "health check start");
    events = uct_palloc(cycle->pool, sizeof(struct epoll_event) * cycle->srvs_n * 2);
    while (1) {
        rc = epoll_wait(check_poll, events, cycle->srvs_n << 1, -1);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            uct_log(cycle->log, UCT_LOG_ERROR, "check epoll wait error: %s",
                strerror(errno));
            return;
        }
        uct_log(cycle->log, UCT_LOG_DEBUG, "check epoll events: %d", rc);
        for (uct_uint_t i = 0; i < rc; i++) {
            check_data = (struct uct_check_data *)events[i].data.ptr;
            srv = check_data->srv;
            if (check_data->srv->timerfd == check_data->fd) {
                // check timer event
                int readn = read(check_data->fd, &expirations, sizeof(expirations));
                if (readn < 0) {
                    uct_log(cycle->log, UCT_LOG_ERROR, "read timerfd error: %s",
                        strerror(errno));
                    return;
                }
                uct_log(cycle->log, UCT_LOG_DEBUG, "check tcp server: %s:%d",
                    srv->upstream_ip, srv->upstream_port);
                int ret = uct_check_tcp(srv->upstream_ip, srv->upstream_port, cycle);
                if (ret == UCT_CHECK_TCP_CONNECTED) {
                    srv->check_successes++;
                    if (srv->is_down && srv->check_successes >= srv->check_rise) {
                        pthread_spin_lock(&srv->lock);
                        uct_log(cycle->log, UCT_LOG_WARN, "server %s:%d is up",
                            srv->upstream_ip, srv->upstream_port);
                        srv->effective_weight += 1;
                        srv->is_down = false;
                        srv->fails = 0;
                        pthread_spin_unlock(&srv->lock);
                        srv->check_successes = 0;
                        srv->check_fails = 0;
                    }
                } else if (ret == UCT_CHECK_TCP_DISCONNECTED) {
                    srv->check_fails++;
                    if (!srv->is_down && srv->check_fails >= srv->check_fall) {
                        pthread_spin_lock(&srv->lock);
                        uct_log(cycle->log, UCT_LOG_WARN, "server %s:%d is down",
                            srv->upstream_ip, srv->upstream_port);
                        srv->effective_weight = 0;
                        srv->is_down = true;
                        srv->fails = 1;
                        pthread_spin_unlock(&srv->lock);
                        srv->check_fails = 0;
                        srv->check_successes = 0;
                    }
                } else if (ret > 0) {
                    // put into epoll
                    ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLONESHOT | EPOLLET;
                    check_data = malloc(sizeof(struct uct_check_data));
                    check_data->fd = ret;
                    check_data->srv = srv;
                    check_data->start_time = time(NULL);
                    ev.data.ptr = check_data;
                    if (epoll_ctl(check_poll, EPOLL_CTL_ADD, ret, &ev) < 0) {
                        uct_log(cycle->log, UCT_LOG_ERROR, "check epoll ctl error: %s",
                            strerror(errno));
                        return;
                    }
                }
            } else {
                // check connect event
                time_t now = time(NULL);
                time_t start = check_data->start_time;
                int err = 0;
                socklen_t len = sizeof(err);
                if (getsockopt(check_data->fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
                    uct_log(cycle->log, UCT_LOG_ERROR, "getsockopt error: %s",
                        strerror(errno));
                    err = errno;
                }
                // epoll_ctl(check_poll, EPOLL_CTL_DEL, check_data->fd, NULL);
                uct_close_socket(check_data->fd);
                free(check_data);
                if (err == 0 && now - start < srv->check_timeout) {
                    srv->check_successes++;
                    if (srv->is_down && srv->check_successes >= srv->check_rise) {
                        pthread_spin_lock(&srv->lock);
                        uct_log(cycle->log, UCT_LOG_WARN, "server %s:%d is up",
                            srv->upstream_ip, srv->upstream_port);
                        srv->effective_weight += 1;
                        srv->is_down = false;
                        srv->fails = 0;
                        pthread_spin_unlock(&srv->lock);
                        srv->check_successes = 0;
                        srv->check_fails = 0;
                    }
                } else {
                    srv->check_fails++;
                    uct_log(cycle->log, UCT_LOG_DEBUG, "check tcp fail: %s:%d",
                        srv->upstream_ip, srv->upstream_port);
                    if (!srv->is_down && srv->check_fails >= srv->check_fall) {
                        pthread_spin_lock(&srv->lock);
                        uct_log(cycle->log, UCT_LOG_WARN, "server %s:%d is down",
                            srv->upstream_ip, srv->upstream_port);
                        srv->effective_weight = 0;
                        srv->is_down = true;
                        srv->fails = 1;
                        pthread_spin_unlock(&srv->lock);
                        srv->check_fails = 0;
                        srv->check_successes = 0;
                    }
                }
            }
        }
    }

}

void *
uct_checker_thread_cycle(void *arg) {
    struct uct_thread_args_s *args;
    uct_cycle_t *cycle;

    args = (struct uct_thread_args_s *)arg;
    cycle = args->cycle;
    uct_check_tcp_cycle(cycle);
    return NULL;
}