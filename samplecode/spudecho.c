#include <bitstring.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>

#include "spudlib.h"
#include "tube.h"
#include "iphelper.h"
#include "sockethelper.h"


#define MYPORT 1402    // the port users will be connecting to
#define MAXBUFLEN 2048
#define MAX_LISTEN_SOCKETS 10

#define UNUSED(x) (void)(x)

int sockfd;

// TODO: replace this with a hashtable
#define MAX_CLIENTS 128
bitstr_t bit_decl(bs_clients, MAX_CLIENTS);
tube_t clients[MAX_CLIENTS];

tube_t* tube_unused()
{
    int n = -1;
    bit_ffc(bs_clients, MAX_CLIENTS, &n);
    if (n == -1) {
        return NULL;
    }
    bit_set(bs_clients, n);
    return &clients[n];
}

tube_t* tube_match(struct SpudMsgFlagsId *flags_id) {
    struct SpudMsgFlagsId search;
    memcpy(&search, flags_id, sizeof(search));
    search.octet[0] &= SPUD_FLAGS_EXCLUDE_MASK;

    // table scan.  I said, replace this with a hashtable.
    for (int i=0; i<MAX_CLIENTS; i++) {
        if (bit_test(bs_clients, i)) {
            if (memcmp(&search, &clients[i].id, sizeof(search)) == 0) {
                return &clients[i];
            }
        }
    }
    return NULL;
}

struct listenConfig {
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
    tube_t *tube;
    struct SpudMsg *sMsg;

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
                    if (!spud_isSpud(buf, numbytes)) {
                        // it's an attack.  Move along.
                        continue;
                    }

                    sMsg = (struct SpudMsg *)buf;
                    tube = tube_match(&sMsg->msgHdr.flags_id);
                    if (tube) {
                        // do states
                        // TODO: check their_addr
                    } else {
                        // get started
                        tube = tube_unused();
                        if (!tube) {
                            // full.
                            // TODO: send back error
                            continue;
                        }
                        tube_ack(tube,
                                 &sMsg->msgHdr.flags_id,
                                 (struct sockaddr *)&their_addr);
                    }

                    // char idStr[SPUD_ID_STRING_SIZE+1];
                    // printf(" \r Spud ID: %s",
                    //        spud_idToString(idStr,
                    //                        sizeof idStr,
                    //                        &sMsg->msgHdr.flags_id));
                    // config->data_handler(
                    //     config,
                    //     (struct sockaddr *)&their_addr,
                    //     NULL,
                    //     buf+sizeof(*sMsg),
                    //     numbytes-sizeof(*sMsg));
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

    memset(clients, 0, MAX_CLIENTS*sizeof(tube_t));
    bit_nclear(bs_clients, 0, MAX_CLIENTS-1);

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

    for (int i=0; i<MAX_CLIENTS; i++) {
        tube_init(&clients[i], sockfd);
    }

    pthread_create(&socketListenThread, NULL, socketListen, (void*)&listenConfig);

    while(1) {
        printf("spudecho: waiting to recvfrom...\n");

        sleep(1000);
    }
}
