/*
 * Copyright (C) 2021 SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>

static void uct_start_worker_threads(uct_cycle_t *cycle, uct_int_t n);
static void *uct_worker_thread_cycle(void *cycle);
static uct_int_t uct_conf_parse(uct_cycle_t *cycle);
static uct_int_t uct_process_options(uct_cycle_t *cycle);

extern uct_array_t *srvs;
extern uct_uint_t srvs_n;
extern u_char *uct_conf_file;

uct_cycle_t *
uct_init_cycle(uct_log_t *log)
{
    uct_pool_t *pool;
    uct_cycle_t *cycle;
    uct_int_t n;

    pool = uct_create_pool(UCT_CYCLE_POOL_SIZE, log);
    if (pool == NULL) {
        return NULL;
    }
    pool->log = log;

    cycle = uct_pcalloc(pool, sizeof(uct_cycle_t));
    if (cycle == NULL) {
        uct_destroy_pool(pool);
        return NULL;
    }

    cycle->pool = pool;
    cycle->log = log;
    cycle->workers_n = 4;

    if(uct_process_options(cycle) != UCT_OK) {
        uct_destroy_pool(pool);
        return NULL;
    }

    if (cycle->conf_file == NULL) {
        cycle->conf_file = "conf/umicat.conf";
    }

    cycle->log_file = uct_palloc(cycle->pool, 64);

    cycle->srvs = uct_array_create(pool, 16, sizeof(uct_upstream_srv_t));
    srvs = cycle->srvs;

    if (uct_conf_parse(cycle) == UCT_ERROR) {
        return NULL;
    }

    n = uct_lock_init(&cycle->mutex, 0, 1, log);
    if (n != UCT_OK) {
        return NULL;
    }

    return cycle;
}

static uct_int_t
uct_conf_parse(uct_cycle_t *cycle)
{
    cJSON *root;
    cJSON *upstream;
    cJSON *uit;
    char *item;
    uct_upstream_srv_t *srv;
    size_t i;
    FILE *conf;
    char *buf;
    uct_file_info_t info;
    size_t size;
    uct_int_t n;

    stat(cycle->conf_file, &info);
    size = info.st_size;
    conf = fopen(cycle->conf_file, "r");
    buf = uct_palloc(cycle->pool, size);
    size = fread(buf, 1, size, conf);

    root = cJSON_Parse(buf);
    if (!root) {
        uct_log(cycle->log, UCT_ERROR, "json parse failed");
        return UCT_ERROR;
    } else {
        item = cJSON_GetObjectItem(root, "mode")->valuestring;
        if (!uct_strcmp(item, "tcp")) {
            cycle->mode = UCT_TCP_MODE;
        } else if (!uct_strcmp(item, "udp")) {
            cycle->mode = UCT_UDP_MODE;
        } else if (!uct_strcmp(item, "http")) {
            cycle->mode = UCT_HTTP_MODE;
        } else {
            uct_log(cycle->log, UCT_ERROR, "unknown mode: %s", item);
            return UCT_ERROR;
        }

        if ((n = cJSON_GetObjectItem(root, "localport")->valueint) > 0) {
            cycle->localport = cJSON_GetObjectItem(root, "localport")->valueint;
        } else {
            cycle->localport = 80;
        }

        item = cJSON_GetObjectItem(root, "policy")->valuestring;
        if (!uct_strcmp(item, "round_robin")) {
            cycle->policy = UCT_ROUND_ROBIN;
        } else if (!uct_strcmp(item, "ip_hash")) {
            cycle->policy = UCT_IP_HASH;
        } else if (!uct_strcmp(item, "least_conn")) {
            cycle->policy = UCT_LEAST_CONN;
        } else {
            uct_log(cycle->log, UCT_ERROR, "unknown policy: %s", item);
            return UCT_ERROR;
        }

        if ((n = cJSON_GetObjectItem(root, "workers")->valueint) > 0) {
            cycle->workers_n = cJSON_GetObjectItem(root, "workers")->valueint;
        } else {
            cycle->workers_n = 2;
        }

        item = cJSON_GetObjectItem(root, "log_file")->valuestring;
        uct_cpystrn(cycle->log_file, (u_char *)item, 64);

        item = cJSON_GetObjectItem(root, "log_level")->valuestring;
        if (!uct_strcmp(item, "debug")) {
            uct_log_set_level(cycle->log, UCT_LOG_DEBUG);
        } else if (!uct_strcmp(item, "info")) {
            uct_log_set_level(cycle->log, UCT_LOG_INFO);
        } else if (!uct_strcmp(item, "notice")) {
            uct_log_set_level(cycle->log, UCT_LOG_NOTICE);
        } else if (!uct_strcmp(item, "warn")) {
            uct_log_set_level(cycle->log, UCT_LOG_WARN);
        } else if (!uct_strcmp(item, "error")) {
            uct_log_set_level(cycle->log, UCT_LOG_ERROR);
        } else if (!uct_strcmp(item, "fatal")) {
            uct_log_set_level(cycle->log, UCT_LOG_FATAL);
        } else {
            uct_log_set_level(cycle->log, UCT_LOG_INFO);
        }

        upstream = cJSON_GetObjectItem(root, "upstream");
        if (upstream) {
            for (i = 0;; i++) {
                uit = cJSON_GetArrayItem(upstream, i);
                if (!uit) {
                    break;
                }
                srv = uct_array_push(cycle->srvs);

                item = cJSON_GetObjectItem(uit, "upstream_ip")->valuestring;
                srv->upstream_ip =
                    uct_pnalloc(cycle->pool, UCT_INET_ADDRSTRLEN * 4);
                uct_cpystrn((u_char *)srv->upstream_ip, (u_char *)item,
                    UCT_INET_ADDRSTRLEN * 4);

                srv->upstream_port =
                    cJSON_GetObjectItem(uit, "upstream_port")->valueint;

                srv->weight = cJSON_GetObjectItem(uit, "weight")->valueint;
                srv->effective_weight = srv->weight;
                srv->current_weight = 0;
                srv->connection_n = 0;
                uct_log(cycle->log, UCT_LOG_INFO,
                    "upstream server %s:%d, weight=%d", srv->upstream_ip,
                    srv->upstream_port, srv->weight);
            }
            cycle->srvs_n = i;
            srvs_n = i;
            uct_log(cycle->log, UCT_LOG_NOTICE, "upstream server: %d",
                cycle->srvs_n);
        }
    }

    cJSON_Delete(root);
    fclose(conf);

    return UCT_OK;
}

void
uct_master_thread_cycle(uct_cycle_t *cycle)
{
    uct_socket_t listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char localport[UCT_INET_PORTSTRLEN];
    uct_listening_pool_t *lpool;
    uct_connection_t *conn;
    uct_int_t n;

    lpool = uct_palloc(cycle->pool, sizeof(uct_listening_pool_t));
    n = uct_listening_pool_init(cycle, lpool);
    if (n != UCT_OK) {
        uct_log(cycle->log, UCT_LOG_FATAL, "listening pool init failed");
        return;
    }

    listenfd =
        uct_open_listenfd(uct_itoa(cycle->localport, localport, 10), cycle);
    if (listenfd < 0) {
        uct_log(cycle->log, UCT_LOG_ERROR, "uct_open_listenfd failed");
        return;
    }

    // uct_nonblocking(listenfd);

    uct_log(cycle->log, UCT_LOG_INFO, "start listen: %d", cycle->localport);

    uct_start_worker_threads(cycle, cycle->workers_n);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = uct_accept(listenfd, (struct sockaddr *)&clientaddr,
            &clientlen, cycle);
        if (connfd == UCT_ERROR) {
            continue;
        }

        conn = uct_connection_init(cycle, connfd);

        uct_getnameinfo((struct sockaddr *)&clientaddr, clientlen,
            conn->ip_text, UCT_INET_ADDRSTRLEN, conn->port_text,
            UCT_INET_PORTSTRLEN, NI_NUMERICHOST | NI_NUMERICSERV, cycle);

        uct_log(cycle->log, UCT_LOG_INFO, "accepted connection from (%s:%s)",
            conn->ip_text, conn->port_text);

        conn = uct_listening_pool_add(lpool, conn);
    }
    uct_close_socket(listenfd);
}

static void
uct_start_worker_threads(uct_cycle_t *cycle, uct_int_t n)
{
    uct_int_t i;
    pthread_t tid;

    uct_log(cycle->log, UCT_LOG_NOTICE, "start worker threads");

    for (i = 0; i < n; i++) {
        if (uct_pthread_create(&tid, NULL, uct_worker_thread_cycle,
                (void *)cycle)) {
            uct_log(cycle->log, UCT_LOG_ERROR, "create thread worker failed");
        }
    }
}

static void *
uct_worker_thread_cycle(void *arg)
{
    pthread_t tid;
    uct_cycle_t *cycle;
    uct_cycle_t *wk_cycle;
    uct_pool_t *pool;
    uct_listening_pool_t *lpool;
    uct_connection_t *cli_conn;
    uct_connection_t *up_conn;
    uct_proxy_t *proxy_conn;

    tid = uct_pthread_self();
    uct_pthread_detach(tid);

    cycle = (uct_cycle_t *)arg;
    lpool = cycle->lpool;

    pool = uct_create_pool(UCT_DEFAULT_POOL_SIZE, cycle->log);
    wk_cycle = uct_palloc(pool, sizeof(uct_cycle_t));
    uct_memcpy(wk_cycle, cycle, sizeof(uct_cycle_t));
    wk_cycle->pool = pool;
    wk_cycle->master = cycle;
    wk_cycle->connection_n = 0;
    // proxy_conn = uct_palloc(wk_cycle->pool, sizeof(uct_proxy_t));
    // uct_queue_init(&proxy_conn->queue);
    // wk_cycle->proxy_conn = proxy_conn;

    if (uct_epoll_init(wk_cycle) != UCT_OK) {
        return NULL;
    }

    uct_log(cycle->log, UCT_LOG_INFO, "thread worker start: %x", tid);

    while (1) {
        cli_conn = uct_listening_pool_get(lpool);
        if (cli_conn) {
            uct_log(cycle->log, UCT_LOG_DEBUG,
                "[THREAD][%x]: get client connection fd: %d", tid,
                cli_conn->fd);

            up_conn = uct_upstream_get_connetion(wk_cycle, cli_conn);
            proxy_conn = uct_pnalloc(up_conn->pool, sizeof(uct_proxy_t));
            proxy_conn->client = cli_conn;
            proxy_conn->upstream = up_conn;
            uct_epoll_add_proxy_connection(wk_cycle, proxy_conn);
            wk_cycle->connection_n++;
        }
        uct_epoll_process_events(wk_cycle);
    }

    return NULL;
}

static uct_int_t
uct_process_options(uct_cycle_t *cycle)
{
    if (uct_conf_file) {
        cycle->conf_file = (char *)uct_conf_file;
    } else {
        cycle->conf_file = NULL;
    }

    return UCT_OK;
}
