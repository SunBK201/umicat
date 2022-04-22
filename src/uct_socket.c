/*
 * Copyright (C) SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

uct_socket_t
uct_socket(int domain, int type, int protocol, uct_cycle_t *cycle)
{
    uct_socket_t rc;

    if ((rc = socket(domain, type, protocol)) < 0)
        uct_log(cycle->log, UCT_LOG_ERROR, "socket error");
    return rc;
}

void
uct_setsockopt(uct_socket_t s, int level, int optname, const void *optval,
    int optlen, uct_cycle_t *cycle)
{
    int rc;

    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
        uct_log(cycle->log, UCT_LOG_ERROR, "setsockopt error");
}

int
uct_bind(uct_socket_t sockfd, struct sockaddr *my_addr, int addrlen,
    uct_cycle_t *cycle)
{
    int rc;

    if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
        uct_log(cycle->log, UCT_LOG_ERROR, "bind error: %d", errno);
    return rc;
}

int
uct_listen(int s, int backlog, uct_cycle_t *cycle)
{
    int rc;

    if ((rc = listen(s, backlog)) < 0)
        uct_log(cycle->log, UCT_LOG_ERROR, "listen error");
    return rc;
}

uct_socket_t
uct_accept(uct_socket_t s, struct sockaddr *addr, socklen_t *addrlen,
    uct_cycle_t *cycle)
{
    uct_socket_t rc;

again:
#if (UCT_HAVE_ACCEPT4)
    rc = accept4(s, addr, addrlen, SOCK_NONBLOCK);
#else
    rc = accept(s, addr, addrlen);
#endif
    if (rc < 0) {
        if ((errno == ECONNABORTED) || (errno == EINTR)) {
            goto again;
        } else if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
            goto again;
        } else {
            uct_log(cycle->log, UCT_LOG_ERROR, "accept error");
            return UCT_ERROR;
        }
    }
    return rc;
}

int
uct_connect(uct_socket_t sockfd, struct sockaddr *serv_addr, int addrlen,
    uct_cycle_t *cycle)
{
    int rc;

    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
        uct_log(cycle->log, UCT_LOG_ERROR, "Connect error: %s",
            strerror(errno));
    return rc;
}

uct_int_t
uct_getaddrinfo(const char *hostname, const char *service,
    const struct addrinfo *hints, struct addrinfo **res, uct_cycle_t *cycle)
{
    int rc;

    if ((rc = getaddrinfo(hostname, service, hints, res)) != 0) {
        uct_log(cycle->log, UCT_LOG_ERROR, "getaddrinfo error: %s",
            gai_strerror(rc));
        return UCT_ERROR;
    }
    return UCT_OK;
}

uct_int_t
uct_getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host,
    size_t hostlen, char *serv, size_t servlen, int flags, uct_cycle_t *cycle)
{
    int rc;

    if ((rc = getnameinfo(sa, salen, host, hostlen, serv, servlen, flags)) !=
        0) {
        uct_log(cycle->log, UCT_LOG_ERROR, "getnameinfo error: %s",
            strerror(errno));
        return UCT_ERROR;
    }
    return UCT_OK;
}

void
uct_freeaddrinfo(struct addrinfo *res)
{
    freeaddrinfo(res);
}

void
uct_inet_ntop(int af, const void *src, char *dst, socklen_t size,
    uct_cycle_t *cycle)
{
    if (!inet_ntop(af, src, dst, size))
        uct_log(cycle->log, UCT_LOG_ERROR, "inet_ntop error");
}

void
uct_inet_pton(int af, const char *src, void *dst, uct_cycle_t *cycle)
{
    int rc;

    rc = inet_pton(af, src, dst);
    if (rc == 0)
        uct_log(cycle->log, UCT_LOG_ERROR,
            "inet_pton error: invalid dotted-decimal address");
    else if (rc < 0)
        uct_log(cycle->log, UCT_LOG_ERROR, "inet_pton error");
}

uct_int_t
uct_open_listenfd(char *port, uct_cycle_t *cycle)
{
    struct addrinfo hints, *listp, *p;
    uct_socket_t listenfd;
    int optval = 1;

    /* Get a list of potential server addresses */
    uct_memzero(&hints, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;             /* Accept connections */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* ... on any IP address */
    hints.ai_flags |= AI_NUMERICSERV;            /* ... using port number */
    uct_getaddrinfo(NULL, port, &hints, &listp, cycle);

    /* Walk the list for one that we can bind to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((listenfd = uct_socket(p->ai_family, p->ai_socktype, p->ai_protocol,
                 cycle)) < 0)
            continue; /* Socket failed, try the next */

        /* Eliminates "Address already in use" error from bind */
        uct_setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
            (const void *)&optval, sizeof(optval), cycle);

        uct_setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT,
            (const void *)&optval, sizeof(optval), cycle);

        /* Bind the descriptor to the address */
        if (uct_bind(listenfd, p->ai_addr, p->ai_addrlen, cycle) == 0)
            break;                  /* Success */
        uct_close_socket(listenfd); /* Bind failed, try the next */
    }

    /* Clean up */
    uct_freeaddrinfo(listp);
    if (!p) /* No address worked */
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (uct_listen(listenfd, UCT_LISTEN_BACKLOG, cycle) < 0) {
        uct_close_socket(listenfd);
        return -1;
    }
    return listenfd;
}

uct_int_t
uct_open_listenfd_udp(char *port, uct_cycle_t *cycle)
{
    struct addrinfo hints, *listp, *p;
    uct_socket_t listenfd;
    int optval = 1;

    /* Get a list of potential server addresses */
    uct_memzero(&hints, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* ... on any IP address */
    hints.ai_flags |= AI_NUMERICSERV;            /* ... using port number */
    uct_getaddrinfo(NULL, port, &hints, &listp, cycle);

    /* Walk the list for one that we can bind to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((listenfd = uct_socket(p->ai_family, p->ai_socktype, p->ai_protocol,
                 cycle)) < 0)
            continue; /* Socket failed, try the next */

        /* Eliminates "Address already in use" error from bind */
        uct_setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
            (const void *)&optval, sizeof(optval), cycle);

        uct_setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT,
            (const void *)&optval, sizeof(optval), cycle);

        /* Bind the descriptor to the address */
        if (uct_bind(listenfd, p->ai_addr, p->ai_addrlen, cycle) == 0)
            break;                  /* Success */
        uct_close_socket(listenfd); /* Bind failed, try the next */
    }

    /* Clean up */
    uct_freeaddrinfo(listp);
    if (!p) /* No address worked */
        return -1;

    return listenfd;
}

uct_int_t
uct_open_clientfd(char *hostname, char *port, uct_cycle_t *cycle)
{
    int clientfd;
    struct addrinfo hints, *listp, *p;

    /* Get a list of potential server addresses */
    uct_memzero(&hints, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV; /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG; /* Recommended for connections */
    uct_getaddrinfo(hostname, port, &hints, &listp, cycle);

    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((clientfd = uct_socket(p->ai_family, p->ai_socktype, p->ai_protocol,
                 cycle)) < 0)
            continue; /* Socket failed, try the next */

        /* Connect to the server */
        if (uct_connect(clientfd, p->ai_addr, p->ai_addrlen, cycle) != -1)
            break;                  /* Success */
        uct_close_socket(clientfd); /* Connect failed, try another */
    }

    /* Clean up */
    uct_freeaddrinfo(listp);
    if (!p) /* All connects failed */
        return -1;
    else /* The last connect succeeded */
        return clientfd;
}

uct_int_t
uct_open_clientfd_udp(char *hostname, char *port, uct_cycle_t *cycle)
{
    int clientfd;
    struct addrinfo hints, *listp, *p;

    /* Get a list of potential server addresses */
    uct_memzero(&hints, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV; /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG; /* Recommended for connections */
    uct_getaddrinfo(hostname, port, &hints, &listp, cycle);

    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((clientfd = uct_socket(p->ai_family, p->ai_socktype, p->ai_protocol,
                 cycle)) < 0)
            continue; /* Socket failed, try the next */
        break;                  /* Success */
    }

    /* Clean up */
    uct_freeaddrinfo(listp);
    if (!p) /* All connects failed */
        return -1;
    else /* The last connect succeeded */
        return clientfd;
}

ssize_t
uct_readn(uct_socket_t fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
        errno = 0;
        if ((nread = read(fd, bufp, nleft)) < 0) {
            if (errno == EINTR) /* Interrupted by sig handler return */
                nread = 0;      /* and call read() again */
            else if (errno == EWOULDBLOCK || errno == EAGAIN) {
                nread = 0;
                break;
            } else
                return -1; /* errno set by read() */
        } else if (nread == 0)
            break; /* EOF */
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft); /* Return >= 0 */
}

ssize_t
uct_writen(uct_socket_t fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR) /* Interrupted by sig handler return */
                nwritten = 0;   /* and call write() again */
            else if (errno == EWOULDBLOCK || errno == EAGAIN) {
                nwritten = 0;
            } else
                return -1; /* errno set by write() */
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}

int
uct_nonblocking(uct_socket_t s)
{
    int  nb;

    nb = 1;

    return ioctl(s, FIONBIO, &nb);
}


int
uct_blocking(uct_socket_t s)
{
    int  nb;

    nb = 0;

    return ioctl(s, FIONBIO, &nb);
}