/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_BUF_H_INCLUDED_
#define _UCT_BUF_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

typedef void *            uct_buf_tag_t;

typedef struct uct_buf_s  uct_buf_t;

struct uct_buf_s {
    u_char          *pos;
    u_char          *last;
    off_t            file_pos;
    off_t            file_last;

    u_char          *start;         /* start of buffer */
    u_char          *end;           /* end of buffer */
    uct_buf_tag_t    tag;
    uct_file_t      *file;
    uct_buf_t       *shadow;


    /* the buf's content could be changed */ 
    unsigned         temporary:1;

    /*
     * the buf's content is in a memory cache or in a read only memory
     * and must not be changed
     */
    unsigned         memory:1;

    /* the buf's content is mmap()ed and must not be changed */
    unsigned         mmap:1;

    unsigned         recycled:1;
    unsigned         in_file:1;
    unsigned         flush:1;
    unsigned         sync:1;
    unsigned         last_buf:1;
    unsigned         last_in_chain:1;

    unsigned         last_shadow:1;
    unsigned         temp_file:1;

    /* STUB */ int   num;
};

struct uct_chain_s {
    uct_buf_t    *buf;
    uct_chain_t  *next;
};


typedef struct {
    uct_int_t    num;
    size_t       size;
} uct_bufs_t;

typedef struct uct_output_chain_ctx_s  uct_output_chain_ctx_t;

typedef uct_int_t (*uct_output_chain_filter_pt)(void *ctx, uct_chain_t *in);

typedef void (*uct_output_chain_aio_pt)(uct_output_chain_ctx_t *ctx,
    uct_file_t *file);

struct uct_output_chain_ctx_s {
    uct_buf_t                   *buf;
    uct_chain_t                 *in;
    uct_chain_t                 *free;
    uct_chain_t                 *busy;

    unsigned                     sendfile:1;
    unsigned                     directio:1;
    unsigned                     unaligned:1;
    unsigned                     need_in_memory:1;
    unsigned                     need_in_temp:1;
    unsigned                     aio:1;

    off_t                        alignment;

    uct_pool_t                  *pool;
    uct_int_t                    allocated;
    uct_bufs_t                   bufs;
    uct_buf_tag_t                tag;

    uct_output_chain_filter_pt   output_filter;
    void                        *filter_ctx;
};


typedef struct {
    uct_chain_t                 *out;
    uct_chain_t                **last;
    uct_connection_t            *connection;
    uct_pool_t                  *pool;
    off_t                        limit;
} uct_chain_writer_ctx_t;


#define UCT_CHAIN_ERROR     (uct_chain_t *) UCT_ERROR

#define uct_buf_in_memory(b)       ((b)->temporary || (b)->memory || (b)->mmap)
#define uct_buf_in_memory_only(b)  (uct_buf_in_memory(b) && !(b)->in_file)

#define uct_buf_special(b)                                                   \
    (((b)->flush || (b)->last_buf || (b)->sync)                              \
     && !uct_buf_in_memory(b) && !(b)->in_file)

#define uct_buf_sync_only(b)                                                 \
    ((b)->sync && !uct_buf_in_memory(b)                                      \
     && !(b)->in_file && !(b)->flush && !(b)->last_buf)

#define uct_buf_size(b)                                                      \
    (uct_buf_in_memory(b) ? (off_t) ((b)->last - (b)->pos):                  \
                            ((b)->file_last - (b)->file_pos))

uct_buf_t *uct_create_temp_buf(uct_pool_t *pool, size_t size);
uct_chain_t *uct_create_chain_of_bufs(uct_pool_t *pool, uct_bufs_t *bufs);


#define uct_alloc_buf(pool)  uct_palloc(pool, sizeof(uct_buf_t))
#define uct_calloc_buf(pool) uct_pcalloc(pool, sizeof(uct_buf_t))

uct_chain_t *uct_alloc_chain_link(uct_pool_t *pool);
#define uct_free_chain(pool, cl)                                             \
    (cl)->next = (pool)->chain;                                              \
    (pool)->chain = (cl)



uct_int_t uct_output_chain(uct_output_chain_ctx_t *ctx, uct_chain_t *in);
uct_int_t uct_chain_writer(void *ctx, uct_chain_t *in);

uct_int_t uct_chain_add_copy(uct_pool_t *pool, uct_chain_t **chain,
    uct_chain_t *in);
uct_chain_t *uct_chain_get_free_buf(uct_pool_t *p, uct_chain_t **free);
void uct_chain_update_chains(uct_pool_t *p, uct_chain_t **free,
    uct_chain_t **busy, uct_chain_t **out, uct_buf_tag_t tag);

off_t uct_chain_coalesce_file(uct_chain_t **in, off_t limit);

uct_chain_t *uct_chain_update_sent(uct_chain_t *in, off_t sent);


#endif /* _UCT_BUF_H_INCLUDED_ */