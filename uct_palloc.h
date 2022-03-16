/*
 * Copyright (C) 2021 SunBK201
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

/* 自定义清理回调的数据结构 */
struct uct_pool_cleanup_s {
    uct_pool_cleanup_pt   handler;  /* 清理的回调函数 */
    void                 *data;     /* 指向存储的数据地址 */
    uct_pool_cleanup_t   *next;     /* 下一个uct_pool_cleanup_t */
};

typedef struct uct_pool_large_s  uct_pool_large_t;

/* 大数据块结构 */
struct uct_pool_large_s {
    uct_pool_large_t     *next;     /* 指向下一个存储地址 通过这个地址可以知道当前块长度 */
    void                 *alloc;    /* 数据块指针地址 */
};

/* 数据区域结构 */
typedef struct {
    u_char               *last;     /* 内存池中未使用内存的开始节点地址 */
    u_char               *end;      /* 内存池的结束地址 */
    uct_pool_t           *next;     /* 指向下一个内存池 */
    uct_uint_t            failed;   /* 失败次数 */
} uct_pool_data_t;

/* 内存池主结构 */
struct uct_pool_s {
    uct_pool_data_t       d;        /* 内存池的数据区域*/
    size_t                max;      /* 最大每次可分配内存 */
    uct_pool_t           *current;  /* 指向当前的内存池指针地址。uct_pool_t链表上最后一个缓存池结构 */
    uct_chain_t          *chain;    /* 缓冲区链表 */
    uct_pool_large_t     *large;    /* 存储大数据的链表 */
    uct_pool_cleanup_t   *cleanup;  /* 可自定义回调函数，清除内存块分配的内存 */
    uct_log_t            *log;      /* 日志 */
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