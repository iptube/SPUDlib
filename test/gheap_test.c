/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

// Note: gheap contains its own tests.  This is just a quick sanity check
// to make sure that using ls_data_malloc/ls_data_free works and that the
// API works the way I expect.

#include "test_utils.h"

#include <sys/time.h>

#include "ls_mem.h"
#define GHEAP_MALLOC ls_data_malloc
#define GHEAP_FREE ls_data_free
#include "../vendor/gheap/gpriority_queue.h"


static int timeval_less(const void *const context,
                       const void *const a,
                       const void *const b)
{
    struct timeval *atv = (struct timeval *)a;
    struct timeval *btv = (struct timeval *)b;
    UNUSED_PARAM(context);

    return timercmp(atv, btv, >);
}

static void timeval_move(void *const dst, const void *const src)
{
  *(struct timeval *)dst = *(struct timeval *)src;
}

static int less(const void *const ctx, const void *const a,
    const void *const b)
{
  (void)ctx;
  // less is more.  I'm going to want a minheap.
  return *(int *)a > *(int *)b;
}

static void move(void *const dst, const void *const src)
{
  *((int *)dst) = *((int *)src);
}

static void nodel(void *item)
{
    UNUSED_PARAM(item);
}

CTEST(gpriority_queue, basic)
{
    static const struct gheap_ctx paged_binary_heap_ctx = {
      .fanout = 2,
      .page_chunks = 512,
      .item_size = sizeof(int),
      .less_comparer = &less,
      .less_comparer_ctx = NULL,
      .item_mover = &move,
    };
    int one = 1;
    int two = 2;
    int three = 3;
    const int *top;

    struct gpriority_queue *pq = gpriority_queue_create(&paged_binary_heap_ctx, nodel);
    gpriority_queue_push(pq, &one);
    gpriority_queue_push(pq, &three);
    gpriority_queue_push(pq, &two);
    ASSERT_EQUAL(1, one);
    ASSERT_EQUAL(2, two);
    ASSERT_EQUAL(3, three);
    top = gpriority_queue_top(pq);
    ASSERT_EQUAL(1, *top);
    gpriority_queue_pop(pq);
    top = gpriority_queue_top(pq);
    ASSERT_EQUAL(2, *top);
    gpriority_queue_pop(pq);
    top = gpriority_queue_top(pq);
    ASSERT_EQUAL(3, *top);
    gpriority_queue_pop(pq);

    gpriority_queue_delete(pq);
}

CTEST(gpriority_queue, timeval) {
    static const struct gheap_ctx paged_binary_heap_ctx = {
      .fanout = 2,
      .page_chunks = 512,
      .item_size = sizeof(struct timeval),
      .less_comparer = &timeval_less,
      .less_comparer_ctx = NULL,
      .item_mover = &timeval_move,
    };
    struct timeval one = {0, 1};
    struct timeval two = {0, 2};
    struct timeval three = {1, 0};
    const struct timeval *top;

    struct gpriority_queue *pq = gpriority_queue_create(&paged_binary_heap_ctx, nodel);
    gpriority_queue_push(pq, &one);
    gpriority_queue_push(pq, &three);
    gpriority_queue_push(pq, &two);
    top = gpriority_queue_top(pq);
    ASSERT_TRUE(timercmp(&one, top, ==));
    gpriority_queue_pop(pq);
    top = gpriority_queue_top(pq);
    ASSERT_TRUE(timercmp(&two, top, ==));
    gpriority_queue_pop(pq);
    top = gpriority_queue_top(pq);
    ASSERT_TRUE(timercmp(&three, top, ==));
    gpriority_queue_pop(pq);

    gpriority_queue_delete(pq);
}
