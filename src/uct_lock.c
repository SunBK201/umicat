/*
 * Copyright (C) SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

uct_int_t
uct_lock_init(sem_t *sem, int pshared, unsigned int value, uct_log_t *log)
{
    if (sem_init(sem, pshared, value) < 0) {
        uct_log(log, UCT_LOG_ERROR, "uct_lock_init error");
        return UCT_ERROR;
    }
    return UCT_OK;
}

uct_int_t
uct_lock_p(sem_t *sem, uct_log_t *log)
{
    if (sem_wait(sem) < 0) {
        uct_log(log, UCT_LOG_ERROR, "uct_lock_p error");
        return UCT_ERROR;
    }
    return UCT_OK;
}

uct_int_t
uct_trylock_p(sem_t *sem, uct_log_t *log)
{
    if (sem_trywait(sem) != 0) {
        if (errno == EAGAIN) {
            return UCT_DECLINED;
        } else {
            uct_log(log, UCT_LOG_ERROR, "uct_lock_p error");
            return UCT_ERROR;
        }
    }
    return UCT_OK;
}

void
uct_lock_v(sem_t *sem, uct_log_t *log)
{
    if (sem_post(sem) < 0)
        uct_log(log, UCT_LOG_ERROR, "uct_lock_v error");
}