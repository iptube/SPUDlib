#include <string.h>
#include <stdio.h>

#include <netinet/ip.h>
#include <pthread.h>
#include <signal.h>

#include "spudlib.h"
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

bool keepGoing = true;
pthread_t sendDataThread;
pthread_t listenThread;

static const int numChar = 53;
static const char* s1= "[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~";

#define UNUSED(x) (void)(x)

struct test_config {

    struct sockaddr_storage remoteAddr;
    struct sockaddr_storage localAddr;
    tube t;

    int numSentPkts;
    int numRcvdPkts;
    int numSentProbe;
};

static void *sendData(struct test_config *config)
{
    struct timespec timer;
    struct timespec remaining;
    unsigned char buf[1024];
    int i;
    ls_err err;

    //How fast? Pretty fast..
    timer.tv_sec = 0;
    timer.tv_nsec = 50000000;

    while (keepGoing) {
        nanosleep(&timer, &remaining);
        config->numSentPkts++;
#ifndef ANDROID
        LOGI("\rTX: %i", config->numSentPkts);
        fflush(stdout);
#endif

        for (i=0; i<1; i++) {
            int len = (numChar*i);
            // TODO: parse this.
            memcpy(buf+len, s1+(config->numSentPkts%numChar)+(numChar*i), numChar);
        }
        if (!tube_data(config->t, buf, strlen(s1), &err)) {
            LS_LOG_ERR(err, "tube_data");
        }
    }
    return NULL;
}

static void *socketListen(void *ptr){
    struct test_config *config = (struct test_config *)ptr;
    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];
    socklen_t addr_len;
    int numbytes;
    spud_message_t sMsg;
    ls_err err;

    while (keepGoing) {
        addr_len = sizeof(their_addr);
        if ((numbytes = recvfrom(config->t->sock, buf,
                                 MAXBUFLEN , 0,
                                 (struct sockaddr *)&their_addr,
                                 &addr_len)) == -1) {
            LOGE("recvfrom (data)");
            continue;
        }
        if (!spud_parse(buf, numbytes, &sMsg, &err)) {
            // It's an attack
            goto cleanup;
        }
        if (!spud_isIdEqual(&config->t->id, &sMsg.header->tube_id)) {
            // it's another kind of attack
            goto cleanup;
        }
        if (!tube_recv(config->t, &sMsg, (struct sockaddr *)&their_addr, &err)) {
            LS_LOG_ERR(err, "tube_recv");
        }
    cleanup:
        spud_unparse(&sMsg);
    }
    if (!tube_close(config->t, &err)) {
        LS_LOG_ERR(err, "tube_close");
    }
    return NULL;
}

static void data_cb(tube t,
                    const cn_cbor *cbor,
                    const struct sockaddr* addr)
{
    struct test_config *config = (struct test_config *)t->data;
    UNUSED(addr);
    config->numRcvdPkts++;
    if (cbor) {
        const cn_cbor *data = cn_cbor_mapget_int(cbor, 0);
        ((char*)data->v.str)[data->length-1] = '\0';  // TODO: cheating
        if (data && (data->type==CN_CBOR_TEXT || data->type==CN_CBOR_BYTES)) {
            LOGI("\r " ESC_7C " RX: %i  %s",
                 config->numRcvdPkts,
                 data->v.str);
        }
    }
}

static void running_cb(tube t,
                       const struct sockaddr* addr)
{
    UNUSED(addr);
    printf("running!\n");

    //Start a thread that sends packet to the destination
    pthread_create(&sendDataThread, NULL, (void *)sendData, t->data);
}

static void close_cb(tube t,
                     const struct sockaddr* addr)
{
    UNUSED(t);
    UNUSED(addr);
}

void done() {
    keepGoing = false;
    pthread_join(sendDataThread, NULL);
    LOGI("\nDONE!\n");
    exit(0);
}

int spudtest(int argc, char **argv)
{
    struct test_config config;
    int sockfd;
    ls_err err;
    char buf[1024];

    if (argc < 2) {
        fprintf(stderr, "spudtest <destination>\n");
        exit(64);
    }

    LOGI("entering spudtest\n");
    memset(&config, 0, sizeof(config));

    if(!getRemoteIpAddr((struct sockaddr_in6*)&config.remoteAddr,
                        argv[1],
                        "1402")) {
        return 1;
    }

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);
    sockaddr_initAsIPv6Any((struct sockaddr_in6 *)&config.localAddr, 0);

    if (bind(sockfd,
             (struct sockaddr *)&config.localAddr,
             ls_sockaddr_get_length((struct sockaddr*) &config.localAddr)) != 0) {
        perror("bind");
        return 1;
    }

    if (!tube_create(sockfd, &config.t, &err)) {
        return 1;
    }
    config.t->data = &config;
    config.t->data_cb = data_cb;
    config.t->running_cb = running_cb;
    config.t->close_cb = close_cb;
    if (!tube_print(config.t, &err)) {
        LS_LOG_ERR(err, "tube_print");
        return 1;
    }
    ls_log(LS_LOG_INFO, "-> %s\n", sockaddr_toString((struct sockaddr*)&config.remoteAddr, buf, sizeof(buf), true));

    //Start and listen to the sockets.
    pthread_create(&listenThread, NULL, (void *)socketListen, &config);
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
