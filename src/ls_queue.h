/**
 * \file
 * \brief
 * Datatypes and functions for queues.
 *
 * \b NOTE: This API is not thread-safe.  Users MUST ensure access to all
 * instances of a hashtable is limited to a single thread.
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#pragma once

#include "ls_basics.h"
#include "ls_error.h"

/** An instance of a queue */
typedef struct _ls_queue ls_queue;

typedef void (* ls_queue_cleanfunc)(ls_queue* q,
                                    void*     data);

/**
 * Create a queue.  If the cleaner function is not null, it will be called on
 * all items let in the queue when the queue is destroyed.
 *
 * \param[in]   cleaner    Function to be called (usually to free memory) when
 *                         the queue is destroyed.  May be NULL.
 * \param[out]  q          The created queue
 * \param[out]  err        The error information (provide NULL to ignore)
 * \return bool            true if successful, false otherwise.
 * \invariant q != NULL
 */
LS_API bool
ls_queue_create(ls_queue_cleanfunc cleaner,
                ls_queue**         q,
                ls_err*            err);

/**
 * Destroy a queue.
 * \param q[in] The queue to destroy
 * \invariant q != NULL
 */
LS_API void
ls_queue_destroy(ls_queue* q);

/**
 * Enqueue a value onto a queue
 * \param[in]  q     The queue to modify
 * \param[in]  value The value to enqueue
 * \param[out] err   The error information (provide NULL to ignore)
 * \return           true if successful, false otherwise.
 * \invariant q != NULL
 * \invariant value != NULL
 */
LS_API bool
ls_queue_enq(ls_queue* q,
             void*     value,
             ls_err*   err);

/**
 * Dequeue the oldest value from the queue.  Does NOT call the cleaner function,
 * because you're about to use the returned value.
 *
 * \param[in]  q The queue to modify
 * \return   [description]
 * \invariant q != NULL
 */
LS_API void*
ls_queue_deq(ls_queue* q);
