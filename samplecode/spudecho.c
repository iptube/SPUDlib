#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h>

#include <stdbool.h>


#include <poll.h>

#include "spudlib.h"
#include "iphelper.h"
#include "sockethelper.h"


#define MYPORT 1402    // the port users will be connecting to
#define MAXBUFLEN 2048
#define MAX_LISTEN_SOCKETS 10

#define UNUSED(x) (void)(x)

int sockfd;

struct listenConfig{

    int sockfd;
    /*Handles normal data like RTP etc */
    void (*data_handler)(struct listenConfig *, struct sockaddr *,
                         void *, unsigned char *, int);
    /*Handles STUN packet*/
    void (*spud_handler)(struct listenConfig *, struct sockaddr *,
                         void *, unsigned char *, int);
};

void teardown()
{
  close(sockfd);
  printf("Quitting...\n");
  exit(0);
}

void dataHandler(struct listenConfig *config,
                 struct sockaddr *from_addr,
                 void *cb,
                 unsigned char *buf,
                 int bufLen)
{
    UNUSED(cb);
    printf(" %s", buf);
    fflush(stdout);
    sendPacket(config->sockfd,
               buf,
               bufLen,
               from_addr,
               0);
}


void spudHandler(struct listenConfig *config,
                 struct sockaddr *from_addr,
                 void *cb,
                 unsigned char *buf,
                 int buflen)
{
    UNUSED(config);
    UNUSED(from_addr);
    UNUSED(cb);
    UNUSED(buf);
    UNUSED(buflen);
    //do something
}

static void *socketListen(void *ptr) {
    struct pollfd ufds[MAX_LISTEN_SOCKETS];
    struct listenConfig *config = (struct listenConfig *)ptr;
    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];
    socklen_t addr_len;
    int rv;
    int numbytes;
    int i;
    int numSockets = 0;
    const int dataSock = 0;

    //Normal send/recieve RTP socket..
    ufds[dataSock].fd = config->sockfd;
    ufds[dataSock].events = POLLIN | POLLERR;
    numSockets++;

    addr_len = sizeof their_addr;

    while(1) {
        rv = poll(ufds, numSockets, -1);
        if (rv == -1) {
            printf("Error in poll\n"); // error occurred in poll()
        } else if (rv == 0) {
            printf("Poll Timeout occurred! (Should not happen)\n");
        } else {
            for (i=0;i<numSockets;i++) {
                if (ufds[i].revents & POLLERR) {
                    fprintf(stderr, "poll socket error\n");
                    return NULL;
                }
                if (ufds[i].revents & POLLIN) {
                    if ((numbytes = recvfrom(config->sockfd, buf,
                                             MAXBUFLEN , 0,
                                             (struct sockaddr *)&their_addr,
                                             &addr_len)) == -1) {
                        perror("recvfrom (data)");
                        return NULL;
                    }
                    if (spud_isSpud(buf, numbytes)) {
                        struct SpudMsg *sMsg;

                        sMsg = (struct SpudMsg *)buf;

                        char idStr[SPUD_ID_STRING_SIZE+1];
                        printf(" \r Spud ID: %s",
                               spud_idToString(idStr,
                                               sizeof idStr,
                                               &sMsg->msgHdr.flags_id));
                        config->data_handler(
                            config,
                            (struct sockaddr *)&their_addr,
                            NULL,
                            buf+sizeof(*sMsg),
                            numbytes-sizeof(*sMsg));
                    }
                    else {
		                config->data_handler(config, (struct sockaddr *)&their_addr, NULL, buf, numbytes);
		            }
		        }
            }
        }
    }
}

int main(void)
{
    pthread_t socketListenThread;
    struct sockaddr_in6 servaddr;

    struct listenConfig listenConfig;

    signal(SIGINT, teardown);

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }
    sockaddr_initAsIPv6Any(&servaddr, MYPORT);
    if (bind(sockfd, (struct sockaddr*)&servaddr, servaddr.sin6_len) != 0) {
        perror("bind");
        return 1;
    }

    listenConfig.sockfd= sockfd;
    listenConfig.spud_handler = spudHandler;
    listenConfig.data_handler = dataHandler;

    pthread_create(&socketListenThread, NULL, socketListen, (void*)&listenConfig);

    while(1) {
        printf("spudecho: waiting to recvfrom...\n");

        sleep(1000);
    }
}
