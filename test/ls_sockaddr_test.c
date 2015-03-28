/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <arpa/inet.h>
#include <string.h>

#include "ls_sockaddr.h"
#include "ls_error.h"
#include "test_utils.h"

CTEST(ls_sockaddr, length)
{
    struct sockaddr_in ip4addr;
    struct sockaddr_in6 ip6addr;

    memset(&ip4addr, 0, sizeof(ip4addr));
    ASSERT_EQUAL(ls_sockaddr_get_length( (struct sockaddr *)&ip4addr ), -1);

    ip4addr.sin_family = AF_INET;
    ip4addr.sin_port = htons(3490);
    inet_pton(AF_INET, "10.0.0.1", &ip4addr.sin_addr);

    ip6addr.sin6_family = AF_INET6;
    ip6addr.sin6_port = htons(4950);
    inet_pton(AF_INET6, "2001:db8:8714:3a90::12", &ip6addr.sin6_addr);

    ASSERT_TRUE( ls_sockaddr_get_length( (struct sockaddr *)&ip4addr ) == sizeof(struct sockaddr_in));
    ASSERT_TRUE( ls_sockaddr_get_length( (struct sockaddr *)&ip6addr ) == sizeof(struct sockaddr_in6));
}

CTEST(ls_sockaddr, get_remote_ip)
{
    ls_err err;
    struct sockaddr_in6 remoteAddr;

    ASSERT_TRUE( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "1.2.3.4",
                                                 "1402",
                                                  &err) );

    ASSERT_FALSE ( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                             "foo.invalid",
                                             "1402",
                                             &err) );
}

CTEST(ls_sockaddr, v6_any)
{
    struct sockaddr_in6 addr;
    struct sockaddr_storage addr2;
    ls_sockaddr_v6_any(&addr, 443);
    ASSERT_EQUAL(ntohs(addr.sin6_port), 443);
    ASSERT_EQUAL(addr.sin6_family, AF_INET6);

    ls_sockaddr_copy((const struct sockaddr*)&addr, (struct sockaddr*)&addr2);
    ASSERT_EQUAL(memcmp(&addr, &addr2,
                            ls_sockaddr_get_length((struct sockaddr*)&addr2)), 0);
}

CTEST(ls_addr, parse)
{
    struct in6_addr addr;
    ls_err err;
    ASSERT_TRUE(ls_addr_parse("::1", &addr, &err));
    ASSERT_TRUE(ls_addr_cmp(&addr, &in6addr_loopback) == 0);
    ASSERT_TRUE(ls_addr_parse("127.0.0.1", &addr, &err));
    ASSERT_TRUE(IN6_IS_ADDR_V4MAPPED(&addr));
    ASSERT_FALSE(ls_addr_parse("fooobaaar", &addr, &err));
}

#include <stdio.h>
CTEST(ls_sockaddr, to_string)
{
    // ls_err err;  /* TODO: determine if used */
    struct sockaddr_in in4;
    struct sockaddr_in6 in6;
    char buf[MAX_SOCKADDR_STR_LEN];
    const char *iret = NULL;

    memset(&in4, 0, sizeof(in4));
    iret = ls_sockaddr_to_string((const struct sockaddr*)&in4,
                                 buf, 1, true);
    ASSERT_TRUE(iret == NULL);

    in4.sin_family = AF_INET;
    in4.sin_port = htons(65534);
    in4.sin_addr.s_addr = (u_int32_t)0xffffffff;

    iret = ls_sockaddr_to_string((const struct sockaddr*)&in4,
                                 buf, 1, true);
    ASSERT_TRUE(iret && !iret[0]);

    ASSERT_TRUE(ls_sockaddr_to_string((const struct sockaddr*)&in4,
                buf, sizeof(buf), false) != NULL);
    ASSERT_STR(buf, "255.255.255.255");

    ASSERT_TRUE(ls_sockaddr_to_string((const struct sockaddr*)&in4,
                buf, sizeof(buf), true) != NULL);
    ASSERT_STR(buf, "255.255.255.255:65534");


    memset(&in6, 0, sizeof(in6));
    in6.sin6_family = AF_INET6;
    in6.sin6_port = htons(65534);
    memset(&in6.sin6_addr, 0xff, sizeof(in6.sin6_addr));

    iret = ls_sockaddr_to_string((const struct sockaddr*)&in6,
                                 buf, 1, true);
    ASSERT_TRUE(iret && !iret[0]);
    ASSERT_TRUE(ls_sockaddr_to_string((const struct sockaddr*)&in6,
                buf, sizeof(buf), false) != NULL);
    ASSERT_STR(buf, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");

    ASSERT_TRUE(ls_sockaddr_to_string((const struct sockaddr*)&in6,
                buf, sizeof(buf), true) != NULL);
    ASSERT_STR(buf, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]:65534");
}
