/*
 * Copyright (C) 2021 SunBK201
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
 * 内存池对齐分配一块内存，返回 void 类型指针
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
 * 内存池非对齐分配一块内存，返回 void 类型指针
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
 * 申请一个新的缓存池 uct_pool_t
 * 新的缓存池会挂载在主缓存池的数据区域 (pool->d->next)
 */
static void *
uct_palloc_block(uct_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    uct_pool_t  *p, *new;

    psize = (size_t) (pool->d.end - (u_char *) pool);

    /* 申请新的块 */
    m = uct_memalign(UCT_POOL_ALIGNMENT, psize, pool->log);
    if (m == NULL) {
        return NULL;
    }

    new = (uct_pool_t *) m;

    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    /* 分配size大小的内存块，返回m指针地址 */
    m += sizeof(uct_pool_data_t);
    m = uct_align_ptr(m, UCT_ALIGNMENT);
    new->d.last = m + size;


	/**
	 * 缓存池的 pool 数据结构会挂载子节点的uct_pool_t数据结构
	 * 子节点的uct_pool_t数据结构中只用到pool->d的结构，只保存数据
	 * 每添加一个子节点，p->d.failed就会+1，当添加超过4个子节点的时候，
	 * pool->current会指向到最新的子节点地址
	 *
	 * 这个逻辑主要是为了防止pool上的子节点过多，导致每次uct_palloc循环pool->d.next链表
	 * 将pool->current设置成最新的子节点之后，每次最大循环4次，不会去遍历整个缓存池链表
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
 * 当分配的内存块大小超出 pool->max 限制的时候, 需要分配在 pool->large 上
 */
static void *
uct_palloc_large(uct_pool_t *pool, size_t size)
{
    void              *p;
    uct_uint_t         n;
    uct_pool_large_t  *large;

    /* 分配一块新的大内存块 */
    p = uct_alloc(size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    /* 去pool->large链表上查询是否有NULL的，只在链表上往下查询3次，主要判断大数据块是否有被释放的，如果没有则只能跳出*/
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    /* 分配一个 uct_pool_large_t 数据结构 */
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
 * 内存块分配在 pool->large 上
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
 * 大内存块释放 pool->large
 */
uct_int_t
uct_pfree(uct_pool_t *pool, void *p)
{
    uct_pool_large_t  *l;

    /* 在pool->large链上循环搜索，并且只释放内容区域，不释放uct_pool_large_t数据结构 */
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
 * 内存池对齐分配一块内存，返回 void 类型指针，并初始化清零
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


/**
 * 分配一个可以用于回调函数清理内存块的内存
 * 内存块仍旧在p->d或p->large上
 *
 * uct_pool_t中的cleanup字段管理着一个特殊的链表，该链表的每一项都记录着一个特殊的需要释放的资源。
 * 对于这个链表中每个节点所包含的资源如何去释放，是自说明的。这也就提供了非常大的灵活性。
 * 意味着，uct_pool_t不仅仅可以管理内存，通过这个机制，也可以管理任何需要释放的资源，
 * 例如，关闭文件，或者删除文件等等的。下面我们看一下这个链表每个节点的类型
 *
 * 一般分两种情况：
 * 1. 文件描述符
 * 2. 外部自定义回调函数可以来清理内存
 */
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


/**
 * 清除 p->cleanup 链表上的内存块（主要是文件描述符）
 * 回调函数：uct_pool_cleanup_file
 */
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


/**
 * 回调函数: 关闭文件
 * uct_pool_run_cleanup_file 方法执行的时候，用了此函数作为回调函数的，都会被清理
 */
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


/**
 * 回调函数: 删除文件
 * uct_pool_run_cleanup_file 方法执行的时候，用了此函数作为回调函数的，都会被清理
 */
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