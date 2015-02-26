#include <string.h>
#include <stdio.h>

#include <netinet/ip.h>
#include <pthread.h>

#include "spudlib.h"
#include "tube.h"
#include "iphelper.h"
#include "sockethelper.h"

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
#define LOGV(...) printf(__VA_ARGS__)
#define LOGE(...) printf(__VA_ARGS__)

#define ESC_7C 	"\033[7C"

#endif

#define MAXBUFLEN 2048
#define MAX_LISTEN_SOCKETS 10

bool keepGoing = true;
pthread_t sendDataThread;
pthread_t listenThread;

static const int numChar = 53;
static const char* s1= "[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~";

#define UNUSED(x) (void)(x)

struct test_config {

    struct sockaddr_storage remoteAddr;
    struct sockaddr_storage localAddr;
    tube_t tube;

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
        tube_data(&config->tube, buf, strlen(s1));
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

    while (keepGoing) {
        addr_len = sizeof(their_addr);
        if ((numbytes = recvfrom(config->tube.sock, buf,
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
    tube_close(&config->tube);
    return NULL;
}

static void data_cb(tube_t *tube,
                    const uint8_t *data,
                    ssize_t length,
                    const struct sockaddr* addr)
{
    struct test_config *config = (struct test_config *)tube->data;
    UNUSED(addr);
    UNUSED(length);
    config->numRcvdPkts++;
    LOGI("\r " ESC_7C " RX: %i  %s", config->numRcvdPkts, data);
}

static void running_cb(struct _tube_t* tube,
                       const struct sockaddr* addr)
{
    UNUSED(addr);
    printf("running!\n");

    //Start a thread that sends packet to the destination
    pthread_create(&sendDataThread, NULL, (void *)sendData, tube->data);
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
    int sockfd;
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
             config.localAddr.ss_len) != 0) {
        perror("bind");
        return 1;
    }

    tube_init(&config.tube, sockfd);
    config.tube.data = &config;
    config.tube.data_cb = data_cb;
    config.tube.running_cb = running_cb;
    config.tube.close_cb = close_cb;
    tube_print(&config.tube);
    printf("-> %s\n", sockaddr_toString((struct sockaddr*)&config.remoteAddr, buf, sizeof(buf), true));


    //Start and listen to the sockets.
    pthread_create(&listenThread, NULL, (void *)socketListen, &config);
#ifndef ANDROID
    signal(SIGINT, done);
#endif

    tube_open(&config.tube, (const struct sockaddr*)&config.remoteAddr);

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
