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

typedef void (*ls_queue_cleanfunc)(ls_queue *q, void *data);

LS_API bool ls_queue_create(ls_queue **q, ls_queue_cleanfunc destroy_cb, ls_err *err);
LS_API void ls_queue_destroy(ls_queue *q);
LS_API bool ls_queue_enq(ls_queue *q, void *value, ls_err *err);
LS_API void * ls_queue_deq(ls_queue *q);
