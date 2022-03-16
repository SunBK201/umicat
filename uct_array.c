/*
 * Copyright (C) 2021 SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

/**
 * 创建并初始化一个数组
 * p:内存池容器
 * n:支持多少个数组元素
 * size:每个元素的大小
 */
uct_array_t *
uct_array_create(uct_pool_t *p, uct_uint_t n, size_t size)
{
    uct_array_t *a;

    /* 在内存池 pool上面 分配一段内存给 uct_array 数据结构*/
    a = uct_palloc(p, sizeof(uct_array_t));
    if (a == NULL) {
        return NULL;
    }

    /* 数组初始化，并且分配内存空间给数组元素 */
    if (uct_array_init(a, p, n, size) != UCT_OK) {
        return NULL;
    }

    return a;
}

/**
 * 数组销毁
 */
void
uct_array_destroy(uct_array_t *a)
{
    uct_pool_t  *p;

    p = a->pool;

    /**
     * 如果数组元素的末尾地址和内存池 pool 的可用开始的地址相同
     * 则将内存池 pool->d.last 移动到数组元素的开始地址，相当于清除当前数组的内容
     */
    if ((u_char *) a->elts + a->size * a->nalloc == p->d.last) {
        p->d.last -= a->size * a->nalloc;
    }

    /**
     * 如果数组的数据结构uct_array_t的末尾地址和内存池pool的可用开始地址相同
     * 则将内存池pool->d.last移动到数组元素的开始地址，相当于清除当前数组的内容
     */
    if ((u_char *) a + sizeof(uct_array_t) == p->d.last) {
        p->d.last = (u_char *) a;
    }
}

/**
 * 添加一个元素，并返回新增元素的地址
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
            /**
             * 如果当前内存池中剩余的空间大于或者等于本次需要新增的空间，
             * 那么本次扩容将只在当前内存池上扩容
             */
            p->d.last += a->size;
            a->nalloc++;

        } else {
            /* allocate a new array */

            /** 
             * 如果扩容的大小超出了当前内存池剩余的容量
             * 或者数组元素的末尾和内存池pool的可用开始的地址不相同
             * 则需要重新分配一个新的内存块存储数组，并且将原数组拷贝到新的地址上
             */

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
 * 添加n个元素
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
