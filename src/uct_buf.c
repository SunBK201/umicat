/*
 * Copyright (C) SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

/**
 * 创建一个缓冲区。需要传入pool和buf的大小
 */
uct_buf_t *
uct_create_temp_buf(uct_pool_t *pool, size_t size)
{
    uct_buf_t *b;

    b = uct_calloc_buf(pool);
    if (b == NULL) {
        return NULL;
    }

    b->start = uct_palloc(pool, size);
    if (b->start == NULL) {
        return NULL;
    }

    /*
     * set by uct_calloc_buf():
     *
     *     b->file_pos = 0;
     *     b->file_last = 0;
     *     b->file = NULL;
     *     b->shadow = NULL;
     *     b->tag = 0;
     *     and flags
     */

    b->pos = b->start;
    b->last = b->start;
    b->end = b->last + size;
    b->temporary = 1;

    return b;
}

/**
 * 创建一个缓冲区的链表结构 uct_chain_t
 */
uct_chain_t *
uct_alloc_chain_link(uct_pool_t *pool)
{
    uct_chain_t  *cl;

    cl = pool->chain;

	/*
	 * 首先从内存池中去取 uct_chain_t，
	 * 被清空的 uct_chain_t 结构都会放在 pool->chain 缓冲链上
	 */
    if (cl) {
        pool->chain = cl->next;
        return cl;
    }

    /* 如果取不到，则从内存池pool上分配一个数据结构 */
    cl = uct_palloc(pool, sizeof(uct_chain_t));
    if (cl == NULL) {
        return NULL;
    }

    return cl;
}


/**
 * 批量创建多个buf，并且用链表串起来
 */
uct_chain_t *
uct_create_chain_of_bufs(uct_pool_t *pool, uct_bufs_t *bufs)
{
    u_char       *p;
    uct_int_t     i;
    uct_buf_t    *b;
    uct_chain_t  *chain, *cl, **ll;

    /* 在内存池 pool 上分配 bufs->num 个 buf 缓冲区, 每个大小为 bufs->size */
    p = uct_palloc(pool, bufs->num * bufs->size);
    if (p == NULL) {
        return NULL;
    }

    ll = &chain;

    /* 循环创建BUF，并且将uct_buf_t挂载到uct_chain_t链表上，并且返回链表 */
    for (i = 0; i < bufs->num; i++) {

        b = uct_calloc_buf(pool);
        if (b == NULL) {
            return NULL;
        }

        /*
         * set by uct_calloc_buf():
         *
         *     b->file_pos = 0;
         *     b->file_last = 0;
         *     b->file = NULL;
         *     b->shadow = NULL;
         *     b->tag = 0;
         *     and flags
         *
         */

        b->pos = p;
        b->last = p;
        b->temporary = 1;

        b->start = p;
        p += bufs->size;
        b->end = p;

        cl = uct_alloc_chain_link(pool);
        if (cl == NULL) {
            return NULL;
        }

        /* 将buf，都挂载到uct_chain_t链表上，最终返回uct_chain_t链表 */
        cl->buf = b;
        *ll = cl;
        ll = &cl->next;
    }

    *ll = NULL;

    return chain;
}

/**
 * 将其它缓冲区链表放到已有缓冲区链表结构的尾部
 * 把 in 添加到 chain 的后面，拼接起来
 */
uct_int_t
uct_chain_add_copy(uct_pool_t *pool, uct_chain_t **chain, uct_chain_t *in)
{
    uct_chain_t  *cl, **ll;

    ll = chain;

    for (cl = *chain; cl; cl = cl->next) {
        ll = &cl->next;
    }

    while (in) {
        cl = uct_alloc_chain_link(pool);
        if (cl == NULL) {
            *ll = NULL;
            return UCT_ERROR;
        }

        cl->buf = in->buf;
        *ll = cl;
        ll = &cl->next;
        in = in->next;
    }

    *ll = NULL;

    return UCT_OK;
}

/**
 * 从空闲的 buf 链表上，获取一个未使用的 buf 链表
 */
uct_chain_t *
uct_chain_get_free_buf(uct_pool_t *p, uct_chain_t **free)
{
    uct_chain_t  *cl;

    // 如果 free 链表中有有空余的 uct_chain_t 节点, 则直接使用该链表中的uct_chain_t节点空间, 否则后面开辟空间
    if (*free) {
        cl = *free;
        *free = cl->next;
        cl->next = NULL;
        return cl;
    }

    cl = uct_alloc_chain_link(p);
    if (cl == NULL) {
        return NULL;
    }

    cl->buf = uct_calloc_buf(p);
    if (cl->buf == NULL) {
        return NULL;
    }

    cl->next = NULL;

    return cl;
}

/**
 * 把新读到的out数据添加到busy表尾部，
 * 然后把busy表中已经处理完毕的buf节点从busy表中摘除放到free表头部
 * 未发送出去的buf节点既会在out链表中，也会在busy链表中
 *
 * 释放BUF
 * 1. 如果buf不为空，则不释放
 * 2. 如果cl->buf->tag标记不一样，则直接还给pool->chain链表
 * 3. 如果buf为空，并且需要释放，则直接释放buf，并且放到free的空闲列表上
 */
void
uct_chain_update_chains(uct_pool_t *p, uct_chain_t **free, uct_chain_t **busy,
    uct_chain_t **out, uct_buf_tag_t tag)
{
    uct_chain_t  *cl;

    if (*out) {
        if (*busy == NULL) {
            *busy = *out;

        } else {
            for (cl = *busy; cl->next; cl = cl->next) { /* void */ }

            cl->next = *out;
        }

        *out = NULL;
    }

    while (*busy) {
        cl = *busy;

        // buf大小不是0，说明还没有输出
        if (uct_buf_size(cl->buf) != 0) {
            break;
        }

        if (cl->buf->tag != tag) { // tag中存储的是函数指针
            *busy = cl->next;
            uct_free_chain(p, cl);
            continue;
        }

        cl->buf->pos = cl->buf->start;
        cl->buf->last = cl->buf->start;

        *busy = cl->next;
        cl->next = *free;
        *free = cl;
    }
}

/* 计算in链表上的文件大小 */
off_t
uct_chain_coalesce_file(uct_chain_t **in, off_t limit)
{
    off_t         total, size, aligned, fprev;
    uct_fd_t      fd;
    uct_chain_t  *cl;

    total = 0;

    cl = *in;
    fd = cl->buf->file->fd;

    do {
        size = cl->buf->file_last - cl->buf->file_pos;

        if (size > limit - total) {
            size = limit - total;

            aligned = (cl->buf->file_pos + size + uct_pagesize - 1)
                       & ~((off_t) uct_pagesize - 1);

            if (aligned <= cl->buf->file_last) {
                size = aligned - cl->buf->file_pos;
            }

            total += size;
            break;
        }

        total += size;
        fprev = cl->buf->file_pos + size;
        cl = cl->next;

    } while (cl
             && cl->buf->in_file
             && total < limit
             && fd == cl->buf->file->fd
             && fprev == cl->buf->file_pos);

    *in = cl;

    return total;
}

/* 计算本次掉用uct_writev发送出去的send字节在in链表中所有数据的那个位置 */
uct_chain_t *
uct_chain_update_sent(uct_chain_t *in, off_t sent)
{
    off_t  size;

    for ( /* void */ ; in; in = in->next) {

        if (uct_buf_special(in->buf)) {
            continue;
        }

        if (sent == 0) {
            break;
        }

        size = uct_buf_size(in->buf);

        if (sent >= size) {
            sent -= size;

            if (uct_buf_in_memory(in->buf)) {
                in->buf->pos = in->buf->last;
            }

            if (in->buf->in_file) {
                in->buf->file_pos = in->buf->file_last;
            }

            continue;
        }

        if (uct_buf_in_memory(in->buf)) {
            in->buf->pos += (size_t) sent;
        }

        if (in->buf->in_file) {
            in->buf->file_pos += sent;
        }

        break;
    }

    return in;
}
