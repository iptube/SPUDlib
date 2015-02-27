#include <stdio.h>
#include <string.h>

#include "sockethelper.h"

int createLocalUDPSocket(int ai_family,
                         const struct sockaddr *localIp,
                         uint16_t port)
{
    int sockfd = 0;

    int rv;
    struct addrinfo hints, *ai, *p;
    char addr[SOCKADDR_MAX_STRLEN];
    char service[8];

    sockaddr_toString(localIp, addr, sizeof addr, false);

    //itoa(port, service, 10);

    snprintf(service, 8, "%d", port);
    //snprintf(service, 8, "%d", 3478);


    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICHOST | AI_ADDRCONFIG;


    if ((rv = getaddrinfo(addr, service, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s ('%s')\n", gai_strerror(rv), addr);
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next) {
        if (sockaddr_isAddrAny(p->ai_addr) ){
            //printf("Ignoring any\n");
            continue;
        }

        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            printf("Bind failed\n");
            close(sockfd);
            continue;
        }

        if (localIp != NULL){
            struct sockaddr_storage ss;
            socklen_t len = sizeof(ss);
            if (getsockname(sockfd, (struct sockaddr *)&ss, &len) == -1){
                perror("getsockname");
            }
            else{
                if (ss.ss_family == AF_INET) {
                    ((struct sockaddr_in *)p->ai_addr)->sin_port = ((struct sockaddr_in *)&ss)->sin_port;
                }else{
                    ((struct sockaddr_in6 *)p->ai_addr)->sin6_port = ((struct sockaddr_in6 *)&ss)->sin6_port;
                }
            }
        }
        break;
    }
    return sockfd;
}

int createSocket(char host[], char port[],
                 int ai_flags,
                 struct addrinfo *servinfo,
                 struct addrinfo **p)
{
    struct addrinfo hints;
    int rv, sockfd;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = ai_flags; // use my IP if not 0

    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through all the results and bind to the first we can
    for((*p) = servinfo; (*p) != NULL; (*p) = (*p)->ai_next) {
        if ((sockfd = socket((*p)->ai_family, (*p)->ai_socktype,
                (*p)->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if (ai_flags != 0 && bind(sockfd, (*p)->ai_addr, (*p)->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind");
            continue;
        }
        break;
    }
    if ((*p) == NULL) {
        fprintf(stderr, "failed to bind socket\n");
        return -2;
    }
    return sockfd;
}

void sendPacket(int sockHandle,
                const uint8_t *buf,
                int bufLen,
                const struct sockaddr *dstAddr,
                uint8_t ttl)
{
    int32_t numbytes;
    //char addrStr[SOCKADDR_MAX_STRLEN];
    uint32_t sock_ttl;
    uint32_t addr_len;

    if(dstAddr->sa_family == AF_INET)
        addr_len = sizeof(struct sockaddr_in);
    else
        addr_len = sizeof(struct sockaddr_in6);

    if(ttl>0){
        /*Special TTL, set it send packet and set it back*/
        //printf("Setting TTL\n");
        int old_ttl = 32;
        unsigned int optlen;
        if(dstAddr->sa_family == AF_INET){
            getsockopt(sockHandle, IPPROTO_IP, IP_TTL, &old_ttl, &optlen);
            //printf("Got old ttl on socket: %i\n", old_ttl);
        }
        else{
            getsockopt(sockHandle, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &old_ttl, &optlen);
        }
        sock_ttl = ttl;

        //sockaddr_toString(dstAddr, addrStr, SOCKADDR_MAX_STRLEN, true);
        //printf("Sending Raw (To: '%s'(%i), Bytes:%i/%i  (Addr size: %u)\n", addrStr, sockHandle, numbytes, bufLen,addr_len);

        if(dstAddr->sa_family == AF_INET)
            setsockopt(sockHandle, IPPROTO_IP, IP_TTL, &sock_ttl, sizeof(sock_ttl));
        else
            setsockopt(sockHandle, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &sock_ttl, sizeof(sock_ttl));

        if ((numbytes = sendto(sockHandle, buf, bufLen, 0, dstAddr, addr_len)) == -1) {
            perror("Stun sendto");
            exit(1);
        }
        if(dstAddr->sa_family == AF_INET){
            //printf("Setting old ttl on socket: %i\n", old_ttl);
            setsockopt(sockHandle, IPPROTO_IP, IP_TTL, &old_ttl, sizeof(sock_ttl));
        }
        else{
            setsockopt(sockHandle, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &old_ttl, optlen);
        }

    }else{
        /*Nothing special, just send the packet*/
        if ((numbytes = sendto(sockHandle, buf, bufLen, 0, dstAddr, addr_len)) == -1) {
            perror("Stun sendto");
            exit(1);
        }
    }
}
