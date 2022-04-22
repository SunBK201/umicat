/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_UPSTREAM_H_INCLUDED_
#define _UCT_UPSTREAM_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

#define UCT_ROUND_ROBIN 0
#define UCT_IP_HASH     1
#define UCT_LEAST_CONN  2

struct uct_upstream_srv_s {
    char *upstream_ip;
    uct_uint_t upstream_port;
    uct_int_t weight;
    uct_int_t current_weight;
    uct_int_t effective_weight;
    uct_uint_t connection_n;
};

uct_connection_t *uct_upstream_get_connetion(uct_cycle_t *wk_cycle,
    uct_connection_t *client);

#endif /* _UCT_UPSTREAM_H_INCLUDED_ */
