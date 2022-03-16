/*
 * Copyright (C) 2021 SunBK201
 */

#ifndef _UCT_ALLOC_H_INCLUDED_
#define _UCT_ALLOC_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

void *uct_alloc(size_t size, uct_log_t *log);
void *uct_calloc(size_t size, uct_log_t *log);

#define uct_free          free

void *uct_memalign(size_t alignment, size_t size, uct_log_t *log);

extern uct_uint_t  uct_pagesize;
extern uct_uint_t  uct_pagesize_shift;
extern uct_uint_t  uct_cacheline_size;


#endif /* _UCT_ALLOC_H_INCLUDED_ */