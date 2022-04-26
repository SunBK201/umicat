/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_PALLOC_H_INCLUDED_
#define _UCT_PALLOC_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

#define UCT_MAX_ALLOC_FROM_POOL  (uct_pagesize - 1)

#define UCT_DEFAULT_POOL_SIZE    (16 * 1024)

#define UCT_POOL_ALIGNMENT       16
#define UCT_MIN_POOL_SIZE                                                     \
    uct_align((sizeof(uct_pool_t) + 2 * sizeof(uct_pool_large_t)),            \
              uct_POOL_ALIGNMENT)

typedef void (*uct_pool_cleanup_pt)(void *data);

typedef struct uct_pool_cleanup_s  uct_pool_cleanup_t;

struct uct_pool_cleanup_s {
    uct_pool_cleanup_pt   handler;
    void                 *data;
    uct_pool_cleanup_t   *next;
};

typedef struct uct_pool_large_s  uct_pool_large_t;

struct uct_pool_large_s {
    uct_pool_large_t     *next;
    void                 *alloc;
};

typedef struct {
    u_char               *last;     /* unused memory */
    u_char               *end;
    uct_pool_t           *next;
    uct_uint_t            failed;
} uct_pool_data_t;

struct uct_pool_s {
    uct_pool_data_t       d;        /* data area of uct_pool_t */
    size_t                max;      /* maximum memory available per allocation */
    uct_pool_t           *current;  /* the current memory pool pointer address */
    uct_chain_t          *chain;    /* uct_chain_t */
    uct_pool_large_t     *large;    /* big block memory chain */
    uct_pool_cleanup_t   *cleanup;  /* custom callback function */
    uct_log_t            *log;      /* log */
};

typedef struct {
    uct_fd_t              fd;
    u_char               *name;
    uct_log_t            *log;
} uct_pool_cleanup_file_t;

uct_pool_t *uct_create_pool(size_t size, uct_log_t *log);
void uct_destroy_pool(uct_pool_t *pool);
void uct_reset_pool(uct_pool_t *pool);

void *uct_palloc(uct_pool_t *pool, size_t size);
void *uct_pnalloc(uct_pool_t *pool, size_t size);
void *uct_pcalloc(uct_pool_t *pool, size_t size);
void *uct_pmemalign(uct_pool_t *pool, size_t size, size_t alignment);
uct_int_t uct_pfree(uct_pool_t *pool, void *p);


uct_pool_cleanup_t *uct_pool_cleanup_add(uct_pool_t *p, size_t size);
void uct_pool_run_cleanup_file(uct_pool_t *p, uct_fd_t fd);
void uct_pool_cleanup_file(void *data);
void uct_pool_delete_file(void *data);

#endif /* _UCT_PALLOC_H_INCLUDED_ */