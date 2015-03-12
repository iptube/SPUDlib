/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include "ls_sockaddr.h"
#include "ls_log.h"

LS_API size_t ls_sockaddr_get_length(const struct sockaddr *addr)
{
    switch (addr->sa_family) {
    case AF_INET:
        return sizeof(struct sockaddr_in);
    case AF_INET6:
        return sizeof(struct sockaddr_in6);
    default:
        assert(false);
        return -1;
    }
}

LS_API bool ls_sockaddr_get_remote_ip_addr(struct sockaddr_in6 *remoteAddr,
                                           const char *fqdn,
                                           const char *port,
                                           ls_err *err)
{
    struct addrinfo hints, *res, *p;
    int status;
    bool found = false;

    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_V4MAPPED;
    hints.ai_family = AF_INET6; // use AI_V4MAPPED for v4 addresses
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_socktype = SOCK_DGRAM;

    if ((status = getaddrinfo(fqdn, port, &hints, &res)) != 0) {
        ls_log(LS_LOG_ERROR, "getaddrinfo: %s\n", gai_strerror(status));
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
