/**
 * \file
 *
 * Copyrights
 *
 * Portions created or assigned to Cisco Systems, Inc. are
 * Copyright (c) 2011 Cisco Systems, Inc.  All Rights Reserved.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_utils.h"

static uint32_t _mallocCnt = 0;
static void *_counting_malloc(size_t size)
{
    ++_mallocCnt;
    return malloc(size);
}
uint32_t _test_get_malloc_count()
{
    return _mallocCnt;
}

static void *_counting_realloc(void *ptr, size_t size)
{
    if (NULL == ptr)
    {
        return _counting_malloc(size);
    }
    return realloc(ptr, size);
}

static int _freeCnt = 0;
static void _counting_free(void *ptr)
{
    if (NULL == ptr)
    {
        return;
    }
    ++_freeCnt;
    free(ptr);
}
uint32_t _test_get_free_count()
{
    return _freeCnt;
}

void _test_init_counting_memory_funcs()
{
    _mallocCnt = 0;
    _freeCnt = 0;
    ls_data_set_memory_funcs(
            _counting_malloc, _counting_realloc, _counting_free);
}

void _test_uninit_counting_memory_funcs()
{
    ls_data_set_memory_funcs(NULL, NULL, NULL);
}
