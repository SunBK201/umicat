/*
 * Copyright (C) SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

/*
 * find the middle queue element if the queue has odd number of elements
 * or the first element of the queue's second part otherwise
 */
/*
 * 每次middle向后移动一步，next向后移动两步，
 * 这样next指到队尾的时候，middle就指到了中间，时间复杂度O(N)
 */
uct_queue_t *
uct_queue_middle(uct_queue_t *queue)
{
    uct_queue_t  *middle, *next;

    middle = uct_queue_head(queue);

    if (middle == uct_queue_last(queue)) {
        return middle;
    }

    next = uct_queue_head(queue);

    for ( ;; ) {
        middle = uct_queue_next(middle);

        next = uct_queue_next(next);

        if (next == uct_queue_last(queue)) {
            return middle;
        }

        next = uct_queue_next(next);

        if (next == uct_queue_last(queue)) {
            return middle;
        }
    }
}


/* the stable insertion sort */
/*
 * 使用插入排序算法对queue队列进行排序
 * 完成后在next方向上为升序，prev方向为降序
 */
void
uct_queue_sort(uct_queue_t *queue,
    uct_int_t (*cmp)(const uct_queue_t *, const uct_queue_t *))
{
    uct_queue_t  *q, *prev, *next;

    q = uct_queue_head(queue);

    if (q == uct_queue_last(queue)) {
        return;
    }

    for (q = uct_queue_next(q); q != uct_queue_sentinel(queue); q = next) {

        prev = uct_queue_prev(q);
        next = uct_queue_next(q);

        uct_queue_remove(q);

        do {
            if (cmp(prev, q) <= 0) {
                break;
            }

            prev = uct_queue_prev(prev);

        } while (prev != uct_queue_sentinel(queue));

        uct_queue_insert_after(prev, q);
    }
}
