/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_LIST_H_INCLUDED_
#define _UCT_LIST_H_INCLUDED_

#include <uct_config.h>

typedef struct uct_list_part_s  uct_list_part_t;

struct uct_list_part_s {
    void             *elts;     /* uct_list_part_s node */
    uct_uint_t        nelts;    /* elements in use, nelts < uct_list_t.nalloc */
    uct_list_part_t  *next;     /* next uct_list_part_s node */
};


typedef struct {
    uct_list_part_t  *last;     /* the newest uct_list_t node */
    uct_list_part_t   part;     /* the first uct_list_t node */
    size_t            size;     /* default element size */
    uct_uint_t        nalloc;   /* capacity of each uct_list_part_t */
    uct_pool_t       *pool;     /* uct_pool_t */
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
