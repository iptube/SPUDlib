/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <check.h>

#include "ls_eventing.h"
#include "ls_mem.h"
#include "ls_htable.h"
#include "test_utils.h"

// uses "private" type defs from source. NOT for use outside unit tests
#include "../src/ls_pool_types.h"
#include "../src/ls_str.h"

Suite * ls_mem_suite (void);

// get access to the variables declared in mem.c
extern ls_data_malloc_func _malloc_func;
extern ls_data_realloc_func _realloc_func;
extern ls_data_free_func _free_func;

// cached external refs
static ls_data_malloc_func mallocFnOrig   = NULL;
static ls_data_realloc_func reallocFnOrig = NULL;
static ls_data_free_func freeFnOrig       = NULL;

static oom_test_data _oom_test = {
                        .ls_AllocCount = 0,
                        .ls_AllocLimit = -1,
                        .failureAttempts = 0,
                        .numMallocCalls = 0,
                        .numReallocCalls = 0,
                        .numFreeCalls = 0
                    };

static void *mallocFn (size_t size)
{
    void *ret = NULL;

    ++_oom_test.numMallocCalls;
    if (_oom_test.ls_AllocLimit < 0 ||
        _oom_test.ls_AllocCount < _oom_test.ls_AllocLimit)
    {
        ret = mallocFnOrig(size);
    }
    ++_oom_test.ls_AllocCount;
    return ret;
}

static void * reallocFn (void * ptr, size_t size)
{
    void *ret = NULL;

    if (NULL == ptr)
    {   //numReallocCalls counts the number of references that should be freed,
        //non NULL realloc pointers are already accounted for in their initial
        //allocation.
        ++_oom_test.numReallocCalls;
    }

    if (_oom_test.ls_AllocLimit < 0 ||
        _oom_test.ls_AllocCount < _oom_test.ls_AllocLimit)
    {
        ret = reallocFnOrig(ptr, size);
    }
    ++_oom_test.ls_AllocCount;
    return ret;
}

static void freeFn (void * ptr)
{
    ++_oom_test.numFreeCalls;
    freeFnOrig(ptr);
}

void oom_set_enabled(bool on)
{
    _oom_test.ls_AllocCount = _oom_test.numMallocCalls = 0;
    _oom_test.numReallocCalls = _oom_test.numFreeCalls = 0;
    _oom_test.ls_AllocLimit = -1;

    if (mallocFnOrig && !on)
    {
        ls_data_set_memory_funcs(mallocFnOrig, reallocFnOrig, freeFnOrig);
        mallocFnOrig  = NULL;
        reallocFnOrig = NULL;
        freeFnOrig    = NULL;
    }
    else if (!mallocFnOrig && on)
    {
        mallocFnOrig  = _malloc_func;
        reallocFnOrig = _realloc_func;
        freeFnOrig    = _free_func;
        ls_data_set_memory_funcs(mallocFn, reallocFn, freeFn);
    }
}

oom_test_data *oom_get_data()
{
    return &_oom_test;
}

int page_count(ls_pool *pool)
{
    int ret = 0;
    _pool_page page = pool->pages;

    while(page)
    {
        ret++;
        page = page->next;
    }
    return ret;
}

int cleaner_count(ls_pool *pool)
{
    int ret = 0;
    _pool_cleaner_ctx cleaner = pool->cleaners;

    while(cleaner)
    {
        ret++;
        cleaner = cleaner->next;
    }
    return ret;
}

_pool_page get_page(ls_pool *pool, int idx)
{
    _pool_page ret = pool->pages;
    int i;

    for (i = 0; (i < idx) && ret; ++i)
    {
        ret = ret->next;
    }
    return ret;
}

_pool_cleaner_ctx get_cleaner(ls_pool *pool, int idx)
{
    _pool_cleaner_ctx ret = pool->cleaners;
    int i;

    for (i = 0; (i < idx) && ret; ++i)
    {
        ret = ret->next;
    }
    return ret;
}

bool cleaner_hit = false;
void* expected = NULL;
void test_cleaner(void* ptr)
{
    cleaner_hit = (expected == ptr);
}

static void mock_evt1_callback1(ls_event_data evt, void *arg)
{
    UNUSED_PARAM(evt);
    UNUSED_PARAM(arg);

    return;
}

START_TEST (ls_data_calloc_test)
{
    void* ptr = ls_data_calloc(1, 4);
    ck_assert(ptr);
    ck_assert_int_eq(0, memcmp("\0\0\0\0", ptr, 4));
    ls_data_free(ptr);
}
END_TEST

START_TEST (ls_data_strdup_test)
{
    char*  dup;
    const char* src = "hi-de-ho";

    dup = ls_data_strdup(src);
    ck_assert(dup != NULL);
    ck_assert_str_eq(src, dup);
    ls_data_free(dup);

    dup = ls_data_strdup("");
    ck_assert(dup != NULL);
    ck_assert_str_eq("", dup);
    ls_data_free(dup);

    dup = ls_data_strdup(NULL);
    ck_assert(dup == NULL);

    OOM_RECORD_ALLOCS(dup = ls_data_strdup(src))
    ls_data_free(dup);
    OOM_TEST_INIT()
    //no ls_err checking
    OOM_TEST(NULL, ls_data_strdup(src))
}
END_TEST

START_TEST (ls_data_strndup_test)
{
    char*  dup;
    const char* src = "hi-de-ho";

    dup = ls_data_strndup(src, ls_strlen(src));
    ck_assert(dup != NULL);
    ck_assert_str_eq(src, dup);
    ls_data_free(dup);

    dup = ls_data_strndup(src, 100);
    ck_assert(dup != NULL);
    ck_assert_str_eq(src, dup);
    ls_data_free(dup);

    dup = ls_data_strndup(src, 3);
    ck_assert(dup != NULL);
    ck_assert_str_eq("hi-", dup);
    ls_data_free(dup);

    dup = ls_data_strndup("", 0);
    ck_assert(dup != NULL);
    ck_assert_str_eq("", dup);
    ls_data_free(dup);

    dup = ls_data_strndup(NULL, 0);
    ck_assert(dup == NULL);


    OOM_RECORD_ALLOCS(dup = ls_data_strndup(src, 5))
    ls_data_free(dup);
    OOM_TEST_INIT()
    //no ls_err checking
    OOM_TEST(NULL, ls_data_strndup(src, 5))
}
END_TEST

#ifndef DISABLE_POOL_PAGES
START_TEST (ls_pool_create_destroy_test)
{
    ls_err err;
    ls_pool *pool;
    _pool_page page;
    _pool_cleaner_ctx cleaner;
    /* create with pages */
    ck_assert(ls_pool_create(1024, &pool, &err));
    ck_assert(pool);
    ck_assert(pool->size == 1024);
    ck_assert(pool->page_size == 1024);

    /* should be 1 page */
    ck_assert_int_eq(1, page_count(pool));
    page = get_page(pool, 0);
    ck_assert(page->size == 1024);
    ck_assert(page->used == 0);

    /* should be one cleaner (for the page) */
    ck_assert_int_eq(1, cleaner_count(pool));
    /* cleaner should be for page */
    cleaner = get_cleaner(pool, 0);
    ck_assert(cleaner->arg == page);
    ls_pool_destroy(pool);

    /* without pages */
    ck_assert(ls_pool_create(0, &pool, &err));
    ck_assert(pool);
    ck_assert_int_eq(0, pool->size);
    ck_assert_int_eq(0, pool->page_size);
    /* should be 0 pages */
    ck_assert_int_eq(0, page_count(pool));
    /* should be 0 cleaners */
    ck_assert_int_eq(0, cleaner_count(pool));
    ls_pool_destroy(pool);

}
END_TEST

START_TEST (ls_pool_malloc_test)
{
    ls_pool *pool;
    ls_err err;
    void *ptr;
    size_t i;

    _pool_page page;
    _pool_cleaner_ctx cleaner;

    ck_assert(ls_pool_create(1024, &pool, &err));
    ck_assert(ls_pool_malloc(pool, 512, &ptr, &err));
    ck_assert(ptr != NULL);
    memset(ptr, 1, 512);
    ck_assert(ls_pool_malloc(pool, 615, &ptr, &err));
    ck_assert(ptr != NULL);

    ck_assert(pool->size == 2048);
    ck_assert(pool->page_size == 1024);

    ck_assert_int_eq(2, page_count(pool));

    page = get_page(pool, 0);
    ck_assert(page);
    ck_assert(page->size == 1024);
    ck_assert_int_eq(page->used, 615);

    page = get_page(pool, 1);
    ck_assert(page->size == 1024);
    ck_assert_int_eq(page->used, 512);

    ck_assert_int_eq(2, cleaner_count(pool));

    cleaner = get_cleaner(pool, 0);
    page = get_page(pool, 0);
    ck_assert(cleaner->arg == page);

    cleaner = get_cleaner(pool, 1);
    page = get_page(pool, 1);
    ck_assert(cleaner->arg == page);

    ls_pool_destroy(pool);

    ck_assert(ls_pool_create(1024, &pool, &err));
    /* test a number of small allocations, enough to fill a page, insure mallocs are word aligned*/
    ck_assert(ls_pool_malloc(pool, 1, &ptr, &err));
    for (i = 1; i < 115; ++i)
    {
        size_t  sz = (i % (sizeof(uintptr_t) * 2)) + 1;
        ck_assert(ls_pool_malloc(pool, sz, &ptr, &err));
    }
    ls_pool_destroy(pool);
}
END_TEST

START_TEST (ls_pool_malloc_overallocate_test)
{
    ls_pool *pool;
    ls_err err;
    void* ptr;
    _pool_page page1;
    _pool_cleaner_ctx cleaner;

    ck_assert(ls_pool_create(1024, &pool, &err));
    ck_assert(ls_pool_malloc(pool, 512, &ptr, &err));
    ck_assert(ptr != NULL);

    ck_assert(ls_pool_malloc(pool, 2048, &ptr, &err));
    ck_assert(ptr != NULL);

    ck_assert(pool->size == 3072);
    ck_assert(pool->page_size == 1024);

    ck_assert_int_eq(1, page_count(pool));

    page1 = get_page(pool, 0);
    ck_assert(page1);
    ck_assert(page1->size == 1024);
    ck_assert_int_eq(page1->used, 512);

    ck_assert_int_eq(2, cleaner_count(pool));

    cleaner = get_cleaner(pool, 0);
    page1 = get_page(pool, 0);
    ck_assert(cleaner->arg == ptr);

    cleaner = get_cleaner(pool, 1);
    ck_assert(cleaner->arg == page1);

    ls_pool_destroy(pool);

}
END_TEST

START_TEST (ls_pool_strdup_shorterpool_test)
{
    ls_pool *pool;
    ls_err err;
    char*  dup;
    _pool_page page;
    const char* src;

    src="\x2d\xa7\x5d\xa5\xc8\x54\x78\xdf\x42\xdf\x0f\x91\x77\x00\x24\x1e\xd2\x82\xf5\x99\x2d\xa7\x5d\xa5\xc8\x54\x78\xdf\x42\xdf\x0f\x91\x77\x00\x24\x1e\xd2\x82\xf5\x99";
    ck_assert(ls_pool_create(8, &pool, &err));
    ck_assert(ls_pool_strdup(pool, src, &dup, &err));
    ck_assert(dup != NULL);
    ck_assert_str_eq(src, dup);

    ck_assert_int_eq(pool->size, 22);
    ck_assert_int_eq(pool->page_size, 8);
    ck_assert_int_eq(1, page_count(pool));
    page = get_page(pool, 0);
    ck_assert(page);
    ck_assert_int_eq(page->used, 0);

    ls_pool_destroy(pool);
}
END_TEST

#endif
START_TEST (ls_pool_calloc_test)
{
    ls_pool *pool;
    ls_err err;
    void* ptr;

    ck_assert(ls_pool_create(1024, &pool, &err));
    ck_assert(ls_pool_calloc(pool, 4, 128, &ptr, &err));
    ck_assert(ptr != NULL);

    ls_pool_destroy(pool);
}
END_TEST

START_TEST (ls_pool_calloc_nobytes_test)
{
    ls_pool *pool;
    ls_err err;
    void* ptr;

    ck_assert(ls_pool_create(1024, &pool, &err));
    ck_assert(ls_pool_calloc(pool, 4, 0, &ptr, &err));
    ck_assert(ptr == NULL);

    ls_pool_destroy(pool);
}
END_TEST

START_TEST (ls_pool_strdup_test)
{
    ls_pool *pool;
    ls_err err;
    char*  dup;
    const char* src = "hi-de-ho";

    ck_assert(ls_pool_create(1024, &pool, &err));
    ck_assert(ls_pool_strdup(pool, src, &dup, &err));
    ck_assert(dup != NULL);
    ck_assert_str_eq(src, dup);

    ck_assert(ls_pool_strdup(pool, "", &dup, &err));
    ck_assert(dup != NULL);
    ck_assert_str_eq("", dup);

    ck_assert(ls_pool_strdup(pool, NULL, &dup, &err));
    ck_assert(dup == NULL);

    ls_pool_destroy(pool);
}
END_TEST

START_TEST (ls_pool_add_cleaner_test)
{
    ls_pool *pool;
    ls_err err;
    void* ptr;

    ck_assert(ls_pool_create(1024, &pool, &err));

    ck_assert(ls_pool_malloc(pool, 512, &ptr, &err));
    ck_assert(ls_pool_add_cleaner(pool, &test_cleaner, ptr, &err));
    expected = ptr;
    ls_pool_destroy(pool);
    ck_assert(cleaner_hit);
    cleaner_hit = false;
    expected = NULL;
}
END_TEST

START_TEST (ls_pool_add_cleaner_nonpool_test)
{
    ls_pool *pool;
    ls_err err;
    void* ptr;

    ck_assert(ls_pool_create(512, &pool, &err));

    ptr = (void *)malloc(512);
    ck_assert(ptr != NULL);

    ck_assert(ls_pool_add_cleaner(pool, &test_cleaner, ptr, &err));
    expected = ptr;
    ls_pool_destroy(pool);
    ck_assert(cleaner_hit);
    cleaner_hit = true;
    expected = NULL;
    free(ptr);
}
END_TEST

START_TEST (ls_data_memory_test)
{
    void *dummy;
    oom_test_data *tdata = oom_get_data();
    oom_set_enabled(true);

    dummy = ls_data_malloc(10);
    ck_assert_int_eq(tdata->numMallocCalls, 1);

    dummy = ls_data_realloc(dummy, 5);
    ck_assert_int_eq(tdata->numReallocCalls, 0);
    ls_data_free(dummy);
    ck_assert_int_eq(tdata->numFreeCalls, 1);

    dummy = ls_data_realloc(NULL, 5);
    ck_assert_int_eq(tdata->numReallocCalls, 1);
    ls_data_free(dummy);
    ck_assert_int_eq(tdata->numFreeCalls, 2);

    // NULL should be no-op
    ls_data_free(NULL);
    ck_assert_int_eq(tdata->numFreeCalls, 2);

    // Reset memory functions
    ls_data_set_memory_funcs(NULL, NULL, NULL);

    dummy = ls_data_malloc(10);
    ck_assert_int_eq(tdata->numMallocCalls, 1);

    dummy = ls_data_realloc(dummy, 5);
    ck_assert_int_eq(tdata->numReallocCalls, 1);

    ls_data_free(dummy);
    ck_assert_int_eq(tdata->numFreeCalls, 2);
    oom_set_enabled(false);
}
END_TEST

/* htable_tests */
START_TEST (ls_htable_no_mem_test)
{
    ls_htable   *table;
    OOM_SIMPLE_TEST(ls_htable_create(7,
                             ls_int_hashcode,
                             ls_int_compare,
                             &table,
                             &err));
    ls_htable_destroy(table);
}
END_TEST
START_TEST (ls_htable_put_no_mem_test)
{
    ls_htable   *table;
    ls_err      err;
    ck_assert(ls_htable_create(7,
                             ls_int_hashcode,
                             ls_int_compare,
                             &table,
                             NULL));
    OOM_RECORD_ALLOCS(ls_htable_put(table, "key1", "value one", NULL, &err))
    OOM_TEST_INIT()
        ls_htable_remove(table, "key1");
    OOM_TEST(&err, ls_htable_put(table, "key1", "value one", NULL, &err))
    ls_htable_destroy(table);
}
END_TEST

START_TEST (ls_event_dispatcher_create_no_mem_test)
{
    ls_event_dispatcher *dispatch;
    void                *source = "the source";
    OOM_SIMPLE_TEST(ls_event_dispatcher_create(
                            source, &dispatch, &err));
    ls_event_dispatcher_destroy(dispatch);
}
END_TEST

START_TEST (ls_event_create_no_mem_test)
{
    ls_event_dispatcher *dispatch;
    ls_err              err;
    void                *source = "the source";
    ls_event            *evt;
    OOM_POOL_TEST_BGN
    ck_assert(ls_event_dispatcher_create(source, &dispatch, &err));
    OOM_RECORD_ALLOCS(ls_event_dispatcher_create_event(dispatch,
                                         "eventOne",
                                         &evt,
                                         &err))
    OOM_TEST_INIT()
        ls_event_dispatcher_destroy(dispatch);
        ck_assert(ls_event_dispatcher_create(source, &dispatch, &err));
    OOM_TEST(&err,
             ls_event_dispatcher_create_event(dispatch,
                                         "eventOne",
                                         &evt,
                                         &err))
    ls_event_dispatcher_destroy(dispatch);
    OOM_POOL_TEST_END

}
END_TEST
START_TEST (ls_event_bind_no_mem_test)
{
    ls_event_dispatcher *dispatch;
    ls_err              err;
    void                *source = "the source";
    ls_event            *evt1;
    OOM_POOL_TEST_BGN

    ck_assert(ls_event_dispatcher_create(source, &dispatch, &err));
    ck_assert(ls_event_dispatcher_create_event(dispatch,
                                             "eventOne",
                                             &evt1,
                                             &err) == true);
    OOM_RECORD_ALLOCS(ls_event_bind(evt1, mock_evt1_callback1, NULL, &err))
    OOM_TEST_INIT()
        ls_event_unbind(evt1, mock_evt1_callback1);
    OOM_TEST(&err, ls_event_bind(evt1, mock_evt1_callback1, NULL, &err))
    ls_event_unbind(evt1, mock_evt1_callback1);
    ls_event_dispatcher_destroy(dispatch);
    OOM_POOL_TEST_END

}
END_TEST
START_TEST (ls_event_trigger_no_mem_test)
{
    ls_event_dispatcher *dispatch;
    ls_err              err;
    void                *source = "the source";
    ls_event            *evt1;
    OOM_POOL_TEST_BGN
    ck_assert(ls_event_dispatcher_create(source, &dispatch, &err));
    ck_assert(ls_event_dispatcher_create_event(dispatch,
                                             "eventOne",
                                             &evt1,
                                             &err) == true);
    OOM_SIMPLE_TEST(ls_event_trigger(evt1, NULL, NULL, NULL, &err))
    ls_event_unbind(evt1, mock_evt1_callback1);
    ls_event_dispatcher_destroy(dispatch);
    OOM_POOL_TEST_END
}
END_TEST

Suite * ls_mem_suite (void)
{
  Suite *s = suite_create ("ls_mem");
  {/* Error test case */
      TCase *tc_ls_mem = tcase_create ("mem");
      //tcase_add_test (tc_ls_mem, ls_mem_test);

      suite_add_tcase (s, tc_ls_mem);
  }

  return s;
}
