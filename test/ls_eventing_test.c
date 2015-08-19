/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_utils.h"
#include "../src/ls_eventing_int.h"
#include "../src/ls_eventing.h"
#include "ls_mem.h"

#include "test_utils.h"

/* fake global source object */
static void*                g_source = "the global source";
static ls_event_dispatcher* g_dispatcher;

/* for auditing */
typedef struct _logitem_t
{
  const char*        message;
  struct _logitem_t* next;
} logitem_t;
typedef struct _log_set_t
{
  unsigned int count;
  logitem_t*   items;
  logitem_t*   itemsend;
  ls_pool*     pool;
} log_t;
typedef void (* fn_ptr_t)();
typedef struct _fn_ptr_wrapper
{
  fn_ptr_t fn;
} fn_ptr_wrapper_t;

static log_t g_audit;

static const char*
log_event_message(const char* cb,
                  ls_event*   notifier,
                  void*       evtdata,
                  void*       evtarg)
{
  char        text[1024];
  const char* out;

  sprintf(text, "%s:%s (notifier=0x%p; source=0x%p; data=0x%p; arg=0x%p)",
          cb,
          ls_event_get_name(notifier),
          (void*)notifier,
          ls_event_get_source(notifier),
          evtdata,
          evtarg);

  if ( !ls_pool_strdup(g_audit.pool, text, (char**)&out, NULL) )
  {
    return NULL;
  }

  return out;
}
static const char*
log_result_message(const char* cb,
                   ls_event*   notifier,
                   void*       evtdata,
                   bool        result,
                   void*       rstarg)
{
  char        text[1024];
  const char* out;

  sprintf(text, "%s:%s == %s (notifier=0x%p; source=0x%p; data=0x%p; arg=0x%p)",
          cb,
          ls_event_get_name(notifier),
          (result ? "true" : "false"),
          (void*)notifier,
          ls_event_get_source(notifier),
          evtdata,
          rstarg);

  if ( !ls_pool_strdup(g_audit.pool, text, (char**)&out, NULL) )
  {
    return NULL;
  }

  return out;
}
static void
loggit(const char* msg)
{
  union
  {
    logitem_t* item;
    void*      itemPtr;
  } itemUnion;

  if ( !ls_pool_malloc(g_audit.pool, sizeof(logitem_t), &itemUnion.itemPtr,
                       NULL) )
  {
    return;
  }

  logitem_t* item = itemUnion.item;
  item->message = msg;
  item->next    = NULL;

  if (g_audit.items == NULL)
  {
    g_audit.items = item;
  }
  else
  {
    g_audit.itemsend->next = item;
  }
  g_audit.itemsend = item;
  g_audit.count++;
}

/**
 * Event callback updates log with actual data
 */
static void
mock_evt1_callback1(ls_event_data* evt,
                    void*          arg)
{
  evt->handled = true;

  const char* msg = log_event_message("mock_evt1_callback1",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  loggit(msg);
}

/**
 * Event callback that unbinds mock_evt1_callback1 during the event to defer
 * it.  This is used in conjunction with mock_evt_rebind1_callback1 which will
 * re-add mock_evt1_callback1 before its deferred removal.
 */
static void
mock_evt_unbind1_callback1(ls_event_data* evt,
                           void*          arg)
{
  const char* msg = log_event_message("mock_evt_unbind1_callback1",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  loggit(msg);

  ls_event_unbind(evt->notifier, mock_evt1_callback1);
}
static void
mock_evt_rebind1_callback1(ls_event_data* evt,
                           void*          arg)
{
  const char* msg = log_event_message("mock_evt_rebind1_callback1",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  loggit(msg);

  ls_event_bind(evt->notifier, mock_evt1_callback1, NULL, NULL);
}

/**
 * Event callback that unbinds event while being triggered
 *
 * Next for functions are essentially the same, just different names
 * so they can bind to the same event
 */
static void
mock_evt_unbind_callback1(ls_event_data* evt,
                          void*          arg)
{
  const char* msg = log_event_message("mock_evt_unbind_callback1",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  loggit(msg);

  ls_event_unbind(evt->notifier, mock_evt_unbind_callback1);
}
static void
mock_evt_unbind_callback2(ls_event_data* evt,
                          void*          arg)
{
  const char* msg = log_event_message("mock_evt_unbind_callback2",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  loggit(msg);

  /* try unbinding twice (second is a noop) */
  ls_event_unbind(evt->notifier, mock_evt_unbind_callback2);
  ls_event_unbind(evt->notifier, mock_evt_unbind_callback2);
}
static void
mock_evt_unbind_callback3(ls_event_data* evt,
                          void*          arg)
{
  const char* msg = log_event_message("mock_evt_unbind_callback3",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  loggit(msg);

  ls_event_unbind(evt->notifier, mock_evt_unbind_callback3);
}
static void
mock_evt_unbind_callback4(ls_event_data* evt,
                          void*          arg)
{
  const char* msg = log_event_message("mock_evt_unbind_callback4",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  loggit(msg);

  ls_event_unbind(evt->notifier, mock_evt_unbind_callback4);
}

/**
 * Event callback updates log with actual data, and marks the event handled
 */
static void
mock_evt1_callback_handled1(ls_event_data* evt,
                            void*          arg)
{
  const char* msg = log_event_message("mock_evt1_callback_handled1",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  loggit(msg);
  evt->handled = true;
}

/**
 * Event callback updates log with actual data
 */
static void
mock_evt1_callback2(ls_event_data* evt,
                    void*          arg)
{
  const char* msg = log_event_message("mock_evt1_callback2",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  loggit(msg);
}

/**
 * Result callback updates log with actual data
 */
static void
mock_evt1_result1(ls_event_data* evt,
                  bool           result,
                  void*          arg)
{
  const char* msg = log_result_message("mock_evt1_result1",
                                       evt->notifier,
                                       evt->data,
                                       result,
                                       arg);
  loggit(msg);
}


/**
 * nesting_callbackA triggers event "arg" using result callback "evt->data"
 */
static void
nesting_callbackA(ls_event_data* evt,
                  void*          arg)
{
  fn_ptr_wrapper_t*        wrapper  = (fn_ptr_wrapper_t*)evt->data;
  ls_event_result_callback callback = NULL;
  if (NULL != wrapper)
  {
    callback = (ls_event_result_callback)wrapper->fn;
  }
  /* trigger first to check recursion */
  ls_event_trigger( (ls_event*)arg, NULL, callback, NULL, NULL );
  loggit( log_event_message("nesting_callbackA",
                            evt->notifier,
                            evt->data,
                            arg) );
}
/* same as above, but fires two arg events instead of one */
static void
double_nesting_callback(ls_event_data* evt,
                        void*          arg)
{
  fn_ptr_wrapper_t*        wrapper  = (fn_ptr_wrapper_t*)evt->data;
  ls_event_result_callback callback = NULL;
  if (NULL != wrapper)
  {
    callback = (ls_event_result_callback)wrapper->fn;
  }
  /* trigger first to check recursion */
  ls_event_trigger( (ls_event*)arg, NULL, callback, NULL, NULL );
  ls_event_trigger( (ls_event*)arg, NULL, callback, NULL, NULL );
  loggit( log_event_message("double_nesting_callback",
                            evt->notifier,
                            evt->data,
                            arg) );
}
static void
nesting_callbackB(ls_event_data* evt,
                  void*          arg)
{
  loggit( log_event_message("nesting_callbackB",
                            evt->notifier,
                            evt->data,
                            arg) );
}
/* sets handled to true */
static void
nesting_callbackC(ls_event_data* evt,
                  void*          arg)
{
  loggit( log_event_message("nesting_callbackC",
                            evt->notifier,
                            evt->data,
                            arg) );
  evt->handled = true;
}
static void
nesting_resultA(ls_event_data* evt,
                bool           result,
                void*          arg)
{
  loggit( log_result_message("nesting_resultA",
                             evt->notifier,
                             evt->data,
                             result,
                             arg) );
}
static void
nesting_resultB(ls_event_data* evt,
                bool           result,
                void*          arg)
{
  loggit( log_result_message("nesting_resultB",
                             evt->notifier,
                             evt->data,
                             result,
                             arg) );
}


/*
 *  callbackA and callbackC are passed the event to be fired
 *  as a bound argument.
 */
static void
evt1_callbackA(ls_event_data* evt,
               void*          arg)
{
  const char* msg = log_event_message("evt1_callbackA",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  /* arg is evt2*/
  loggit(msg);
  ls_event_trigger( (ls_event*)arg, NULL, NULL, NULL, NULL );
}
static void
evt3_callbackB(ls_event_data* evt,
               void*          arg)
{
  const char* msg = log_event_message("evt3_callbackB",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  loggit(msg);
}
static void
evt2_callbackC(ls_event_data* evt,
               void*          arg)
{
  const char* msg = log_event_message("evt2_callbackC",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  /* arg is evt3 */
  loggit(msg);
  ls_event_trigger( (ls_event*)arg, NULL, NULL, NULL, NULL );
}
static void
evt2_callbackD(ls_event_data* evt,
               void*          arg)
{
  const char* msg = log_event_message("evt2_callbackD",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  loggit(msg);
}

/**
 *
 * Event callback that binds event while being triggered
 *
 */
static void
mock_evt_bind1_callback1(ls_event_data* evt,
                         void*          arg)
{
  const char* msg = log_event_message("mock_evt_bind1_callback1",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  loggit(msg);

  ls_event_bind(evt->notifier, mock_evt1_callback1, NULL, NULL);
}
static void
mock_evt_bind1_callback2(ls_event_data* evt,
                         void*          arg)
{
  const char* msg = log_event_message("mock_evt_bind1_callback2",
                                      evt->notifier,
                                      evt->data,
                                      arg);
  loggit(msg);

  ls_event_bind(evt->notifier, mock_evt1_callback2, NULL, NULL);
}

static int _mallocCnt = 0;
static void*
_counting_malloc(size_t size)
{
  ++_mallocCnt;
  return malloc(size);
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
  ++_freeCnt;
  free(ptr);
}

static bool g_oom_malloc_called = false;
static void*
mock_oom_malloc(size_t size)
{
  UNUSED_PARAM(size);
  g_oom_malloc_called = true;
  return NULL;
}
static void*
mock_oom_realloc(void*  ptr,
                 size_t size)
{
  UNUSED_PARAM(ptr);
  UNUSED_PARAM(size);
  g_oom_malloc_called = true;
  return NULL;
}
static bool g_nofail_callback_called = false;
static void
mock_nofail_callback(ls_event_data* evt,
                     void*          arg)
{
  UNUSED_PARAM(evt);
  UNUSED_PARAM(arg);
  g_nofail_callback_called = true;
}

void* g_destroy_first_alloc = NULL;
static void*
destroy_test_malloc(size_t size)
{
  void* ret = malloc(size);
  if (!g_destroy_first_alloc)
  {
    g_destroy_first_alloc = ret;
  }
  return ret;
}
static bool g_destroy_correctly_deferred = false;
static bool g_destroy_first_free         = false;
static void
destroy_test_free(void* ptr)
{
  if ( ptr && (ptr == g_destroy_first_alloc) )
  {
    /* record if the remembered pointer was freed */
    g_destroy_first_free = true;
  }
  free(ptr);
}
static void
destroying_callback(ls_event_data* evt,
                    void*          arg)
{
  ls_event*            next_evt   = evt->data;
  ls_event_dispatcher* dispatcher = arg;

  if (next_evt)
  {
    if ( !ls_event_trigger(next_evt, NULL, NULL, NULL, NULL) )
    {
      ls_log(LS_LOG_DEBUG, "event trigger failed in destroying_callback");
    }
  }

  g_destroy_correctly_deferred = true;
  ls_event_dispatcher_destroy(dispatcher);
  if (g_destroy_first_free)
  {
    ls_log(LS_LOG_ERROR, "dispatcher destruction not correctly deferred"
                         " in destroying_callback");
    g_destroy_correctly_deferred = false;
  }
}

static void
async_callback(ls_event_data* evt,
               void*          arg)
{
  UNUSED_PARAM(arg);

  uint32_t* call_count = evt->data;
  ++*call_count;
}

CTEST_DATA(event) {
  void* placeholder;
};

CTEST_SETUP(event)
{
  ls_event* mock1, * mock2;
  UNUSED_PARAM(data);

  memset( &g_audit, 0, sizeof(log_t) );
  ls_pool_create(0, &g_audit.pool, NULL);

  ASSERT_TRUE( ls_event_dispatcher_create(g_source, &g_dispatcher, NULL) );
  ls_event_dispatcher_create_event(g_dispatcher,
                                   "mockEvent1",
                                   &mock1,
                                   NULL);
  ls_event_dispatcher_create_event(g_dispatcher,
                                   "mockEvent2",
                                   &mock2,
                                   NULL);
}

CTEST_TEARDOWN(event)
{
  ls_event_dispatcher_destroy(g_dispatcher);
  g_dispatcher = NULL;
  UNUSED_PARAM(data);

  /* reset audit trail */
  ls_pool_destroy(g_audit.pool);
  memset( &g_audit, 0, sizeof(log_t) );
}

CTEST2(event, dispatcher_create_destroy)
{
  UNUSED_PARAM(data);
  ls_event_dispatcher* dispatch;
  ls_err               err;
  void*                source = "the source";

  ASSERT_TRUE( ls_event_dispatcher_create(source, &dispatch, &err) );
  ASSERT_TRUE( dispatch->source == source);
  ASSERT_NOT_NULL(dispatch->events);
  ASSERT_NULL(dispatch->running);
  ASSERT_NULL(dispatch->moment_queue_tail);

  ls_event_dispatcher_destroy(dispatch);
}

CTEST2(event, create)
{
  UNUSED_PARAM(data);
  ls_event* evt1, * evt2, * evt3;
  ls_err    err;

  ASSERT_NULL( ls_event_dispatcher_get_event(g_dispatcher, "EventOne") );
  ASSERT_NULL( ls_event_dispatcher_get_event(g_dispatcher, "eventOne") );
  ASSERT_NULL( ls_event_dispatcher_get_event(g_dispatcher, "eventone") );
  ASSERT_NULL( ls_event_dispatcher_get_event(g_dispatcher, "EVENTONE") );
  ASSERT_NULL( ls_event_dispatcher_get_event(g_dispatcher, "SecondEvent") );
  ASSERT_NULL( ls_event_dispatcher_get_event(g_dispatcher, "secondEvent") );
  ASSERT_NULL( ls_event_dispatcher_get_event(g_dispatcher, "secondevent") );
  ASSERT_NULL( ls_event_dispatcher_get_event(g_dispatcher, "SECONDEVENT") );

  ASSERT_TRUE(ls_event_dispatcher_create_event(g_dispatcher,
                                               "eventOne",
                                               &evt1,
                                               &err) == true);
  ASSERT_TRUE(evt1->dispatcher == g_dispatcher);
  ASSERT_NULL(evt1->bindings);
  ASSERT_STR(ls_event_get_name(evt1), "eventOne");
  ASSERT_TRUE(ls_event_get_source(evt1) == g_source);
  ASSERT_TRUE(ls_event_dispatcher_get_event(g_dispatcher, "EventOne") == evt1);
  ASSERT_TRUE(ls_event_dispatcher_get_event(g_dispatcher, "eventOne") == evt1);
  ASSERT_TRUE(ls_event_dispatcher_get_event(g_dispatcher, "eventone") == evt1);
  ASSERT_TRUE(ls_event_dispatcher_get_event(g_dispatcher, "EVENTONE") == evt1);
  ASSERT_NULL( ls_event_dispatcher_get_event(g_dispatcher, "SecondEvent") );
  ASSERT_NULL( ls_event_dispatcher_get_event(g_dispatcher, "secondEvent") );
  ASSERT_NULL( ls_event_dispatcher_get_event(g_dispatcher, "secondevent") );
  ASSERT_NULL( ls_event_dispatcher_get_event(g_dispatcher, "SECONDEVENT") );

  ASSERT_TRUE(ls_event_dispatcher_create_event(g_dispatcher,
                                               "secondEvent",
                                               &evt2,
                                               &err) == true);
  ASSERT_TRUE(evt2->dispatcher == g_dispatcher);
  ASSERT_NULL(evt2->bindings);
  ASSERT_STR(ls_event_get_name(evt2), "secondEvent");
  ASSERT_TRUE( ls_event_get_source(evt2) == g_source);
  ASSERT_TRUE( ls_event_dispatcher_get_event(g_dispatcher, "EventOne") == evt1);
  ASSERT_TRUE( ls_event_dispatcher_get_event(g_dispatcher, "eventOne") == evt1);
  ASSERT_TRUE( ls_event_dispatcher_get_event(g_dispatcher, "eventone") == evt1);
  ASSERT_TRUE( ls_event_dispatcher_get_event(g_dispatcher, "EVENTONE") == evt1);
  ASSERT_TRUE( ls_event_dispatcher_get_event(g_dispatcher,
                                             "SecondEvent") == evt2);
  ASSERT_TRUE( ls_event_dispatcher_get_event(g_dispatcher,
                                             "secondEvent") == evt2);
  ASSERT_TRUE( ls_event_dispatcher_get_event(g_dispatcher,
                                             "secondevent") == evt2);
  ASSERT_TRUE( ls_event_dispatcher_get_event(g_dispatcher,
                                             "SECONDEVENT") == evt2);
  ASSERT_TRUE( evt1 != evt2);

  /* create an event but only retrieve the pointer indirectly */
  ASSERT_TRUE( ls_event_dispatcher_create_event(g_dispatcher, "eventTheThird",
                                                NULL, NULL) );
  evt3 = ls_event_dispatcher_get_event(g_dispatcher, "eventTheThird");
  ASSERT_NOT_NULL(evt3);
  ASSERT_TRUE(evt1 != evt3);
  ASSERT_TRUE(evt2 != evt3);
}

CTEST2(event, bindings)
{
  UNUSED_PARAM(data);
  ls_event*           evt1;
  ls_err              err;
  ls_event_binding_t* b;
  void*               arg1, * arg2;

  evt1 = ls_event_dispatcher_get_event(g_dispatcher, "mockEvent1");
  ASSERT_NULL(evt1->bindings);

  /* ensure unbinding when nothing is bound doesn't segfault */
  ls_event_unbind(evt1, mock_evt1_callback1);

  ASSERT_TRUE( ls_event_bind(evt1, mock_evt1_callback1, NULL, &err) );
  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback1);
  ASSERT_NULL(b->arg);
  ASSERT_NULL(b->next);

  ls_event_unbind(evt1, mock_evt1_callback1);
  ASSERT_NULL(evt1->bindings);

  arg1 = "first bound argument";
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt1_callback1, arg1, &err) );
  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback1);
  ASSERT_TRUE(b->arg == arg1);
  ASSERT_NULL(b->next);

  ASSERT_TRUE( ls_event_bind(evt1, mock_evt1_callback2, NULL, &err) );
  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback1);
  ASSERT_TRUE(b->arg == arg1);
  ASSERT_NOT_NULL(b->next);
  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback2);
  ASSERT_NULL(b->arg);
  ASSERT_NULL(b->next);

  ls_event_unbind(evt1, mock_evt1_callback2);
  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback1);
  ASSERT_TRUE(b->arg == arg1);
  ASSERT_NULL(b->next);

  arg2 = "second bound argument";
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt1_callback2, arg2, &err) );
  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback1);
  ASSERT_TRUE(b->arg == arg1);
  ASSERT_NOT_NULL(b->next);
  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback2);
  ASSERT_TRUE(b->arg == arg2);
  ASSERT_NULL(b->next);

  /* reregister; should not change position */
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt1_callback1, NULL, &err) );
  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback1);
  ASSERT_NULL(b->arg);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback2);
  ASSERT_TRUE(b->arg == arg2);
  ASSERT_NULL(b->next);

  ls_event_unbind(evt1, mock_evt1_callback1);
  ls_event_unbind(evt1, mock_evt1_callback2);
}

CTEST2(event, trigger_simple)
{
  UNUSED_PARAM(data);
  ls_event*  evt1;
  ls_err     err;
  logitem_t* item;

  evt1 = ls_event_dispatcher_get_event(g_dispatcher, "mockEvent1");

  ASSERT_TRUE( ls_event_bind(evt1, mock_evt1_callback1, NULL, &err) );
  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );

  ASSERT_EQUAL(g_audit.count, 1);
  item = g_audit.items;
  ASSERT_STR( item->message, log_event_message("mock_evt1_callback1",
                                               evt1,
                                               NULL,
                                               NULL) );

  ls_event_unbind(evt1, mock_evt1_callback1);
}

CTEST2(event, trigger_simple_results)
{
  UNUSED_PARAM(data);
  ls_event*  evt1;
  ls_err     err;
  logitem_t* item;

  evt1 = ls_event_dispatcher_get_event(g_dispatcher, "mockEvent1");

  ASSERT_TRUE( ls_event_bind(evt1, mock_evt1_callback_handled1, NULL, &err) );
  ASSERT_TRUE( ls_event_trigger(evt1, NULL, mock_evt1_result1, NULL, &err) );

  ASSERT_EQUAL(g_audit.count, 2);
  item = g_audit.items;
  ASSERT_STR( item->message, log_event_message("mock_evt1_callback_handled1",
                                               evt1,
                                               NULL,
                                               NULL) );
  item = item->next;
  ASSERT_STR( item->message, log_result_message("mock_evt1_result1",
                                                evt1,
                                                NULL,
                                                true,
                                                NULL) );

  ls_event_unbind(evt1, mock_evt1_callback_handled1);
}

CTEST2(event, create_errors)
{
  UNUSED_PARAM(data);
  ls_event*            evt1, * evt2;
  ls_err               err;
  ls_event_dispatcher* dispatch;
  void*                source = "The source";

  ASSERT_TRUE( ls_event_dispatcher_create(source, &dispatch, &err) );
  ASSERT_TRUE( !ls_event_dispatcher_create_event(dispatch, "", &evt1, NULL) );
  ASSERT_FALSE( ls_event_dispatcher_create_event(dispatch,
                                                 "",
                                                 &evt1,
                                                 &err) );
  ASSERT_EQUAL(err.code, LS_ERR_INVALID_ARG);

  ASSERT_TRUE( ls_event_dispatcher_create_event(dispatch,
                                                "eventOne",
                                                &evt1,
                                                &err) );
  evt1 = ls_event_dispatcher_get_event(g_dispatcher, "eventOne");

  ASSERT_FALSE( ls_event_dispatcher_create_event(dispatch,
                                                 "eventOne",
                                                 &evt2,
                                                 NULL) );
  ASSERT_FALSE( ls_event_dispatcher_create_event(dispatch,
                                                 "eventOne",
                                                 &evt2,
                                                 &err) );
  ASSERT_EQUAL(err.code, LS_ERR_INVALID_STATE);

  ls_event_dispatcher_destroy(dispatch);
}

CTEST2(event, trigger_nested)
{
  UNUSED_PARAM(data);
  ls_event*  evt1, * evt2;
  ls_err     err;
  logitem_t* item;

  ASSERT_NOT_NULL( ( evt1 =
                       ls_event_dispatcher_get_event(g_dispatcher,
                                                     "mockEvent1") ) );
  ASSERT_NOT_NULL( ( evt2 =
                       ls_event_dispatcher_get_event(g_dispatcher,
                                                     "mockEvent2") ) );
  /* bind evt2 to evt1 callbackA so it will  be triggered */
  ASSERT_TRUE( ls_event_bind(evt1, nesting_callbackA, evt2, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, nesting_callbackB, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt2, nesting_callbackB, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt2, nesting_callbackC, NULL, &err) ); /*handled =
                                                                      * true */
  /* evt1 callbackA will trigger evt2 with resultB as the result cb.*/
  union
  {
    fn_ptr_wrapper_t* resultBwrapper;
    void*             resultBwrapperPtr;
  } resultBwrapperUnion;
  ASSERT_TRUE( ls_pool_malloc(g_audit.pool, sizeof(fn_ptr_wrapper_t),
                              &resultBwrapperUnion.resultBwrapperPtr, NULL) );
  fn_ptr_wrapper_t* resultBwrapper = resultBwrapperUnion.resultBwrapper;
  resultBwrapper->fn = (fn_ptr_t)nesting_resultB;
  ASSERT_TRUE( ls_event_trigger(evt1, resultBwrapper, nesting_resultA, NULL,
                                &err) );

  ASSERT_EQUAL(g_audit.count, 6);
  item = g_audit.items;
  /* callbackA logs *after* it triggers evt2. If breath-first is working
   *  all of evt1 should finish before any of evt2.
   *  Note that callbackC sets handled to true, and therefore resultB
   *  will be true*/
  ASSERT_STR( item->message, log_event_message("nesting_callbackA",
                                               evt1,
                                               resultBwrapper,
                                               evt2) );
  item = item->next;
  ASSERT_STR( item->message, log_event_message("nesting_callbackB",
                                               evt1,
                                               resultBwrapper,
                                               NULL) );
  item = item->next;
  ASSERT_STR( item->message, log_result_message("nesting_resultA",
                                                evt1,
                                                resultBwrapper,
                                                false,
                                                NULL) );
  item = item->next;
  ASSERT_STR( item->message, log_event_message("nesting_callbackB",
                                               evt2,
                                               NULL,
                                               NULL) );
  item = item->next;
  ASSERT_STR( item->message, log_event_message("nesting_callbackC",
                                               evt2,
                                               NULL,
                                               NULL) );
  item = item->next;
  ASSERT_STR( item->message, log_result_message("nesting_resultB",
                                                evt2,
                                                NULL,
                                                true,
                                                NULL) );

  ls_event_unbind(evt1, nesting_callbackA);
  ls_event_unbind(evt1, nesting_callbackB);
  ls_event_unbind(evt2, nesting_callbackB);
  ls_event_unbind(evt2, nesting_callbackC);
}

CTEST2(event, trigger_double_nested)
{
  UNUSED_PARAM(data);
  ls_event*  evt1, * evt2;
  ls_err     err;
  logitem_t* item;

  ASSERT_NOT_NULL( ( evt1 =
                       ls_event_dispatcher_get_event(g_dispatcher,
                                                     "mockEvent1") ) );
  ASSERT_NOT_NULL( ( evt2 =
                       ls_event_dispatcher_get_event(g_dispatcher,
                                                     "mockEvent2") ) );
  /* trigger two evt2 events from evt1 */
  ASSERT_TRUE( ls_event_bind(evt1, double_nesting_callback, evt2, &err) );
  ASSERT_TRUE( ls_event_bind(evt2, nesting_callbackB, NULL, &err) );

  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );

  ASSERT_EQUAL(g_audit.count, 3);
  item = g_audit.items;
  ASSERT_STR( item->message, log_event_message("double_nesting_callback",
                                               evt1,
                                               NULL,
                                               evt2) );
  item = item->next;
  ASSERT_STR( item->message, log_event_message("nesting_callbackB",
                                               evt2,
                                               NULL,
                                               NULL) );
  item = item->next;
  ASSERT_STR( item->message, log_event_message("nesting_callbackB",
                                               evt2,
                                               NULL,
                                               NULL) );

  ls_event_unbind(evt1, double_nesting_callback);
  ls_event_unbind(evt2, nesting_callbackB);
}

CTEST2(event, trigger_multi_source)
{
  ls_event*            evt1, * evt2, * evt3;
  ls_err               err;
  logitem_t*           item;
  ls_event_dispatcher* dispatcher1, * dispatcher2;
  void*                source1 = "the first source";
  void*                source2 = "the second source";

  UNUSED_PARAM(data);
  ASSERT_TRUE( ls_event_dispatcher_create(source1, &dispatcher1, &err) );
  ASSERT_TRUE( ls_event_dispatcher_create(source2, &dispatcher2, &err) );
  ls_event_dispatcher_create_event(dispatcher1,
                                   "Event1",
                                   &evt1,
                                   NULL);
  ls_event_dispatcher_create_event(dispatcher2,
                                   "Event2",
                                   &evt2,
                                   NULL);
  ls_event_dispatcher_create_event(dispatcher1,
                                   "Event3",
                                   &evt3,
                                   NULL);
  /* callbackA will fire evt2:callbackC which will fire evt3:callbackB,
   *  pass events along to these callbacks as bound arguments, simplifies
   *  trigger logic*/
  ASSERT_TRUE( ls_event_bind(evt1, evt1_callbackA, (void*)evt2, &err) );
  ASSERT_TRUE( ls_event_bind(evt3, evt3_callbackB, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt2, evt2_callbackC, (void*)evt3, &err) );
  ASSERT_TRUE( ls_event_bind(evt2, evt2_callbackD, NULL, &err) );

  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL,NULL, &err) );

  ASSERT_EQUAL(g_audit.count, 4);
  item = g_audit.items;
  /* The callbackA for event1 should fire first */
  ASSERT_STR( item->message, log_event_message("evt1_callbackA",
                                               evt1,
                                               NULL,
                                               evt2) );
  item = item->next;
  /* Both the callbackC and callbackD for event2 should fire next
   *   because of breadth-first approach*/
  ASSERT_STR( item->message, log_event_message("evt2_callbackC",
                                               evt2,
                                               NULL,
                                               evt3) );
  item = item->next;
  ASSERT_STR( item->message, log_event_message("evt2_callbackD",
                                               evt2,
                                               NULL,
                                               NULL) );
  item = item->next;
  /* Finally, the callbackD for event1 fired last */
  ASSERT_STR( item->message, log_event_message("evt3_callbackB",
                                               evt3,
                                               NULL,
                                               NULL) );

  /* ls_event_dispatcher_destroy unbinds callbacks (if any) when
   *  destroying events */
  ls_event_dispatcher_destroy(dispatcher1);
  ls_event_dispatcher_destroy(dispatcher2);
}

/* Concurrent unbind tests
 * Various forms of unbinding from an event during the events trigger
 * execution
 */
CTEST2(event, trigger_event_unbind)
{
  ls_event*           evt1;
  ls_err              err;
  ls_event_binding_t* b;
  logitem_t*          item;

  UNUSED_PARAM(data);
  evt1 = ls_event_dispatcher_get_event(g_dispatcher, "mockEvent1");

  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_unbind_callback1, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt1_callback1, NULL, &err) );

  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );

  ASSERT_EQUAL(g_audit.count, 2);
  item = g_audit.items;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_unbind_callback1", evt1,
                                NULL, NULL) );
  ASSERT_NOT_NULL(item->next);
  ASSERT_STR( item->next->message,
              log_event_message("mock_evt1_callback1", evt1,
                                NULL, NULL) );

  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback1);
  ASSERT_NULL(b->next);

  ls_event_unbind(evt1, mock_evt1_callback1);
  b = evt1->bindings;
  ASSERT_NULL(b);

}

CTEST2(event, trigger_event_multiple_unbind)
{
  ls_event*           evt1;
  ls_err              err;
  ls_event_binding_t* b;
  logitem_t*          item;

  UNUSED_PARAM(data);
  evt1 = ls_event_dispatcher_get_event(g_dispatcher, "mockEvent1");

  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_unbind_callback1, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_unbind_callback2, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_unbind_callback3, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_unbind_callback4, NULL, &err) );

  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );

  ASSERT_EQUAL(g_audit.count, 4);
  item = g_audit.items;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_unbind_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_NOT_NULL(item);
  ASSERT_STR( item->message,
              log_event_message("mock_evt_unbind_callback2", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_NOT_NULL(item);
  ASSERT_STR( item->message,
              log_event_message("mock_evt_unbind_callback3", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_NOT_NULL(item);
  ASSERT_STR( item->message,
              log_event_message("mock_evt_unbind_callback4", evt1,
                                NULL, NULL) );

  ASSERT_NULL(item->next);

  b = evt1->bindings;
  ASSERT_NULL(b);

}

CTEST2(event, trigger_nested_unbind)
{
  ls_event*           evt1, * evt2;
  ls_err              err;
  logitem_t*          item;
  ls_event_binding_t* b;

  UNUSED_PARAM(data);
  ASSERT_NOT_NULL( ( evt1 =
                       ls_event_dispatcher_get_event(g_dispatcher,
                                                     "mockEvent1") ) );
  ASSERT_NOT_NULL( ( evt2 =
                       ls_event_dispatcher_get_event(g_dispatcher,
                                                     "mockEvent2") ) );
  /* bind evt2 to evt1 callbackA so it will  be triggered */
  ASSERT_TRUE( ls_event_bind(evt1, nesting_callbackA, evt2, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, nesting_callbackB, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt2, mock_evt_unbind_callback1, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt2, nesting_callbackC, NULL, &err) ); /*handled =
                                                                      * true */
  /* evt1 callbackA will trigger evt2 with resultB as the result cb.*/
  union
  {
    fn_ptr_wrapper_t* resultBwrapper;
    void*             resultBwrapperPtr;
  } resultBwrapperUnion;
  ASSERT_TRUE( ls_pool_malloc(g_audit.pool, sizeof(fn_ptr_wrapper_t),
                              &resultBwrapperUnion.resultBwrapperPtr, NULL) );
  fn_ptr_wrapper_t* resultBwrapper = resultBwrapperUnion.resultBwrapper;
  resultBwrapper->fn = (fn_ptr_t)nesting_resultB;
  ASSERT_TRUE( ls_event_trigger(evt1, resultBwrapper, nesting_resultA, NULL,
                                &err) );

  ASSERT_EQUAL(g_audit.count, 6);
  item = g_audit.items;
  /* callbackA logs *after* it triggers evt2. If breath-first is working
   *  all of evt1 should finish before any of evt2.
   *  Note that callbackC sets handled to true, and therefore resultB
   *  will be true*/
  ASSERT_STR( item->message, log_event_message("nesting_callbackA",
                                               evt1,
                                               resultBwrapper,
                                               evt2) );
  item = item->next;
  ASSERT_STR( item->message, log_event_message("nesting_callbackB",
                                               evt1,
                                               resultBwrapper,
                                               NULL) );
  item = item->next;
  ASSERT_STR( item->message, log_result_message("nesting_resultA",
                                                evt1,
                                                resultBwrapper,
                                                false,
                                                NULL) );
  item = item->next;
  ASSERT_STR( item->message, log_event_message("mock_evt_unbind_callback1",
                                               evt2,
                                               NULL,
                                               NULL) );
  item = item->next;
  ASSERT_STR( item->message, log_event_message("nesting_callbackC",
                                               evt2,
                                               NULL,
                                               NULL) );
  item = item->next;
  ASSERT_STR( item->message, log_result_message("nesting_resultB",
                                                evt2,
                                                NULL,
                                                true,
                                                NULL) );

  ls_event_unbind(evt1, nesting_callbackA);
  ls_event_unbind(evt1, nesting_callbackB);

  b = evt2->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == nesting_callbackC);
  ASSERT_NULL(b->next);

  ls_event_unbind(evt2, nesting_callbackC);
  b = evt2->bindings;
  ASSERT_NULL(b);

}

CTEST2(event, trigger_event_unbind_middle)
{
  ls_event*           evt1;
  ls_err              err;
  ls_event_binding_t* b;
  logitem_t*          item;

  UNUSED_PARAM(data);
  evt1 = ls_event_dispatcher_get_event(g_dispatcher, "mockEvent1");

  ASSERT_TRUE( ls_event_bind(evt1, mock_evt1_callback1, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_unbind_callback1, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt1_callback2, NULL, &err) );

  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );

  ASSERT_EQUAL(g_audit.count, 3);
  item = g_audit.items;
  ASSERT_STR( item->message,
              log_event_message("mock_evt1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_NOT_NULL(item);
  ASSERT_STR( item->message,
              log_event_message("mock_evt_unbind_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_NOT_NULL(item);
  ASSERT_STR( item->message,
              log_event_message("mock_evt1_callback2", evt1,
                                NULL, NULL) );

  ASSERT_NULL(item->next);

  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback1);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback2);
  ASSERT_NULL(b->next);

  ls_event_unbind(evt1, mock_evt1_callback1);
  ls_event_unbind(evt1, mock_evt1_callback2);

  b = evt1->bindings;
  ASSERT_NULL(b);

}

CTEST2(event, trigger_event_unbind_rebind)
{
  ls_event*           evt1;
  ls_err              err;
  ls_event_binding_t* b;
  logitem_t*          item;

  UNUSED_PARAM(data);
  evt1 = ls_event_dispatcher_get_event(g_dispatcher, "mockEvent1");

  ASSERT_TRUE( ls_event_bind(evt1, mock_evt1_callback1, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_unbind1_callback1, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_rebind1_callback1, NULL, &err) );

  /* rebind middle binding -- should not change its order */
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_unbind1_callback1, NULL, &err) );

  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );

  ASSERT_EQUAL(g_audit.count, 3);
  item = g_audit.items;
  ASSERT_STR( item->message,
              log_event_message("mock_evt1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_NOT_NULL(item);
  ASSERT_STR( item->message,
              log_event_message("mock_evt_unbind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_NOT_NULL(item);
  ASSERT_STR( item->message,
              log_event_message("mock_evt_rebind1_callback1", evt1,
                                NULL, NULL) );

  ASSERT_NULL(item->next);

  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback1);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt_unbind1_callback1);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt_rebind1_callback1);
  ASSERT_NULL(b->next);


  ls_event_unbind(evt1, mock_evt_unbind1_callback1);
  ls_event_unbind(evt1, mock_evt_rebind1_callback1);
  ls_event_unbind(evt1, mock_evt1_callback1);

  b = evt1->bindings;
  ASSERT_NULL(b);

}

CTEST2(event, trigger_event_simple_defer_bind)
{
  ls_event*           evt1;
  ls_err              err;
  ls_event_binding_t* b;
  logitem_t*          item;

  UNUSED_PARAM(data);
  evt1 = ls_event_dispatcher_get_event(g_dispatcher, "mockEvent1");

  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_bind1_callback1, NULL, &err) );

  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );

  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt_bind1_callback1);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback1);
  ASSERT_NULL(b->next);

  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );

  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt_bind1_callback1);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback1);
  ASSERT_NULL(b->next);

  ASSERT_EQUAL(g_audit.count, 3);
  item = g_audit.items;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_bind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_bind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt1_callback1", evt1,
                                NULL, NULL) );
  ASSERT_NULL(item->next);
}

CTEST2(event, trigger_event_multiple_defer_bind)
{
  ls_event*           evt1;
  ls_err              err;
  ls_event_binding_t* b;
  logitem_t*          item;

  UNUSED_PARAM(data);
  evt1 = ls_event_dispatcher_get_event(g_dispatcher, "mockEvent1");

  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_bind1_callback1, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_bind1_callback2, NULL, &err) );

  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );
  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );

  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt_bind1_callback1);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt_bind1_callback2);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback1);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback2);
  ASSERT_NULL(b->next);

  ASSERT_EQUAL(g_audit.count, 6);
  item = g_audit.items;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_bind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_bind1_callback2", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_bind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_bind1_callback2", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt1_callback2", evt1,
                                NULL, NULL) );
  ASSERT_NULL(item->next);
}

CTEST2(event, trigger_event_defer_bind_rebind)
{
  ls_event*           evt1;
  ls_err              err;
  ls_event_binding_t* b;
  logitem_t*          item;

  UNUSED_PARAM(data);
  evt1 = ls_event_dispatcher_get_event(g_dispatcher, "mockEvent1");

  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_bind1_callback1, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_rebind1_callback1, NULL, &err) );

  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );
  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );

  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt_bind1_callback1);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt_rebind1_callback1);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback1);
  ASSERT_NULL(b->next);

  ASSERT_EQUAL(g_audit.count, 5);
  item = g_audit.items;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_bind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_rebind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_bind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_rebind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt1_callback1", evt1,
                                NULL, NULL) );
  ASSERT_NULL(item->next);
}

CTEST2(event, trigger_event_defer_bind_unbind)
{
  ls_event*           evt1;
  ls_err              err;
  ls_event_binding_t* b;
  logitem_t*          item;

  UNUSED_PARAM(data);
  evt1 = ls_event_dispatcher_get_event(g_dispatcher, "mockEvent1");

  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_bind1_callback1, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_unbind1_callback1, NULL, &err) );

  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );

  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt_bind1_callback1);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt_unbind1_callback1);
  ASSERT_NULL(b->next);

  ASSERT_EQUAL(g_audit.count, 2);
  item = g_audit.items;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_bind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_unbind1_callback1", evt1,
                                NULL, NULL) );
  ASSERT_NULL(item->next);
}

CTEST2(event, trigger_event_defer_bind_unbind_rebind)
{
  ls_event*           evt1;
  ls_err              err;
  ls_event_binding_t* b;
  logitem_t*          item;

  UNUSED_PARAM(data);
  evt1 = ls_event_dispatcher_get_event(g_dispatcher, "mockEvent1");

  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_bind1_callback1, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_unbind1_callback1, NULL, &err) );
  ASSERT_TRUE( ls_event_bind(evt1, mock_evt_rebind1_callback1, NULL, &err) );

  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );
  ASSERT_TRUE( ls_event_trigger(evt1, NULL, NULL, NULL, &err) );

  b = evt1->bindings;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt_bind1_callback1);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt_unbind1_callback1);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt_rebind1_callback1);
  ASSERT_NOT_NULL(b->next);

  b = b->next;
  ASSERT_NOT_NULL(b);
  ASSERT_TRUE(b->cb == mock_evt1_callback1);
  ASSERT_NULL(b->next);

  ASSERT_EQUAL(g_audit.count, 7);
  item = g_audit.items;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_bind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_unbind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_rebind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_bind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_unbind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt_rebind1_callback1", evt1,
                                NULL, NULL) );
  item = item->next;
  ASSERT_STR( item->message,
              log_event_message("mock_evt1_callback1", evt1,
                                NULL, NULL) );
  ASSERT_NULL(item->next);
}

CTEST2(event, trigger_prepared)
{
  ls_event_trigger_data* trigger_data;
  ls_event*              evt;

  UNUSED_PARAM(data);
  /* can't use audit trail since that would require memory allocation */
  evt = ls_event_dispatcher_get_event(g_dispatcher, "mockEvent1");
  ASSERT_TRUE( ls_event_bind(evt, mock_nofail_callback, NULL, NULL) );

  ASSERT_TRUE( ls_event_prepare_trigger(g_dispatcher, &trigger_data, NULL) );

  g_oom_malloc_called      = false;
  g_nofail_callback_called = false;
  ls_data_set_memory_funcs(mock_oom_malloc, mock_oom_realloc, NULL);

  ls_event_trigger_prepared(evt, NULL, NULL, NULL, trigger_data);

  ls_data_set_memory_funcs(NULL, NULL, NULL);
  ASSERT_TRUE(g_nofail_callback_called);

  ls_event_unbind(evt, mock_evt1_callback_handled1);
}

CTEST2(event, trigger_prepare_unprepare)
{
  ls_event_trigger_data* trigger_data;

  UNUSED_PARAM(data);
  ls_data_set_memory_funcs(_counting_malloc, _counting_realloc, _counting_free);

  ASSERT_TRUE( ls_event_prepare_trigger(g_dispatcher, &trigger_data, NULL) );
  ls_event_unprepare_trigger(trigger_data);

  ASSERT_EQUAL(_mallocCnt, _freeCnt);

  ls_data_set_memory_funcs(NULL, NULL, NULL);
}

CTEST2(event, trigger_deferred_destroy)
{
  ls_event*            evt        = NULL;
  ls_event_dispatcher* dispatcher = NULL;

  UNUSED_PARAM(data);
  g_destroy_first_free  = false;
  g_destroy_first_alloc = NULL;
  ls_data_set_memory_funcs(destroy_test_malloc, NULL, destroy_test_free);
  ASSERT_TRUE( ls_event_dispatcher_create(g_source, &dispatcher, NULL) );
  ASSERT_TRUE( ls_event_dispatcher_create_event(dispatcher,
                                                "destroyEvt", &evt, NULL) );
  ASSERT_TRUE( ls_event_bind(evt, destroying_callback, dispatcher, NULL) );
  ASSERT_TRUE( ls_event_trigger(evt, evt, NULL, NULL, NULL) );

  /* can't use audit trail since the evt is destroyed during the callback */
  ASSERT_NOT_NULL(g_destroy_first_alloc);
  ASSERT_TRUE(g_destroy_correctly_deferred);

  ls_data_set_memory_funcs(NULL, NULL, NULL);
}

CTEST2(event, oom)
{
  ls_event_dispatcher* dispatcher;
  ls_event*            evt;

  UNUSED_PARAM(data);
  /* create dispatcher */
  OOM_SIMPLE_TEST( ls_event_dispatcher_create(
                     g_source, &dispatcher, &err) );

  OOM_TEST_INIT();
  OOM_TEST( NULL, ls_event_dispatcher_create(
              g_source, &dispatcher, NULL) );

  /* create event (use two events: an event cannot be created twice) */
  ls_err err;
  OOM_RECORD_ALLOCS( ls_event_dispatcher_create_event(
                       dispatcher, "ev", &evt, &err) );
  OOM_TEST_INIT();
  OOM_TEST_CONDITIONAL_CHECK(&err,
                             ls_event_dispatcher_create_event(
                               dispatcher, "ev2", &evt, &err),
                             true);
  OOM_TEST_INIT();
  OOM_TEST( NULL, ls_event_dispatcher_create_event(
              dispatcher, "ev2", &evt, NULL) );

  /* bind event (use two callbacks: rebinding changes the code path) */
  OOM_RECORD_ALLOCS( ls_event_bind(
                       evt, async_callback, dispatcher, &err) );
  OOM_TEST_INIT();
  OOM_TEST_CONDITIONAL_CHECK(&err,
                             ls_event_bind(evt, destroying_callback,
                                           dispatcher, &err),
                             true);
  OOM_TEST_INIT();
  OOM_TEST( NULL, ls_event_bind(evt, destroying_callback,
                                dispatcher, NULL) );

  /* trigger event */
  uint32_t call_count = 0;
  OOM_SIMPLE_TEST( ls_event_trigger(
                     evt, &call_count, NULL, NULL, &err) );
  OOM_TEST_INIT();
  OOM_TEST( NULL, ls_event_trigger(
              evt, &call_count, NULL, NULL, NULL) );

  ls_event_dispatcher_destroy(dispatcher);
}
