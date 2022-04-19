/*
 * Copyright (C) 2021 SunBK201
 */

#include <uct_config.h>
#include <uct_core.h>
#include <umicat.h>

static uct_int_t uct_check_user();
static uct_int_t uct_os_init();
static uct_int_t uct_get_options(int argc, char *const *argv);
static void uct_show_version_info(void);

static uct_uint_t uct_show_help;
static uct_uint_t uct_show_version;
u_char *uct_conf_file;

int
main(int argc, char *const *argv)
{
    uct_cycle_t *cycle;
    uct_log_t *log;

    if (uct_check_user() != UCT_OK) {
        uct_write_stderr("umicat: you are not root user\n");
        return 1;
    }

    if (uct_get_options(argc, argv) != UCT_OK) {
        return 1;
    }

    if (uct_show_version) {
        uct_show_version_info();
        return 0;
    }

    log = malloc(sizeof(uct_log_t));
    if (log == NULL) {
        return UCT_ERROR;
    }
    log = uct_log_init(log);

    if (uct_os_init() != UCT_OK) {
        uct_log(log, UCT_LOG_FATAL, "uct_os_init failed");
        return UCT_ERROR;
    }

    cycle = uct_init_cycle(log);
    if (cycle == NULL) {
        uct_log(log, UCT_LOG_FATAL, "uct_init_cycle failed");
        return UCT_ERROR;
    }

    if (cycle->mode == UCT_TCP_MODE)
        uct_master_thread_cycle(cycle);
    else if (cycle->mode == UCT_UDP_MODE)
        uct_master_thread_cycle_udp(cycle);
    else
        uct_log(log, UCT_LOG_FATAL, "umicat mode false");

    return UCT_OK;
}

uct_int_t
uct_check_user()
{
    if (getuid() != 0) {
        return UCT_DECLINED;
    }
    return UCT_OK;
}

uct_int_t
uct_os_init()
{
    uct_uint_t n;

    uct_pagesize = getpagesize();
    uct_cacheline_size = UCT_CPU_CACHE_LINE;

    for (n = uct_pagesize; n >>= 1; uct_pagesize_shift++) {
        /* void */
    }

    return UCT_OK;
}

static uct_int_t
uct_get_options(int argc, char *const *argv)
{
    u_char *p;
    uct_int_t i;

    for (i = 1; i < argc; i++) {
        p = (u_char *)argv[i];

        if (*p++ != '-') {
            printf("umicat: invalid option: \"%s\"\n", argv[i]);
            return UCT_ERROR;
        }

        while (*p) {
            switch (*p++) {
            case '?':
            case 'h':
                uct_show_version = 1;
                uct_show_help = 1;
                break;
            case 'v':
                uct_show_version = 1;
                break;
            case 'c':
                if (*p) {
                    uct_conf_file = p;
                    goto next;
                }
                if (argv[++i]) {
                    uct_conf_file = (u_char *) argv[i];
                    goto next;
                }
                printf("umicat: option \"-c\" requires file name\n");
                return UCT_ERROR;
            default:
                printf("umicat: invalid option: \"%c\"\n", *(p - 1));
                return UCT_ERROR;
            }
        }
    next:
        continue;
    }
    return UCT_OK;
}

static void
uct_show_version_info(void)
{
    printf("umicat version: %s\n", UMICAT_VERSION);
    printf("built on: %s %s\n", __DATE__, __TIME__);

    if (uct_show_help) {
        uct_write_stderr(
            "Usage: umicat [-?hv] [-c filename] " UCT_LINEFEED
            "Options:" UCT_LINEFEED "  -?,-h         : this help" UCT_LINEFEED
            "  -v            : show version and exit" UCT_LINEFEED
            "  -c filename   : set configuration file (default: " UCT_CONF_PATH
            ")" UCT_LINEFEED);
    }
}