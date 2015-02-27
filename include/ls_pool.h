/**
 * \file
 * \brief
 * Memory pool typedefs. private, not for use outside library and unit tests.
 * \see pools.h
 *
 * Copyrights
 *
 * Portions created or assigned to Cisco Systems, Inc. are
 * Copyright (c) 2010 Cisco Systems, Inc.  All Rights Reserved.
 */

#pragma once

#include <stdlib.h>

/**
 * Pools use "page"s, blocks of allocated mem, for allocation
 * and release efficiency. When possible pools "malloc" by just
 * return pointers into a page. When releasing pools free entire
 * pages, not pointer by pointer, very desirable behavior when
 * managing recursive data structures like DOMs.
 *
 * If a allocation request could not fit in any page, memory is
 * allocated directly. The resultant pointer is freed when the
 * pool is destroyed. If a request would fit in a page but the
 * current page doesn't have enough free spoace a new page is
 * allocated and used for the request.
 *
 * A page is a node in a LL. Not really necessary for
 * alloc/free logic but a list makes testing page much easier.
 * Without this list pages are still accessable through the
 * cleaner list (where the page is the arg passed to
 * a _pool_page_free cleaner) but it's confusing.
 */
typedef struct pool_page
{
    void*  block;
    size_t size;
    size_t used;
    struct pool_page *next;
} *_pool_page;

/**
 * pool_cleaners are callbacks fired when a pool is being destroyed.
 * A pool_cleaner_ctx is a node in a LL of cleaners. Each "context"
 * contains a cleaner ref and an argument to pass to the cleaner.
 * Typically the argument is the pointer that should be freed.
 * It is up to the cleaner to free arg if it is a pointer. pages
 * are freed using a page cleaner etc.
 */
typedef struct pool_cleaner_ctx
{
    ls_pool_cleaner cleaner;
    void*           arg;
    struct pool_cleaner_ctx *next;
} *_pool_cleaner_ctx;

/**
 * A pool is a head pointer to the page linked list, a head
 * pointer to the cleaners LL, the total number of bytes
 * allocated by this pool (pages*page size)+off page allocations
 * and the page size given to the pool at creation.
 */
typedef struct _ls_pool_int
{
    size_t size;
    size_t page_size;
    struct pool_cleaner_ctx *cleaners;
    struct pool_cleaner_ctx *tail;
    struct pool_page        *pages;
} _ls_pool;
