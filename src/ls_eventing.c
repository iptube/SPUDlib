/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <assert.h>
#include <string.h>

#include "ls_eventing.h"
#include "ls_htable.h"
#include "ls_str.h"
#include "ls_log.h"
#include "ls_eventing_int.h"

/* Internal Constants */
static const int    DISPATCH_BUCKETS = 7;
static const size_t MOMENT_POOLSIZE  = 0;

/**
 * Event triggering data.
 */
typedef struct _ls_event_trigger_t
{
  ls_event_moment_t* moment;
  ls_pool*           pool;
} ls_event_trigger_t;


#define PUSH_EVENTING_NDC _ndcDepth = _push_eventing_ndc(dispatch, __func__)
#define POP_EVENTING_NDC if (_ndcDepth > 0) {ls_log_pop_ndc(_ndcDepth); }
static int
_push_eventing_ndc(ls_event_dispatcher* dispatch,
                   const char*          entrypoint)
{
  assert(entrypoint);

  return ls_log_push_ndc("eventing dispatcher=%p; entrypoint=%s",
                         (void*)dispatch, entrypoint);
}

/* Internal Functions */
/**
 * Searched the notifier for an existing binding of the given callback.
 * This function updates {pmatch} and {pins} as appropriate, including
 * setting values to NULL if there is no match.
 *
 * returns whether the binding was found
 */
static inline bool
_remove_binding(ls_event_notifier_t*     notifier,
                ls_event_notify_callback cb,
                ls_event_binding_t**     pmatch,
                ls_event_binding_t**     pins)
{
  ls_event_binding_t* curr = notifier->bindings;
  ls_event_binding_t* prev = NULL;
  bool                head = true;

  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  /* initialize to not found */
  *pmatch = NULL;
  while (curr != NULL)
  {
    if (curr->cb == cb)
    {
      /* callback matches; remove from list */
      if (head)
      {
        notifier->bindings = curr->next;
      }
      else
      {
        prev->next = curr->next;
      }

      /* remember found binding */
      *pmatch = curr;

      /* finish breaking the list */
      curr->next = NULL;

      break;
    }

    head = false;
    prev = curr;
    curr = curr->next;
  }

  /* update insert point */
  if (pins)
  {
    *pins = prev;
  }

  return (*pmatch != NULL);
}

static inline void
_process_pending_unbinds(ls_event* event)
{
  ls_event_binding_t* prev = NULL;
  ls_event_binding_t* cur  = event->bindings;

  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  while (cur != NULL)
  {
    ls_event_binding_t* remove = cur;
    if (!cur->unbound)
    {
      prev   = cur;
      remove = NULL;
    }
    else
    {
      if (prev != NULL)
      {
        prev->next = cur->next;
      }
      else
      {
        event->bindings = cur->next;
      }
    }

    cur = cur->next;

    ls_data_free(remove);
  }
}

static inline void
_set_bind_status(ls_event* event)
{
  ls_event_binding_t* cur = event->bindings;
  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  while (cur != NULL)
  {
    cur->normal_bound = true;
    cur               = cur->next;
  }
}

static void
_moment_destroy(ls_event_moment_t* moment)
{
  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  assert(moment);

  ls_pool_destroy(moment->evt.pool);
}

static void
_handle_trigger_int(ls_event_dispatcher* dispatch)
{
  ls_event_moment_t* moment = dispatch->next_moment;
  ls_event_data*     evt;
  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  assert(moment);
  evt = &moment->evt;

  ls_log(LS_LOG_DEBUG, "processing event '%s'", evt->name);

  assert(NULL == dispatch->running);
  dispatch->running = evt->notifier;
  _set_bind_status(evt->notifier);

  /* process callbacks */
  for (ls_event_binding_t* binding = moment->bindings;
       NULL != binding;
       binding = binding->next)
  {
    if (binding->normal_bound)
    {
      bool handled = evt->handled;

      binding->cb(evt, binding->arg);

      /* prevent callbacks from "unhandling" */
      evt->handled = handled || evt->handled;
    }
  }

  /* report event results */
  if (moment->result_cb)
  {
    moment->result_cb(evt, evt->handled, moment->result_arg);
  }

  /* clean up and prepare for next moment */
  _process_pending_unbinds(evt->notifier);
  dispatch->next_moment = moment->next;
  _moment_destroy(moment);

  if (NULL == dispatch->next_moment)
  {
    dispatch->moment_queue_tail = NULL;
  }
  dispatch->running = NULL;
}

/**
 * Enqueues an event and, if this dispatcher is synchronous and is not currently
 * handling an event, processes it immediately.  If this dispatcher is async,
 * processing is scheduled.
 */
static inline void
_dispatch_trigger(ls_event_dispatcher* dispatcher,
                  ls_event_moment_t*   moment)
{
  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  if (NULL != dispatcher->moment_queue_tail)
  {
    dispatcher->moment_queue_tail->next = moment;
  }
  if (NULL == dispatcher->next_moment)
  {
    dispatcher->next_moment = moment;
  }
  dispatcher->moment_queue_tail = moment;

  if (dispatcher->running != NULL)
  {
    ls_log(LS_LOG_DEBUG, "already processing events; deferring event '%s'",
           moment->evt.name);
    return;
  }

  /* handle queued triggers synchronously */
  while (NULL != dispatcher->next_moment && !dispatcher->destroy_pending)
  {
    _handle_trigger_int(dispatcher);
  }

  if (dispatcher->destroy_pending)
  {
    ls_event_dispatcher_destroy(dispatcher);
  }
}

static void
_clean_event(bool  replace,
             bool  delete_key,
             void* key,
             void* data)
{
  ls_event*           event = data;
  ls_event_binding_t* curr  = event->bindings;

  UNUSED_PARAM(key);
  UNUSED_PARAM(delete_key);
#ifdef NDEBUG
  UNUSED_PARAM(replace);
#else
  assert(!replace);
#endif

  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  /* Clean up callbacks */
  while (curr != NULL)
  {
    ls_event_binding_t* remove = curr;
    curr = curr->next;
    ls_data_free(remove);
    remove = NULL;
  }

  ls_data_free( (char*)ls_event_get_name(event) );
  ls_data_free( event);
}

static bool
_prepare_trigger(ls_event_dispatcher*    dispatch,
                 ls_event_trigger_data** trigger_data,
                 ls_err*                 err)
{

  union
  {
    ls_event_trigger_data* tdata;
    void*                  tdataPtr;
  } tdataUnion;

  ls_pool* pool;

  union
  {
    ls_event_moment_t* moment;
    void*              momentPtr;
  } momentUnion;
  UNUSED_PARAM(dispatch);

  LS_LOG_TRACE_FUNCTION_NO_ARGS;
  assert(trigger_data);

  if ( !ls_pool_create(MOMENT_POOLSIZE, &pool, err) )
  {
    ls_log(LS_LOG_WARN, "unable to allocate pool with block size %zd",
           MOMENT_POOLSIZE);
    return false;
  }

  if ( !ls_pool_calloc(pool,
                       1,
                       sizeof(struct _ls_event_moment_t),
                       &momentUnion.momentPtr,
                       err) )
  {
    ls_log(LS_LOG_WARN, "unable to allocate moment");
    ls_pool_destroy(pool);
    return false;
  }

  if ( !ls_pool_malloc(pool,
                       sizeof(ls_event_trigger_t), &tdataUnion.tdataPtr, err) )
  {
    ls_log(LS_LOG_WARN, "unable to allocate event trigger data");
    ls_pool_destroy(pool);
    return false;
  }

  tdataUnion.tdata->pool   = pool;
  tdataUnion.tdata->moment = momentUnion.moment;
  *trigger_data            = tdataUnion.tdata;

  return true;
}

static void
_trigger_prepared(ls_event_dispatcher*     dispatch,
                  ls_event*                event,
                  void*                    data,
                  ls_event_result_callback result_cb,
                  void*                    result_arg,
                  ls_event_trigger_data*   trigger_data)
{
  ls_event_data*     evt;
  ls_event_moment_t* moment;

  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  assert(event);
  assert(trigger_data);
  assert(trigger_data->pool);
  assert(trigger_data->moment);

  ls_log(LS_LOG_DEBUG, "triggering event '%s'", event->name);

  moment = trigger_data->moment;

  /* setup the event moment */
  moment->result_cb  = result_cb;
  moment->result_arg = result_arg;
  moment->bindings   = event->bindings;

  /* setup event data */
  evt           = &moment->evt;
  evt->source   = dispatch->source;
  evt->name     = event->name;
  evt->notifier = event;
  evt->data     = data;
  evt->selected = NULL;
  evt->pool     = trigger_data->pool;
  evt->handled  = false;

  /* enqueue, and maybe run */
  /* do not use the dispatcher after this line as it may have been destroyed */
  _dispatch_trigger(dispatch, moment);
}


/* External Functions */
LS_API bool
ls_event_dispatcher_create(void*                 source,
                           ls_event_dispatcher** outdispatch,
                           ls_err*               err)
{
  ls_htable*           events   = NULL;
  ls_event_dispatch_t* dispatch = NULL;
  int                  _ndcDepth;

  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  assert(source != NULL);
  assert(outdispatch != NULL);

  if ( !ls_htable_create(DISPATCH_BUCKETS,
                         ls_strcase_hashcode,
                         ls_strcase_compare,
                         &events,
                         err) )
  {
    return false;
  }

  dispatch = ls_data_calloc( 1, sizeof(ls_event_dispatch_t) );
  if (dispatch == NULL)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    ls_htable_destroy(events);
    return false;
  }

  PUSH_EVENTING_NDC;
  if (_ndcDepth == 0)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    ls_htable_destroy(events);
    ls_data_free(dispatch);
    return false;
  }
  ls_log(LS_LOG_TRACE, "creating new event dispatcher");

  dispatch->source = source;
  dispatch->events = events;
  *outdispatch     = dispatch;

  POP_EVENTING_NDC;

  return true;
}

LS_API void
ls_event_dispatcher_destroy(ls_event_dispatcher* dispatch)
{
  ls_event_moment_t* moment;
  int                _ndcDepth;
  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  assert(dispatch != NULL);
  PUSH_EVENTING_NDC;

  if (NULL != dispatch->running)
  {
    ls_log(LS_LOG_DEBUG,
           "currently processing events; deferring dispatcher destruction");
    dispatch->destroy_pending = true;
    POP_EVENTING_NDC;
    return;
  }

  ls_log(LS_LOG_DEBUG, "destroying dispatcher");

  moment = dispatch->next_moment;
  while (moment)
  {
    ls_event_moment_t* next_moment = moment->next;
    _moment_destroy(moment);
    moment = next_moment;
  }

  ls_htable_destroy(dispatch->events);
  ls_data_free(dispatch);

  POP_EVENTING_NDC;
}

LS_API ls_event*
ls_event_dispatcher_get_event(ls_event_dispatcher* dispatch,
                              const char*          name)
{
  ls_event* evt;
  int       _ndcDepth;

  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  assert(dispatch);
  PUSH_EVENTING_NDC;
  evt = ls_htable_get(dispatch->events, name);

  POP_EVENTING_NDC;

  return evt;
}

LS_API bool
ls_event_dispatcher_create_event(ls_event_dispatcher* dispatch,
                                 const char*          name,
                                 ls_event**           event,
                                 ls_err*              err)
{
  ls_event_notifier_t* notifier = NULL;
  char*                evt_name = NULL;
  bool                 retval   = true;
  int                  _ndcDepth;
  size_t               nameLen;

  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  assert(dispatch);
  assert(name);
  PUSH_EVENTING_NDC;

  if (name[0] == '\0')
  {
    LS_ERROR(err, LS_ERR_INVALID_ARG);
    retval = false;
    goto ls_event_dispatcher_create_event_done_label;
  }
  if (ls_htable_get(dispatch->events, name) != NULL)
  {
    LS_ERROR(err, LS_ERR_INVALID_STATE);
    retval = false;
    goto ls_event_dispatcher_create_event_done_label;
  }

  nameLen  = ls_strlen(name);
  evt_name = (char*)ls_data_malloc(nameLen + 1);
  if (evt_name == NULL)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    retval = false;
    goto ls_event_dispatcher_create_event_done_label;
  }
  memcpy(evt_name, name, nameLen + 1);

  notifier = ls_data_calloc( 1, sizeof(ls_event_notifier_t) );
  if (notifier == NULL)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    retval = false;
    goto ls_event_dispatcher_create_event_done_label;
  }

  notifier->dispatcher = dispatch;
  notifier->source     = dispatch->source;
  notifier->name       = evt_name;

  if ( !ls_htable_put(dispatch->events,
                      evt_name,
                      notifier,
                      _clean_event,
                      err) )
  {
    retval = false;
    goto ls_event_dispatcher_create_event_done_label;
  }
  evt_name = NULL;

  if (event)
  {
    *event = notifier;
  }
  notifier = NULL;

ls_event_dispatcher_create_event_done_label:
  if (evt_name != NULL)
  {
    ls_data_free( (char*)evt_name );
    evt_name = NULL;
  }

  if (notifier != NULL)
  {
    ls_data_free(notifier);
    notifier = NULL;
  }

  POP_EVENTING_NDC;
  return retval;
}

LS_API const char*
ls_event_get_name(ls_event* event)
{
  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  assert(event);

  return event->name;
}

LS_API const void*
ls_event_get_source(ls_event* event)
{
  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  assert(event);

  return event->source;
}

LS_API bool
ls_event_bind(ls_event*                event,
              ls_event_notify_callback cb,
              void*                    arg,
              ls_err*                  err)
{
  ls_event_dispatcher* dispatch;
  ls_event_binding_t*  binding = NULL;
  ls_event_binding_t*  prev    = NULL;
  int                  _ndcDepth;

  assert(event);
  assert(cb);

  dispatch = event->dispatcher;
  PUSH_EVENTING_NDC;
  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  /* look for existing binding first */
  if ( !_remove_binding(event, cb, &binding, &prev) )
  {
    /* no match found; allocate a new one */
    binding = ls_data_calloc( 1, sizeof(ls_event_binding_t) );
    if (binding == NULL)
    {
      LS_ERROR(err, LS_ERR_NO_MEMORY);
      POP_EVENTING_NDC;
      return false;
    }
  }
  else
  {
    /* Keep previous binding status so that we know whether it is
     * bound before the current event or within the current event
     */
    bool normal_bound = binding->normal_bound;
    memset( binding, 0, sizeof(ls_event_binding_t) );
    binding->normal_bound = normal_bound;
  }

  /* update binding properties */
  binding->cb  = cb;
  binding->arg = arg;

  /* (re-)insert into list */
  if (event->bindings == NULL)
  {
    /* first binding; place on the front */
    event->bindings = binding;
  }
  else if (prev != NULL)
  {
    /* append the binding to the end */
    if (prev->next == NULL)
    {
      prev->next = binding;
    }
    else
    {
      binding->next = prev->next;
      prev->next    = binding;
    }
  }
  else   /* prev == NULL */
  {
    binding->next   = event->bindings;
    event->bindings = binding;
  }

  POP_EVENTING_NDC;
  return true;
}

LS_API void
ls_event_unbind(ls_event*                event,
                ls_event_notify_callback cb)
{
  ls_event_dispatcher* dispatch;
  int                  _ndcDepth;

  assert(event);
  assert(event->dispatcher);
  assert(cb);

  dispatch = event->dispatcher;
  PUSH_EVENTING_NDC;
  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  /* If we're currently running the event we're requesting the unbind from
   * we need to defer the operation until the event has finished its
   * trigger
   */
  if (event != event->dispatcher->running)
  {
    ls_event_binding_t* binding;
    if ( _remove_binding(event, cb, &binding, NULL) )
    {
      /* match found; lists already updated */
      ls_data_free(binding);
      binding = NULL;
    }
  }
  else
  {
    /* we are guaranteed to find the event (even if it is already unbound) */
    ls_event_binding_t* cur = event->bindings;
    while (true)
    {
      if (cur->cb == cb)
      {
        cur->unbound = true;
        break;
      }

      cur = cur->next;
      assert(cur);
    }
  }

  POP_EVENTING_NDC;
}

LS_API bool
ls_event_prepare_trigger(ls_event_dispatcher*    dispatch,
                         ls_event_trigger_data** trigger_data,
                         ls_err*                 err)
{
  bool ret;
  int  _ndcDepth;

  PUSH_EVENTING_NDC;
  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  ret = _prepare_trigger(dispatch, trigger_data, err);

  POP_EVENTING_NDC;
  return ret;
}

LS_API void
ls_event_unprepare_trigger(ls_event_trigger_data* trigger_data)
{
  ls_event*            evt;
  ls_event_dispatcher* dispatch;
  int                  _ndcDepth;

  assert(trigger_data);
  assert(trigger_data->moment);

  evt      = trigger_data->moment->evt.notifier;
  dispatch = evt ? evt->dispatcher : NULL;

  PUSH_EVENTING_NDC;
  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  ls_pool_destroy(trigger_data->pool);

  POP_EVENTING_NDC;
}

LS_API void
ls_event_trigger_prepared(ls_event*                event,
                          void*                    data,
                          ls_event_result_callback result_cb,
                          void*                    result_arg,
                          ls_event_trigger_data*   trigger_data)
{
  ls_event_dispatcher* dispatch;
  int                  _ndcDepth;

  assert(event);

  dispatch = event->dispatcher;

  PUSH_EVENTING_NDC;
  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  _trigger_prepared(dispatch, event, data,
                    result_cb, result_arg, trigger_data);

  POP_EVENTING_NDC;
}

LS_API bool
ls_event_trigger(ls_event*                event,
                 void*                    data,
                 ls_event_result_callback result_cb,
                 void*                    result_arg,
                 ls_err*                  err)
{
  ls_event_dispatcher*   dispatch;
  ls_event_trigger_data* trigger_data;
  int                    _ndcDepth;

  assert(event);
  dispatch = event->dispatcher;

  PUSH_EVENTING_NDC;
  LS_LOG_TRACE_FUNCTION_NO_ARGS;

  if ( !_prepare_trigger(dispatch, &trigger_data, err) )
  {
    POP_EVENTING_NDC;
    return false;
  }

  _trigger_prepared(dispatch, event, data,
                    result_cb, result_arg, trigger_data);

  POP_EVENTING_NDC;

  return true;
}
