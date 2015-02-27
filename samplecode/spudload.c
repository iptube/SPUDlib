#include <string.h>
#include <stdio.h>

#include <netinet/ip.h>
#include <pthread.h>

#include "spudlib.h"
#include "tube.h"
#include "iphelper.h"
#include "sockethelper.h"

#define MAXBUFLEN 2048
#define NUM_TUBES 100

bool keepGoing = true;

pthread_t sendDataThread;
pthread_t listenThread;
int sockfd = -1;
tube_t tubes[NUM_TUBES];
struct sockaddr_in6 remoteAddr;

static int markov()
{
    struct timespec timer;
    struct timespec remaining;
    unsigned char buf[1024];
    tube_t *tube;

    //How fast? Pretty fast..
    timer.tv_sec = 0;
    timer.tv_nsec = 50000000;

    while (keepGoing) {
        nanosleep(&timer, &remaining);
        tube = &tubes[arc4random_uniform(NUM_TUBES)];
        switch (tube->state) {
        case TS_START:
            fprintf(stderr, "invalid tube state\n");
            return 1;
        case TS_UNKNOWN:
            tube_open(tube, (struct sockaddr*)&remoteAddr);
            break;
        case TS_OPENING:
            // keep waiting by the mailbox, Charlie Brown
            break;
        case TS_RUNNING:
            if (arc4random_uniform(100) < 10) {
                tube_close(tube);
            } else {
                tube_data(tube, buf, sizeof(buf));
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
    struct test_config *config = (struct test_config *)ptr;
    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];
    socklen_t addr_len;
    int numbytes;
    spud_message_t sMsg;

    while (keepGoing) {
        addr_len = sizeof(their_addr);
        if ((numbytes = recvfrom(sockfd, buf,
                                 MAXBUFLEN , 0,
                                 (struct sockaddr *)&their_addr,
                                 &addr_len)) == -1) {
            LOGE("recvfrom (data)");
            continue;
        }
        if (!spud_cast(buf, numbytes, &sMsg)) {
            // It's an attack
            continue;
        }
        if (!spud_isIdEqual(&config->tube.id, &sMsg.header->flags_id)) {
            // it's another kind of attack
            continue;
        }
        tube_recv(&config->tube, &sMsg, (struct sockaddr *)&their_addr);
    }
    close(sockfd)
    return NULL;
}

static void data_cb(tube_t *tube,
                    const uint8_t *data,
                    ssize_t length,
                    const struct sockaddr* addr)
{
    UNUSED(addr);
    UNUSED(length);
}

static void running_cb(struct _tube_t* tube,
                       const struct sockaddr* addr)
{
    UNUSED(addr);
}

static void close_cb(struct _tube_t* tube,
                     const struct sockaddr* addr)
{
    UNUSED(tube);
    UNUSED(addr);
}

void done() {
    keepGoing = false;
    pthread_join(listenThread, NULL);
    LOGI("\nDONE!\n");
    exit(0);
}

int spudtest(int argc, char **argv)
{
    struct test_config config;
    char buf[1024];

    if (argc < 2) {
        fprintf(stderr, "spudload <destination>\n");
        exit(64);
    }

    if(!getRemoteIpAddr(&remoteAddr,
                        argv[1],
                        "1402")) {
        return 1;
    }

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);
    sockaddr_initAsIPv6Any((struct sockaddr_in6 *)&config.localAddr, 0);

    if (bind(sockfd,
             (struct sockaddr *)&config.localAddr,
             config.localAddr.ss_len) != 0) {
        perror("bind");
        return 1;
    }

    for (int i=0; i<NUM_TUBES; i++) {
        tube_init(&tubes[i], sockfd);
        tubes[i].tube.data_cb = data_cb;
        tubes[i].running_cb   = running_cb;
        tubes[i].close_cb     = close_cb;
    }

    //Start and listen to the sockets.
    pthread_create(&listenThread, NULL, (void *)socketListen, &config);
    signal(SIGINT, done);

    return markov();
}

int main(int argc, char **argv)
{
    return spudtest(argc, argv);
}
