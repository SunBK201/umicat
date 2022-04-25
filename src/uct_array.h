/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_ARRAY_H_INCLUDED_
#define _UCT_ARRAY_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

typedef struct {
    void *elts;        /* the first element of uct_array_t */
    uct_uint_t nelts;  /* number of elements in use */
    size_t size;       /* element size */
    uct_uint_t nalloc; /* limit of elements */
    uct_pool_t *pool;  /* uct_pool_t */
} uct_array_t;


uct_array_t *uct_array_create(uct_pool_t *p, uct_uint_t n, size_t size);
void uct_array_destroy(uct_array_t *a);
void *uct_array_push(uct_array_t *a);
void *uct_array_push_n(uct_array_t *a, uct_uint_t n);


#define uct_array_loc(array, loc)                                         \
    ((u_char *)(array)->elts + (loc)*array->size)


static uct_inline uct_int_t
uct_array_init(uct_array_t *array, uct_pool_t *pool, uct_uint_t n,
    size_t size)
{
    array->nelts  = 0;
    array->size   = size;
    array->nalloc = n;
    array->pool   = pool;

    array->elts = uct_palloc(pool, n * size);
    if (array->elts == NULL) {
        return UCT_ERROR;
    }

    return UCT_OK;
}

#endif /* _UCT_ARRAY_H_INCLUDED_ */