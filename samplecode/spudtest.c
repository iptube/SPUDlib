/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <string.h>
#include <stdio.h>

#include <netinet/ip.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "spud.h"
#include "tube.h"
#include "ls_error.h"
#include "ls_log.h"
#include "ls_sockaddr.h"

#define LOGI(...) printf(__VA_ARGS__)
#define LOGV(...) ls_log(LS_LOG_VERBOSE, __VA_ARGS__)
#define LOGE(...) ls_log(LS_LOG_ERROR, __VA_ARGS__)

#define ESC_7C 	"\033[7C"

#define MAXBUFLEN 2048

pthread_t sendDataThread;
pthread_t listenThread;

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

static void *sendData(void *arg)
{
    struct timespec timer;
    struct timespec remaining;
    unsigned char buf[1024];
    int i;
    ls_err err;
    UNUSED_PARAM(arg);

    //How fast? Pretty fast..
    timer.tv_sec = 0;
    timer.tv_nsec = 50000000;

    while (tube_manager_running(mgr)) {
        nanosleep(&timer, &remaining);
        config.numSentPkts++;

        for (i=0; i<1; i++) {
            int len = (numChar*i);
            // TODO: parse this.
            memcpy(buf+len, s1+(config.numSentPkts%numChar)+(numChar*i), numChar);
        }
        if (!tube_data(config.t, buf, strlen(s1), &err)) {
            LS_LOG_ERR(err, "tube_data");
        }
    }
    return NULL;
}

static void *socketListen(void *arg){
    ls_err err;
    UNUSED_PARAM(arg);

    if (!tube_manager_loop(mgr, &err)) {
        LS_LOG_ERR(err, "tube_manager_loop");
    }
    return NULL;
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
            LOGI("\r " ESC_7C " RX: %i  %s",
                 config.numRcvdPkts,
                 data->v.str);
        }
    }
}

static void running_cb(ls_event_data evt, void *arg)
{
    UNUSED_PARAM(evt);
    printf("running!\n");

    //Start a thread that sends packet to the destination
    pthread_create(&sendDataThread, NULL, sendData, arg);
}

void done() {
    tube_manager_stop(mgr);
    pthread_join(sendDataThread, NULL);
    LOGI("\nDONE!\n");
    exit(0);
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
    char buf[1024];
    struct in6_addr addr;
    bool has_addr = false;;

    while ((ch = getopt(argc, argv, "?hvs:")) != -1) {
        switch (ch) {
        case 'v':
            ls_log_set_level(LS_LOG_VERBOSE);
            break;
        case 's':
            if (!ls_addr_parse(optarg, &addr, &err)) {
                LS_LOG_ERR(err, "Invalid address");
                return 1;
            }
            has_addr = true;
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

    LOGI("entering spudtest\n");
    memset(&config, 0, sizeof(config));

    if(!ls_sockaddr_get_remote_ip_addr((struct sockaddr_in6*)&config.remoteAddr,
                                       argv[0],
                                       "1402",
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

    if (!tube_create(mgr, &config.t, &err)) {
        LS_LOG_ERR(err, "tube_create");
        return 1;
    }

    if (has_addr) {
        LOGI("source address: %s\n", inet_ntop(AF_INET6, &addr, buf, sizeof(buf)));
    }

    if (!tube_manager_bind_event(mgr, EV_RUNNING_NAME, running_cb, &err) ||
        !tube_manager_bind_event(mgr, EV_DATA_NAME, data_cb, &err)) {
        LS_LOG_ERR(err, "tube_manager_bind_event");
        return 1;
    }

    if (!tube_print(config.t, &err)) {
        LS_LOG_ERR(err, "tube_print");
        return 1;
    }
    ls_log(LS_LOG_INFO, "-> %s\n", ls_sockaddr_to_string((struct sockaddr*)&config.remoteAddr, buf, sizeof(buf), true));

    //Start and listen to the sockets.
    pthread_create(&listenThread, NULL, socketListen, NULL);
    signal(SIGINT, done);

    if (!tube_open(config.t, (const struct sockaddr*)&config.remoteAddr, &err)) {
        LS_LOG_ERR(err, "tube_open");
        return 1;
    }

    //Just wait a bit
    sleep(5);
    done();
    //done exits...
    return 0;
}

int main(int argc, char **argv)
{
    return spudtest(argc, argv);
}
