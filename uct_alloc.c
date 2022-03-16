/*
 * Copyright (C) 2021 SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

uct_uint_t uct_pagesize;
uct_uint_t uct_pagesize_shift;
uct_uint_t uct_cacheline_size;

void *
uct_alloc(size_t size, uct_log_t *log)
{
	void *p;

	p = malloc(size);
	if (p == NULL) {
		uct_log(log, UCT_LOG_ERROR, "malloc(%u) failed", size);
	}

	uct_log(log, UCT_LOG_DEBUG, "malloc: %p:%u", p, size);

	return p;
}

void *
uct_calloc(size_t size, uct_log_t *log)
{
	void *p;

	p = uct_alloc(size, log);

	if (p) {
		uct_memzero(p, size);
	}

	return p;
}

void *
uct_memalign(size_t alignment, size_t size, uct_log_t *log)
{
	void *p;
	int err;

	err = posix_memalign(&p, alignment, size);

	if (err) {
		uct_log(log, UCT_LOG_ERROR, "posix_memalign(%u, %u) failed", alignment, size);
		p = NULL;
	}

	uct_log(log, UCT_LOG_DEBUG, "posix_memalign: %p:%u @%u", p, size,
	    alignment);

	return p;
}