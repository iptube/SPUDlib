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
typedef void (* ls_timer_func)(ls_timer* tim);

/**
 * Create a a timer that fires at a given time.
 *
 * \param[in]  actual  The UTC wall-clock time at which to fire a timer
 * \param[in]  cb      The function to call at the given time
 * \param[in]  context A parameter to pass into the
 * \param[out] tim     The created timer
 * \param[out] err     The error information (provide NULL to ignore)
 * \return             true if successful, false otherwise.
 */
LS_API bool
ls_timer_create(const struct timeval* actual,
                ls_timer_func         cb,
                void*                 context,
                ls_timer**            tim,
                ls_err*               err);

/**
 * Createa a timer that fires at a given millisecond offset from a given time.
 * Usually the given time is "now", or an approximation thereof.
 *
 * \param[in]  now     The current UTC wall-clock time
 * \param[in]  ms      The offset from "now" in milliseconds
 * \param[in]  cb      The function to call at the given time
 * \param[in]  context A parameter to pass into the
 * \param[out] tim     The created timer
 * \param[out] err     The error information (provide NULL to ignore)
 * \return             true if successful, false otherwise.
 */
LS_API bool
ls_timer_create_ms(const struct timeval* now,
                   unsigned long         ms,
                   ls_timer_func         cb,
                   void*                 context,
                   ls_timer**            tim,
                   ls_err*               err);

/**
 * Destroy a timer.  Does *not* fire the callback.
 *
 * \param[in] tim The timer to destroy
 */
LS_API void
ls_timer_destroy(ls_timer* tim);

/**
 * Has the timer been cancelled?
 *
 * \param[in]  tim The timer to query
 * \return         true if cancelled, otherwise false
 */
LS_API bool
ls_timer_is_cancelled(ls_timer* tim);

/**
 * Cancel a timer.
 *
 * \param[in] tim The timer to cancel
 */
LS_API void
ls_timer_cancel(ls_timer* tim);

/**
 * Get the context pointer from the timer.
 *
 * \param[in]  tim The timer to query
 * \return         The context pointer
 */
LS_API void*
ls_timer_get_context(ls_timer* tim);

/**
 * Get the time that the timer is due to fire
 *
 * \param[in]  tim The timer to query
 * \return         The UTC wall-clock time the timer is due to fire.  Will be
 *                 NULL if cancelled.
 */
LS_API struct timeval*
ls_timer_get_time(ls_timer* tim);

/**
 * Is a < b ?
 *
 * \param[in]  a First timer
 * \param[in]  b Second timer
 * \return       true if a < b
 */
LS_API bool
ls_timer_less(ls_timer* a,
              ls_timer* b);

/**
 * Is a > b ?
 *
 * \param[in]  a First timer
 * \param[in]  b Second timer
 * \return       true if a > b
 */
LS_API bool
ls_timer_greater(ls_timer* a,
                 ls_timer* b);

/**
 * Is a > b ?
 *
 * \param[in]  a First timer
 * \param[in]  b Comparison time
 * \return       true if a > b
 */
LS_API bool
ls_timer_greater_tv(ls_timer*       a,
                    struct timeval* b);

/**
 * Execute the timer callback.
 *
 * \param[in] tim The timer to fire
 */
LS_API void
ls_timer_exec(ls_timer* tim);
