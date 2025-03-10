/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_CONNECTION_H_INCLUDED_
#define _UCT_CONNECTION_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

typedef struct uct_listening_pool_s uct_listening_pool_t;

struct uct_connection_s {
    uct_socket_t fd;
    uct_pool_t *pool;
    char *ip_text;
    char *port_text;
    uct_upstream_srv_t *upstream_srv;
    uct_queue_t queue;
};

struct uct_listening_pool_s {
    uct_connection_t *listen_q;
    uct_cycle_t *cycle;
    // sem_t mutex;
    sem_t items;
    pthread_spinlock_t mutex;
};

struct uct_proxy_s {
    uct_connection_t *client;
    uct_connection_t *upstream;
    uct_queue_t queue;
};

uct_connection_t *uct_connection_init(uct_cycle_t *cycle, uct_socket_t s);
uct_int_t uct_listening_pool_init(uct_cycle_t *cycle,
    uct_listening_pool_t *lpool);
uct_connection_t *uct_listening_pool_add(uct_listening_pool_t *lpool,
    uct_connection_t *conn);
uct_connection_t *uct_listening_pool_get(uct_listening_pool_t *lpool);
uct_int_t uct_epoll_init(uct_cycle_t *wk_cycle);
uct_int_t uct_epoll_add_proxy_connection(uct_cycle_t *wk_cycle, uct_proxy_t *c);
uct_int_t uct_epoll_process_events(uct_cycle_t *wk_cycle);
uct_int_t uct_epoll_init_udp(uct_cycle_t *wk_cycle);
uct_int_t uct_epoll_add_proxy_connection_udp(uct_cycle_t *wk_cycle, uct_proxy_t *c);
uct_int_t uct_epoll_process_events_udp(uct_cycle_t *wk_cycle);
uct_int_t uct_epoll_add(int epoll, uct_fd_t fd, void *c);

#endif /* _UCT_CONNECTION_H_INCLUDED_ */
