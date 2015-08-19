/**
 * \file
 * \brief
 * Just enough eventing information to bind to events and process callbacks.
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */
#pragma once

#include "ls_basics.h"
#include "ls_mem.h"

/**
 * Datatype for an event (notifier). It manages the callbacks and triggerings
 * for a given event.
 */
typedef struct _ls_event_t ls_event;

/** Event data passed to bound callbacks. */
typedef struct _ls_event_data_t
{
  /** Event source */
  void* source;
  /** Event name */
  const char* name;
  /** Event object */
  ls_event* notifier;
  /** Data specific to this triggering of an event */
  void* data;
  /** Possible selection. Reserved for future use. */
  void* selected;
  /** Pool to use for any modification to this event data */
  ls_pool* pool;
  /**
   * Flag to indicate the event has been handled in some manner.
   * Callbacks may set this value to true; the eventing logic will
   * ensure this value, once set to true, is propagated to all further
   * callbacks for this event.
   */
  bool handled;
} ls_event_data;

/**
 * Callback executed when an event is triggered. Callbacks should set the
 * handled flag in the ls_event_data to true to indicate the event was handled.
 *
 * \param[in] evt Event information
 * \param[in] arg An argument bound to this callback
 */
typedef void (* ls_event_notify_callback)(ls_event_data* evt,
                                          void*          arg);
