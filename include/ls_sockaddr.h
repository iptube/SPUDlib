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
