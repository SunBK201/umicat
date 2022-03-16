/*
 * Copyright (C) 2021 SunBK201
 */

#ifndef _UCT_BUF_H_INCLUDED_
#define _UCT_BUF_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

typedef void *            uct_buf_tag_t;

typedef struct uct_buf_s  uct_buf_t;

struct uct_buf_s {
    u_char          *pos;           /* 待处理数据的开始标记 */
    u_char          *last;          /* 待处理数据的结尾标记 */
    off_t            file_pos;      /* 处理文件时, 待处理的文件开始标记 */
    off_t            file_last;     /* 处理文件时, 待处理的文件结尾标记 */

    u_char          *start;         /* start of buffer */
    u_char          *end;           /* end of buffer */
    uct_buf_tag_t    tag;           /* 一个void*类型的指针，使用者可以关联任意的对象上去，只要对使用者有意义 */
    uct_file_t      *file;          /* 引用的文件 */
    uct_buf_t       *shadow;


    /* the buf's content could be changed */ 
    unsigned         temporary:1;   /* 标志位，为1时，内存可修改 */

    /*
     * the buf's content is in a memory cache or in a read only memory
     * and must not be changed
     */
    unsigned         memory:1;      /* 标志位，为1时，内存只读 */

    /* the buf's content is mmap()ed and must not be changed */
    unsigned         mmap:1;        /* 标志位，为1时，mmap映射过来的内存，不可修改 */

    unsigned         recycled:1;    /* 标志位，为1时，可回收 */
    unsigned         in_file:1;     /* 标志位，为1时，表示处理的是文件 */
    unsigned         flush:1;       /* 标志位，为1时，表示需要进行flush操作 */
    unsigned         sync:1;        /* 标志位，为1时，表示可以进行同步操作，容易引起堵塞 */
    unsigned         last_buf:1;    /* 标志位，为1时，表示为缓冲区链表uct_chain_t上的最后一块待处理缓冲区 */
    unsigned         last_in_chain:1;   /* 标志位，为1时，表示为缓冲区链表uct_chain_t上的最后一块缓冲区 */

    unsigned         last_shadow:1; /* 标志位，为1时，表示是否是最后一个影子缓冲区 */
    unsigned         temp_file:1;   /* 标志位，为1时，表示当前缓冲区是否属于临时文件 */

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

// 返回这个buf里面的内容是否在内存里
#define uct_buf_in_memory(b)       ((b)->temporary || (b)->memory || (b)->mmap)
#define uct_buf_in_memory_only(b)  (uct_buf_in_memory(b) && !(b)->in_file)

// 返回该buf是否是一个特殊的buf，只含有特殊的标志和没有包含真正的数据
#define uct_buf_special(b)                                                   \
    (((b)->flush || (b)->last_buf || (b)->sync)                              \
     && !uct_buf_in_memory(b) && !(b)->in_file)

#define uct_buf_sync_only(b)                                                 \
    ((b)->sync && !uct_buf_in_memory(b)                                      \
     && !(b)->in_file && !(b)->flush && !(b)->last_buf)

// 返回该buf所含数据的大小，不管这个数据是在文件里还是在内存里
#define uct_buf_size(b)                                                      \
    (uct_buf_in_memory(b) ? (off_t) ((b)->last - (b)->pos):                  \
                            ((b)->file_last - (b)->file_pos))

uct_buf_t *uct_create_temp_buf(uct_pool_t *pool, size_t size);
uct_chain_t *uct_create_chain_of_bufs(uct_pool_t *pool, uct_bufs_t *bufs);


#define uct_alloc_buf(pool)  uct_palloc(pool, sizeof(uct_buf_t))
#define uct_calloc_buf(pool) uct_pcalloc(pool, sizeof(uct_buf_t))

uct_chain_t *uct_alloc_chain_link(uct_pool_t *pool);
/*
pool中的chain指向一个uct_chain_t数据，其值是由宏uct_free_chain进行赋予的，
指向之前用完了的，可以释放的uct_chain_t数据
*/
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