#include <arpa/inet.h>
#include <netinet/in.h>

#include <check.h>
#include "tube.h"
#include "ls_sockaddr.h"

Suite * tube_suite (void);


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

START_TEST (tube_utilities_test)
{
    tube_manager *mgr;
    tube *t;
    spud_tube_id id;
    ls_err err;
    int local_data = 1337;
    int *out;
    char buf[24];

    fail_unless( tube_manager_create(0, &mgr, &err),
                 ls_err_message( err.code ));
    fail_if(tube_manager_running(mgr));
    fail_unless( tube_manager_socket(mgr, 0, &err),
                 ls_err_message( err.code ));
    fail_unless(tube_manager_running(mgr));
    ck_assert_int_eq(tube_manager_size(mgr), 0);

    fail_unless( tube_create(mgr, &t, &err),
                 ls_err_message( err.code ) );
    tube_set_data(t, &local_data);
    out = tube_get_data(t);
    ck_assert_int_eq(*out, 1337);
    ck_assert_int_eq(tube_get_state(t), TS_UNKNOWN);

    fail_unless( tube_get_id(t, &id, &err) );
    fail_unless( tube_id_to_string(t, buf, sizeof(buf)) == buf);

    tube_manager_stop(mgr);
    fail_if(tube_manager_running(mgr));
    tube_destroy(t);
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

    ck_assert_int_eq(tube_manager_size(mgr), 1);

    tube_manager_destroy(mgr); // should also destroy tube
}
END_TEST

START_TEST (tube_ack_test)
{
    tube *t;
    tube_manager *mgr;
    ls_err err;
    struct sockaddr_in6 remoteAddr;
    spud_tube_id id;

    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "127.0.0.1",
                                                "1402",
                                                &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_manager_create(0, &mgr, &err));
    fail_unless( tube_manager_socket(mgr, 0, &err));

    fail_unless( tube_create(mgr, &t, &err) );

    fail_unless( spud_create_id(&id, &err), ls_err_message( err.code ) );
    fail_unless( tube_ack(t, &id, (const struct sockaddr*)&remoteAddr, &err),
                 ls_err_message( err.code ) );
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

Suite * tube_suite (void)
{
  Suite *s = suite_create ("tube");
  {/* tube *test case */
      TCase *tc_tube = tcase_create ("TUBE");
      tcase_add_test (tc_tube, tube_create_test);
      tcase_add_test (tc_tube, tube_manager_bind_event_test);
      tcase_add_test (tc_tube, tube_utilities_test);
      tcase_add_test (tc_tube, tube_print_test);
      tcase_add_test (tc_tube, tube_open_test);
      tcase_add_test (tc_tube, tube_ack_test);
      tcase_add_test (tc_tube, tube_data_test);
      tcase_add_test (tc_tube, tube_close_test);

      suite_add_tcase (s, tc_tube);
  }

  return s;
}
