
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
#include <stdio.h>

#include <stdbool.h>
#include <sys/socket.h>
#include <poll.h>

#include <arpa/inet.h>

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <time.h>
#include <pthread.h>

#include<signal.h>

#include "spudlib.h"

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

static const int numChar = 53;
static const char* s1= "[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~[_]~~ c[_]~~ c[_]~~ COFFEE BREAK c[_]~~ c[_]~~ c[_]~~";

struct test_config{

    struct sockaddr_storage remoteAddr;
    struct sockaddr_storage localAddr;
    int sockfd;
    int icmpSocket;

    int numSentPkts;
    int numRcvdPkts;
    int numSentProbe;
    int numRcvdICMP;
    void (*data_handler)(struct test_config *, struct sockaddr *,
                         unsigned char *, int);

    void (*icmp_handler)(struct test_config *, struct sockaddr *,
                         int);
};

static void data_handler(struct test_config *config, struct sockaddr *saddr,
                         unsigned char *buf, int len){

    config->numRcvdPkts++;
    LOGI("\r " ESC_7C " RX: %i  %s", config->numRcvdPkts, buf);
}


static void icmp_handler(struct test_config *config,
                         struct sockaddr *saddr,
                         int icmpType){

    char src_str[INET6_ADDRSTRLEN];

    config->numRcvdICMP++;
    LOGI("\r " ESC_35C " RX ICMP: %i ",config->numRcvdICMP);
    LOGI(ESC_iB,config->numRcvdICMP-1);
    LOGI("\n  <-  %s (ICMP type:%i)" ESC_K,
           inet_ntop(AF_INET, saddr, src_str,INET_ADDRSTRLEN),
           icmpType);
    LOGI(ESC_iA,config->numRcvdICMP);

    //For graefull SIGINT
    lines = config->numRcvdICMP;
}

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
                   false,
                   0);
    }
}

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
    int keyLen = 16;
    char md5[keyLen];


    //Normal send/recieve RTP socket..
    ufds[0].fd = config->sockfd;
    ufds[0].events = POLLIN | POLLERR;
    numSockets++;

    addr_len = sizeof their_addr;

    while(1){
        rv = poll(ufds, numSockets, -1);
        if (rv == -1) {
            LOGE("poll"); // error occurred in poll()
        } else if (rv == 0) {
            LOGI("Timeout occurred! (Should not happen)\n");
        } else {
            for(i=0;i<numSockets;i++){
                if (ufds[i].revents & POLLIN) {
                    if ((numbytes = recvfrom(config->sockfd, buf,
                                                 MAXBUFLEN , 0,
                                                 (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                            LOGE("recvfrom (data)");
                    }
                    config->data_handler(config, (struct sockaddr *)&their_addr, buf, numbytes);


                }

            }
        }
    }
 }




void done(){
    LOGI(ESC_iB,lines);
    LOGI("\nDONE!\n");
    exit(0);
}

int spudtest(int argc, char **argv)
{
    struct test_config config;
    pthread_t sendDataThread;
    pthread_t listenThread;

    int i;
#ifndef ANDROID
    signal(SIGINT, done);
#endif
	LOGI("entering spudtest");

    memset(&config, 0, sizeof(config));
    config.icmp_handler = icmp_handler;
    config.data_handler = data_handler;


    if(!getRemoteIpAddr((struct sockaddr *)&config.remoteAddr,
                            argv[2],
                        1402)){
        LOGI("Error getting remote IPaddr");
        exit(1);
        }

    if(!getLocalInterFaceAddrs( (struct sockaddr *)&config.localAddr,
                                argv[1],
                                config.remoteAddr.ss_family,
                                IPv6_ADDR_NORMAL,
                                false)){
        LOGI("Error local getting IPaddr on %s\n", argv[1]);
        exit(1);
    }
    /* Setting up UDP socket and a ICMP sockhandle */
    config.sockfd = createLocalUDPSocket(config.remoteAddr.ss_family, (struct sockaddr *)&config.localAddr, 0);

    //Start and listen to the sockets.
    pthread_create(&listenThread, NULL, (void *)socketListen, &config);

    //Start a thread that sends packet to the destination (Simulate RTP)
    pthread_create(&sendDataThread, NULL, (void *)sendData, &config);

    //Do other stuff here..

    //Just wait a bit
    sleep(5);
    pthread_cancel(sendDataThread);
    sleep(1);
    done();
    //done exits...
    return 0;
}

#ifdef ANDROID
int traceroute(const char* hostname, int port)
{
	char *argv[3];
	int argc = 3;

	argv[1] = (char*)malloc(1024);
	argv[2] = (char*)malloc(1024);
	strcpy(argv[1], "wlan0");
	strcpy(argv[2], hostname);

	spudtest(argc, argv);

	free(argv[1]);
	free(argv[2]);
	return 0;
}

#else
int main(int argc, char **argv)
{
	return spudtest(argc, argv);
}
#endif
