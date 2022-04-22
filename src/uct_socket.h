/*
 * Copyright (C) SunBK201
 */

#ifndef _UCT_SOCKET_H_INCLUDED_
#define _UCT_SOCKET_H_INCLUDED_

#include <uct_config.h>
#include <uct_core.h>

#ifndef UCT_HAVE_ACCEPT4
#define UCT_HAVE_ACCEPT4 1
#endif

typedef int uct_socket_t;

#define UCT_INET_ADDRSTRLEN (sizeof("255.255.255.255"))
#define UCT_INET6_ADDRSTRLEN                                                   \
    (sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"))
#define UCT_UNIX_ADDRSTRLEN                                                    \
    (sizeof("unix:") + sizeof(struct sockaddr_un) -                            \
        offsetof(struct sockaddr_un, sun_path))

#define UCT_INET_PORTSTRLEN (sizeof("65535"))

#define UCT_LISTEN_BACKLOG  (1024 * 4)

int uct_socket(int domain, int type, int protocol, uct_cycle_t *cycle);
void uct_setsockopt(int s, int level, int optname, const void *optval,
    int optlen, uct_cycle_t *cycle);
int uct_bind(int sockfd, struct sockaddr *my_addr, int addrlen,
    uct_cycle_t *cycle);
int uct_listen(int s, int backlog, uct_cycle_t *cycle);
int uct_accept(int s, struct sockaddr *addr, socklen_t *addrlen,
    uct_cycle_t *cycle);
int uct_connect(int sockfd, struct sockaddr *serv_addr, int addrlen,
    uct_cycle_t *cycle);
uct_int_t uct_getaddrinfo(const char *hostname, const char *service,
    const struct addrinfo *hints, struct addrinfo **res, uct_cycle_t *cycle);
uct_int_t uct_getnameinfo(const struct sockaddr *sa, socklen_t salen,
    char *host, size_t hostlen, char *serv, size_t servlen, int flags,
    uct_cycle_t *cycle);
void uct_freeaddrinfo(struct addrinfo *res);
void uct_inet_ntop(int af, const void *src, char *dst, socklen_t size,
    uct_cycle_t *cycle);
void uct_inet_pton(int af, const char *src, void *dst, uct_cycle_t *cycle);
uct_int_t uct_open_listenfd(char *port, uct_cycle_t *cycle);
uct_int_t uct_open_listenfd_udp(char *port, uct_cycle_t *cycle);
uct_int_t uct_open_clientfd(char *hostname, char *port, uct_cycle_t *cycle);
uct_int_t uct_open_clientfd_udp(char *hostname, char *port, uct_cycle_t *cycle);
ssize_t uct_readn(uct_socket_t fd, void *usrbuf, size_t n);
ssize_t uct_writen(uct_socket_t fd, void *usrbuf, size_t n);
int uct_nonblocking(uct_socket_t s);
int uct_blocking(uct_socket_t s);

#define uct_close_socket close

#endif /* _UCT_SOCKET_H_INCLUDED_ */
