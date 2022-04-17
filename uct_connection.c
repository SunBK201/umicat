/*
 * Copyright (C) 2021 SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

extern uct_array_t *srvs;
extern uct_uint_t srvs_n;

uct_connection_t *
uct_connection_init(uct_cycle_t *cycle, uct_socket_t s)
{
    uct_connection_t *conn;
    uct_pool_t *pool;

    pool = uct_create_pool(UCT_DEFAULT_POOL_SIZE, cycle->log);
    if (pool == NULL) {
        return NULL;
    }
    conn = uct_pnalloc(pool, sizeof(uct_connection_t));
    if (conn == NULL) {
        return NULL;
    }
    conn->fd = s;
    conn->pool = pool;
    conn->ip_text = uct_pnalloc(pool, UCT_INET_ADDRSTRLEN * 4);
    conn->port_text = uct_pnalloc(pool, UCT_INET_PORTSTRLEN);

    return conn;
}

uct_int_t
uct_listening_pool_init(uct_cycle_t *cycle, uct_listening_pool_t *lpool)
{
    uct_connection_t *listen;
    uct_int_t n;

    // n = uct_lock_init(&lpool->mutex, 0, 1, cycle->log);
    // if (n != UCT_OK) {
    //     return UCT_ERROR;
    // }
    n = uct_lock_init(&lpool->items, 0, 0, cycle->log);
    if (n != UCT_OK) {
        return UCT_ERROR;
    }
    pthread_spin_init(&lpool->mutex, 1);

    listen = uct_palloc(cycle->pool, sizeof(uct_connection_t));
    uct_queue_init(&listen->queue);
    lpool->listen_q = listen;

    lpool->cycle = cycle;
    cycle->lpool = lpool;

    return UCT_OK;
}

uct_connection_t *
uct_listening_pool_add(uct_listening_pool_t *lpool, uct_connection_t *conn)
{
    uct_int_t n;

    // n = uct_lock_p(&lpool->mutex, lpool->cycle->log);
    n = pthread_spin_lock(&lpool->mutex);
    if (n != UCT_OK) {
        return NULL;
    }

    uct_queue_insert_head(&lpool->listen_q->queue, &conn->queue);

#ifdef UCT_DEBUG
    tp = uct_queue_data(uct_queue_last(&lpool->listen_q->queue),
        uct_connection_t, queue);
    uct_log(lpool->cycle->log, UCT_LOG_DEBUG, "connfd: %d", conn->fd);
#endif

    // uct_lock_v(&lpool->mutex, lpool->cycle->log);
    n = pthread_spin_unlock(&lpool->mutex);
    uct_lock_v(&lpool->items, lpool->cycle->log);

    return UCT_OK;
}

uct_connection_t *
uct_listening_pool_get(uct_listening_pool_t *lpool)
{
    uct_queue_t *q;
    uct_connection_t *conn;
    uct_int_t n;

    n = uct_trylock_p(&lpool->items, lpool->cycle->log);
    if (n != UCT_OK) {
        return NULL;
    }
    // n = uct_trylock_p(&lpool->mutex, lpool->cycle->log);
    n = pthread_spin_lock(&lpool->mutex);
    if (n != UCT_OK) {
        uct_lock_v(&lpool->items, lpool->cycle->log);
        return NULL;
    }

    q = uct_queue_last(&lpool->listen_q->queue);
    uct_queue_remove(q);

    // uct_lock_v(&lpool->mutex, lpool->cycle->log);
    pthread_spin_unlock(&lpool->mutex);

    conn = uct_queue_data(q, uct_connection_t, queue);

    return conn;
}

uct_int_t
uct_epoll_init(uct_cycle_t *wk_cycle)
{
    wk_cycle->client_epoll = epoll_create(1024);
    wk_cycle->upstream_epoll = epoll_create(1024);

    wk_cycle->client_events =
        uct_palloc(wk_cycle->pool, sizeof(struct epoll_event) * 1024);
    wk_cycle->upstream_events =
        uct_palloc(wk_cycle->pool, sizeof(struct epoll_event) * 1024);
    if (wk_cycle->client_events == NULL || wk_cycle->upstream_events == NULL) {
        return UCT_ERROR;
    }
    return UCT_OK;
}

uct_int_t
uct_epoll_add_proxy_connection(uct_cycle_t *wk_cycle, uct_proxy_t *c)
{
    struct epoll_event ev;

    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = c;

    if (epoll_ctl(wk_cycle->client_epoll, EPOLL_CTL_ADD, c->client->fd, &ev) ==
        -1) {
        uct_log(wk_cycle->log, UCT_LOG_ERROR,
            "epoll_ctl(EPOLL_CTL_ADD, %d) failed", c->client->fd);
        return UCT_ERROR;
    }
    if (epoll_ctl(wk_cycle->upstream_epoll, EPOLL_CTL_ADD, c->upstream->fd,
            &ev) == -1) {
        uct_log(wk_cycle->log, UCT_LOG_ERROR,
            "epoll_ctl(EPOLL_CTL_ADD, %d) failed", c->client->fd);
        return UCT_ERROR;
    }
    return UCT_OK;
}

uct_int_t
uct_epoll_process_events(uct_cycle_t *wk_cycle)
{
    int events;
    uct_int_t i;
    uct_proxy_t *p;
    uct_connection_t *client_conn;
    uct_connection_t *upstream_conn;
    struct epoll_event *client_events;
    struct epoll_event *upstream_events;
    void *buf;
    uct_uint_t n;
    uct_uint_t timeout;

    timeout = 1;
    client_events = wk_cycle->client_events;
    upstream_events = wk_cycle->upstream_events;

    events = epoll_wait(wk_cycle->client_epoll, client_events, 1024, timeout);
    for (i = 0; i < events; i++) {
        p = client_events[i].data.ptr;
        client_conn = p->client;
        upstream_conn = p->upstream;
        buf = uct_pnalloc(client_conn->pool, UCT_BUF_SIZE);

        while ((n = uct_readn(client_conn->fd, buf, UCT_BUF_SIZE)) > 0) {
            uct_writen(upstream_conn->fd, buf, n);
            uct_log(wk_cycle->log, UCT_LOG_DEBUG,
                "Proxy: %s:%s -> %s:%s, %lu Bytes", client_conn->ip_text,
                client_conn->port_text, upstream_conn->ip_text,
                upstream_conn->port_text, n);
        }
        if (n == 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                uct_log(wk_cycle->log, UCT_LOG_INFO,
                    "Disconnected by client: %s:%s <-> %s:%s",
                    client_conn->ip_text, client_conn->port_text,
                    upstream_conn->ip_text, upstream_conn->port_text);
                uct_close_socket(client_conn->fd);
                uct_close_socket(upstream_conn->fd);
                uct_lock_p(&wk_cycle->master->mutex, wk_cycle->log);
                upstream_conn->upstream_srv->connection_n--;
                uct_lock_v(&wk_cycle->master->mutex, wk_cycle->log);
                epoll_ctl(wk_cycle->client_epoll, EPOLL_CTL_DEL,
                    client_conn->fd, NULL);
                epoll_ctl(wk_cycle->upstream_epoll, EPOLL_CTL_DEL,
                    upstream_conn->fd, NULL);
                uct_destroy_pool(client_conn->pool);
                uct_destroy_pool(upstream_conn->pool);
                wk_cycle->connection_n--;
            }
        }
    }

    events =
        epoll_wait(wk_cycle->upstream_epoll, upstream_events, 1024, timeout);
    for (i = 0; i < events; i++) {
        p = upstream_events[i].data.ptr;
        upstream_conn = p->upstream;
        client_conn = p->client;
        buf = uct_pnalloc(upstream_conn->pool, UCT_BUF_SIZE);

        while ((n = uct_readn(upstream_conn->fd, buf, UCT_BUF_SIZE)) > 0) {
            uct_writen(client_conn->fd, buf, n);
            uct_log(wk_cycle->log, UCT_LOG_DEBUG,
                "Proxy: %s:%s -> %s:%s, %lu Bytes", upstream_conn->ip_text,
                upstream_conn->port_text, client_conn->ip_text,
                client_conn->port_text, n);
        }
        if (n == 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                uct_log(wk_cycle->log, UCT_LOG_INFO,
                    "Disconnected by upstream: %s:%s <-> %s:%s",
                    client_conn->ip_text, client_conn->port_text,
                    upstream_conn->ip_text, upstream_conn->port_text);
                uct_close_socket(upstream_conn->fd);
                uct_close_socket(client_conn->fd);
                uct_lock_p(&wk_cycle->master->mutex, wk_cycle->log);
                upstream_conn->upstream_srv->connection_n--;
                uct_lock_v(&wk_cycle->master->mutex, wk_cycle->log);
                epoll_ctl(wk_cycle->upstream_epoll, EPOLL_CTL_DEL,
                    upstream_conn->fd, NULL);
                epoll_ctl(wk_cycle->client_epoll, EPOLL_CTL_DEL,
                    client_conn->fd, NULL);
                uct_destroy_pool(upstream_conn->pool);
                uct_destroy_pool(client_conn->pool);
                wk_cycle->connection_n--;
            }
        }
    }

    return UCT_OK;
}