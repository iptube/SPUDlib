/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <string.h>
#include <stdlib.h>

#include "../src/ls_queue.h"
#include "test_utils.h"

CTEST(ls_queue, create)
{
  ls_queue* q;
  ls_err    err;
  ASSERT_TRUE( ls_queue_create(NULL, &q, &err) );
  ls_queue_destroy(q);
}

CTEST(ls_queue, enq)
{
  ls_queue* q;
  ls_err    err;
  char*     val = "VALUE";

  ASSERT_TRUE( ls_queue_create(NULL, &q, &err) );
  ASSERT_TRUE( ls_queue_enq(q, val, &err) );
  ASSERT_STR(ls_queue_deq(q), "VALUE");
  ls_queue_destroy(q);
}

static void
cleaner(ls_queue* q,
        void*     data)
{
  UNUSED_PARAM(q);
  ls_data_free(data);
}

CTEST(ls_queue, cleanup)
{
  ls_queue* q;
  ls_err    err;
  int       i;

  ASSERT_TRUE( ls_queue_create(cleaner, &q, &err) );
  for (i = 0; i < 10; i++)
  {
    ASSERT_TRUE( ls_queue_enq(q, ls_data_malloc(1), &err) );
  }
  ls_queue_destroy(q);
}

static bool
_oom_test(ls_err* err)
{
  ls_queue* q;
  char*     data = "data";
  bool      ret  = true;

  if ( !ls_queue_create(NULL, &q, err) )
  {
    return false;
  }
  if ( !ls_queue_enq(q, data, err) )
  {
    ret = false;
  }
  ls_queue_destroy(q);
  return ret;
}

CTEST(ls_queue, oom)
{
  OOM_SIMPLE_TEST( _oom_test(&err) );
  OOM_TEST_INIT();
  OOM_TEST( NULL, _oom_test(NULL) );
}
