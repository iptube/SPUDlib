#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/errno.h>
#include <stdio.h>

#include <check.h>
#include "tube.h"
#include "ls_sockaddr.h"

Suite * tube_suite (void);
static tube_manager *_mgr;

uint8_t spud[] = { 0xd8, 0x00, 0x00, 0xd8,
                   0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                   0x00,
                   0xa1, 0x00,
                   0x41, 0x61 };
bool first = true;

static ssize_t _mock_sendmsg(int socket,
                             const struct msghdr *hdr,
                             int flags)
{
    int i;
    ssize_t count = 0;
    UNUSED_PARAM(socket);
    UNUSED_PARAM(flags);
    for (i=0; i<hdr->msg_iovlen; i++) {
        count += hdr->msg_iov[i].iov_len;
    }
    // I totally sent it.  Really.
    return count;
}

static ssize_t _mock_recvmsg(int socket,
                             struct msghdr *hdr,
                             int flags)
{
    struct timespec timer = {0, 1000000}; // 1ms
    UNUSED_PARAM(socket);
    UNUSED_PARAM(flags);

    if (nanosleep(&timer, NULL) == -1) {
        errno = EINTR;
        return -1;
    }

    // TODO: Do something cool.  Maybe a queue or something.
    if ((hdr->msg_iovlen < 1) || hdr->msg_iov[0].iov_len < sizeof(spud)) {
        errno = EMSGSIZE;
        return -1;
    }
    if (first) {
        memcpy(hdr->msg_iov[0].iov_base, spud, 1);
        first = false;
    } else {
        memcpy(hdr->msg_iov[0].iov_base, spud, sizeof(spud));
    }

    return sizeof(spud);
}

static void _setup(void)
{
    ls_err err;
    tube_set_socket_functions(_mock_sendmsg, _mock_recvmsg);
    fail_unless( tube_manager_create(0, &_mgr, &err),
                 ls_err_message( err.code ));
    fail_if(tube_manager_running(_mgr));
    fail_unless( tube_manager_socket(_mgr, 0, &err),
                 ls_err_message( err.code ));
}

static void _teardown(void)
{
    tube_manager_destroy(_mgr);
    tube_set_socket_functions(NULL, NULL);
};

START_TEST (tube_create_test)
{
    tube *t;
    ls_err err;
    fail_unless( tube_create(_mgr, &t, &err) );
    tube_destroy(t);
}
END_TEST

static void test_cb(ls_event_data evt, void *arg){
    UNUSED_PARAM(evt);
    UNUSED_PARAM(arg);
}

START_TEST (tube_manager_bind_event_test)
{
    ls_err err;

    fail_unless( tube_manager_bind_event(_mgr, EV_RUNNING_NAME, test_cb, &err),
                 ls_err_message( err.code ));
    fail_unless( tube_manager_bind_event(_mgr, EV_DATA_NAME, test_cb, &err),
                 ls_err_message( err.code ));
    fail_unless( tube_manager_bind_event(_mgr, EV_CLOSE_NAME, test_cb, &err),
                 ls_err_message( err.code ));
    fail_unless( tube_manager_bind_event(_mgr, EV_ADD_NAME, test_cb, &err),
                 ls_err_message( err.code ));
    fail_unless( tube_manager_bind_event(_mgr, EV_REMOVE_NAME, test_cb, &err),
                 ls_err_message( err.code ));
}
END_TEST

START_TEST (tube_utilities_test)
{
    tube *t;
    spud_tube_id id;
    ls_err err;
    int local_data = 1337;
    int *out;
    char buf[24];
    char buf2[24];

    fail_unless(tube_manager_running(_mgr));
    ck_assert_int_eq(tube_manager_size(_mgr), 0);

    fail_unless( tube_create(_mgr, &t, &err),
                 ls_err_message( err.code ) );
    tube_set_data(t, &local_data);
    out = tube_get_data(t);
    ck_assert_int_eq(*out, 1337);
    ck_assert_int_eq(tube_get_state(t), TS_UNKNOWN);

    tube_get_id(t, &id);
    spud_id_to_string(buf, sizeof(buf), &id);

    fail_unless( tube_id_to_string(t, buf2, sizeof(buf2)) == buf2);
    ck_assert_str_eq(buf, buf2);

    tube_manager_stop(_mgr);
    fail_if(tube_manager_running(_mgr));
}
END_TEST

START_TEST (tube_print_test)
{
    tube *t;
    ls_err err;

    fail_unless( tube_create(_mgr, &t, &err),
                 ls_err_message( err.code ) );
    fail_unless( tube_print(t, &err),
                 ls_err_message( err.code ) );
}
END_TEST

START_TEST (tube_open_test)
{
    tube *t;
    ls_err err;
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "::1",
                                                "1402",
                                                &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_create(_mgr, &t, &err) );
    fail_unless( tube_open(t, (const struct sockaddr*)&remoteAddr, &err),
                 ls_err_message( err.code ) );

    ck_assert_int_eq(tube_manager_size(_mgr), 1);
}
END_TEST

START_TEST (tube_ack_test)
{
    tube *t;
    ls_err err;
    struct sockaddr_in6 remoteAddr;
    spud_tube_id id;

    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "127.0.0.1",
                                                "1402",
                                                &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_create(_mgr, &t, &err) );

    fail_unless( spud_create_id(&id, &err), ls_err_message( err.code ) );
    fail_unless( tube_ack(t, &id, (const struct sockaddr*)&remoteAddr, &err),
                 ls_err_message( err.code ) );
}
END_TEST

START_TEST (tube_data_test)
{
    tube *t;
    ls_err err;
    uint8_t data[] = "SPUD_makeUBES_FUN";
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "127.0.0.1",
                                                "1402",
                                                &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_create(_mgr, &t, &err) );

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
    tube_manager_remove(_mgr, t);
    ck_assert_int_eq(tube_manager_size(_mgr), 0);
}
END_TEST

START_TEST (tube_close_test)
{
    tube *t;
    ls_err err;
    struct sockaddr_in6 remoteAddr;

    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "127.0.0.1",
                                                "1402",
                                                &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_create(_mgr, &t, &err) );
    fail_unless( tube_open(t, (const struct sockaddr*)&remoteAddr, &err),
                 ls_err_message( err.code ) );
    fail_unless( tube_close(t, &err),
                 ls_err_message( err.code ) );
}
END_TEST

bool listen_return;

void *listen_run(void *p)
{
    ls_err *err = p;
    listen_return = tube_manager_loop(_mgr, err);
    return &listen_return;
}

START_TEST (tube_manager_loop_test)
{
    ls_err listen_err;
    void *ret;
    int r;
    struct timespec timer = {0, 5000000}; // 5ms

    pthread_t listen_thread;
    ck_assert_int_eq(pthread_create(&listen_thread, NULL, listen_run, &listen_err), 0);

    nanosleep(&timer, NULL);
    tube_manager_stop(_mgr);
    r = pthread_join(listen_thread, &ret);
    if (r != 0) {
        printf("pthread_join (%d): '%s'\n", errno, sys_errlist[errno]);
        ck_assert_int_eq(r, 0);
    }
    ck_assert_int_eq(*((int*)ret), (int)true);
}
END_TEST

Suite * tube_suite (void)
{
  Suite *s = suite_create ("tube");
  {/* tube *test case */
      TCase *tc_tube = tcase_create ("TUBE");
      tcase_add_checked_fixture(tc_tube, _setup, _teardown);
      tcase_add_test (tc_tube, tube_create_test);
      tcase_add_test (tc_tube, tube_manager_bind_event_test);
      tcase_add_test (tc_tube, tube_utilities_test);
      tcase_add_test (tc_tube, tube_print_test);
      tcase_add_test (tc_tube, tube_open_test);
      tcase_add_test (tc_tube, tube_ack_test);
      tcase_add_test (tc_tube, tube_data_test);
      tcase_add_test (tc_tube, tube_close_test);
      tcase_add_test (tc_tube, tube_manager_loop_test);

      suite_add_tcase (s, tc_tube);
  }

  return s;
}
