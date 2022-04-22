/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_LOCK_H_INCLUDED_
#define _UCT_LOCK_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

uct_int_t uct_lock_init(sem_t *sem, int pshared, unsigned int value, uct_log_t *log);
uct_int_t uct_lock_p(sem_t *sem, uct_log_t *log);
uct_int_t uct_trylock_p(sem_t *sem, uct_log_t *log);
void uct_lock_v(sem_t *sem, uct_log_t *log);

#endif /* _UCT_LOCK_H_INCLUDED_ */
