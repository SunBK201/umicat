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
 * 初始化一个链表
 */
#define uct_queue_init(q)                                                     \
    (q)->prev = q;                                                            \
    (q)->next = q

/**
 * 判断链表是否为空
 */
#define uct_queue_empty(h)                                                    \
    (h == (h)->prev)

/**
 * 在链表头部插入x节点，即是在h之后插入x节点
 */
#define uct_queue_insert_head(h, x)                                           \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x

/* 在q节点之后插入x节点 */
#define uct_queue_insert_after   uct_queue_insert_head

/* 将x节点插入到链表尾部，也就是h的前面 */
#define uct_queue_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x

/* 获取链表的首个节点，也就是h的next指向的节点 */
#define uct_queue_head(h)                                                     \
    (h)->next

/* 获取尾节点，也就是h的prev指向的节点 */
#define uct_queue_last(h)                                                     \
    (h)->prev

/* 获取哨兵指针，也就是h */
#define uct_queue_sentinel(h)                                                 \
    (h)

/* 获取的q的下一节点 */
#define uct_queue_next(q)                                                     \
    (q)->next

/* 获取q的前一节点 */
#define uct_queue_prev(q)                                                     \
    (q)->prev


#if (UCT_DEBUG)

#define uct_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;                                              \
    (x)->prev = NULL;                                                         \
    (x)->next = NULL

#else

/* 将x节点从链表中删除 */
#define uct_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next

#endif


/* 将h链表从q节点拆分成h和n两个链表，n的首个节点是q */
#define uct_queue_split(h, q, n)                                              \
    (n)->prev = (h)->prev;                                                    \
    (n)->prev->next = n;                                                      \
    (n)->next = q;                                                            \
    (h)->prev = (q)->prev;                                                    \
    (h)->prev->next = h;                                                      \
    (q)->prev = n;

/* h链表和n链表合并，n追加到h链表的后面 */
#define uct_queue_add(h, n)                                                   \
    (h)->prev->next = (n)->next;                                              \
    (n)->next->prev = (h)->prev;                                              \
    (h)->prev = (n)->prev;                                                    \
    (h)->prev->next = h;

/* 获取数据结构体指针 */
#define uct_queue_data(q, type, link)                                         \
    (type *) ((u_char *) q - offsetof(type, link))


uct_queue_t *uct_queue_middle(uct_queue_t *queue);
void uct_queue_sort(uct_queue_t *queue,
    uct_int_t (*cmp)(const uct_queue_t *, const uct_queue_t *));

#endif /* _UCT_QUEUE_H_INCLUDED_ */
