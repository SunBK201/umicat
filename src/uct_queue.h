/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_QUEUE_H_INCLUDED_
#define _UCT_QUEUE_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

typedef struct uct_queue_s  uct_queue_t;

struct uct_queue_s {
    uct_queue_t  *prev;
    uct_queue_t  *next;
};

/**
 * initialize uct_queue_t
 */
#define uct_queue_init(q)                                                     \
    (q)->prev = q;                                                            \
    (q)->next = q


#define uct_queue_empty(h)                                                    \
    (h == (h)->prev)

/**
 * insert the x node after h
 */
#define uct_queue_insert_head(h, x)                                           \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x

#define uct_queue_insert_after   uct_queue_insert_head

/* insert the x node in front of the h */
#define uct_queue_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x

#define uct_queue_head(h)                                                     \
    (h)->next

#define uct_queue_last(h)                                                     \
    (h)->prev

#define uct_queue_sentinel(h)                                                 \
    (h)

#define uct_queue_next(q)                                                     \
    (q)->next

#define uct_queue_prev(q)                                                     \
    (q)->prev


#if (UCT_DEBUG)

#define uct_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;                                              \
    (x)->prev = NULL;                                                         \
    (x)->next = NULL

#else

#define uct_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next

#endif


#define uct_queue_split(h, q, n)                                              \
    (n)->prev = (h)->prev;                                                    \
    (n)->prev->next = n;                                                      \
    (n)->next = q;                                                            \
    (h)->prev = (q)->prev;                                                    \
    (h)->prev->next = h;                                                      \
    (q)->prev = n;

#define uct_queue_add(h, n)                                                   \
    (h)->prev->next = (n)->next;                                              \
    (n)->next->prev = (h)->prev;                                              \
    (h)->prev = (n)->prev;                                                    \
    (h)->prev->next = h;

#define uct_queue_data(q, type, link)                                         \
    (type *) ((u_char *) q - offsetof(type, link))


uct_queue_t *uct_queue_middle(uct_queue_t *queue);
void uct_queue_sort(uct_queue_t *queue,
    uct_int_t (*cmp)(const uct_queue_t *, const uct_queue_t *));

#endif /* _UCT_QUEUE_H_INCLUDED_ */
