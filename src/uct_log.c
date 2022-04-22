/*
 * Copyright (C) SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

static const char *uct_level_strings[] = {"DEBUG", "INFO", "NOTICE", "WARN",
    "ERROR", "FATAL"};

uct_log_t *
uct_log_init(uct_log_t *log)
{
    log->log_level = UCT_LOG_INFO;
    log->file = fopen("umicat.log", "a");
    log->quiet = false;
    log->data = NULL;
    log->lock = NULL;
    return log;
}

static void
uct_log_write_stdout(uct_logev_t *ev)
{
    char buf[16];

    buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
    fprintf(ev->data, "[%s][%s]: ", buf, uct_level_strings[ev->log_level]);
    vfprintf(ev->data, ev->fmt, ev->ap);
    fprintf(ev->data, "\n");
    fflush(ev->data);
}

static void
uct_log_write_file(uct_log_t *log, uct_logev_t *ev)
{
    char buf[64];
    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
    fprintf(log->file, "[%s][%s]: ", buf, uct_level_strings[ev->log_level]);
    vfprintf(log->file, ev->fmt, ev->ap);
    fprintf(log->file, "\n");
    fflush(log->file);
}

static void
lock(uct_log_t *log)
{
    if (log->lock) {
        log->lock(true, log->data);
    }
}

static void
unlock(uct_log_t *log)
{
    if (log->lock) {
        log->lock(false, log->data);
    }
}

void
uct_log_set_lock(uct_log_t *log, lockfunc fn, void *udata)
{
    log->lock = fn;
    log->data = udata;
}

void
uct_log_set_level(uct_log_t *log, int level)
{
    log->log_level = level;
}

void
uct_log_set_quiet(uct_log_t *log, bool enable)
{
    log->quiet = enable;
}

static void
uct_init_event(uct_logev_t *ev, void *data)
{
    if (!ev->time) {
        time_t t = time(NULL);
        ev->time = localtime(&t);
    }
    ev->data = data;
}

void
uct_log(uct_log_t *log, int level, const char *fmt, ...)
{
    uct_logev_t ev = {
        .log_level = level,
        .fmt = fmt,
    };

    lock(log);

    if (!log->quiet && level >= log->log_level) {
        uct_init_event(&ev, stderr);
        va_start(ev.ap, fmt);
        uct_log_write_stdout(&ev);
        va_end(ev.ap);
        va_start(ev.ap, fmt);
        uct_log_write_file(log, &ev);
        va_end(ev.ap);
    }

    unlock(log);
}