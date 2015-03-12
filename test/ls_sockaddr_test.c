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

START_TEST (ls_sockaddr_test_length_assert)
{
    struct sockaddr_in ip4addr;

    ip4addr.sin_family = 12;
    ip4addr.sin_port = htons(3490);
    inet_pton(AF_INET, "10.0.0.1", &ip4addr.sin_addr);
    //This should trigger the assert()
    ls_sockaddr_get_length( (struct sockaddr *)&ip4addr );
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

}
END_TEST


Suite * ls_sockaddr_suite (void)
{
  Suite *s = suite_create ("ls_sockaddr");
  {/* Sockaddr test case */
      TCase *tc_ls_sockaddr = tcase_create ("Sockaddr");
      tcase_add_test (tc_ls_sockaddr, ls_sockaddr_length_test);
      tcase_add_test_raise_signal(tc_ls_sockaddr, ls_sockaddr_test_length_assert, 6);
      tcase_add_test (tc_ls_sockaddr, ls_sockaddr_get_remote_ip);
      suite_add_tcase (s, tc_ls_sockaddr);
  }

  return s;
}
