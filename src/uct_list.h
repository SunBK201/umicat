/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_LIST_H_INCLUDED_
#define _UCT_LIST_H_INCLUDED_

#include <uct_config.h>

typedef struct uct_list_part_s  uct_list_part_t;

struct uct_list_part_s {
    void             *elts;     /* 节点的内存起始位置 */
    uct_uint_t        nelts;    /* 表示数组中已经使用了多少个元素, nelts必须小于uct_list_t结构体中的nalloc */
    uct_list_part_t  *next;     /* 指向下一个链表节点*/
};


typedef struct {
    uct_list_part_t  *last;     /* 指向最新的链表节点 */
    uct_list_part_t   part;     /* 第一个链表节点 */
    size_t            size;     /* 链表默认的元素大小 */
    uct_uint_t        nalloc;   /* 每个uct_list_part_t数组的容量 */
    uct_pool_t       *pool;     /* 内存池 */
} uct_list_t;


uct_list_t *uct_list_create(uct_pool_t *pool, uct_uint_t n, size_t size);

static uct_inline uct_int_t
uct_list_init(uct_list_t *list, uct_pool_t *pool, uct_uint_t n, size_t size)
{
    list->part.elts = uct_palloc(pool, n * size);
    if (list->part.elts == NULL) {
        return UCT_ERROR;
    }

    list->part.nelts = 0;
    list->part.next = NULL;
    list->last = &list->part;
    list->size = size;
    list->nalloc = n;
    list->pool = pool;

    return UCT_OK;
}

void *uct_list_push(uct_list_t *list);


#endif /* _UCT_LIST_H_INCLUDED_ */
