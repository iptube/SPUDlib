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
  struct sockaddr_in  ip4addr;
  struct sockaddr_in6 ip6addr;

  memset( &ip4addr, 0, sizeof(ip4addr) );
  ASSERT_EQUAL(ls_sockaddr_get_length( (struct sockaddr*)&ip4addr ), -1);

  ip4addr.sin_family = AF_INET;
  ip4addr.sin_port   = htons(3490);
  inet_pton(AF_INET, "10.0.0.1", &ip4addr.sin_addr);

  ip6addr.sin6_family = AF_INET6;
  ip6addr.sin6_port   = htons(4950);
  inet_pton(AF_INET6, "2001:db8:8714:3a90::12", &ip6addr.sin6_addr);

  ASSERT_TRUE( ls_sockaddr_get_length( (struct sockaddr*)&ip4addr ) ==
               sizeof(struct sockaddr_in) );
  ASSERT_TRUE( ls_sockaddr_get_length( (struct sockaddr*)&ip6addr ) ==
               sizeof(struct sockaddr_in6) );
}

CTEST(ls_sockaddr, port)
{
  struct sockaddr_storage addr;
  memset( &addr, 0, sizeof(addr) );

  ASSERT_EQUAL( -1, ls_sockaddr_get_port( (struct sockaddr*)&addr ) );

  ls_sockaddr_v6_any( (struct sockaddr_in6*)&addr, 443 );
  ASSERT_EQUAL( 443, ls_sockaddr_get_port( (struct sockaddr*)&addr ) );
  ls_sockaddr_set_port( (struct sockaddr*)&addr, 65535 );
  ASSERT_EQUAL( 65535, ls_sockaddr_get_port( (struct sockaddr*)&addr ) );

  ls_sockaddr_v4_any( (struct sockaddr_in*)&addr, 443 );
  ASSERT_EQUAL( 443, ls_sockaddr_get_port( (struct sockaddr*)&addr ) );
  ls_sockaddr_set_port( (struct sockaddr*)&addr, 65535 );
  ASSERT_EQUAL( 65535, ls_sockaddr_get_port( (struct sockaddr*)&addr ) );
}

CTEST(ls_sockaddr, get_remote_ip)
{
  ls_err                  err;
  struct sockaddr_storage remoteAddr;

  ASSERT_TRUE( ls_sockaddr_get_remote_ip_addr("1.2.3.4",
                                              "1402",
                                              (struct sockaddr*)&remoteAddr,
                                              sizeof(remoteAddr),
                                              &err) );

  ASSERT_FALSE( ls_sockaddr_get_remote_ip_addr("foo.invalid",
                                               "1402",
                                               (struct sockaddr*)&remoteAddr,
                                               sizeof(remoteAddr),
                                               &err) );

  ASSERT_FALSE( ls_sockaddr_get_remote_ip_addr("localhost",
                                               "1402",
                                               (struct sockaddr*)&remoteAddr,
                                               0,
                                               &err) );
}

CTEST(ls_sockaddr, v6_any)
{
  struct sockaddr_in6     addr;
  struct sockaddr_storage addr2;
  ls_sockaddr_v6_any(&addr, 443);
  ASSERT_EQUAL(ntohs(addr.sin6_port), 443);
  ASSERT_EQUAL(addr.sin6_family,      AF_INET6);

  ls_sockaddr_copy( (const struct sockaddr*)&addr, (struct sockaddr*)&addr2 );
  ASSERT_EQUAL(memcmp( &addr, &addr2,
                       ls_sockaddr_get_length( (struct sockaddr*)&addr2 ) ), 0);
}

CTEST(ls_addr, parse)
{
  struct sockaddr_storage addr;
  ls_err                  err;
  ASSERT_TRUE( ls_sockaddr_parse("::1", (struct sockaddr*)&addr, sizeof(addr),
                                 &err) );
  ASSERT_EQUAL( AF_INET6, addr.ss_family);
  ASSERT_EQUAL( 0,
                ls_addr_cmp6(&( (struct sockaddr_in6*)&addr )->sin6_addr,
                             &in6addr_loopback) );
  ASSERT_TRUE( ls_sockaddr_parse("127.0.0.1", (struct sockaddr*)&addr,
                                 sizeof(addr), &err) );
  ASSERT_EQUAL( AF_INET, addr.ss_family);
  ASSERT_EQUAL( 0x7f000001,
                ntohl( (u_int32_t)( (struct sockaddr_in*)&addr )->sin_addr.
                       s_addr ) );
  ASSERT_FALSE( ls_sockaddr_parse("fooobaaar", (struct sockaddr*)&addr,
                                  sizeof(addr), &err) );
}

#include <stdio.h>
CTEST(ls_sockaddr, to_string)
{
  /* ls_err err;  / * TODO: determine if used * / */
  struct sockaddr_in  in4;
  struct sockaddr_in6 in6;
  char                buf[MAX_SOCKADDR_STR_LEN];
  const char*         iret = NULL;

  memset( &in4, 0, sizeof(in4) );
  iret = ls_sockaddr_to_string(NULL,
                               buf, 1, true);
  ASSERT_STR("", iret);

  iret = ls_sockaddr_to_string( (const struct sockaddr*)&in4,
                                buf, 1, true );
  ASSERT_STR("", iret);

  iret = ls_sockaddr_to_string( (const struct sockaddr*)&in4,
                                buf, sizeof(buf), true );
  ASSERT_STR("", iret);

  in4.sin_family      = AF_INET;
  in4.sin_port        = htons(65534);
  in4.sin_addr.s_addr = (u_int32_t)0xffffffff;

  iret = ls_sockaddr_to_string( (const struct sockaddr*)&in4,
                                buf, 1, true );
  ASSERT_TRUE(iret && !iret[0]);

  ASSERT_TRUE(ls_sockaddr_to_string( (const struct sockaddr*)&in4,
                                     buf, sizeof(buf), false ) != NULL);
  ASSERT_STR(buf, "255.255.255.255");

  ASSERT_TRUE(ls_sockaddr_to_string( (const struct sockaddr*)&in4,
                                     buf, sizeof(buf), true ) != NULL);
  ASSERT_STR(buf, "255.255.255.255:65534");


  memset( &in6,           0,    sizeof(in6) );
  in6.sin6_family = AF_INET6;
  in6.sin6_port   = htons(65534);
  memset( &in6.sin6_addr, 0xff, sizeof(in6.sin6_addr) );

  iret = ls_sockaddr_to_string( (const struct sockaddr*)&in6,
                                buf, 1, true );
  ASSERT_TRUE(iret && !iret[0]);
  ASSERT_TRUE(ls_sockaddr_to_string( (const struct sockaddr*)&in6,
                                     buf, sizeof(buf), false ) != NULL);
  ASSERT_STR(buf, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");

  ASSERT_TRUE(ls_sockaddr_to_string( (const struct sockaddr*)&in6,
                                     buf, sizeof(buf), true ) != NULL);
  ASSERT_STR(buf, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]:65534");
}

CTEST(ls_sockaddr, cmp)
{
  struct sockaddr_in  a1;
  struct sockaddr_in  a2;
  struct sockaddr_in  b1;
  struct sockaddr_in  b2;
  struct sockaddr_in6 c1;
  struct sockaddr_in6 c2;

  ls_err err;

  ASSERT_EQUAL(ls_sockaddr_cmp(NULL, NULL), 0);

  ASSERT_TRUE( ls_sockaddr_get_remote_ip_addr("1.2.3.4",
                                              "1402",
                                              (struct sockaddr*)&a1,
                                              sizeof(a1),
                                              &err) );

  ASSERT_TRUE( ls_sockaddr_get_remote_ip_addr("1.2.3.4",
                                              "1403",
                                              (struct sockaddr*)&a2,
                                              sizeof(a2),
                                              &err) );

  ASSERT_TRUE(ls_sockaddr_cmp(NULL, (const struct sockaddr*)&a2) < 0);
  ASSERT_TRUE(ls_sockaddr_cmp( (const struct sockaddr*)&a1, NULL ) > 0);
  ASSERT_TRUE(ls_sockaddr_cmp( (const struct sockaddr*)&a1,
                               (const struct sockaddr*)&a2 ) < 0);
  ASSERT_TRUE(ls_sockaddr_cmp( (const struct sockaddr*)&a2,
                               (const struct sockaddr*)&a1 ) > 0);
  ASSERT_EQUAL(ls_sockaddr_cmp( (const struct sockaddr*)&a1,
                                (const struct sockaddr*)&a1 ), 0);

  memset( &b1, 0, sizeof(b1) );
  b1.sin_addr.s_addr = INADDR_ANY;
  b1.sin_port        = htons(1402);
  b1.sin_family      = AF_INET;
  ASSERT_TRUE(ls_sockaddr_cmp( (const struct sockaddr*)&b1,
                               (const struct sockaddr*)&a1 ) < 0);
  ASSERT_TRUE(ls_sockaddr_cmp( (const struct sockaddr*)&a1,
                               (const struct sockaddr*)&b1 ) > 0);

  b2                 = b1;
  b2.sin_addr.s_addr = 0x00000001;
  ASSERT_TRUE(ls_sockaddr_cmp( (const struct sockaddr*)&b1,
                               (const struct sockaddr*)&b2 ) < 0);
  ASSERT_TRUE(ls_sockaddr_cmp( (const struct sockaddr*)&b2,
                               (const struct sockaddr*)&b1 ) > 0);

  b2.sin_addr.s_addr = INADDR_ANY;
  b2.sin_port        = htons(1403);
  ASSERT_TRUE(ls_sockaddr_cmp( (const struct sockaddr*)&b1,
                               (const struct sockaddr*)&b2 ) < 0);
  ASSERT_TRUE(ls_sockaddr_cmp( (const struct sockaddr*)&b2,
                               (const struct sockaddr*)&b1 ) > 0);

  memset( &c1, 0, sizeof(c1) );
  memset( &c2, 0, sizeof(c1) );
  ASSERT_EQUAL( 0,
                ls_sockaddr_cmp( (const struct sockaddr*)&c1,
                                 (const struct sockaddr*)&c2 ) );

  ls_sockaddr_v6_any(&c1, 443);
  ls_sockaddr_v6_any(&c2, 443);
  ASSERT_EQUAL( 0,
                ls_sockaddr_cmp( (const struct sockaddr*)&c1,
                                 (const struct sockaddr*)&c2 ) );
  ASSERT_TRUE(ls_sockaddr_cmp( (const struct sockaddr*)&b1,
                               (const struct sockaddr*)&c2 ) < 0);
}
