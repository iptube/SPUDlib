/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include "test_utils.h"
#include "ls_timer.h"

static void
timer_cb(ls_timer* tim)
{
  UNUSED_PARAM(tim);
}

CTEST(ls_timer, basic)
{
  struct timeval  small = {100, 100};
  struct timeval  now;
  struct timeval* then;
  struct timeval  diff;

  ls_timer* past;
  ls_timer* soon;
  ls_err    err;
  ASSERT_TRUE( ls_timer_create(&small, timer_cb, NULL, &past, &err) );
  ASSERT_FALSE( gettimeofday(&now, NULL) );
  ASSERT_TRUE( ls_timer_create_ms(&now, 100, timer_cb, &err, &soon, &err) );
  then = ls_timer_get_time(soon);
  timersub(then,&now,&diff);
  ASSERT_EQUAL(100000, diff.tv_usec);
  ASSERT_EQUAL(0,      diff.tv_sec);
  ASSERT_TRUE(ls_timer_get_context(soon) == &err);
  ls_timer_exec(soon);

  ASSERT_TRUE( ls_timer_less(past, soon) );
  ASSERT_FALSE( ls_timer_greater(past, soon) );
  ASSERT_TRUE( ls_timer_greater_tv(soon, &now) );
  ASSERT_FALSE( ls_timer_is_cancelled(past) );
  ls_timer_cancel(past);
  ASSERT_TRUE( ls_timer_is_cancelled(past) );
  ASSERT_NULL( ls_timer_get_time(past) );
  ls_timer_destroy(past);
  ls_timer_destroy(soon);
}

CTEST(ls_timer, alloc_fail)
{
  struct timeval small = {100, 100};
  ls_timer*      past;
  ls_err         err;

  OOM_RECORD_ALLOCS( ls_timer_create(&small, timer_cb, NULL, &past, &err) );
  ls_timer_destroy(past);
  OOM_TEST_INIT()
  OOM_TEST_CONDITIONAL_CHECK(&err,
                             ls_timer_create(&small,
                                             timer_cb,
                                             NULL,
                                             &past,
                                             &err), true);
}
