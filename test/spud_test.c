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
#include "ls_str.h"

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

    fail_unless( ls_sockaddr_get_length( (struct sockaddr *)&ip4addr ) == sizeof(struct sockaddr_in));
    fail_unless( ls_sockaddr_get_length( (struct sockaddr *)&ip6addr ) == sizeof(struct sockaddr_in6));
}
END_TEST

START_TEST (ls_sockaddr_test_assert)
{
    struct sockaddr_in ip4addr;

    ip4addr.sin_family = 12;
    ip4addr.sin_port = htons(3490);
    inet_pton(AF_INET, "10.0.0.1", &ip4addr.sin_addr);
    //This should trigger the assert()
    printf("Length %zd:\n",ls_sockaddr_get_length( (struct sockaddr *)&ip4addr ));
}
END_TEST



START_TEST (tube_create_test)
{
    tube t;
    int sockfd;
    ls_err err;
    ls_event_dispatcher dispatcher;
    fail_unless( ls_event_dispatcher_create(&t, &dispatcher, &err) );
    fail_unless( tube_create(sockfd, NULL, &t, &err) );
    fail_unless( tube_create(sockfd, dispatcher, &t, &err) );

}
END_TEST

START_TEST (tube_destroy_test)
{
    tube t;
    int sockfd;
    ls_err err;
    fail_unless( tube_create(sockfd, NULL, &t, &err) );
    tube_destroy(t);
}
END_TEST

static void running_cb(ls_event_data evt, void *arg){}
static void close_cb(ls_event_data evt, void *arg){}
static void data_cb(ls_event_data evt, void *arg){}

START_TEST (tube_bind_events_test)
{
    tube t;
    int sockfd;
    ls_err err;
    char arg[12];
    ls_event_dispatcher dispatcher;

    fail_unless( ls_event_dispatcher_create(&t, &dispatcher, &err) );

    fail_unless( tube_bind_events(dispatcher,
                                  running_cb,
                                  data_cb,
                                  close_cb,
                                  arg,
                                  &err), ls_err_message( err.code ) );
}
END_TEST

START_TEST (tube_print_test)
{
    tube t;
    int sockfd;
    ls_err err;

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);

    fail_unless( tube_create(sockfd, NULL, &t, &err) );
    fail_unless( tube_print(t, &err),
                 ls_err_message( err.code ) );
}
END_TEST

START_TEST (tube_open_test)
{
    tube t;
    int sockfd;
    ls_err err;
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_get_remote_ip_addr(&remoteAddr,
                                       "1.2.3.4",
                                       "1402",
                                       &err),
                 ls_err_message( err.code ) );

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);

    fail_unless( tube_create(sockfd, NULL, &t, &err) );
    fail_unless( tube_open(t, (const struct sockaddr*)&remoteAddr, &err),
                 ls_err_message( err.code ) );
}
END_TEST

START_TEST (tube_ack_test)
{
    tube t;
    int sockfd;
    ls_err err;
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_get_remote_ip_addr(&remoteAddr,
                                       "1.2.3.4",
                                       "1402",
                                       &err),
                 ls_err_message( err.code ) );

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);

    fail_unless( tube_create(sockfd, NULL, &t, &err) );
    fail_unless( tube_ack(t,
                          &t->id,
                          (const struct sockaddr*)&remoteAddr,
                          &err),
                 ls_err_message( err.code ) );
}
END_TEST

START_TEST (tube_data_test)
{
    tube t;
    int sockfd;
    ls_err err;
    char data[] = "SPUD_make_TUBES_FUN";
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_get_remote_ip_addr(&remoteAddr,
                                       "1.2.3.4",
                                       "1402",
                                       &err),
                 ls_err_message( err.code ) );

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);

    fail_unless( tube_create(sockfd, NULL, &t, &err) );

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

}
END_TEST

START_TEST (tube_close_test)
{
    tube t;
    int sockfd;
    ls_err err;
    char data[] = "SPUD_make_TUBES_FUN";
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_get_remote_ip_addr(&remoteAddr,
                                       "1.2.3.4",
                                       "1402",
                                       &err),
                 ls_err_message( err.code ) );

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);

    fail_unless( tube_create(sockfd, NULL, &t, &err) );

    fail_unless( tube_open(t, (const struct sockaddr*)&remoteAddr, &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_close(t,
                            &err),
                 ls_err_message( err.code ) );

}
END_TEST

START_TEST (tube_recv_test)
{
    tube t;
    int sockfd;
    ls_err err;
    char data[] = "SPUD_make_TUBES_FUN";
    spud_message_t sMsg = {NULL, NULL};
    spud_header_t smh;
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_get_remote_ip_addr(&remoteAddr,
                                       "1.2.3.4",
                                       "1402",
                                       &err),
                 ls_err_message( err.code ) );

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);

    fail_unless( tube_create(sockfd, NULL, &t, &err) );

    fail_unless( tube_open(t, (const struct sockaddr*)&remoteAddr, &err),
                 ls_err_message( err.code ) );

    fail_unless( spud_init( &smh, &t->id, &err) );

    smh.flags = 0;
    sMsg.header = &smh;
    t->state = TS_RUNNING;
    fail_unless( tube_recv(t,
                           &sMsg,
                           (const struct sockaddr*)&remoteAddr,
                           &err),
                 ls_err_message( err.code ) );

    smh.flags |= SPUD_OPEN;
    fail_unless( tube_recv(t,
                           &sMsg,
                           (const struct sockaddr*)&remoteAddr,
                           &err),
                 ls_err_message( err.code ) );

    smh.flags |= SPUD_ACK;
    t->state = TS_OPENING;
    fail_unless( tube_recv(t,
                           &sMsg,
                           (const struct sockaddr*)&remoteAddr,
                           &err),
                 ls_err_message( err.code ) );


    smh.flags |= SPUD_CLOSE;
    t->state = TS_RUNNING;
    fail_unless( tube_recv(t,
                           &sMsg,
                           (const struct sockaddr*)&remoteAddr,
                           &err),
                 ls_err_message( err.code ) );

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

  {/* TUBE test case */
      TCase *tc_tube = tcase_create ("TUBE");
      tcase_add_checked_fixture (tc_tube, spudlib_setup, spudlib_teardown);
      tcase_add_test (tc_tube, tube_create_test);
      tcase_add_test (tc_tube, tube_destroy_test);
      //tcase_add_test (tc_tube, tube_bind_events_test);
      //tcase_add_test (tc_tube, tube_print_test);
      //tcase_add_test (tc_tube, tube_open_test);
      //tcase_add_test (tc_tube, tube_ack_test);
      //tcase_add_test (tc_tube, tube_data_test);
      //tcase_add_test (tc_tube, tube_close_test);
      tcase_add_test (tc_tube, tube_recv_test);

      suite_add_tcase (s, tc_tube);
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
