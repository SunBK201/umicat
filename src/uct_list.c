/*
 * Copyright (C) SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>


uct_list_t *
uct_list_create(uct_pool_t *pool, uct_uint_t n, size_t size)
{
    uct_list_t  *list;

    list = uct_palloc(pool, sizeof(uct_list_t));
    if (list == NULL) {
        return NULL;
    }

    if (uct_list_init(list, pool, n, size) != UCT_OK) {
        return NULL;
    }

    return list;
}


void *
uct_list_push(uct_list_t *l)
{
    void             *elt;
    uct_list_part_t  *last;

    last = l->last;

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
