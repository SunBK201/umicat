/*
 * Copyright (C) SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

/**
 * create and initialize a uct_array_t
 */
uct_array_t *
uct_array_create(uct_pool_t *p, uct_uint_t n, size_t size)
{
    uct_array_t *a;

    a = uct_palloc(p, sizeof(uct_array_t));
    if (a == NULL) {
        return NULL;
    }

    if (uct_array_init(a, p, n, size) != UCT_OK) {
        return NULL;
    }

    return a;
}

/**
 * destroy uct_array_t
 */
void
uct_array_destroy(uct_array_t *a)
{
    uct_pool_t  *p;

    p = a->pool;

    /**
     * If the end address of the array element is the same as the available
     * start address of the pool, move the pool pool->d.last to the start
     * address of the array element, which is equivalent to clearing the
     * contents of the current array.
     */
    if ((u_char *)a->elts + a->size * a->nalloc == p->d.last) {
        p->d.last -= a->size * a->nalloc;
    }

    /**
     * If the end address of the array's data structure uct_array_t is the
     * same as the available start address of the memory pool pool, move
     * the memory pool pool->d.last to the start address of the array
     * element, which is equivalent to clearing the current array's
     * contents
     */
    if ((u_char *)a + sizeof(uct_array_t) == p->d.last) {
        p->d.last = (u_char *)a;
    }
}

/**
 * Adds an element
 */
void *
uct_array_push(uct_array_t *a)
{
    void        *elt, *new;
    size_t       size;
    uct_pool_t  *p;

    if (a->nelts == a->nalloc) {

        /* the array is full */

        size = a->size * a->nalloc;

        p = a->pool;

        if ((u_char *) a->elts + size == p->d.last
            && p->d.last + a->size <= p->d.end)
        {
            /*
             * the array allocation is the last in the pool
             * and there is space for new allocation
             */
            p->d.last += a->size;
            a->nalloc++;

        } else {
            /* allocate a new array */

            new = uct_palloc(p, 2 * size);
            if (new == NULL) {
                return NULL;
            }

            uct_memcpy(new, a->elts, size);
            a->elts = new;
            a->nalloc *= 2;
        }
    }

    elt = (u_char *) a->elts + a->size * a->nelts;
    a->nelts++;

    return elt;
}

/**
 * Add n elements
 */
void *
uct_array_push_n(uct_array_t *a, uct_uint_t n)
{
    void        *elt, *new;
    size_t       size;
    uct_uint_t   nalloc;
    uct_pool_t  *p;

    size = n * a->size;

    if (a->nelts + n > a->nalloc) {

        /* the array is full */

        p = a->pool;

        if ((u_char *) a->elts + a->size * a->nalloc == p->d.last
            && p->d.last + size <= p->d.end)
        {
            /*
             * the array allocation is the last in the pool
             * and there is space for new allocation
             */

            p->d.last += size;
            a->nalloc += n;

        } else {
            /* allocate a new array */

            nalloc = 2 * ((n >= a->nalloc) ? n : a->nalloc);

            new = uct_palloc(p, nalloc * a->size);
            if (new == NULL) {
                return NULL;
            }

            uct_memcpy(new, a->elts, a->nelts * a->size);
            a->elts = new;
            a->nalloc = nalloc;
        }
    }

    elt = (u_char *) a->elts + a->size * a->nelts;
    a->nelts += n;

    return elt;
}
