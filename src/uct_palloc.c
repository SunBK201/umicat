/*
 * Copyright (C) SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

static uct_inline void *uct_palloc_small(uct_pool_t *pool, size_t size,
    uct_uint_t align);
static void *uct_palloc_block(uct_pool_t *pool, size_t size);
static void *uct_palloc_large(uct_pool_t *pool, size_t size);


uct_pool_t *
uct_create_pool(size_t size, uct_log_t *log)
{
    uct_pool_t  *p;

    p = uct_memalign(UCT_POOL_ALIGNMENT, size, log);
    if (p == NULL) {
        return NULL;
    }

    p->d.last = (u_char *) p + sizeof(uct_pool_t);
    p->d.end = (u_char *) p + size;
    p->d.next = NULL;
    p->d.failed = 0;

    size = size - sizeof(uct_pool_t);
    p->max = (size < UCT_MAX_ALLOC_FROM_POOL) ? size : UCT_MAX_ALLOC_FROM_POOL;

    p->current = p;
    p->chain = NULL;
    p->large = NULL;
    p->cleanup = NULL;
    p->log = log;

    return p;
}


void
uct_destroy_pool(uct_pool_t *pool)
{
    uct_pool_t          *p, *n;
    uct_pool_large_t    *l;
    uct_pool_cleanup_t  *c;

    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            uct_log(pool->log, UCT_LOG_DEBUG, "run cleanup: %p", c);
            c->handler(c->data);
        }
    }

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            uct_free(l->alloc);
        }
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        uct_free(p);

        if (n == NULL) {
            break;
        }
    }
}


void
uct_reset_pool(uct_pool_t *pool)
{
    uct_pool_t        *p;
    uct_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            uct_free(l->alloc);
        }
    }

    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(uct_pool_t);
        p->d.failed = 0;
    }

    pool->current = pool;
    pool->chain = NULL;
    pool->large = NULL;
}


/**
 * uct_pool allocates a block of memory (alignment)
 */
void *
uct_palloc(uct_pool_t *pool, size_t size)
{
    if (size <= pool->max) {
        return uct_palloc_small(pool, size, 1);
    }

    return uct_palloc_large(pool, size);
}


/**
 * uct_pool allocates a block of memory (unaligned)
 */
void *
uct_pnalloc(uct_pool_t *pool, size_t size)
{
    if (size <= pool->max) {
        return uct_palloc_small(pool, size, 0);
    }

    return uct_palloc_large(pool, size);
}


static uct_inline void *
uct_palloc_small(uct_pool_t *pool, size_t size, uct_uint_t align)
{
    u_char      *m;
    uct_pool_t  *p;

    p = pool->current;

    do {
        m = p->d.last;

        if (align) {
            m = uct_align_ptr(m, UCT_ALIGNMENT);
        }

        if ((size_t) (p->d.end - m) >= size) {
            p->d.last = m + size;

            return m;
        }

        p = p->d.next;

    } while (p);

    return uct_palloc_block(pool, size);
}


/**
 * create a new uct_pool_t and mounted in pool->d->next
 */
static void *
uct_palloc_block(uct_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    uct_pool_t  *p, *new;

    psize = (size_t) (pool->d.end - (u_char *) pool);

    m = uct_memalign(UCT_POOL_ALIGNMENT, psize, pool->log);
    if (m == NULL) {
        return NULL;
    }

    new = (uct_pool_t *) m;

    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    m += sizeof(uct_pool_data_t);
    m = uct_align_ptr(m, UCT_ALIGNMENT);
    new->d.last = m + size;


    /**
     * The pool data structure of the cache pool will mount the uct_pool_t
     * data structure of the child nodes The uct_pool_t data structure of
     * the child nodes will only use the structure of pool->d to save data
     * only Every time a child node is added, p->d.failed will be +1, and
     * when more than 4 child nodes are added, pool->current will point to
     * the latest child node address
     *
     * This logic is mainly to prevent too many child nodes on the pool,
     * resulting in each uct_palloc loop pool->d.next chain after setting
     * pool->current to the latest child node, each time the maximum loop 4
     * times, will not go through the entire cache pool chain
     */
    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->current = p->d.next;
        }
    }

    p->d.next = new;

    return m;
}


/**
 * When the allocated memory block size exceeds 
 * the pool->max limit, it needs to be allocated on pool->large
 */
static void *
uct_palloc_large(uct_pool_t *pool, size_t size)
{
    void              *p;
    uct_uint_t         n;
    uct_pool_large_t  *large;

    p = uct_alloc(size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    /**
     * Go to the pool->large chain to query whether there is NULL, only 3
     * times down the chain, mainly to determine whether the large data
     * blocks have been released, if not then only jump out.
     */
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = uct_palloc_small(pool, sizeof(uct_pool_large_t), 1);
    if (large == NULL) {
        uct_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


/**
 * memory block is allocated on pool->large
 */
void *
uct_pmemalign(uct_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    uct_pool_large_t  *large;

    p = uct_memalign(alignment, size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    large = uct_palloc_small(pool, sizeof(uct_pool_large_t), 1);
    if (large == NULL) {
        uct_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


/**
 * free pool->large
 */
uct_int_t
uct_pfree(uct_pool_t *pool, void *p)
{
    uct_pool_large_t  *l;

    /**
     * Loop through the pool->large chain and only release the content
     * area, not the uct_pool_large_t data structure.
     */
    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            uct_log(pool->log, UCT_LOG_DEBUG, "free: %p", l->alloc);
            uct_free(l->alloc);
            l->alloc = NULL;

            return UCT_OK;
        }
    }

    return UCT_DECLINED;
}


/**
 * uct_pool allocates a block of memory (alignment)(initialize 0)
 */
void *
uct_pcalloc(uct_pool_t *pool, size_t size)
{
    void *p;

    p = uct_palloc(pool, size);
    if (p) {
        uct_memzero(p, size);
    }

    return p;
}


uct_pool_cleanup_t *
uct_pool_cleanup_add(uct_pool_t *p, size_t size)
{
    uct_pool_cleanup_t  *c;

    c = uct_palloc(p, sizeof(uct_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    if (size) {
        c->data = uct_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    c->handler = NULL;
    c->next = p->cleanup;

    p->cleanup = c;

    uct_log(p->log, UCT_LOG_DEBUG, "add cleanup: %p", c);

    return c;
}


void
uct_pool_run_cleanup_file(uct_pool_t *p, uct_fd_t fd)
{
    uct_pool_cleanup_t       *c;
    uct_pool_cleanup_file_t  *cf;

    for (c = p->cleanup; c; c = c->next) {
        if (c->handler == uct_pool_cleanup_file) {

            cf = c->data;

            if (cf->fd == fd) {
                c->handler(cf);
                c->handler = NULL;
                return;
            }
        }
    }
}


void
uct_pool_cleanup_file(void *data)
{
    uct_pool_cleanup_file_t  *c = data;

    uct_log(c->log, UCT_LOG_DEBUG, "file cleanup: fd:%d", c->fd);

    if (uct_close_file(c->fd) == UCT_FILE_ERROR) {
	    uct_log(c->log, UCT_LOG_WARN, uct_close_file_n " \"%s\" failed",
		c->name);
    }
}


void
uct_pool_delete_file(void *data)
{
    uct_pool_cleanup_file_t  *c = data;

    uct_err_t  err;

    uct_log(c->log, UCT_LOG_DEBUG, "file cleanup: fd:%d %s", c->fd, c->name);

    if (uct_delete_file(c->name) == UCT_FILE_ERROR) {
	    err = uct_errno;

	    if (err != ENOENT) {
		    uct_log(c->log, UCT_LOG_FATAL,
			    uct_delete_file_n " \"%s\" failed", c->name);
	    }
    }

    if (uct_close_file(c->fd) == UCT_FILE_ERROR) {
	    uct_log(c->log, UCT_LOG_ERROR, uct_close_file_n " \"%s\" failed",
		c->name);
    }
}