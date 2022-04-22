/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_CORE_H_INCLUDED_
#define _UCT_CORE_H_INCLUDED_

#include <uct_config.h>

typedef struct uct_cycle_s uct_cycle_t;
typedef struct uct_pool_s uct_pool_t;
typedef struct uct_chain_s uct_chain_t;
typedef struct uct_queue_s uct_queue_t;
typedef struct uct_file_s uct_file_t;
typedef struct uct_connection_s uct_connection_t;
typedef struct uct_log_s uct_log_t;
typedef struct uct_logev_s uct_logev_t;
typedef struct uct_upstream_srv_s uct_upstream_srv_t;
typedef struct uct_proxy_s uct_proxy_t;


#include <umicat.h>
#include <uct_json.h>
#include <uct_log.h>
#include <uct_string.h>
#include <uct_file.h>
#include <uct_alloc.h>
#include <uct_palloc.h>
#include <uct_buf.h>
#include <uct_array.h>
#include <uct_queue.h>
#include <uct_list.h>
#include <uct_socket.h>
#include <uct_lock.h>
#include <uct_connection.h>
#include <uct_cycle.h>
#include <uct_upstream.h>

// #define UCT_DEBUG

#endif /* _UCT_CORE_H_INCLUDED_ */
