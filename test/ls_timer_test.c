/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include "test_utils.h"
#include "ls_timer.h"

static void timer_cb(ls_timer *tim)
{
    UNUSED_PARAM(tim);
}

CTEST(ls_timer, basic)
{
    struct timeval small = {100, 100};
    struct timeval now;
    ls_timer *past;
    ls_timer *soon;
    ls_err err;
    ASSERT_TRUE(ls_timer_create(&small, timer_cb, NULL, &past, &err));
    ASSERT_FALSE(gettimeofday(&now, NULL));
    ASSERT_TRUE(ls_timer_create_ms(&now, 100, timer_cb, NULL, &soon, &err));

    ASSERT_TRUE(ls_timer_less(past, soon));
    ASSERT_FALSE(ls_timer_is_cancelled(past));
    ls_timer_cancel(past);
    ASSERT_TRUE(ls_timer_is_cancelled(past));
    ls_timer_destroy(past);
    ls_timer_destroy(soon);
}

CTEST(ls_pktinfo, alloc_fail)
{
    struct timeval small = {100, 100};
    ls_timer *past;
    ls_err err;

    OOM_RECORD_ALLOCS(ls_timer_create(&small, timer_cb, NULL, &past, &err));
    ls_timer_destroy(past);
    OOM_TEST_INIT()
    OOM_TEST_CONDITIONAL_CHECK(&err, ls_timer_create(&small, timer_cb, NULL, &past, &err), true);
}
