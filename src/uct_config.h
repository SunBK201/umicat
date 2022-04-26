/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_CONFIG_H_INCLUDED_
#define _UCT_CONFIG_H_INCLUDED_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <semaphore.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <libgen.h>

#define _XOPEN_SOURCE_EXTENDED 1

typedef intptr_t uct_int_t;
typedef uintptr_t uct_uint_t;
typedef unsigned char u_char;

#define uct_inline inline

/**
 * d is the number of aligned
 * a is the alignment width and must be a power of 2
 */
#define uct_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define uct_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#define UCT_ALIGNMENT   sizeof(unsigned long)    /* platform word */
#define UCT_BUF_SIZE    65535

#define UCT_OK		    0
#define UCT_ERROR	    -1
#define UCT_DECLINED    -2

#define UCT_LINEFEED    "\x0a"

typedef int         uct_err_t;
#define uct_errno errno

#define UCT_MAX_SIZE_T_VALUE  9223372036854775807LL
#define UCT_MAX_OFF_T_VALUE  9223372036854775807LL
#define UCT_MAX_TIME_T_VALUE  9223372036854775807LL
#define UCT_MAX_INT_T_VALUE  9223372036854775807

#ifndef UCT_CPU_CACHE_LINE
#define UCT_CPU_CACHE_LINE  64
#endif

#define LF		    (u_char)'\n'
#define CR		    (u_char)'\r'
#define CRLF		    "\r\n"

#define uct_abs(value)	    (((value) >= 0) ? (value) : -(value))
#define uct_max(val1, val2) ((val1 < val2) ? (val2) : (val1))
#define uct_min(val1, val2) ((val1 > val2) ? (val2) : (val1))

#endif /* _UCT_CONFIG_H_INCLUDED_ */
