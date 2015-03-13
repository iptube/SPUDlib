/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <string.h>
#include <stdio.h>

#include <netinet/ip.h>
#include <pthread.h>
#include <signal.h>

#include "spud.h"
#include "tube.h"
#include "iphelper.h"
#include "sockethelper.h"
#include "ls_error.h"
#include "ls_log.h"
#include "ls_sockaddr.h"

#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#include "stdio.h"
#define TAG "hiut"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define ESC_7C 	""

#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGV(...) ls_log(LS_LOG_VERBOSE, __VA_ARGS__)
#define LOGE(...) ls_log(LS_LOG_ERROR, __VA_ARGS__)

#define ESC_7C 	"\033[7C"

#endif

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
#ifndef ANDROID
        LOGI("\rTX: %i", config.numSentPkts);
        fflush(stdout);
#endif

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
    pthread_create(&sendDataThread, NULL, (void *)sendData, arg);
}

void done() {
    tube_manager_stop(mgr);
    pthread_join(sendDataThread, NULL);
    LOGI("\nDONE!\n");
    exit(0);
}

int spudtest(int argc, char **argv)
{
    ls_err err;
    char buf[1024];

    if (argc < 2) {
        fprintf(stderr, "spudtest <destination>\n");
        exit(64);
    }

    LOGI("entering spudtest\n");
    memset(&config, 0, sizeof(config));

    if(!ls_sockaddr_get_remote_ip_addr((struct sockaddr_in6*)&config.remoteAddr,
                                       argv[1],
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

    if (!tube_manager_bind_event(mgr, EV_RUNNING_NAME, running_cb, &err) ||
        !tube_manager_bind_event(mgr, EV_DATA_NAME, data_cb, &err)) {
        LS_LOG_ERR(err, "tube_manager_bind_event");
        return 1;
    }

    if (!tube_print(config.t, &err)) {
        LS_LOG_ERR(err, "tube_print");
        return 1;
    }
    ls_log(LS_LOG_INFO, "-> %s\n", sockaddr_toString((struct sockaddr*)&config.remoteAddr, buf, sizeof(buf), true));

    //Start and listen to the sockets.
    pthread_create(&listenThread, NULL, (void *)socketListen, NULL);
#ifndef ANDROID
    signal(SIGINT, done);
#endif

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

#ifdef ANDROID
int traceroute(const char* hostname, int port)
{
    char *argv[2];
    int argc = 2;

    argv[1] = (char*)malloc(1024);
    strcpy(argv[1], hostname);

    spudtest(argc, argv);

    free(argv[1]);
    return 0;
}

#else
int main(int argc, char **argv)
{
    return spudtest(argc, argv);
}
#endif
