#include <stdlib.h>
#include <stdio.h>

#include <netinet/in.h>
#include <netdb.h>

#include <check.h>

#include "spudlib.h"
#include "ls_error.h"

void spudlib_setup (void);
void spudlib_teardown (void);
Suite * spudlib_suite (void);

void
spudlib_setup (void)
{

}

void
spudlib_teardown (void)
{

}

START_TEST (empty)
{

    fail_unless( 1==1,
                 "Test app fails");

}
END_TEST

START_TEST (is_spud)
{
    int len = 1024;
    unsigned char buf[len];

    spud_header_t hdr;
    //should this be in init() instead?
    memcpy(hdr.magic, SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE);

    //copy the whole spud msg into the buffer..
    memcpy(buf, &hdr, sizeof(hdr));

    fail_unless( spud_isSpud((const uint8_t *)&buf,len),
                 "isSpud() failed");
}
END_TEST

START_TEST (createId)
{
    int len = 1024;
    unsigned char buf[len];
    char idStr[len];
    ls_err err;

    spud_header_t hdr;
    //should this be in init() instead?
    fail_unless(spud_init(&hdr, NULL, &err));

    printf("ID: %s\n", spud_idToString(idStr, len, &hdr.tube_id, NULL));

    fail_if(spud_isSpud((const uint8_t *)&buf,len),
            "isSpud() failed");


    //copy the whole spud msg into the buffer..
    memcpy(buf, &hdr, sizeof(hdr));

    fail_unless(spud_isSpud((const uint8_t *)&buf,len),
                "isSpud() failed");
}
END_TEST

START_TEST (isIdEqual)
{
    ls_err err;

    spud_header_t msgA;
    spud_header_t msgB;//Equal to A
    spud_header_t msgC;//New random

    fail_unless(spud_init(&msgA, NULL, &err));
    fail_unless(spud_init(&msgB, &msgA.tube_id, &err));
    fail_unless(spud_init(&msgC, NULL, &err));

    fail_unless( spud_isIdEqual(&msgA.tube_id, &msgB.tube_id));
    fail_if( spud_isIdEqual(&msgA.tube_id, &msgC.tube_id));
    fail_if( spud_isIdEqual(&msgB.tube_id, &msgC.tube_id));
}
END_TEST

START_TEST (spud_parse_test)
{


}
END_TEST

START_TEST (ls_sockaddr_test)
{
    struct sockaddr_in ip4addr;
    struct sockaddr_in6 ip6addr;

    ip4addr.sin_family = AF_INET;
    ip4addr.sin_port = htons(3490);
    inet_pton(AF_INET, "10.0.0.1", &ip4addr.sin_addr);

    ip6addr.sin6_family = AF_INET6;
    ip6addr.sin6_port = htons(4950);
    inet_pton(AF_INET6, "2001:db8:8714:3a90::12", &ip6addr.sin6_addr);

    fail_unless( ls_sockaddr_get_length( &ip4addr ) == sizeof(struct sockaddr_in));
    fail_unless( ls_sockaddr_get_length( &ip6addr ) == sizeof(struct sockaddr_in6));
}
END_TEST

START_TEST (ls_sockaddr_test_assert)
{
    struct sockaddr_in ip4addr;

    ip4addr.sin_family = 12;
    ip4addr.sin_port = htons(3490);
    inet_pton(AF_INET, "10.0.0.1", &ip4addr.sin_addr);
    //This should trigger the assert()
    printf("Length %i:\n",ls_sockaddr_get_length( &ip4addr ));

}
END_TEST



Suite * spudlib_suite (void)
{
  Suite *s = suite_create ("spudlib");

  {/* Core test case */
      TCase *tc_core = tcase_create ("Core");
      tcase_add_checked_fixture (tc_core, spudlib_setup, spudlib_teardown);
      tcase_add_test (tc_core, empty);
      tcase_add_test (tc_core, is_spud);
      tcase_add_test (tc_core, createId);
      tcase_add_test (tc_core, isIdEqual);

      suite_add_tcase (s, tc_core);
  }

  {/* Sockaddr test case */
      TCase *tc_ls_sockaddr = tcase_create ("Sockaddr");
      tcase_add_checked_fixture (tc_ls_sockaddr, spudlib_setup, spudlib_teardown);
      tcase_add_test (tc_ls_sockaddr, ls_sockaddr_test);
      tcase_add_test_raise_signal(tc_ls_sockaddr, ls_sockaddr_test_assert, 6);
      suite_add_tcase (s, tc_ls_sockaddr);
  }

  return s;
}
