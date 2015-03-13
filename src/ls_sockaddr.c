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
        //assert(false);
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
        if (err != NULL)
        {
            err->code = -1000 - status;
            err->message = gai_strerror(status);
            err->function = __func__;
            err->file = __FILE__;
            err->line = __LINE__;
        }
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
    if (!found) {
        LS_ERROR(err, LS_ERR_NOT_FOUND);
    }
    return found;
}

LS_API void ls_sockaddr_v6_any(struct sockaddr_in6 * sa, int port)
{
    memset(sa, 0, sizeof(*sa));
    sa->sin6_family = AF_INET6;
    sa->sin6_addr = in6addr_any;
    //sa->sin6_len = sizeof(*sa);
    sa->sin6_port = htons(port);
}

LS_API void ls_sockaddr_copy(const struct sockaddr *src, struct sockaddr *dest) {
    memcpy(dest, src, ls_sockaddr_get_length(src));
}
