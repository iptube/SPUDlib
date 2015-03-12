/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#pragma once
#include <stdint.h>

typedef enum IPv6_ADDR_TYPES {IPv6_ADDR_NONE,
                              IPv6_ADDR_ULA,
                              IPv6_ADDR_PRIVACY,
                              IPv6_ADDR_NORMAL} IPv6_ADDR_TYPE;


bool getLocalInterFaceAddrs(struct sockaddr *addr,
                            char *iface, int ai_family,
                            IPv6_ADDR_TYPE ipv6_addr_type,
                            bool force_privacy);
