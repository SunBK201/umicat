/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_FILE_H_INCLUDED_
#define _UCT_FILE_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

#define UCT_INVALID_FILE         -1
#define UCT_FILE_ERROR           -1

#define uct_stdout               STDOUT_FILENO
#define uct_stderr               STDERR_FILENO

typedef int                      uct_fd_t;
typedef struct stat              uct_file_info_t;

struct uct_file_s {
    uct_fd_t                   fd;
    uct_str_t                  name;
    uct_file_info_t            info;

    off_t                      offset;
    off_t                      sys_offset;

    uct_log_t                 *log;

    unsigned                   valid_info:1;
    unsigned                   directio:1;
};

static uct_inline ssize_t
uct_write_fd(uct_fd_t fd, void *buf, size_t n)
{
    return write(fd, buf, n);
}

static uct_inline void
uct_write_stderr(char *text)
{
    (void) uct_write_fd(uct_stderr, text, uct_strlen(text));
}


static uct_inline void
uct_write_stdout(char *text)
{
    (void) uct_write_fd(uct_stdout, text, uct_strlen(text));
}

#define uct_close_file           close
#define uct_close_file_n         "close()"

#define uct_delete_file(name)    unlink((const char *) name)
#define uct_delete_file_n        "unlink()"

int uct_copy_file(const char *to, const char *from);
uct_int_t uct_same_file(const char *filename1, const char *filename2);

#endif /* _UCT_FILE_H_INCLUDED_ */