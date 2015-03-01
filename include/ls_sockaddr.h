#pragma once

#include <assert.h>
#include <netinet/in.h>

#include "ls_basics.h"

LS_API size_t ls_sockaddr_get_length(const struct sockaddr *addr);
