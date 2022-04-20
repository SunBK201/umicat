/*
 * Copyright (C) 2021 SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

int
uct_copy_file(const char *to, const char *from)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return UCT_ERROR;

    fd_to = open(to, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            } else if (errno != EINTR) {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0) {
        if (close(fd_to) < 0) {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return UCT_OK;
    }

out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return UCT_ERROR;
}

uct_int_t
uct_same_file(const char *filename1, const char *filename2)
{
    struct stat stat1, stat2;

    if (stat(filename1, &stat1) < 0)
        return UCT_ERROR;
    if (stat(filename2, &stat2) < 0) {
        if (errno == ENOENT)
            return UCT_OK;
        else
            return UCT_ERROR;
    }

    return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}