#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#define _OPEN_SYS_SOCK_IPV6
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <string.h>

#include <sockaddr_util.h>

#ifndef   NI_MAXHOST
#define   NI_MAXHOST 1025
#endif


#include "iphelper.h"

bool getLocalInterFaceAddrs(struct sockaddr *addr,
                            char *iface, int ai_family,
                            IPv6_ADDR_TYPE ipv6_addr_type,
                            bool force_privacy)
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    int rc;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        if(ifa->ifa_addr->sa_family != ai_family){
            continue;
        }

        if (sockaddr_isAddrLoopBack(ifa->ifa_addr))
            continue;

        if( strcmp(iface, ifa->ifa_name)!=0 && strcmp(iface, "default\0"))
            continue;

        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET6 ){
            if( sockaddr_isAddrDeprecated(ifa->ifa_addr, ifa->ifa_name, sizeof(ifa->ifa_name)) ){
                continue;
            }
            if (sockaddr_isAddrLinkLocal(ifa->ifa_addr)){
                continue;
            }
            if (sockaddr_isAddrSiteLocal(ifa->ifa_addr)){
                continue;
            }

            if(force_privacy){
                if( !sockaddr_isAddrTemporary(ifa->ifa_addr,
                                              ifa->ifa_name,
                                              sizeof(ifa->ifa_name)) ){
                    continue;
                }
            }

            switch(ipv6_addr_type){
                case IPv6_ADDR_NONE:
                    //huh
                    break;
                case IPv6_ADDR_ULA:
                    if (!sockaddr_isAddrULA(ifa->ifa_addr)){
                        continue;
                    }

                    break;
                case IPv6_ADDR_PRIVACY:
                    if( !sockaddr_isAddrTemporary(ifa->ifa_addr,
                                                  ifa->ifa_name,
                                                  sizeof(ifa->ifa_name)) ){
                        continue;
                    }
                    break;
                case IPv6_ADDR_NORMAL:
                    if (sockaddr_isAddrULA(ifa->ifa_addr)){
                        continue;
                    }
                    break;
            }//switch

        }//IPv6

        s = getnameinfo(ifa->ifa_addr,
                        (family == AF_INET) ? sizeof(struct sockaddr_in) :
                        sizeof(struct sockaddr_in6),
                        host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if (s != 0) {
            //printf("getnameinfo() failed: %s\n", gai_strerror(s));
            exit(EXIT_FAILURE);
        }
        break; //for

    }//for
    freeifaddrs(ifaddr);

    if (sockaddr_initFromString(addr, host))
        return true;

    return false;
}


bool getRemoteIpAddr(struct sockaddr_in6 *remoteAddr, const char *fqdn, const char *port)
{
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];
    bool found = false;

    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_V4MAPPED;
    hints.ai_family = AF_INET6; // use AI_V4MAPPED for v4 addresses
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_socktype = SOCK_DGRAM;

    if ((status = getaddrinfo(fqdn, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return false;
    }

    for(p = res;p != NULL; p = p->ai_next) {
        // copy the first match
        if (p->ai_family == AF_INET6) { // Should always be v6 (mapped for v4)
            memcpy(remoteAddr, p->ai_addr, sizeof(struct sockaddr_in6));
            found = true;
            break;
        }
    }
    freeaddrinfo(res); // free the linked list
    return found;
}
