/**
 * \file
 * \brief
 * Timer state management, allowing for cancellation
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#pragma once

#include <sys/time.h>

#include "ls_basics.h"
#include "ls_error.h"

/** An instance of a timer */
typedef struct _ls_timer ls_timer;

/**
 * Type of the function called when a timer expires.
 * See tube_manager_schedule and tube_manager_schedule_ms.
 */
typedef void (*ls_timer_func)(ls_timer *tim);

LS_API bool ls_timer_create(const struct timeval *actual,
                            ls_timer_func         cb,
                            void                 *context,
                            ls_timer            **tim,
                            ls_err               *err);

LS_API bool ls_timer_create_ms(const struct timeval *now,
                               unsigned long         ms,
                               ls_timer_func         cb,
                               void                 *context,
                               ls_timer            **tim,
                               ls_err               *err);

LS_API void ls_timer_destroy(ls_timer *tim);

LS_API bool ls_timer_is_cancelled(ls_timer *tim);

LS_API void ls_timer_cancel(ls_timer *tim);

LS_API void * ls_timer_get_context(ls_timer *tim);

LS_API struct timeval * ls_timer_get_time(ls_timer *tim);

LS_API bool ls_timer_less(ls_timer *a, ls_timer *b);

LS_API bool ls_timer_greater(ls_timer *a, ls_timer *b);

LS_API bool ls_timer_greater_tv(ls_timer *a, struct timeval *b);

LS_API void ls_timer_exec(ls_timer *tim);
