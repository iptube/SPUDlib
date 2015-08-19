/**
 * \file
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define CTEST_MAIN
#include "test_utils.h"

int
main(int         argc,
     const char* argv[])
{
  return ctest_main(argc, argv);
}

static uint32_t _mallocCnt = 0;
static void*
_counting_malloc(size_t size)
{
  ++_mallocCnt;
  return malloc(size);
}
uint32_t
_test_get_malloc_count()
{
  return _mallocCnt;
}

static void*
_counting_realloc(void*  ptr,
                  size_t size)
{
  if (NULL == ptr)
  {
    return _counting_malloc(size);
  }
  return realloc(ptr, size);
}

static int _freeCnt = 0;
static void
_counting_free(void* ptr)
{
  if (NULL == ptr)
  {
    return;
  }
  ++_freeCnt;
  free(ptr);
}
uint32_t
_test_get_free_count()
{
  return _freeCnt;
}

void
_test_init_counting_memory_funcs()
{
  _mallocCnt = 0;
  _freeCnt   = 0;
  ls_data_set_memory_funcs(
    _counting_malloc, _counting_realloc, _counting_free);
}

void
_test_uninit_counting_memory_funcs()
{
  ls_data_set_memory_funcs(NULL, NULL, NULL);
}

static oom_test_data _oom_test = {
  .ls_AllocCount   = 0,
  .ls_AllocLimit   = -1,
  .failureAttempts = 0,
  .numMallocCalls  = 0,
  .numReallocCalls = 0,
  .numFreeCalls    = 0
};

/* get access to the variables declared in mem.c */
extern ls_data_malloc_func  _malloc_func;
extern ls_data_realloc_func _realloc_func;
extern ls_data_free_func    _free_func;

/* cached external refs */
static ls_data_malloc_func  mallocFnOrig  = NULL;
static ls_data_realloc_func reallocFnOrig = NULL;
static ls_data_free_func    freeFnOrig    = NULL;

static void*
mallocFn (size_t size)
{
  void* ret = NULL;

  ++_oom_test.numMallocCalls;
  if ( (_oom_test.ls_AllocLimit < 0) ||
       (_oom_test.ls_AllocCount < _oom_test.ls_AllocLimit) )
  {
    ret = mallocFnOrig(size);
  }
  ++_oom_test.ls_AllocCount;
  return ret;
}

static void*
reallocFn (void*  ptr,
           size_t size)
{
  void* ret = NULL;

  if (NULL == ptr)
  {     /* numReallocCalls counts the number of references that should be freed,
         * */
        /* non NULL realloc pointers are already accounted for in their initial
         * */
        /* allocation. */
    ++_oom_test.numReallocCalls;
  }

  if ( (_oom_test.ls_AllocLimit < 0) ||
       (_oom_test.ls_AllocCount < _oom_test.ls_AllocLimit) )
  {
    ret = reallocFnOrig(ptr, size);
  }
  ++_oom_test.ls_AllocCount;
  return ret;
}

static void
freeFn (void* ptr)
{
  ++_oom_test.numFreeCalls;
  freeFnOrig(ptr);
}

void
oom_set_enabled(bool on)
{
  _oom_test.ls_AllocCount   = _oom_test.numMallocCalls = 0;
  _oom_test.numReallocCalls = _oom_test.numFreeCalls = 0;
  _oom_test.ls_AllocLimit   = -1;

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

oom_test_data*
oom_get_data()
{
  return &_oom_test;
}
