/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#pragma once

#include <assert.h>
#include <netinet/in.h>

#include "ls_basics.h"
#include "ls_error.h"

#define  MAX_SOCKADDR_STR_LEN INET6_ADDRSTRLEN + 10

LS_API size_t ls_sockaddr_get_length(const struct sockaddr *addr);

LS_API bool ls_sockaddr_get_remote_ip_addr(struct sockaddr_in6 *remoteAddr,
                                           const char *fqdn,
                                           const char *port,
                                           ls_err *err);

LS_API void ls_sockaddr_v6_any(struct sockaddr_in6 * sa, int port);

LS_API void ls_sockaddr_copy(const struct sockaddr *src, struct sockaddr *dest);

LS_API const char *ls_sockaddr_to_string(const struct sockaddr *sa,
                                         char *dest,
                                         size_t destlen,
                                         bool addport);

LS_API bool ls_addr_parse(const char *src,
                          struct in6_addr *addr,
                          ls_err *err);

LS_API int ls_addr_cmp(const struct in6_addr *a, const struct in6_addr *b);

LS_API int ls_sockaddr_cmp(const struct sockaddr * sa,
                           const struct sockaddr * sb);
