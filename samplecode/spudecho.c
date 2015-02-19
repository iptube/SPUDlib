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

#include "iphelper.h"
#include "sockethelper.h"

#define MYPORT "1402"    // the port users will be connecting to
#define MAXBUFLEN 2048
#define MAX_LISTEN_SOCKETS 10

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

void dataHandler(struct listenConfig *config, struct sockaddr *from_addr, void *cb, unsigned char *buf, int bufLen)
{
  printf(".");
  sendPacket(config->sockfd,
             buf,
             bufLen,
             from_addr,
             false,
             0);
}


void spudHandler(struct listenConfig *config, struct sockaddr *from_addr, void *cb, unsigned char *buf, int buflen)
{
    //do something
}



static void *socketListen(void *ptr){
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
    
    while(1){
        rv = poll(ufds, numSockets, -1);
        if (rv == -1) {
            printf("Error in poll\n"); // error occurred in poll()
        } else if (rv == 0) {
            printf("Poll Timeout occurred! (Should not happen)\n");
        } else {
            for(i=0;i<numSockets;i++){
                if (ufds[i].revents & POLLIN) {
                    if(i == 0){
                        if ((numbytes = recvfrom(config->sockfd, buf, 
                                                 MAXBUFLEN , 0, 
                                                 (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                            printf("recvfrom (data)");
                        }
                        config->data_handler(config, (struct sockaddr *)&their_addr, NULL, buf, numbytes);
                    }
                }
            }
        }
    }
}



int main(void)
{
    struct addrinfo *servinfo, *p;
    int numbytes;
    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];

    pthread_t socketListenThread;

    struct listenConfig listenConfig;    

    signal(SIGINT, teardown);
    
    sockfd = createSocket(NULL, MYPORT, AI_PASSIVE, servinfo, &p);
    
    
    listenConfig.sockfd= sockfd;   
    listenConfig.spud_handler = spudHandler;
    listenConfig.data_handler = dataHandler;
    
    


    pthread_create( &socketListenThread, NULL, socketListen, (void*)&listenConfig);

    while(1) {
        printf("stunserver: waiting to recvfrom...\n");

        sleep(1000);
    }
}
