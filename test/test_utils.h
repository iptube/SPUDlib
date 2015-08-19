/**
 * \file
 * test_utils.h
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */


#pragma once

#include "ls_log.h"
#define CTEST_SEGFAULT
#include "ctest.h"

/**
 * Out of memory test data
 */
typedef struct _oom_test_data_int
{
  int ls_AllocCount;     /* # of non-3rdparty allocation calls */
  /* The most ls_ allocs allowed during a failure test, -1 disables limiting */
  int ls_AllocLimit;
  /* the number of fail tests to be attempted. see OOM_RECORD_ALLOCS */
  int failureAttempts;
  int numMallocCalls, numReallocCalls, numFreeCalls;
} oom_test_data;

/**
 * Enable or disable oom testing. OOM testing swaps ls_data*
 * allocation functions with ones that count and limit calls.
 *
 * \param enabled True to swap in oom testing allocation functions,
 *        false for default
 */
void
oom_set_enabled(bool enabled);

/**
 * Get the oom_test_data associated with the current tests. User does not
 * own the structure reference returned and should not free it.
 *
 * \retval oom_test_data the structure containing current oom testing data.
 *         will never return NULL.
 */
oom_test_data*
oom_get_data();

/**
 * Several macros are defined to simplify out-of-memory testing.
 *
 * OOM_RECORD_ALLOCS takes an expression to test. Custom allocation functions
 * are swapped in and then the expression is evaluated (and the result tested
 * to ensure the expression returned "true"). The number of allocations required
 * for a good evaluation is then stored in the failureAttempts variable and
 * used in later macros.
 *
 * Tests expr result using ck_assert
 */
#define OOM_RECORD_ALLOCS(expr) \
  { \
    bool _oom_result; \
    oom_set_enabled(true); \
    _oom_result                     = (expr); \
    oom_get_data()->failureAttempts = oom_get_data()->ls_AllocCount; \
    oom_set_enabled(false); \
    ASSERT_TRUE(_oom_result); \
    ls_log(LS_LOG_INFO, "testing %u alloc failures for expression: '%s'", \
           oom_get_data()->failureAttempts, #expr); \
  }

/**
 * Two macros work together to iterate over the failureAttempts.
 * OOM_TEST_INIT should be followed by cleanup and initialization logic invoked
 * with every fail test attempt. These functions are typically paired with
 * OOM_RECORD_ALLOCS so the cleanup/init order works correctly.
 *
 * A typical OOM test using OOM_TEST_INIT, OOM_TEST might look like:
 *
 *       ls_jid_ctx ctx;
 *       ls_jid jid;
 *       ls_err err;
 *
 *       ck_assert(ls_jid_context_create(0, &ctx, NULL));
 *       OOM_RECORD_ALLOCS(ls_jid_create(ctx, "foo@bar/baz", &jid, &err))
 *       oom_get_data()->failureAttempts = 1;
 *       OOM_TEST_INIT()
 *           ls_jid_context_destroy(ctx);
 *           ck_assert(ls_jid_context_create(0, &ctx, NULL));
 *       OOM_TEST(&err, ls_jid_create(ctx, "foo@bar/baz", &jid, &err))
 *      ls_jid_context_destroy(ctx);
 */
#define OOM_TEST_INIT() \
  { \
    oom_test_data* oom_data = oom_get_data(); \
    for (int _oom_idx = 0; _oom_idx < oom_data->failureAttempts; ++_oom_idx) \
    { \
      ls_log(LS_LOG_INFO, "starting OOM iteration %d of %d", \
             _oom_idx + 1, oom_data->failureAttempts); \
      oom_set_enabled(false); \

#define OOM_TEST_CONDITIONAL_CHECK(err, expr, check_err) \
  bool _oom_result; \
  ls_err* _oom_err = err; \
  if (_oom_err) { _oom_err->code = LS_ERR_NONE; } \
  oom_set_enabled(true); \
  oom_data->ls_AllocLimit = _oom_idx; \
  _oom_result             = (expr); \
  ASSERT_TRUE(!check_err || !_oom_result); \
  if (check_err && _oom_err) { \
    if (_oom_err->code != LS_ERR_NO_MEMORY) { \
      ls_log(LS_LOG_ERROR, \
             "unexpected error value (%d) on iteration %d", \
             _oom_err->code, _oom_idx + 1); \
    } \
    ASSERT_EQUAL(_oom_err->code, LS_ERR_NO_MEMORY); \
  } \
  } \
  oom_set_enabled(false); \
  }

#define OOM_TEST(err, expr) OOM_TEST_CONDITIONAL_CHECK(err, (expr), true)
#define OOM_TEST_NO_CHECK(err, expr) \
  OOM_TEST_CONDITIONAL_CHECK(err, (expr), false)

/**
 * OOM_SIMPLE_TEST is given the expression to test and assumes no intermediate
 * cleanup is required. The macro defines the ls_err "err" that the test
 * function *must* use (err->code is checked to make sure it was set correctly).
 * The given expression is executed successfully exactly one time.
 *
 * A simple test to check DOM context construction might look like:
 *
 *  ls_dom_ctx      ctx;
 *
 *  OOM_SIMPLE_TEST(ls_dom_context_create(&ctx, &err));
 *  ls_dom_context_destroy(ctx);
 */
#define OOM_SIMPLE_TEST_CONDITIONAL_CHECK(expr, check_err) \
  { \
    ls_err err; \
    OOM_RECORD_ALLOCS( (expr) ) \
    OOM_TEST_INIT() \
    OOM_TEST_CONDITIONAL_CHECK(&err, (expr), check_err) \
  }

#define OOM_SIMPLE_TEST(expr) OOM_SIMPLE_TEST_CONDITIONAL_CHECK( (expr), true )
#define OOM_SIMPLE_TEST_NO_CHECK(expr) \
  OOM_SIMPLE_TEST_CONDITIONAL_CHECK( (expr), false )


/**
 * If a function uses a ls_pool its OOM tests should be wrapped in this macro.
 * NOTE: Any initialization and destruction logic should be included between
 * OOM_POOL_TEST_BGN && OOM_POOL_TEST_END. This will ensure pre-allocation
 * logic gets fully excercised.
 */
#define OOM_POOL_TEST_BGN \
  for (size_t itrs = 0; itrs < 2; ++itrs) \
  { \
    ls_pool_enable_paging(itrs == 0); \

/**
 * The end pair of OOM_POOL_TEST_BGN
 */
#define OOM_POOL_TEST_END \
  } \
  ls_pool_enable_paging(true);

/* generic memory allocation/free counting functions */
void
_test_init_counting_memory_funcs();
uint32_t
_test_get_malloc_count();
uint32_t
_test_get_free_count();
void
_test_uninit_counting_memory_funcs();
