/*
 * Copyright (C) SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

/**
 * 创建链表
 */
uct_list_t *
uct_list_create(uct_pool_t *pool, uct_uint_t n, size_t size)
{
    uct_list_t  *list;

    /* 从内存池上面分配一块内存，存储uct_list_t数据结构 */
    list = uct_palloc(pool, sizeof(uct_list_t));
    if (list == NULL) {
        return NULL;
    }

    if (uct_list_init(list, pool, n, size) != UCT_OK) {
        return NULL;
    }

    return list;
}

/* 
 * 使用一个list元素, 返回元素的指针地址
 * 如果l中的last的elts用完了，则在l链表中新创建一个uct_list_part_t
 */
void *
uct_list_push(uct_list_t *l)
{
    void             *elt;
    uct_list_part_t  *last;

    last = l->last;
    
    /* 如果最后一个链表节点的元素已经用完，则需要创建一个新的链表*/
    if (last->nelts == l->nalloc) {

        /* the last part is full, allocate a new list part */

        last = uct_palloc(l->pool, sizeof(uct_list_part_t));
        if (last == NULL) {
            return NULL;
        }

        last->elts = uct_palloc(l->pool, l->nalloc * l->size);
        if (last->elts == NULL) {
            return NULL;
        }

        last->nelts = 0;
        last->next = NULL;

        l->last->next = last;
        l->last = last;
    }

    elt = (char *) last->elts + l->size * last->nelts;
    last->nelts++;

    return elt;
}
