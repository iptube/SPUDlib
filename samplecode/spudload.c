#include <string.h>
#include <stdio.h>

#include <netinet/ip.h>
#include <pthread.h>
#include <signal.h>

#include "spud.h"
#include "tube.h"
#include "iphelper.h"
#include "sockethelper.h"
#include "ls_log.h"
#include "ls_sockaddr.h"
#include "ls_htable.h"
#include "gauss.h"

#define MAXBUFLEN 2048
#define NUM_TUBES 1024

bool keepGoing = true;

pthread_t listenThread;
int sockfd = -1;
tube tubes[NUM_TUBES];
ls_htable tube_table;
struct sockaddr_in6 remoteAddr;
struct sockaddr_in6 localAddr;
uint8_t data[1024];

static int markov()
{
    struct timespec timer;
    struct timespec remaining;
    tube t;
    tube old = NULL;
    ls_err err;

    timer.tv_sec = 0;

    while (keepGoing) {
        timer.tv_nsec = gauss(50000000, 10000000);
        nanosleep(&timer, &remaining);
        t = tubes[random() % NUM_TUBES];
        switch (t->state) {
        case TS_START:
            ls_log(LS_LOG_ERROR, "invalid tube state");
            return 1;
        case TS_UNKNOWN:
            // tube_open(t, (struct sockaddr*)&remoteAddr, &err);
            // TODO: tube_open as a race condition
            if (!spud_createId(&t->id, &err)) {
                LS_LOG_ERR(err, "spud_createId");
                return 1;
            }
            t->state = TS_OPENING;
            if (!ls_htable_put(tube_table, &t->id, t, (void**)&old, &err)) {
                LS_LOG_ERR(err, "ls_htable_put");
                return 1;
            }
            ls_log(LS_LOG_VERBOSE, "Created.  Hashtable size: %d",
                   ls_htable_get_count(tube_table));
            if (old) {
                ls_log(LS_LOG_WARN, "state fail: old id in hashtable");
            }
            if (!tube_send(t, SPUD_OPEN, false, false, NULL, NULL, 0, &err)) {
                LS_LOG_ERR(err, "tube_send");
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
                if (!ls_htable_remove(tube_table, &t->id)) {
                    ls_log(LS_LOG_WARN, "state fail: old id did not exist");
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
    UNUSED_PARAM(ptr);
    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];
    socklen_t addr_len;
    int numbytes;
    spud_message_t sMsg = {NULL, NULL};
    ls_err err;
    tube t;
    char idStr[SPUD_ID_STRING_SIZE+1];
    spud_tube_id_t uid;

    while (keepGoing) {
        addr_len = sizeof(their_addr);
        if ((numbytes = recvfrom(sockfd, buf,
                                 MAXBUFLEN , 0,
                                 (struct sockaddr *)&their_addr,
                                 &addr_len)) == -1) {
            LS_LOG_PERROR("recvfrom (data)");
            continue;
        }
        if (!spud_parse(buf, numbytes, &sMsg, &err)) {
            // It's an attack
            ls_log(LS_LOG_WARN, "spud_cast %d, %s",
                   err.code, ls_err_message(err.code));
            goto cleanup;
        }
        if (!spud_copyId(&sMsg.header->tube_id, &uid, &err)) {
            LS_LOG_ERR(err, "spud_copyId");
            goto cleanup;
        }

        t = ls_htable_get(tube_table, &uid);
        if (!t) {
             // it's another kind of attack
            ls_log(LS_LOG_WARN, "Unknown ID: %s",
                   spud_idToString(idStr,
                                   sizeof(idStr),
                                   &sMsg.header->tube_id, NULL));
            goto cleanup;
        }
        // TODO: figure out which socket this came in on
        tube_recv(t, &sMsg, (struct sockaddr *)&their_addr, &err);
    cleanup:
        spud_unparse(&sMsg);
    }
    return NULL;
}

static void data_cb(ls_event_data evt, void *arg)
{
    UNUSED_PARAM(evt);
    UNUSED_PARAM(arg);
}

static void running_cb(ls_event_data evt, void *arg)
{
    UNUSED_PARAM(evt);
    UNUSED_PARAM(arg);
}

static void close_cb(ls_event_data evt, void *arg)
{
    tubeData *td = evt->data;
    UNUSED_PARAM(arg);

    if (!ls_htable_remove(tube_table, &td->t->id)) {
        ls_log(LS_LOG_WARN, "state fail: old id did not exist");
    }
}

void done() {
    keepGoing = false;
    pthread_cancel(listenThread);
    pthread_join(listenThread, NULL);
    close(sockfd);
    ls_log(LS_LOG_INFO, "DONE!");
    exit(0);
}

unsigned int hash_id(const void *id) {
    // treat the 8 bytes of tube ID like a long long.
    uint64_t key = *(uint64_t *)id;

    // from
    // https://gist.github.com/badboy/6267743#64-bit-to-32-bit-hash-functions
    key = (~key) + (key << 18);
    key = key ^ (key >> 31);
    key = key * 21;
    key = key ^ (key >> 11);
    key = key + (key << 6);
    key = key ^ (key >> 22);
    return (unsigned int) key;
}

int compare_id(const void *key1, const void *key2) {
    int ret = 0;
    uint64_t k1 = *(uint64_t *)key1;
    uint64_t k2 = *(uint64_t *)key2;
    if (k1<k2) {
        ret = -1;
    } else {
        ret = (k1==k2) ? 0 : 1;
    }
    return ret;
}

int spudtest(int argc, char **argv)
{
    ls_err err;
    size_t i;
    const char nums[] = "0123456789";
    ls_event_dispatcher dispatcher;

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

    if(!getRemoteIpAddr(&remoteAddr,
                        argv[1],
                        "1402")) {
        return 1;
    }

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);
    sockaddr_initAsIPv6Any(&localAddr, 0);

    if (bind(sockfd,
             (struct sockaddr *)&localAddr,
             ls_sockaddr_get_length((struct sockaddr *)&localAddr)) != 0) {
        LS_LOG_PERROR("bind");
        return 1;
    }

    if (!ls_htable_create(65521, hash_id, compare_id, &tube_table, &err)) {
        LS_LOG_ERR(err, ls_htable_create);
        return 1;
    }

    if (!ls_event_dispatcher_create(tube_table, &dispatcher, &err)) {
        LS_LOG_ERR(err, "ls_event_dispatcher_create");
        return 1;
    }

    if (!tube_bind_events(dispatcher,
                          running_cb, data_cb, close_cb,
                          tube_table, &err)) {
        LS_LOG_ERR(err, "tube_bind_events");
        return 1;
    }

    for (int i=0; i<NUM_TUBES; i++) {
        tube_create(sockfd, dispatcher, &tubes[i], &err);
        memcpy(&tubes[i]->peer,
               &remoteAddr,
               ls_sockaddr_get_length((struct sockaddr*)&remoteAddr));
    }

    //Start and listen to the sockets.
    pthread_create(&listenThread, NULL, socketListen, NULL);
    signal(SIGINT, done);

    return markov();
}

int main(int argc, char **argv)
{
    return spudtest(argc, argv);
}
