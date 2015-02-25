
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sys/socket.h>
#include <poll.h>

#include <arpa/inet.h>

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <time.h>
#include <pthread.h>

#include<signal.h>

#include "spudlib.h"
#include "tube.h"

#ifdef __linux
#include <linux/types.h>	// required for linux/errqueue.h
#include <linux/errqueue.h>	// SO_EE_ORIGIN_ICMP
#endif

#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#include "stdio.h"
#define TAG "hiut"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define ESC_7C 	""
#define ESC_35C ""
#define ESC_16C ""
#define ESC_iB 	""
#define ESC_iA  ""
#define ESC_K   ""

#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGV(...) printf(__VA_ARGS__)
#define LOGE(...) printf(__VA_ARGS__)

#define ESC_7C 	"\033[7C"
#define ESC_35C "\033[35C"
#define ESC_16C "\033[16C"
#define ESC_iB 	"\033[%iB"
#define ESC_iA  "\033[%iA"
#define ESC_K   "\033[K"

#endif

#include "iphelper.h"
#include "sockethelper.h"


#define MAXBUFLEN 2048
#define MAX_LISTEN_SOCKETS 10

int lines; //Just to gracefully handle SIGINT

//static const int numChar = 13;
//static const char* s1 = "º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`°º¤ø,¸¸,ø¤º°`° ";

// static const int numChar = 53;
// static const char* s1= "[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~";

#define UNUSED(x) (void)(x)

struct test_config {

    struct sockaddr_storage remoteAddr;
    struct sockaddr_storage localAddr;
    tube_t tube;

    int numSentPkts;
    int numRcvdPkts;
    int numSentProbe;
    void (*data_handler)(struct test_config *, struct sockaddr *,
                         unsigned char *, int);
};

static void data_handler(struct test_config *config, struct sockaddr *saddr,
                         unsigned char *buf, int len)
{
    UNUSED(saddr);
    UNUSED(len);
    config->numRcvdPkts++;
    LOGI("\r " ESC_7C " RX: %i  %s", config->numRcvdPkts, buf);
}

/*
static void *sendData(struct test_config *config)
{
    struct timespec timer;
    struct timespec remaining;
    int len = 1024;
    unsigned char buf[len];
    //char * data = "Tuber is cooool\0";
    struct SpudMsg msg;
    int i;

    //How fast? Pretty fast..
    timer.tv_sec = 0;
    timer.tv_nsec = 50000000;

    //Create SPUD message
    memcpy(msg.msgHdr.magic, SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE);
    spud_createId(&msg.msgHdr.flags_id);

    for(;;) {
        nanosleep(&timer, &remaining);
        config->numSentPkts++;
#ifndef ANDROID
        LOGI("\rTX: %i", config->numSentPkts);
        fflush(stdout);
#endif

        memcpy(buf, &msg, sizeof msg);
        for(i=0;i<1;i++){
            int len = sizeof msg +(numChar*i);

            memcpy(buf+len, s1+(config->numSentPkts%numChar)+(numChar*i), numChar);
        }

        sendPacket(config->sockfd,
                   (uint8_t *)buf,
                   sizeof msg + strlen(s1),
                   (struct sockaddr *)&config->remoteAddr,
                   0);
    }
}
*/

static void *socketListen(void *ptr){
    struct pollfd ufds[MAX_LISTEN_SOCKETS];
    struct test_config *config = (struct test_config *)ptr;
    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];
    socklen_t addr_len;
    int rv;
    int numbytes;
    int i;
    int numSockets = 0;
    struct SpudMsg sMsg;

    //Normal send/recieve RTP socket..
    ufds[0].fd = config->tube.sock;
    ufds[0].events = POLLIN | POLLERR;
    numSockets++;

    addr_len = sizeof their_addr;

    while(1) {
        rv = poll(ufds, numSockets, -1);
        if (rv == -1) {
            LOGE("poll"); // error occurred in poll()
        } else if (rv == 0) {
            LOGI("Timeout occurred! (Should not happen)\n");
        } else {
            for (i=0; i<numSockets; i++) {
                if (ufds[i].revents & POLLERR) {
                    LOGE("Poll socket");
                    return NULL;
                }
                if (ufds[i].revents & POLLIN) {
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
                    // TODO: check their_addr
                    printf("ack\n");
                }
            }
        }
    }
}


void done() {
    LOGI(ESC_iB,lines);
    LOGI("\nDONE!\n");
    exit(0);
}

int spudtest(int argc, char **argv)
{
    struct test_config config;
    int sockfd;
    char buf[1024];

    // pthread_t sendDataThread;
    pthread_t listenThread;

    if (argc < 2) {
        fprintf(stderr, "spudtest <destination>\n");
        exit(64);
    }

#ifndef ANDROID
    signal(SIGINT, done);
#endif
	LOGI("entering spudtest\n");

    memset(&config, 0, sizeof(config));
    config.data_handler = data_handler;

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
    tube_print(&config.tube);
    printf("-> %s\n", sockaddr_toString((struct sockaddr*)&config.remoteAddr, buf, sizeof(buf), true));

    //Start and listen to the sockets.
    pthread_create(&listenThread, NULL, (void *)socketListen, &config);

    tube_open(&config.tube, (const struct sockaddr*)&config.remoteAddr);

    //Start a thread that sends packet to the destination (Simulate RTP)
    //pthread_create(&sendDataThread, NULL, (void *)sendData, &config);


    //Do other stuff here..

    //Just wait a bit
    sleep(5);
    //pthread_cancel(sendDataThread);
    sleep(1);
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
