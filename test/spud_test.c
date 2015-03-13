/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <stdlib.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <check.h>

#include "tube.h"
#include "spud.h"
#include "ls_error.h"
#include "ls_sockaddr.h"

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

    spud_header hdr;
    //should this be in init() instead?
    memcpy(hdr.magic, SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE);

    //copy the whole spud msg into the buffer..
    memcpy(buf, &hdr, sizeof(hdr));

    fail_unless( spud_is_spud((const uint8_t *)&buf,len),
                 "isSpud() failed");
}
END_TEST

START_TEST (createId)
{
    int len = 1024;
    unsigned char buf[len];
    char idStr[len];
    ls_err err;

    spud_header hdr;
    //should this be in init() instead?
    fail_unless(spud_init(&hdr, NULL, &err));

    memset(idStr, 0, len);
    spud_id_to_string(idStr, len, &hdr.tube_id, NULL);
    ck_assert_int_eq(strlen(idStr), SPUD_ID_STRING_SIZE);

    fail_if(spud_is_spud((const uint8_t *)&buf,len),
            "isSpud() failed");


    //copy the whole spud msg into the buffer..
    memcpy(buf, &hdr, sizeof(hdr));

    fail_unless(spud_is_spud((const uint8_t *)&buf,len),
                "isSpud() failed");
}
END_TEST

START_TEST (isIdEqual)
{
    ls_err err;

    spud_header msgA;
    spud_header msgB;//Equal to A
    spud_header msgC;//New random

    fail_unless(spud_init(&msgA, NULL, &err));
    fail_unless(spud_init(&msgB, &msgA.tube_id, &err));
    fail_unless(spud_init(&msgC, NULL, &err));

    fail_unless( spud_is_id_equal(&msgA.tube_id, &msgB.tube_id));
    fail_if( spud_is_id_equal(&msgA.tube_id, &msgC.tube_id));
    fail_if( spud_is_id_equal(&msgB.tube_id, &msgC.tube_id));
}
END_TEST

START_TEST (spud_parse_test)
{
    spud_message msg;
    ls_err err;
    uint8_t buf[] = { 0xd8, 0x00, 0x00, 0xd8,
                      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                      0x00,
                      0xa1, 0x00,
                      0x41, 0x61 };

    fail_if(spud_parse(NULL, 0, NULL, &err));
    fail_if(spud_parse(buf, 0, NULL, &err));
    fail_if(spud_parse(buf, 0, &msg, &err));
    fail_unless(spud_parse(buf, 13, &msg, &err));
    fail_unless(spud_parse(buf, sizeof(buf), &msg, &err));
}
END_TEST

START_TEST (tube_create_test)
{
    tube *t;
    int sockfd;
    ls_err err;
    tube_manager *mgr;
    fail_unless( tube_manager_create(0, &mgr, &err));
    fail_unless( tube_create(mgr, &t, &err) );
    tube_destroy(t);
    tube_manager_destroy(mgr);
}
END_TEST


static void test_cb(ls_event_data evt, void *arg){}

START_TEST (tube_manager_bind_event_test)
{
    ls_err err;
    tube_manager *mgr;
    fail_unless( tube_manager_create(0, &mgr, &err),
                 ls_err_message( err.code ));

    fail_unless( tube_manager_bind_event(mgr, EV_RUNNING_NAME, test_cb, &err),
                 ls_err_message( err.code ));
    fail_unless( tube_manager_bind_event(mgr, EV_DATA_NAME, test_cb, &err),
                 ls_err_message( err.code ));
    fail_unless( tube_manager_bind_event(mgr, EV_CLOSE_NAME, test_cb, &err),
                 ls_err_message( err.code ));
    fail_unless( tube_manager_bind_event(mgr, EV_ADD_NAME, test_cb, &err),
                 ls_err_message( err.code ));
    fail_unless( tube_manager_bind_event(mgr, EV_REMOVE_NAME, test_cb, &err),
                 ls_err_message( err.code ));
    tube_manager_destroy(mgr);
}
END_TEST

START_TEST (tube_print_test)
{
    tube *t;
    ls_err err;
    tube_manager *mgr;
    fail_unless( tube_manager_create(0, &mgr, &err),
                 ls_err_message( err.code ));
    fail_unless( tube_manager_socket(mgr, 0, &err),
                 ls_err_message( err.code ));

    fail_unless( tube_create(mgr, &t, &err),
                 ls_err_message( err.code ) );
    fail_unless( tube_print(t, &err),
                 ls_err_message( err.code ) );
    tube_manager_destroy(mgr);
}
END_TEST

START_TEST (tube_open_test)
{
    tube *t;
    tube_manager *mgr;
    ls_err err;
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "127.0.0.1",
                                                "1402",
                                                &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_manager_create(0, &mgr, &err));
    fail_unless( tube_manager_socket(mgr, 0, &err));

    fail_unless( tube_create(mgr, &t, &err) );
    fail_unless( tube_open(t, (const struct sockaddr*)&remoteAddr, &err),
                 ls_err_message( err.code ) );
    tube_manager_destroy(mgr); // should also destroy tube
}
END_TEST

START_TEST (tube_ack_test)
{
    tube *t;
    tube_manager *mgr;
    ls_err err;
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "127.0.0.1",
                                                "1402",
                                                &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_manager_create(0, &mgr, &err));
    fail_unless( tube_manager_socket(mgr, 0, &err));

    fail_unless( tube_create(mgr, &t, &err) );
    fail_unless( spud_create_id(&t->id, &err), ls_err_message( err.code ) );
    ls_sockaddr_copy((const struct sockaddr*)&remoteAddr, (struct sockaddr*)&t->peer);

    fail_unless( tube_ack(t, &err),
                 ls_err_message( err.code ) );
    tube_destroy(t);
    tube_manager_destroy(mgr);
}
END_TEST

START_TEST (tube_data_test)
{
    tube *t;
    tube_manager *mgr;
    ls_err err;
    uint8_t data[] = "SPUD_makeUBES_FUN";
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "127.0.0.1",
                                                "1402",
                                                &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_manager_create(0, &mgr, &err));
    fail_unless( tube_manager_socket(mgr, 0, &err));

    fail_unless( tube_create(mgr, &t, &err) );

    fail_unless( tube_open(t, (const struct sockaddr*)&remoteAddr, &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_data(t,
                           data,
                           17,
                           &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_data(t,
                           NULL,
                           0,
                           &err),
                 ls_err_message( err.code ) );
    tube_manager_destroy(mgr); // should also destroy tube
}
END_TEST

START_TEST (tube_close_test)
{
    tube *t;
    tube_manager *mgr;
    ls_err err;
    char data[] = "SPUD_makeUBES_FUN";
    struct sockaddr_in6 remoteAddr;

    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "127.0.0.1",
                                                "1402",
                                                &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_manager_create(17, &mgr, &err));
    fail_unless( tube_manager_socket(mgr, 0, &err));
    fail_unless( tube_create(mgr, &t, &err) );
    fail_unless( tube_open(t, (const struct sockaddr*)&remoteAddr, &err),
                 ls_err_message( err.code ) );
    fail_unless( tube_close(t, &err),
                 ls_err_message( err.code ) );
    tube_manager_destroy(mgr);
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
      tcase_add_test (tc_core, spud_parse_test);

      suite_add_tcase (s, tc_core);
  }

  {/* tube *test case */
      TCase *tc_tube = tcase_create ("TUBE");
      tcase_add_checked_fixture (tc_tube, spudlib_setup, spudlib_teardown);
      tcase_add_test (tc_tube, tube_create_test);
      tcase_add_test (tc_tube, tube_manager_bind_event_test);
      tcase_add_test (tc_tube, tube_print_test);
      tcase_add_test (tc_tube, tube_open_test);
      tcase_add_test (tc_tube, tube_ack_test);
      tcase_add_test (tc_tube, tube_data_test);
      tcase_add_test (tc_tube, tube_close_test);

      suite_add_tcase (s, tc_tube);
  }


  return s;
}
