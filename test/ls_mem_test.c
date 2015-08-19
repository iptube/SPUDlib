/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */
#include <string.h>

#include "../src/ls_eventing.h"
#include "ls_mem.h"
#include "ls_htable.h"
#include "test_utils.h"

/* uses "private" type defs from source. NOT for use outside unit tests */
#include "../src/ls_pool_types.h"
#include "../src/ls_str.h"

int
page_count(ls_pool* pool)
{
  int         ret  = 0;
  _pool_page* page = pool->pages;

  while (page)
  {
    ret++;
    page = page->next;
  }
  return ret;
}

int
cleaner_count(ls_pool* pool)
{
  int                ret     = 0;
  _pool_cleaner_ctx* cleaner = pool->cleaners;

  while (cleaner)
  {
    ret++;
    cleaner = cleaner->next;
  }
  return ret;
}

_pool_page*
get_page(ls_pool* pool,
         int      idx)
{
  _pool_page* ret = pool->pages;
  int         i;

  for (i = 0; (i < idx) && ret; ++i)
  {
    ret = ret->next;
  }
  return ret;
}

_pool_cleaner_ctx*
get_cleaner(ls_pool* pool,
            int      idx)
{
  _pool_cleaner_ctx* ret = pool->cleaners;
  int                i;

  for (i = 0; (i < idx) && ret; ++i)
  {
    ret = ret->next;
  }
  return ret;
}

bool  cleaner_hit = false;
void* expected    = NULL;
void
test_cleaner(void* ptr)
{
  cleaner_hit = (expected == ptr);
}

static void
mock_evt1_callback1(ls_event_data* evt,
                    void*          arg)
{
  UNUSED_PARAM(evt);
  UNUSED_PARAM(arg);

}

CTEST(ls_data, calloc)
{
  void* ptr = ls_data_calloc(1, 4);
  ASSERT_NOT_NULL(ptr);
  ASSERT_TRUE(memcmp("\0\0\0\0", ptr, 4) == 0);
  ls_data_free(ptr);
}

CTEST(ls_data, strdup)
{
  char*       dup;
  const char* src = "hi-de-ho";

  dup = ls_data_strdup(src);
  ASSERT_NOT_NULL(dup);
  ASSERT_STR(src, dup);
  ls_data_free(dup);

  dup = ls_data_strdup("");
  ASSERT_NOT_NULL(dup);
  ASSERT_STR("", dup);
  ls_data_free(dup);

  dup = ls_data_strdup(NULL);
  ASSERT_NULL(dup);

  OOM_RECORD_ALLOCS( dup = ls_data_strdup(src) )
  ls_data_free(dup);
  OOM_TEST_INIT()
  /* no ls_err checking */
  OOM_TEST( NULL, ls_data_strdup(src) )
}

CTEST(ls_data, strndup)
{
  char*       dup;
  const char* src = "hi-de-ho";

  dup = ls_data_strndup( src, ls_strlen(src) );
  ASSERT_NOT_NULL(dup);
  ASSERT_STR(src, dup);
  ls_data_free(dup);

  dup = ls_data_strndup(src, 100);
  ASSERT_NOT_NULL(dup);
  ASSERT_STR(src, dup);
  ls_data_free(dup);

  dup = ls_data_strndup(src, 3);
  ASSERT_NOT_NULL(dup);
  ASSERT_STR("hi-", dup);
  ls_data_free(dup);

  dup = ls_data_strndup("", 0);
  ASSERT_NOT_NULL(dup);
  ASSERT_STR("", dup);
  ls_data_free(dup);

  dup = ls_data_strndup(NULL, 0);
  ASSERT_NULL(dup);


  OOM_RECORD_ALLOCS( dup = ls_data_strndup(src, 5) )
  ls_data_free(dup);
  OOM_TEST_INIT()
  /* no ls_err checking */
  OOM_TEST( NULL, ls_data_strndup(src, 5) )
}

#ifndef DISABLE_POOL_PAGES
CTEST(ls_pool, create_destroy)
{
  ls_err             err;
  ls_pool*           pool;
  _pool_page*        page;
  _pool_cleaner_ctx* cleaner;

  /* create with pages */
  ASSERT_TRUE( ls_pool_create(1024, &pool, &err) );
  ASSERT_NOT_NULL(pool);
  ASSERT_EQUAL( pool->size,      1024);
  ASSERT_EQUAL( pool->page_size, 1024);

  /* should be 1 page */
  ASSERT_EQUAL( 1,               page_count(pool) );
  page = get_page(pool, 0);
  ASSERT_EQUAL( page->size,      1024);
  ASSERT_EQUAL( page->used,      0);

  /* should be one cleaner (for the page) */
  ASSERT_EQUAL( 1,               cleaner_count(pool) );
  /* cleaner should be for page */
  cleaner = get_cleaner(pool, 0);
  ASSERT_TRUE(cleaner->arg == page);
  ls_pool_destroy(pool);

  /* without pages */
  ASSERT_TRUE( ls_pool_create(0, &pool, &err) );
  ASSERT_NOT_NULL(pool);
  ASSERT_EQUAL( 0, pool->size);
  ASSERT_EQUAL( 0, pool->page_size);
  /* should be 0 pages */
  ASSERT_EQUAL( 0, page_count(pool) );
  /* should be 0 cleaners */
  ASSERT_EQUAL( 0, cleaner_count(pool) );
  ls_pool_destroy(pool);

}

CTEST(ls_pool, malloc)
{
  ls_pool* pool;
  ls_err   err;
  void*    ptr;
  size_t   i;

  _pool_page*        page;
  _pool_cleaner_ctx* cleaner;

  ASSERT_TRUE( ls_pool_create(1024, &pool, &err) );
  ASSERT_TRUE( ls_pool_malloc(pool, 512, &ptr, &err) );
  ASSERT_NOT_NULL(ptr);
  memset(ptr, 1, 512);
  ASSERT_TRUE( ls_pool_malloc(pool, 615, &ptr, &err) );
  ASSERT_NOT_NULL(ptr);

  ASSERT_EQUAL( pool->size,      2048);
  ASSERT_EQUAL( pool->page_size, 1024);

  ASSERT_EQUAL( 2,               page_count(pool) );

  page = get_page(pool, 0);
  ASSERT_NOT_NULL(page);
  ASSERT_EQUAL( page->size, 1024);
  ASSERT_EQUAL( page->used, 615);

  page = get_page(pool, 1);
  ASSERT_EQUAL( page->size, 1024);
  ASSERT_EQUAL( page->used, 512);

  ASSERT_EQUAL( 2,          cleaner_count(pool) );

  cleaner = get_cleaner(pool, 0);
  page    = get_page(pool, 0);
  ASSERT_TRUE(cleaner->arg == page);

  cleaner = get_cleaner(pool, 1);
  page    = get_page(pool, 1);
  ASSERT_TRUE(cleaner->arg == page);

  ls_pool_destroy(pool);

  ASSERT_TRUE( ls_pool_create(1024, &pool, &err) );
  /* test a number of small allocations, enough to fill a page, insure mallocs
   * are word aligned*/
  ASSERT_TRUE( ls_pool_malloc(pool, 1, &ptr, &err) );
  for (i = 1; i < 115; ++i)
  {
    size_t sz = ( i % (sizeof(uintptr_t) * 2) ) + 1;
    ASSERT_TRUE( ls_pool_malloc(pool, sz, &ptr, &err) );
  }
  ls_pool_destroy(pool);
}

CTEST(ls_pool, malloc_overallocate)
{
  ls_pool*           pool;
  ls_err             err;
  void*              ptr;
  _pool_page*        page1;
  _pool_cleaner_ctx* cleaner;

  ASSERT_TRUE( ls_pool_create(1024, &pool, &err) );
  ASSERT_TRUE( ls_pool_malloc(pool, 512, &ptr, &err) );
  ASSERT_NOT_NULL(ptr);

  ASSERT_TRUE( ls_pool_malloc(pool, 2048, &ptr, &err) );
  ASSERT_NOT_NULL(ptr);

  ASSERT_EQUAL( pool->size,      3072);
  ASSERT_EQUAL( pool->page_size, 1024);

  ASSERT_EQUAL( 1,               page_count(pool) );

  page1 = get_page(pool, 0);
  ASSERT_NOT_NULL(page1);
  ASSERT_EQUAL( page1->size, 1024);
  ASSERT_EQUAL( page1->used, 512);

  ASSERT_EQUAL( 2,           cleaner_count(pool) );

  cleaner = get_cleaner(pool, 0);
  page1   = get_page(pool, 0);
  ASSERT_TRUE(cleaner->arg == ptr);

  cleaner = get_cleaner(pool, 1);
  ASSERT_TRUE(cleaner->arg == page1);

  ls_pool_destroy(pool);

}

CTEST(ls_pool, strdup_shorterpool)
{
  ls_pool*    pool;
  ls_err      err;
  char*       dup;
  _pool_page* page;
  const char* src;

  src =
    "\x2d\xa7\x5d\xa5\xc8\x54\x78\xdf\x42\xdf\x0f\x91\x77\x00\x24\x1e\xd2\x82\xf5\x99\x2d\xa7\x5d\xa5\xc8\x54\x78\xdf\x42\xdf\x0f\x91\x77\x00\x24\x1e\xd2\x82\xf5\x99";
  ASSERT_TRUE( ls_pool_create(8, &pool, &err) );
  ASSERT_TRUE( ls_pool_strdup(pool, src, &dup, &err) );
  ASSERT_NOT_NULL(dup);
  ASSERT_STR(src, dup);

  ASSERT_EQUAL( pool->size,      22);
  ASSERT_EQUAL( pool->page_size, 8);
  ASSERT_EQUAL( 1,               page_count(pool) );
  page = get_page(pool, 0);
  ASSERT_NOT_NULL(page);
  ASSERT_EQUAL(page->used, 0);

  ls_pool_destroy(pool);
}

#endif
CTEST(ls_pool, calloc)
{
  ls_pool* pool;
  ls_err   err;
  void*    ptr;

  ASSERT_TRUE( ls_pool_create(1024, &pool, &err) );
  ASSERT_TRUE( ls_pool_calloc(pool, 4, 128, &ptr, &err) );
  ASSERT_NOT_NULL(ptr);

  ls_pool_destroy(pool);
}

CTEST(ls_pool, calloc_nobytes)
{
  ls_pool* pool;
  ls_err   err;
  void*    ptr;

  ASSERT_TRUE( ls_pool_create(1024, &pool, &err) );
  ASSERT_TRUE( ls_pool_calloc(pool, 4, 0, &ptr, &err) );
  ASSERT_NULL(ptr);

  ls_pool_destroy(pool);
}

CTEST(ls_pool, strdup)
{
  ls_pool*    pool;
  ls_err      err;
  char*       dup;
  const char* src = "hi-de-ho";

  ASSERT_TRUE( ls_pool_create(1024, &pool, &err) );
  ASSERT_TRUE( ls_pool_strdup(pool, src, &dup, &err) );
  ASSERT_NOT_NULL(dup);
  ASSERT_STR(src, dup);

  ASSERT_TRUE( ls_pool_strdup(pool, "", &dup, &err) );
  ASSERT_NOT_NULL(dup);
  ASSERT_STR("", dup);

  ASSERT_TRUE( ls_pool_strdup(pool, NULL, &dup, &err) );
  ASSERT_NULL(dup);

  ls_pool_destroy(pool);
}

CTEST(ls_pool, add_cleaner)
{
  ls_pool* pool;
  ls_err   err;
  void*    ptr;

  ASSERT_TRUE( ls_pool_create(1024, &pool, &err) );

  ASSERT_TRUE( ls_pool_malloc(pool, 512, &ptr, &err) );
  ASSERT_TRUE( ls_pool_add_cleaner(pool, &test_cleaner, ptr, &err) );
  expected = ptr;
  ls_pool_destroy(pool);
  ASSERT_TRUE(cleaner_hit);
  cleaner_hit = false;
  expected    = NULL;
}

CTEST(ls_pool, add_cleaner_nonpool)
{
  ls_pool* pool;
  ls_err   err;
  void*    ptr;

  ASSERT_TRUE( ls_pool_create(512, &pool, &err) );

  ptr = (void*)malloc(512);
  ASSERT_NOT_NULL(ptr);

  ASSERT_TRUE( ls_pool_add_cleaner(pool, &test_cleaner, ptr, &err) );
  expected = ptr;
  ls_pool_destroy(pool);
  ASSERT_TRUE(cleaner_hit);
  cleaner_hit = true;
  expected    = NULL;
  free(ptr);
}

CTEST(ls_data, memory)
{
  void*          dummy;
  oom_test_data* tdata = oom_get_data();
  oom_set_enabled(true);

  dummy = ls_data_malloc(10);
  ASSERT_EQUAL(tdata->numMallocCalls,  1);

  dummy = ls_data_realloc(dummy, 5);
  ASSERT_EQUAL(tdata->numReallocCalls, 0);
  ls_data_free(dummy);
  ASSERT_EQUAL(tdata->numFreeCalls,    1);

  dummy = ls_data_realloc(NULL, 5);
  ASSERT_EQUAL(tdata->numReallocCalls, 1);
  ls_data_free(dummy);
  ASSERT_EQUAL(tdata->numFreeCalls, 2);

  /* NULL should be no-op */
  ls_data_free(NULL);
  ASSERT_EQUAL(tdata->numFreeCalls, 2);

  /* Reset memory functions */
  ls_data_set_memory_funcs(NULL, NULL, NULL);

  dummy = ls_data_malloc(10);
  ASSERT_EQUAL(tdata->numMallocCalls,  1);

  dummy = ls_data_realloc(dummy, 5);
  ASSERT_EQUAL(tdata->numReallocCalls, 1);

  ls_data_free(dummy);
  ASSERT_EQUAL(tdata->numFreeCalls, 2);
  oom_set_enabled(false);
}

/* htable_tests */
CTEST(ls_htable, no_mem)
{
  ls_htable* table;
  OOM_SIMPLE_TEST( ls_htable_create(7,
                                    ls_int_hashcode,
                                    ls_int_compare,
                                    &table,
                                    &err) );
  ls_htable_destroy(table);
}
CTEST(ls_htable, put_no_mem)
{
  ls_htable* table;
  ls_err     err;
  ASSERT_TRUE( ls_htable_create(7,
                                ls_int_hashcode,
                                ls_int_compare,
                                &table,
                                NULL) );
  OOM_RECORD_ALLOCS( ls_htable_put(table, "key1", "value one", NULL, &err) )
  OOM_TEST_INIT()
  ls_htable_remove(table, "key1");
  OOM_TEST( &err, ls_htable_put(table, "key1", "value one", NULL, &err) )
  ls_htable_destroy(table);
}

CTEST(ls_event, dispatcher_create_no_mem)
{
  ls_event_dispatcher* dispatch;
  void*                source = "the source";
  OOM_SIMPLE_TEST( ls_event_dispatcher_create(
                     source, &dispatch, &err) );
  ls_event_dispatcher_destroy(dispatch);
}

CTEST(ls_event, create_no_mem)
{
  ls_event_dispatcher* dispatch;
  ls_err               err;
  void*                source = "the source";
  ls_event*            evt;
  OOM_POOL_TEST_BGN
  ASSERT_TRUE( ls_event_dispatcher_create(source, &dispatch, &err) );
  OOM_RECORD_ALLOCS( ls_event_dispatcher_create_event(dispatch,
                                                      "eventOne",
                                                      &evt,
                                                      &err) )
  OOM_TEST_INIT()
  ls_event_dispatcher_destroy(dispatch);
  ASSERT_TRUE( ls_event_dispatcher_create(source, &dispatch, &err) );
  OOM_TEST( &err,
            ls_event_dispatcher_create_event(dispatch,
                                             "eventOne",
                                             &evt,
                                             &err) )
  ls_event_dispatcher_destroy(dispatch);
  OOM_POOL_TEST_END

}
CTEST(ls_event, bind_no_mem)
{
  ls_event_dispatcher* dispatch;
  ls_err               err;
  void*                source = "the source";
  ls_event*            evt1;
  OOM_POOL_TEST_BGN

  ASSERT_TRUE( ls_event_dispatcher_create(source, &dispatch, &err) );
  ASSERT_TRUE(ls_event_dispatcher_create_event(dispatch,
                                               "eventOne",
                                               &evt1,
                                               &err) == true);
  OOM_RECORD_ALLOCS( ls_event_bind(evt1, mock_evt1_callback1, NULL, &err) )
  OOM_TEST_INIT()
  ls_event_unbind(evt1, mock_evt1_callback1);
  OOM_TEST( &err, ls_event_bind(evt1, mock_evt1_callback1, NULL, &err) )
  ls_event_unbind(evt1, mock_evt1_callback1);
  ls_event_dispatcher_destroy(dispatch);
  OOM_POOL_TEST_END

}
CTEST(ls_event, trigger_no_mem)
{
  ls_event_dispatcher* dispatch;
  ls_err               err;
  void*                source = "the source";
  ls_event*            evt1;
  OOM_POOL_TEST_BGN
  ASSERT_TRUE( ls_event_dispatcher_create(source, &dispatch, &err) );
  ASSERT_TRUE(ls_event_dispatcher_create_event(dispatch,
                                               "eventOne",
                                               &evt1,
                                               &err) == true);
  OOM_SIMPLE_TEST( ls_event_trigger(evt1, NULL, NULL, NULL, &err) )
  ls_event_unbind(evt1, mock_evt1_callback1);
  ls_event_dispatcher_destroy(dispatch);
  OOM_POOL_TEST_END
}
