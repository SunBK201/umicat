/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_ARRAY_H_INCLUDED_
#define _UCT_ARRAY_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

typedef struct {
    void        *elts;      /* 指向数组第一个元素指针 */
    uct_uint_t   nelts;     /* 已使用元素的个数 */
    size_t       size;      /* 每个元素(elts)的大小，元素大小固定 */
    uct_uint_t   nalloc;    /* 元素个数上限 */
    uct_pool_t  *pool;      /* 内存池 */
} uct_array_t;


uct_array_t *uct_array_create(uct_pool_t *p, uct_uint_t n, size_t size);
void uct_array_destroy(uct_array_t *a);
void *uct_array_push(uct_array_t *a);
void *uct_array_push_n(uct_array_t *a, uct_uint_t n);

#define uct_array_loc(array, loc)                             \
    ((u_char *)(array)->elts + (loc) * array->size)

static uct_inline uct_int_t
uct_array_init(uct_array_t *array, uct_pool_t *pool, uct_uint_t n, size_t size)
{
    /*
     * set "array->nelts" before "array->elts", otherwise MSVC thinks
     * that "array->nelts" may be used without having been initialized
     */

    array->nelts = 0;
    array->size = size;
    array->nalloc = n;
    array->pool = pool;

    array->elts = uct_palloc(pool, n * size);
    if (array->elts == NULL) {
        return UCT_ERROR;
    }

    return UCT_OK;
}



#endif /* _UCT_ARRAY_H_INCLUDED_ */