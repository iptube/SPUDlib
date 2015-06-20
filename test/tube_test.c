#include <pthread.h>
#include <time.h>
#include <sys/errno.h>
#include <stdio.h>
#include <string.h>

#include "test_utils.h"
#include "tube_manager.h"
#include "ls_sockaddr.h"

// Number of tubes to create for the foreach test.
static const int TMGR_FOREACH_NUMTUBES = 3;

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
    for (i=0; i<(int)hdr->msg_iovlen; i++) {
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

CTEST_DATA(tube)
{
    tube_manager *mgr;
    bool listen_return;
    ls_err err;
};

CTEST_SETUP(tube)
{
    tube_manager_set_socket_functions(_mock_sendmsg, _mock_recvmsg);
    ASSERT_TRUE( tube_manager_create(0, &data->mgr, &data->err));
    ASSERT_TRUE( tube_manager_running(data->mgr));
    ASSERT_TRUE( tube_manager_socket(data->mgr, 0, &data->err));
}

CTEST_TEARDOWN(tube)
{
    tube_manager_destroy(data->mgr);
    tube_manager_set_socket_functions(NULL, NULL);
}

CTEST2(tube, create)
{
    tube *t;
    ASSERT_TRUE( tube_create(&t, &data->err) );
    tube_destroy(t);
}

static void test_cb(ls_event_data evt, void *arg){
    UNUSED_PARAM(evt);
    UNUSED_PARAM(arg);
}

CTEST2(tube, manager_bind_event)
{
    ASSERT_TRUE( tube_manager_bind_event(data->mgr, EV_LOOPSTART_NAME, test_cb, &data->err));
    ASSERT_TRUE( tube_manager_bind_event(data->mgr, EV_RUNNING_NAME, test_cb, &data->err));
    ASSERT_TRUE( tube_manager_bind_event(data->mgr, EV_DATA_NAME, test_cb, &data->err));
    ASSERT_TRUE( tube_manager_bind_event(data->mgr, EV_CLOSE_NAME, test_cb, &data->err));
    ASSERT_TRUE( tube_manager_bind_event(data->mgr, EV_ADD_NAME, test_cb, &data->err));
    ASSERT_TRUE( tube_manager_bind_event(data->mgr, EV_REMOVE_NAME, test_cb, &data->err));
}

CTEST2(tube, utilities)
{
    tube *t;
    spud_tube_id *id;
    int local_data = 1337;
    int *out;
    char buf[24];
    char buf2[24];

    ASSERT_TRUE(tube_manager_running(data->mgr));
    ASSERT_EQUAL(tube_manager_size(data->mgr), 0);

    ASSERT_TRUE( tube_create(&t, &data->err));
    tube_set_data(t, &local_data);
    out = tube_get_data(t);
    ASSERT_EQUAL(*out, 1337);
    ASSERT_EQUAL(tube_get_state(t), TS_UNKNOWN);

    tube_get_id(t, &id);
    spud_id_to_string(buf, sizeof(buf), id);

    ASSERT_TRUE( tube_id_to_string(t, buf2, sizeof(buf2)) == buf2);
    ASSERT_STR(buf, buf2);

    ASSERT_TRUE(tube_manager_stop(data->mgr, &data->err));
    ASSERT_FALSE(tube_manager_running(data->mgr));
}

CTEST2(tube, print)
{
    tube *t;

    ASSERT_TRUE( tube_create(&t, &data->err));
    ASSERT_TRUE( tube_print(t, &data->err));
}

CTEST2(tube, open)
{
    tube *t;
    struct sockaddr_in6 remoteAddr;
    ASSERT_TRUE( ls_sockaddr_get_remote_ip_addr("::1",
                                                "1402",
                                                (struct sockaddr*)&remoteAddr,
                                                sizeof(remoteAddr),
                                                &data->err));

    ASSERT_TRUE( tube_manager_open_tube(data->mgr, (const struct sockaddr*)&remoteAddr, &t, &data->err));
    ASSERT_EQUAL(tube_manager_size(data->mgr), 1);
}

CTEST2(tube, tube_data)
{
    tube *t;
    uint8_t udata[] = "SPUD_makeUBES_FUN";
    struct sockaddr_storage remoteAddr;
    ASSERT_TRUE( ls_sockaddr_get_remote_ip_addr("127.0.0.1",
                                                "1402",
                                                (struct sockaddr*)&remoteAddr,
                                                sizeof(remoteAddr),
                                                &data->err));

    ASSERT_TRUE( tube_manager_open_tube(data->mgr, (const struct sockaddr*)&remoteAddr, &t, &data->err));

    ASSERT_TRUE( tube_data(t,
                           udata,
                           17,
                           &data->err));

    ASSERT_TRUE( tube_data(t,
                           NULL,
                           0,
                           &data->err));
    tube_manager_remove(data->mgr, t);
    ASSERT_EQUAL(tube_manager_size(data->mgr), 0);
}

CTEST2(tube, close)
{
    tube *t;
    struct sockaddr_in6 remoteAddr;

    ASSERT_TRUE( ls_sockaddr_get_remote_ip_addr("127.0.0.1",
                                                "1402",
                                                (struct sockaddr *)&remoteAddr,
                                                sizeof(remoteAddr),
                                                &data->err));

    ASSERT_TRUE( tube_manager_open_tube(data->mgr, (const struct sockaddr*)&remoteAddr, &t, &data->err));
    ASSERT_TRUE( tube_close(t, &data->err));
}

void *listen_run(void *p)
{
    CTEST_DATA(tube) *data = p;
    data->listen_return = tube_manager_loop(data->mgr,& data->err);
    if (!data->listen_return) {
        LS_LOG_ERR(data->err, "tube_manager_loop");
    }
    return data;
}

CTEST2(tube, manager_loop)
{
    void *ret;
    int r;
    struct timespec timer = {0, 5000000}; // 5ms

    pthread_t listen_thread;
    ASSERT_EQUAL(pthread_create(&listen_thread, NULL, listen_run, data), 0);

    nanosleep(&timer, NULL);
    ASSERT_TRUE(tube_manager_stop(data->mgr, &data->err));
    r = pthread_join(listen_thread, &ret);
    if (r != 0) {
        printf("pthread_join (%d): '%s'\n", errno, strerror(errno));
        ASSERT_EQUAL(r, 0);
    }
    ASSERT_TRUE(ret == data);
    ASSERT_EQUAL((int)true, data->listen_return);
}

CTEST2(tube, manager_policy)
{
    ASSERT_FALSE(tube_manager_is_responder(data->mgr));
    tube_manager_set_policy_responder(data->mgr, true);
    ASSERT_TRUE(tube_manager_is_responder(data->mgr));
}

CTEST2(tube, send_pdec)
{
    tube *t;
    uint8_t ip[]   = {192, 168, 0, 0};
    uint8_t token[] = {42, 42, 42, 42, 42};
    char url[]= "http://example.com";
    struct sockaddr_in6 remoteAddr;
    cn_cbor_context ctx;
    cn_cbor *map;

    ASSERT_TRUE(ls_sockaddr_get_remote_ip_addr("127.0.0.1",
                                               "1402",
                                               (struct sockaddr*)&remoteAddr,
                                               sizeof(remoteAddr),
                                               &data->err));

    ASSERT_TRUE( tube_manager_open_tube(data->mgr, (const struct sockaddr*)&remoteAddr, &t, &data->err));

    ASSERT_TRUE(path_mandatory_keys_create(ip, sizeof(ip), token, sizeof(token), url, &ctx, &map, &data->err));

    ASSERT_TRUE( tube_send_pdec(t,map,true, &data->err) );

    ls_pool_destroy((ls_pool*)ctx.context);

    tube_manager_remove(data->mgr, t);
    ASSERT_EQUAL(tube_manager_size(data->mgr), 0);
}

CTEST2(tube, print_tubes)
{
    tube *t;
    spud_tube_id id;
    ASSERT_TRUE( tube_create(&t, &data->err) );
    tube_set_info(t, -1, NULL, NULL);
    ASSERT_TRUE(spud_create_id(&id, &data->err));
    tube_set_info(t, -1, NULL, &id);
    ASSERT_TRUE(tube_manager_add(data->mgr, t, &data->err));
    tube_manager_print_tubes(data->mgr);
}

static int _mock_tube_walker(void *data,
                             const spud_tube_id *tube_id,
                             tube *t)
{
    UNUSED_PARAM(tube_id);
    UNUSED_PARAM(t);
    int *num_tubes = (int *) data;
    (*num_tubes)++;
    return 1;
}

CTEST2(tube, manager_foreach)
{
    tube * tubes[TMGR_FOREACH_NUMTUBES];
    spud_tube_id ids[TMGR_FOREACH_NUMTUBES];

    for (int i = 0; i < TMGR_FOREACH_NUMTUBES; ++i) {
        ASSERT_TRUE(tube_create(&tubes[i], &data->err));
        ASSERT_TRUE(spud_create_id(&ids[i], &data->err));
        tube_set_info(tubes[i], -1, NULL, &ids[i]);
        ASSERT_TRUE(tube_manager_add(data->mgr, tubes[i], &data->err));
    }

    int num_tubes = 0;
    tube_manager_foreach(data->mgr, _mock_tube_walker, (void *) &num_tubes);
    ASSERT_TRUE(num_tubes == TMGR_FOREACH_NUMTUBES);
}
