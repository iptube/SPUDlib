/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <string.h>
#include <stdio.h>

#include <netinet/ip.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

#include "spud.h"
#include "tube.h"
#include "ls_log.h"
#include "ls_sockaddr.h"
#include "ls_htable.h"
#include "gauss.h"

#define MAXBUFLEN 2048
#define NUM_TUBES 1024

pthread_t listenThread;
tube_manager *mgr = NULL;
tube *tubes[NUM_TUBES];
struct sockaddr_in6 remoteAddr;
struct sockaddr_in6 localAddr;
uint8_t data[1024];

static int markov()
{
    struct timespec timer;
    struct timespec remaining;
    tube *t;
    ls_err err;
    int i;
    int *iptr;

    timer.tv_sec = 0;

    while (tube_manager_running(mgr)) {
        timer.tv_nsec = gauss(50000000, 10000000);
        nanosleep(&timer, &remaining);
        i = random() % NUM_TUBES;
        t = tubes[i];
        if (!t) {
            if (!tube_create(mgr, &t, &err)) {
                LS_LOG_ERR(err, "tube_create");
                return 1;
            }
            tubes[i] = t;
            iptr = malloc(sizeof(*iptr));
            *iptr = i;
            tube_set_data(t, iptr);
        }
        switch (tube_get_state(t)) {
        case TS_START:
            ls_log(LS_LOG_ERROR, "invalid tube state");
            return 1;
        case TS_UNKNOWN:
            if (!tube_open(t, (struct sockaddr*)&remoteAddr, &err)) {
                LS_LOG_ERR(err, "tube_open");
                return 1;
            }
            break;
        case TS_OPENING:
            // keep waiting by the mailbox, Charlie Brown
            break;
        case TS_RUNNING:
            // .1% chance of close
            if ((random()%10000) < 10) {
                if (!tube_close(t, &err)) {
                    LS_LOG_ERR(err, "tube_close");
                    return 1;
                }
            } else {
                // TODO: put something intersting in the buffer
                if (!tube_data(t, data, random() % sizeof(data), &err)) {
                    LS_LOG_ERR(err, "tube_data");
                    return 1;
                }
            }
        case TS_RESUMING:
            // can't get here yet
            break;
        }
    }
    return 0;
}

static void *socketListen(void *ptr)
{
    ls_err err;
    UNUSED_PARAM(ptr);

    if (!tube_manager_loop(mgr, &err)) {
        LS_LOG_ERR(err, "tube_manager_loop");
    }
    return NULL;
}


static void remove_cb(ls_event_data evt, void *arg)
{
    tube *t = evt->data;
    int *i = tube_get_data(t);
    UNUSED_PARAM(arg);

    tubes[*i] = NULL;
    ls_data_free(i);
}

void done() {
    tube_manager_stop(mgr);
    pthread_cancel(listenThread);
    pthread_join(listenThread, NULL);
    ls_log(LS_LOG_INFO, "DONE!");
    exit(0);
}

int spudtest(int argc, char **argv)
{
    ls_err err;
    size_t i;
    const char nums[] = "0123456789";

    if (argc < 2) {
        fprintf(stderr, "spudload <destination>\n");
        exit(64);
    }
#if defined(__APPLE__)
    srandomdev();
#endif
#if defined(__LINUX__)
    srandom(time(NULL));
#endif
    for (i=0; i<sizeof(data); i++) {
        data[i] = nums[i % 10];
    }

    if(!ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                       argv[1],
                                       "1402",
                                       &err)) {
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

    if (!tube_manager_bind_event(mgr, EV_REMOVE_NAME, remove_cb, &err)) {
        LS_LOG_ERR(err, "tube_manager_bind_event");
        return 1;
    }

    memset(tubes, 0, sizeof(tubes));

    //Start and listen to the sockets.
    pthread_create(&listenThread, NULL, socketListen, NULL);
    signal(SIGINT, done);

    return markov();
}

int main(int argc, char **argv)
{
    return spudtest(argc, argv);
}
