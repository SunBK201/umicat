/*
 * Copyright (C) 2021 SunBK201
 */

#ifndef _UCT_LOG_H_INCLUDED_
#define _UCT_LOG_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

#define UCT_LOG_DEBUG  0
#define UCT_LOG_INFO   1
#define UCT_LOG_NOTICE 2
#define UCT_LOG_WARN   3
#define UCT_LOG_ERROR  4
#define UCT_LOG_FATAL  5

typedef void (*lockfunc)(bool lock, void *data);

struct uct_log_s {
    int log_level;
    FILE *file;
    void *data;
    bool quiet;
    lockfunc lock;
};

struct uct_logev_s {
    int log_level;
    struct tm *time;
    void *data;
    const char *fmt;
    va_list ap;
};

uct_log_t *uct_log_init(uct_log_t *log);
void uct_log_set_lock(uct_log_t *log, lockfunc fn, void *udata);
void uct_log_set_level(uct_log_t *log, int level);
void uct_log_set_quiet(uct_log_t *log, bool enable);

void uct_log(uct_log_t *log, int level, const char *fmt, ...);

#endif /* _UCT_LOG_H_INCLUDED_ */