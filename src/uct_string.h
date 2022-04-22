/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_STRING_H_INCLUDED_
#define _UCT_STRING_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

typedef struct {
    size_t      len;
    u_char     *data;
} uct_str_t;

typedef struct {
    uct_str_t   key;
    uct_str_t   value;
} uct_keyval_t;

/* 初始化一个字符串 */
#define uct_string(str)     { sizeof(str) - 1, (u_char *) str }
/* 定义变量时，使用uct_null_string初始化字符串为空字符串 */
#define uct_null_string     { 0, NULL }

/* 设置一个字符串 */
#define uct_str_set(str, text)                                               \
    (str)->len = sizeof(text) - 1; (str)->data = (u_char *) text
#define uct_str_null(str)   (str)->len = 0; (str)->data = NULL


#define uct_tolower(c)      (u_char) ((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)
#define uct_toupper(c)      (u_char) ((c >= 'a' && c <= 'z') ? (c & ~0x20) : c)

void uct_strlow(u_char *dst, u_char *src, size_t n);


#define uct_strncmp(s1, s2, n)  strncmp((const char *) s1, (const char *) s2, n)


/* msvc and icc7 compile strcmp() to inline loop */
#define uct_strcmp(s1, s2)  strcmp((const char *) s1, (const char *) s2)


#define uct_strstr(s1, s2)  strstr((const char *) s1, (const char *) s2)
#define uct_strlen(s)       strlen((const char *) s)

size_t uct_strnlen(u_char *p, size_t n);

#define uct_strchr(s1, c)   strchr((const char *) s1, (int) c)

static inline u_char *
uct_strlchr(u_char *p, u_char *last, u_char c)
{
    while (p < last) {

        if (*p == c) {
            return p;
        }

        p++;
    }

    return NULL;
}

#define uct_memzero(buf, n)       (void) memset(buf, 0, n)
#define uct_memset(buf, c, n)     (void) memset(buf, c, n)

#define uct_memcpy(dst, src, n)   (void) memcpy(dst, src, n)
#define uct_cpymem(dst, src, n)   (((u_char *) memcpy(dst, src, n)) + (n))

#define uct_memmove(dst, src, n)   (void) memmove(dst, src, n)
#define uct_movemem(dst, src, n)   (((u_char *) memmove(dst, src, n)) + (n))

#define uct_memcmp(s1, s2, n)  memcmp((const char *) s1, (const char *) s2, n)

u_char *uct_cpystrn(u_char *dst, u_char *src, size_t n);
u_char *uct_pstrdup(uct_pool_t *pool, uct_str_t *src);

uct_int_t uct_strcasecmp(u_char *s1, u_char *s2);
uct_int_t uct_strncasecmp(u_char *s1, u_char *s2, size_t n);

u_char *uct_strnstr(u_char *s1, char *s2, size_t n);

u_char *uct_strstrn(u_char *s1, char *s2, size_t n);
u_char *uct_strcasestrn(u_char *s1, char *s2, size_t n);
u_char *uct_strlcasestrn(u_char *s1, u_char *last, u_char *s2, size_t n);

uct_int_t uct_rstrncmp(u_char *s1, u_char *s2, size_t n);
uct_int_t uct_rstrncasecmp(u_char *s1, u_char *s2, size_t n);
uct_int_t uct_memn2cmp(u_char *s1, u_char *s2, size_t n1, size_t n2);
uct_int_t uct_dns_strcmp(u_char *s1, u_char *s2);
uct_int_t uct_filename_cmp(u_char *s1, u_char *s2, size_t n);

uct_int_t uct_atoi(u_char *line, size_t n);
char *uct_itoa(int num, char *str, int radix);
uct_int_t uct_atofp(u_char *line, size_t n, size_t point);
ssize_t uct_atosz(u_char *line, size_t n);
off_t uct_atoof(u_char *line, size_t n);
time_t uct_atotm(u_char *line, size_t n);
uct_int_t uct_hextoi(u_char *line, size_t n);

u_char *uct_hex_dump(u_char *dst, u_char *src, size_t len);


#define uct_base64_encoded_length(len)  (((len + 2) / 3) * 4)
#define uct_base64_decoded_length(len)  (((len + 3) / 4) * 3)

void uct_encode_base64(uct_str_t *dst, uct_str_t *src);
void uct_encode_base64url(uct_str_t *dst, uct_str_t *src);
uct_int_t uct_decode_base64(uct_str_t *dst, uct_str_t *src);
uct_int_t uct_decode_base64url(uct_str_t *dst, uct_str_t *src);

uint32_t uct_utf8_decode(u_char **p, size_t n);
size_t uct_utf8_length(u_char *p, size_t n);
u_char *uct_utf8_cpystrn(u_char *dst, u_char *src, size_t n, size_t len);

#define UCT_ESCAPE_URI            0
#define UCT_ESCAPE_ARGS           1
#define UCT_ESCAPE_URI_COMPONENT  2
#define UCT_ESCAPE_HTML           3
#define UCT_ESCAPE_REFRESH        4
#define UCT_ESCAPE_MEMCACHED      5
#define UCT_ESCAPE_MAIL_AUTH      6

#define UCT_UNESCAPE_URI       1
#define UCT_UNESCAPE_REDIRECT  2

uintptr_t uct_escape_uri(u_char *dst, u_char *src, size_t size,
    uct_uint_t type);
void uct_unescape_uri(u_char **dst, u_char **src, size_t size, uct_uint_t type);
uintptr_t uct_escape_html(u_char *dst, u_char *src, size_t size);
uintptr_t uct_escape_json(u_char *dst, u_char *src, size_t size);


void uct_sort(void *base, size_t n, size_t size,
    uct_int_t (*cmp)(const void *, const void *));
#define uct_qsort             qsort


#define uct_value_helper(n)   #n
#define uct_value(n)          uct_value_helper(n)

#endif /* _UCT_STRING_H_INCLUDED_ */