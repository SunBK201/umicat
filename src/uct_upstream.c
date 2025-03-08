/*
 * Copyright (C) SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

static uct_uint_t uct_upstream_round_robin_get(uct_array_t *srvs, uct_uint_t n,
    sem_t *mutex, uct_log_t *log);
static uct_connection_t *uct_upstream_round_robin_get_conn(
    uct_cycle_t *wk_cycle);
static uct_uint_t uct_upstream_ip_hash_get(uct_array_t *srvs, uct_uint_t n,
    sem_t *mutex, uct_log_t *log, uct_connection_t *client);
static uct_connection_t *uct_upstream_ip_hash_get_conn(uct_cycle_t *wk_cycle,
    uct_connection_t *client);
static uct_uint_t uct_upstream_least_conn_get(uct_array_t *srvs, uct_uint_t n,
    sem_t *mutex, uct_log_t *log);
static uct_connection_t *uct_upstream_least_conn_get_conn(
    uct_cycle_t *wk_cycle);
static uct_uint_t uct_upstream_random_get(uct_array_t *srvs, uct_uint_t n);
static uct_connection_t * uct_upstream_random_get_conn(uct_cycle_t *wk_cycle);
static uct_inline uct_int_t power(uct_int_t i, uct_int_t j);

uct_array_t *srvs;
uct_uint_t srvs_n;

uct_connection_t *
uct_upstream_get_connetion(uct_cycle_t *wk_cycle, uct_connection_t *client)
{
    uct_connection_t *conn;

    if (wk_cycle->master->policy == UCT_ROUND_ROBIN) {
        conn = uct_upstream_round_robin_get_conn(wk_cycle);
    } else if (wk_cycle->master->policy == UCT_IP_HASH) {
        conn = uct_upstream_ip_hash_get_conn(wk_cycle, client);
    } else if (wk_cycle->master->policy == UCT_LEAST_CONN) {
        conn = uct_upstream_least_conn_get_conn(wk_cycle);
    } else if (wk_cycle->master->policy == UCT_RANDOM) {
        conn = uct_upstream_random_get_conn(wk_cycle);
    } else {
        uct_log(wk_cycle->log, UCT_LOG_ERROR, "upstream mode error, mode: %d",
            wk_cycle->master->policy);
        return NULL;
    }
    if (conn == NULL) {
        return NULL;
    }
    uct_log(wk_cycle->log, UCT_LOG_DEBUG, "get upstream connection: %s:%s",
        conn->ip_text, conn->port_text);

    if(wk_cycle->mode == UCT_TCP_MODE)
        uct_nonblocking(conn->fd);

    return conn;
}


uct_int_t
uct_upstream_connect(uct_upstream_srv_t *srv, uct_cycle_t *cycle) {
    uct_socket_t fd;
    time_t now;
    char *upstream_port;

    upstream_port = uct_pnalloc(cycle->pool, UCT_INET_PORTSTRLEN);
    uct_itoa(srv->upstream_port, upstream_port, 10);
    if (cycle->mode == UCT_TCP_MODE) {
        fd = uct_open_clientfd(srv->upstream_ip, upstream_port, cycle);
    } else if (cycle->mode == UCT_UDP_MODE) {
        fd = uct_open_clientfd_udp(srv->upstream_ip, upstream_port, cycle);
    } else {
        uct_log(cycle->log, UCT_LOG_ERROR, "upstream mode error, mode: %d",
            cycle->mode);
        return -1;
    }
    if (fd <= 0) {
        uct_log(cycle->log, UCT_LOG_ERROR,
            "open upstream connection error, ip: %s, port: %d",
            srv->upstream_ip, srv->upstream_port);
            
        if (pthread_spin_lock(&srv->lock) != UCT_OK) {
            return -1;
        }
        if (srv->max_fails) {
            srv->effective_weight -= srv->weight / srv->max_fails;
        }
        if (srv->effective_weight < 0) {
            srv->effective_weight = 0;
        }
        
        now = time(NULL);
        if (now - srv->last_fail_time > srv->fail_timeout) {
            srv->last_fail_time = now;
            srv->fails = 1;
        } else {
            srv->fails++;
        }
        if (srv->fails && srv->fails >= srv->max_fails && now - srv->last_fail_time <= srv->fail_timeout) {
            srv->is_down = true;
            srv->effective_weight = 0;
            srv->last_fail_time = now;
            uct_log(cycle->log, UCT_LOG_ERROR,
                "upstream server %s:%d temporarily down", srv->upstream_ip, srv->upstream_port);
        }

        pthread_spin_unlock(&srv->lock);
        return -1;
    } else {
        if (pthread_spin_lock(&srv->lock) != UCT_OK) {
            return -1;
        }
        if (srv->is_down) {
            srv->is_down = false;
            srv->fails = 0;
            uct_log(cycle->log, UCT_LOG_INFO,
                "upstream server %s:%d up", srv->upstream_ip, srv->upstream_port);
        }
        if (srv->effective_weight < srv->weight) {
            srv->effective_weight++;
        }
        srv->connection_n++;
        pthread_spin_unlock(&srv->lock);
    }
    return fd;
}


static uct_uint_t
uct_upstream_round_robin_get(uct_array_t *srvs, uct_uint_t n, sem_t *mutex,
    uct_log_t *log)
{
    uct_int_t total_weight;
    uct_uint_t i;
    uct_upstream_srv_t *srv;
    uct_int_t best;
    uct_int_t max;
    time_t now;

    best = 0;
    max = 0;
    total_weight = 0;
    srv = srvs->elts;

    uct_lock_p(mutex, log);
    now = time(NULL);
    for (i = 0; i < n; i++) {
        if (srv[i].is_down && srv[i].is_fallback && now - srv[i].last_fail_time < srv[i].fail_timeout) {
            continue;
        }
        if (now - srv[i].last_fail_time > srv[i].fail_timeout && srv[i].is_down && srv[i].effective_weight == 0) {
            srv[i].effective_weight += 1;
        }
        total_weight += srv[i].effective_weight;
        srv[i].current_weight += srv[i].effective_weight;
    }

    for (i = 0; i < n; i++) {
        if (srv[i].is_down && srv[i].is_fallback && now - srv[i].last_fail_time < srv[i].fail_timeout) {
            continue;
        }
        if (srv[i].current_weight > max) {
            max = srv[i].current_weight;
            best = i;
        }
    }
    srv[best].current_weight -= total_weight;

    uct_lock_v(mutex, log);

    return best;
}

static uct_connection_t *
uct_upstream_round_robin_get_conn(uct_cycle_t *wk_cycle)
{
    uct_socket_t fd;
    uct_connection_t *conn;
    uct_cycle_t *cycle;
    uct_upstream_srv_t *srv;
    char *upstream_port;
    uct_uint_t best;

    cycle = wk_cycle->master;
    best = uct_upstream_round_robin_get(cycle->srvs, cycle->srvs_n,
        &cycle->mutex, cycle->log);
    srv = (uct_upstream_srv_t *)uct_array_loc(cycle->srvs, best);

    upstream_port = uct_pnalloc(wk_cycle->pool, UCT_INET_PORTSTRLEN);
    uct_itoa(srv->upstream_port, upstream_port, 10);

    fd = uct_upstream_connect(srv, wk_cycle);
    if (fd <= 0) {
        return NULL;
    }

    conn = uct_connection_init(wk_cycle, fd);
    conn->ip_text = srv->upstream_ip;
    conn->port_text = uct_itoa(srv->upstream_port, upstream_port, 10);
    conn->upstream_srv = srv;

    return conn;
}

static uct_inline uct_int_t
power(uct_int_t x, uct_int_t n)
{
    if (n == 0)
        return 1;
    uct_int_t res = power(x, n / 2);
    if (n % 2 != 0)
        return res * res * x;
    else
        return res * res;
}

static uct_uint_t
uct_upstream_ip_hash_get(uct_array_t *srvs, uct_uint_t n, sem_t *mutex,
    uct_log_t *log, uct_connection_t *client)
{
    uct_int_t hash;
    uct_int_t best;
    uct_int_t i;
    uct_int_t len;

    len = uct_strlen(client->ip_text);
    hash = 0;
    for (i = 0; i < len; i++) {
        hash = hash + client->ip_text[0] * power(13, i);
    }
    best = hash % n;

    return best;
}

static uct_connection_t *
uct_upstream_ip_hash_get_conn(uct_cycle_t *wk_cycle, uct_connection_t *client)
{
    uct_socket_t fd;
    uct_connection_t *conn;
    uct_cycle_t *cycle;
    uct_upstream_srv_t *srv;
    char *upstream_port;
    uct_uint_t best;

    cycle = wk_cycle->master;
    best = uct_upstream_ip_hash_get(cycle->srvs, cycle->srvs_n, &cycle->mutex,
        cycle->log, client);
    srv = (uct_upstream_srv_t *)uct_array_loc(cycle->srvs, best);

    upstream_port = uct_pnalloc(wk_cycle->pool, UCT_INET_PORTSTRLEN);
    uct_itoa(srv->upstream_port, upstream_port, 10);

    fd = uct_upstream_connect(srv, wk_cycle);
    if (fd <= 0) {
        return NULL;
    }

    conn = uct_connection_init(wk_cycle, fd);
    conn->ip_text = srv->upstream_ip;
    conn->port_text = uct_itoa(srv->upstream_port, upstream_port, 10);
    conn->upstream_srv = srv;

    return conn;
}

static uct_uint_t
uct_upstream_least_conn_get(uct_array_t *srvs, uct_uint_t n, sem_t *mutex,
    uct_log_t *log)
{
    size_t i;
    uct_uint_t best;
    uct_int_t min;
    uct_upstream_srv_t *srv;
    time_t now;

    best = 0;
    srv = srvs->elts;

    uct_lock_p(mutex, log);

    min = UCT_MAX_INT_T_VALUE;
    now = time(NULL);
    for (i = 0; i < n; i++) {
        if (srv[i].is_down && srv[i].is_fallback && now - srv[i].last_fail_time < srv[i].fail_timeout) {
            continue;
        }
        if (srv[i].connection_n < min) {
            min = srv[i].connection_n;
            best = i;
        }
    }
    srv[best].connection_n++;
    uct_lock_v(mutex, log);

    return best;
}

static uct_connection_t *
uct_upstream_least_conn_get_conn(uct_cycle_t *wk_cycle)
{
    uct_socket_t fd;
    uct_connection_t *conn;
    uct_cycle_t *cycle;
    char *upstream_port;
    uct_uint_t best;
    uct_upstream_srv_t *srv;

    cycle = wk_cycle->master;
    best = uct_upstream_least_conn_get(cycle->srvs, cycle->srvs_n,
        &cycle->mutex, cycle->log);
    srv = (uct_upstream_srv_t *)uct_array_loc(cycle->srvs, best);

    upstream_port = uct_pnalloc(wk_cycle->pool, UCT_INET_PORTSTRLEN);
    uct_itoa(srv->upstream_port, upstream_port, 10);

    fd = uct_upstream_connect(srv, wk_cycle);
    if (fd <= 0) {
        return NULL;
    }

    conn = uct_connection_init(wk_cycle, fd);
    conn->ip_text = srv->upstream_ip;
    conn->port_text = uct_itoa(srv->upstream_port, upstream_port, 10);
    conn->upstream_srv = srv;

    return conn;
}

static uct_uint_t 
uct_upstream_random_get(uct_array_t *srvs, uct_uint_t n)
{
    uct_int_t total_weight;
    uct_uint_t i;
    uct_upstream_srv_t *srv;
    uct_int_t best;
    uct_int_t randnum;
    struct timespec seed;
    time_t now;

    best = 0;
    total_weight = 0;
    srv = srvs->elts;

    now = time(NULL);
    for (i = 0; i < n; i++) {
        if (srv[i].is_down && srv[i].is_fallback && now - srv[i].last_fail_time < srv[i].fail_timeout) {
            continue;
        }
        if (now - srv[i].last_fail_time > srv[i].fail_timeout && srv[i].is_down && srv[i].effective_weight == 0) {
            srv[i].effective_weight += 1;
        }
        total_weight += srv[i].effective_weight;
    }

    clock_gettime(CLOCK_REALTIME, &seed);
    srand(seed.tv_sec + seed.tv_nsec);
    randnum = rand() % total_weight;

    now = time(NULL);
    for (i = 0; i < n; i++) {
        if (srv[i].is_down && srv[i].is_fallback && now - srv[i].last_fail_time < srv[i].fail_timeout) {
            continue;
        }
        if (randnum < srv[i].effective_weight) {
            best = i;
            break;
        }
        randnum -= srv[i].effective_weight;
    }

    return best;
}

static uct_connection_t * 
uct_upstream_random_get_conn(uct_cycle_t *wk_cycle)
{
    uct_socket_t fd;
    uct_connection_t *conn;
    uct_cycle_t *cycle;
    uct_upstream_srv_t *srv;
    char *upstream_port;
    uct_uint_t best;

    cycle = wk_cycle->master;
    best = uct_upstream_random_get(cycle->srvs, cycle->srvs_n);
    srv = (uct_upstream_srv_t *)uct_array_loc(cycle->srvs, best);

    upstream_port = uct_pnalloc(wk_cycle->pool, UCT_INET_PORTSTRLEN);
    uct_itoa(srv->upstream_port, upstream_port, 10);

    fd = uct_upstream_connect(srv, wk_cycle);
    if (fd <= 0) {
        return NULL;
    }

    conn = uct_connection_init(wk_cycle, fd);
    conn->ip_text = srv->upstream_ip;
    conn->port_text = uct_itoa(srv->upstream_port, upstream_port, 10);
    conn->upstream_srv = srv;

    return conn;
}