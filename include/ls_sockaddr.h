/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#pragma once

#include <assert.h>
#include <netinet/in.h>

#include "ls_basics.h"
#include "ls_error.h"

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
