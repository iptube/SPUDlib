/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <check.h>

#include <arpa/inet.h>

#include "ls_sockaddr.h"
#include "ls_error.h"

Suite * ls_sockaddr_suite (void);

START_TEST (ls_sockaddr_length_test)
{
    struct sockaddr_in ip4addr;
    struct sockaddr_in6 ip6addr;

    memset(&ip4addr, 0, sizeof(ip4addr));
    ck_assert_int_eq(ls_sockaddr_get_length( (struct sockaddr *)&ip4addr ), -1);

    ip4addr.sin_family = AF_INET;
    ip4addr.sin_port = htons(3490);
    inet_pton(AF_INET, "10.0.0.1", &ip4addr.sin_addr);

    ip6addr.sin6_family = AF_INET6;
    ip6addr.sin6_port = htons(4950);
    inet_pton(AF_INET6, "2001:db8:8714:3a90::12", &ip6addr.sin6_addr);

    fail_unless( ls_sockaddr_get_length( (struct sockaddr *)&ip4addr ) == sizeof(struct sockaddr_in));
    fail_unless( ls_sockaddr_get_length( (struct sockaddr *)&ip6addr ) == sizeof(struct sockaddr_in6));
}
END_TEST

START_TEST (ls_sockaddr_get_remote_ip)
{
    ls_err err;
    struct sockaddr_in6 remoteAddr;

    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "1.2.3.4",
                                                 "1402",
                                                  &err),
                     ls_err_message( err.code ) );

    fail_if ( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                             "foo.invalid",
                                             "1402",
                                             &err),
                     ls_err_message( err.code ) );
}
END_TEST

START_TEST (ls_sockaddr_v6_any_test)
{
    struct sockaddr_in6 addr;
    struct sockaddr_storage addr2;
    ls_sockaddr_v6_any(&addr, 443);
    ck_assert_int_eq(ntohs(addr.sin6_port), 443);
    ck_assert_int_eq(addr.sin6_family, AF_INET6);

    ls_sockaddr_copy((const struct sockaddr*)&addr, (struct sockaddr*)&addr2);
    ck_assert_int_eq(memcmp(&addr, &addr2,
                            ls_sockaddr_get_length((struct sockaddr*)&addr2)), 0);
}
END_TEST

#include <stdio.h>
START_TEST (ls_sockaddr_to_string_test)
{
    ls_err err;
    struct sockaddr_in in4;
    struct sockaddr_in6 in6;
    char buf[MAX_SOCKADDR_STR_LEN];
    const char *iret = NULL;

    memset(&in4, 0, sizeof(in4));
    iret = ls_sockaddr_to_string((const struct sockaddr*)&in4,
                                 buf, 1, true);
    fail_unless(iret == NULL);

    in4.sin_family = AF_INET;
    in4.sin_port = htons(65534);
    in4.sin_addr.s_addr = (u_int32_t)0xffffffff;

    iret = ls_sockaddr_to_string((const struct sockaddr*)&in4,
                                 buf, 1, true);
    fail_unless(iret && !iret[0]);

    fail_unless(ls_sockaddr_to_string((const struct sockaddr*)&in4,
                buf, sizeof(buf), false) != NULL);
    ck_assert_str_eq(buf, "255.255.255.255");

    fail_unless(ls_sockaddr_to_string((const struct sockaddr*)&in4,
                buf, sizeof(buf), true) != NULL);
    ck_assert_str_eq(buf, "255.255.255.255:65534");


    memset(&in6, 0, sizeof(in6));
    in6.sin6_family = AF_INET6;
    in6.sin6_port = htons(65534);
    memset(&in6.sin6_addr, 0xff, sizeof(in6.sin6_addr));

    iret = ls_sockaddr_to_string((const struct sockaddr*)&in6,
                                 buf, 1, true);
    fail_unless(iret && !iret[0]);
    fail_unless(ls_sockaddr_to_string((const struct sockaddr*)&in6,
                buf, sizeof(buf), false) != NULL);
    ck_assert_str_eq(buf, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");

    fail_unless(ls_sockaddr_to_string((const struct sockaddr*)&in6,
                buf, sizeof(buf), true) != NULL);
    ck_assert_str_eq(buf, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]:65534");
}
END_TEST

Suite * ls_sockaddr_suite (void)
{
  Suite *s = suite_create ("ls_sockaddr");
  {/* Sockaddr test case */
      TCase *tc_ls_sockaddr = tcase_create ("Sockaddr");
      tcase_add_test (tc_ls_sockaddr, ls_sockaddr_length_test);
      tcase_add_test (tc_ls_sockaddr, ls_sockaddr_get_remote_ip);
      tcase_add_test (tc_ls_sockaddr, ls_sockaddr_v6_any_test);
      tcase_add_test (tc_ls_sockaddr, ls_sockaddr_to_string_test);

      suite_add_tcase (s, tc_ls_sockaddr);
  }

  return s;
}
