#ifndef _UCT_CHECK_H_INCLUDED_
#define _UCT_CHECK_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

#define UCT_CHECK_MODE_TCP 1
#define UCT_CHECK_MODE_UDP 2
#define UCT_CHECK_MODE_HTTP 3
#define UCT_CHECK_MODE_PING 4
#define UCT_CHECK_MODE_TLS 5
#define UCT_CHECK_MODE_SSH 6

#define UCT_CHECK_TCP_CONNECTED 0
#define UCT_CHECK_TCP_CONNECTING -1
#define UCT_CHECK_TCP_DISCONNECTED -2

struct uct_check_data {
    uct_upstream_srv_t *srv;
    uct_uint_t fd;
    uct_uint_t start_time;
};

void * uct_checker_thread_cycle(void *arg);

#endif /* _UCT_CHECK_H_INCLUDED_ */