/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_CYCLE_H_INCLUDED_
#define _UCT_CYCLE_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

#define UCT_CYCLE_POOL_SIZE UCT_DEFAULT_POOL_SIZE

#define uct_pthread_create  pthread_create
#define uct_pthread_detach  pthread_detach
#define uct_pthread_self    pthread_self

#define UCT_TCP_MODE        0
#define UCT_UDP_MODE        1
#define UCT_HTTP_MODE       2

struct uct_cycle_s {
    uct_cycle_t *master;
    uct_pool_t *pool;
    uct_log_t *log;
    uct_uint_t workers_n;
    char *conf_file;
    uct_uint_t mode;
    uct_uint_t localport;
    uct_socket_t listenfd;
    uct_listening_pool_t *lpool;
    uct_uint_t connection_n;
    uct_connection_t *connections;
    uct_proxy_t *proxy_conn;
    uct_uint_t policy;
    u_char *log_file;
    uct_array_t *srvs;
    uct_uint_t srvs_n;
    int client_epoll;
    struct epoll_event *client_events;
    struct epoll_event *upstream_events;
    int upstream_epoll;
    sem_t mutex;
};

struct uct_thread_args_s {
    uct_cycle_t *cycle;
    uct_uint_t id;
};

uct_cycle_t *uct_init_cycle(uct_log_t *log);
void uct_master_thread_cycle(uct_cycle_t *cycle);
void uct_master_thread_cycle_udp(uct_cycle_t *cycle);

#endif /* _UCT_CYCLE_H_INCLUDED_ */
