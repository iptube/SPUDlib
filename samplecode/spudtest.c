/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <string.h>
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>

#include "spud.h"
#include "tube_manager.h"
#include "ls_error.h"
#include "ls_log.h"
#include "ls_sockaddr.h"

#define ESC_7C 	"\033[7C"

#define MAXBUFLEN 2048

static const int numChar = 53;
static const char* s1= "[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~";

tube_manager *mgr;

struct test_config {

    struct sockaddr_storage remoteAddr;
    struct sockaddr_storage localAddr;
    tube *t;

    int numSentPkts;
    int numRcvdPkts;
    int numSentProbe;
};
struct test_config config;

static void sendData(ls_timer *tim)
{
    unsigned char buf[1024];
    int i;
    ls_err err;
    UNUSED_PARAM(tim);

    config.numSentPkts++;

    for (i=0; i<1; i++) {
        int len = (numChar*i);
        // TODO: parse this.
        memcpy(buf+len, s1+(config.numSentPkts%numChar)+(numChar*i), numChar);
    }

    if (!tube_data(config.t, buf, strlen(s1), &err)) {
        LS_LOG_ERR(err, "tube_data");
    }

    if (!tube_manager_schedule_ms(mgr, 50, sendData, NULL, NULL, &err)) {
        LS_LOG_ERR(err, "tube_manager_schedule_ms");
        return;
    }
}

static void data_cb(ls_event_data evt, void *arg)
{
    tube_event_data *td = evt->data;
    UNUSED_PARAM(arg);

    config.numRcvdPkts++;
    if (td->cbor) {
        const cn_cbor *data = cn_cbor_mapget_int(td->cbor, 0);
        ((char*)data->v.str)[data->length-1] = '\0';  // TODO: cheating
        if (data && (data->type==CN_CBOR_TEXT || data->type==CN_CBOR_BYTES)) {
            // printf("\r " ESC_7C " RX: %i  %s",
            //        config.numRcvdPkts,
            //        data->v.str);
            printf(" RX: %i  %s\n",
                   config.numRcvdPkts,
                   data->v.str);
        }
    }
}

static void running_cb(ls_event_data evt, void *arg)
{
    UNUSED_PARAM(evt);
    UNUSED_PARAM(arg);
    ls_log(LS_LOG_INFO, "running!");

    sendData(NULL);
}

static void loopstart_cb(ls_event_data evt, void *arg)
{
    ls_err err;
    UNUSED_PARAM(evt);
    UNUSED_PARAM(arg);

    if (!tube_manager_open_tube(mgr,
                                (const struct sockaddr*)&config.remoteAddr,
                                &config.t,
                                &err)) {
        LS_LOG_ERR(err, "tube_open");
        return;
    }

    if (!tube_print(config.t, &err)) {
        LS_LOG_ERR(err, "tube_print");
        return;
    }
}

void done(ls_timer *tim)
{
    ls_err err;
    UNUSED_PARAM(tim);
    printf("done\n");
    if (!tube_manager_stop(mgr, &err)) {
        LS_LOG_ERR(err, "tube_manager_stop");
    }
}

void done_sig(int sig)
{
    UNUSED_PARAM(sig);
    ls_log(LS_LOG_VERBOSE, "Signal caught: %d", sig);
    done(NULL);
}

static void usage(void)
{
    fprintf(stderr,
        "spudtest [-h] [-v] [-s <source>] <destination>\n"
        "  -h          Print this help message\n"
        "  -v          Increase verbosity\n"
        "  -s <source> Use the given source IP address\n");

    exit(64);
}

int spudtest(int argc, char **argv)
{
    ls_err err;
    int ch;
    struct sockaddr_storage addr;
    //bool has_addr = false;;

    while ((ch = getopt(argc, argv, "?hvs:")) != -1) {
        switch (ch) {
        case 'v':
            ls_log_set_level(LS_LOG_VERBOSE);
            break;
        case 's':
            if (!ls_sockaddr_parse(optarg, (struct sockaddr*)&addr, sizeof(addr), &err)) {
                LS_LOG_ERR(err, "Invalid address");
                return 1;
            }
            //has_addr = true;
            break;
        case 'h':
        case '?':
        default:
            usage();
            break;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 1) {
        usage();
    }

    ls_log(LS_LOG_INFO, "entering spudtest");
    memset(&config, 0, sizeof(config));

    if(!ls_sockaddr_get_remote_ip_addr(argv[0],
                                       "1402",
                                       (struct sockaddr*)&config.remoteAddr,
                                       sizeof(config.remoteAddr),
                                       &err)) {
        LS_LOG_ERR(err, "ls_sockaddr_get_remote_ip_addr");
        return 1;
    }

    if (!tube_manager_create(0, &mgr, &err)) {
        LS_LOG_ERR(err, "tube_manager_create");
        return 1;
    }
    if (!tube_manager_socket(mgr, 0, &err)) {
        LS_LOG_ERR(err, "tube_manager_socket");
        return 1;
    }

    // TODO: make source addresses work (again?).
    // if (has_addr) {
    //     LOGI("source address: %s\n", inet_ntop(AF_INET6, &addr, buf, sizeof(buf)));
    // }

    if (!tube_manager_bind_event(mgr, EV_LOOPSTART_NAME, loopstart_cb, &err) ||
        !tube_manager_bind_event(mgr, EV_RUNNING_NAME, running_cb, &err) ||
        !tube_manager_bind_event(mgr, EV_DATA_NAME, data_cb, &err)) {
        LS_LOG_ERR(err, "tube_manager_bind_event");
        return 1;
    }

    if (!tube_manager_signal(mgr, SIGINT, done_sig, &err)) {
        LS_LOG_ERR(err, "tube_manager_signal");
        return 1;
    }

    if (!tube_manager_schedule_ms(mgr, 5000, done, NULL, NULL, &err)) {
        LS_LOG_ERR(err, "tube_manager_schedule_ms");
        return 1;
    }

    if (!tube_manager_loop(mgr, &err)) {
        LS_LOG_ERR(err, "tube_manager_loop");
    }

    return 0;
}

int main(int argc, char **argv)
{
    return spudtest(argc, argv);
}
