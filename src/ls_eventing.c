/**
 *
 * Copyrights
 *
 * Portions created or assigned to Cisco Systems, Inc. are
 * Copyright (c) 2010-2011 Cisco Systems, Inc.  All Rights Reserved.
 */

#include "ls_eventing.h"
#include "ls_htable.h"
#include "ls_str.h"

#include <assert.h>
#include <string.h>

#include "ls_eventing_int.h"

/* Internal Constants */
static const int DISPATCH_BUCKETS = 7;
static const size_t MOMENT_POOLSIZE = 0;

/* Internal Functions */
/**
 * Searched the notifier for an existing binding of the given callback.
 * This function updates {pmatch} and {pprev} as appropriate, including
 * setting values to NULL if there is no match.
 */
static bool _find_binding(ls_event_notifier_t *notifier,
                          ls_event_notify_callback cb,
                          ls_event_binding_t **pmatch,
                          ls_event_binding_t **pins)
{
    ls_event_binding_t *curr = notifier->bindings;
    ls_event_binding_t *prev = NULL;
    bool head;

    /* initialize to not found */
    *pmatch = NULL;
    while (curr != NULL)
    {
        head = (notifier->bindings == curr);

        if (curr->cb == cb)
        {
            /* callback matches; remove from list */
            if (prev != NULL)
            {
                prev->next = curr->next;
            }
            else if (head == true)
            {
                notifier->bindings = curr->next;
            }

            /* update prev to insert point */
            if (curr->next != NULL && head == false)
            {
                prev = curr->next;
            }

            /* remember found binding */
            *pmatch = curr;

            /* finish breaking the list */
            curr->next = NULL;

            break;
        }

        prev = curr;
        curr = curr->next;
    }

    /* update insert point */
    *pins = prev;

    return (*pmatch != NULL);
}

static void _process_pending_unbinds(ls_event event)
{
    ls_event_binding_t *prev = NULL;

    ls_event_binding_t *cur = event->bindings;
    while (cur != NULL)
    {
        if (cur->unbound == true)
        {
            ls_event_binding_t *remove = cur;

            if (prev != NULL)
            {
                prev->next = cur->next;
            }
            else if (cur == event->bindings)
            {
                event->bindings = cur->next;
            }
            cur = cur->next;

            ls_data_free(remove);
            remove = NULL;
            continue;
        }

        prev = cur;
        cur = cur->next;
    }
}

static void _set_bind_status(ls_event event)
{
    ls_event_binding_t *cur = event->bindings;
    while (cur != NULL)
    {
        cur->normal_bound = true;
        cur = cur->next;
    }
}

/**
 * Enques and runs an event. If this dispatcher is not currently running,
 * the given moment is processed immediately. Otherwise, it is enqueued and
 * processed in the order it was added.
 */
static void _dispatch_trigger(ls_event_dispatch_t *dispatcher,
                              ls_event_moment_t *moment)
{
    /* always enqueue! */
    /* \TODO optimize for moments with no callbacks? */
    if (dispatcher->queue != NULL)
    {
        /* add to tail */
        dispatcher->queue->next = moment;
    }
    dispatcher->queue = moment;

    if (dispatcher->running != NULL)
    {
        /* already processing events */
        return;
    }

    /* this is the only event (currently); moment is the start */
    dispatcher->running = moment->evt.notifier;
    while (moment != NULL)
    {
        ls_event_binding_t  *binding;
        ls_event_data       evt = &moment->evt;
        ls_pool             pool;

        _set_bind_status(evt->notifier);

        /* process notify callbacks */
        for (binding = moment->bindings;
             binding != NULL;
             binding = binding->next)
        {
            if (binding->normal_bound)
            {
                bool    handled = evt->handled;

                binding->cb(evt, binding->arg);
                /* prevent callbacks from unhandling */
                evt->handled = handled | evt->handled;
                /* \TODO break if handled? */
            }
        }

        /* report event results */
        if (moment->result_cb)
        {
            moment->result_cb(evt, evt->handled, moment->result_arg);
        }

        /* moveon and cleanup */
        _process_pending_unbinds(evt->notifier);

        pool = evt->pool;
        moment = moment->next;
        if (moment)
        {
            dispatcher->running = moment->evt.notifier;
        }
        ls_pool_destroy(pool);
    }

    /* done processing events */
    dispatcher->queue = NULL;
    dispatcher->running = NULL;
}

static int _clean_event(void *user_data, const void *key, void *data)
{
    ls_event event = (ls_event)data;
    ls_event_binding_t *curr = (EXPAND_NOTIFIER(event))->bindings;

    UNUSED_PARAM(user_data);
    UNUSED_PARAM(key);

    /* Clean up callbacks */
    while (curr != NULL)
    {
        ls_event_binding_t *remove = curr;
        curr = curr->next;
        ls_data_free(remove);
        remove = NULL;
    }

    ls_data_free((char *)ls_event_get_name(event));
    ls_data_free(EXPAND_NOTIFIER(event));

    return 1;
}

/* External Functions */
LS_API bool ls_event_dispatcher_create(const void *source,
                                       ls_event_dispatcher *dispatch,
                                       ls_err *err)
{
    ls_htable           events = NULL;
    ls_event_dispatch_t *dispatcher = NULL;

    assert(source != NULL);
    assert(dispatch != NULL);

    if (!ls_htable_create(DISPATCH_BUCKETS,
                          ls_strcase_hashcode,
                          ls_strcase_compare,
                          &events,
                          err))
    {
        return false;
    }
    dispatcher = (ls_event_dispatch_t *)ls_data_malloc(sizeof(ls_event_dispatch_t));
    if (dispatcher == NULL)
    {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        ls_htable_destroy(events);
        return false;
    }

    memset(dispatcher, 0, sizeof(ls_event_dispatch_t));
    dispatcher->source = source;
    dispatcher->events = events;
    *dispatch = (ls_event_dispatcher)dispatcher;

    return true;
}
LS_API void ls_event_dispatcher_destroy(ls_event_dispatcher dispatch)
{
    ls_event_dispatch_t *dispatcher;

    assert(dispatch != NULL);
    dispatcher = EXPAND_DISPATCHER(dispatch);

    ls_htable_clear(dispatcher->events, _clean_event, NULL);
    ls_htable_destroy(dispatcher->events);
    ls_data_free(dispatcher);
}

LS_API ls_event ls_event_dispatcher_get_event(
                        ls_event_dispatcher dispatch,
                        const char *name)
{
    ls_event_dispatch_t *dispatcher;
    ls_event            evt;

    assert(dispatch != NULL);
    assert(name != NULL);
    dispatcher = EXPAND_DISPATCHER(dispatch);

    evt = (ls_event)ls_htable_get(dispatcher->events, name);

    return evt;
}

LS_API bool ls_event_dispatcher_create_event(
                    ls_event_dispatcher dispatch,
                    const char *name,
                    ls_event *event,
                    ls_err *err)
{
    ls_event_dispatch_t *dispatcher;
    ls_event_notifier_t *notifier = NULL;
    char                *evt_name = NULL;
    bool                retval = true;
    size_t              nameLen;

    assert(dispatch != NULL);
    assert(name != NULL);
    assert(event != NULL);
    dispatcher = EXPAND_DISPATCHER(dispatch);

    if (name[0] == '\0')
    {
        LS_ERROR(err, LS_ERR_INVALID_ARG);
        retval = false;
        goto finish;
    }
    if (ls_htable_get(dispatcher->events, name) != NULL)
    {
        LS_ERROR(err, LS_ERR_INVALID_STATE);
        retval = false;
        goto finish;
    }

    nameLen = ls_strlen(name);
    evt_name = (char *)ls_data_malloc(nameLen + 1);
    if (evt_name == NULL)
    {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        retval = false;
        goto finish;
    }
    memcpy(evt_name, name, nameLen + 1);

    notifier = (ls_event_notifier_t *)ls_data_malloc(sizeof(ls_event_notifier_t));
    if (notifier == NULL)
    {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        retval = false;
        goto finish;
    }

    memset(notifier, 0, sizeof(ls_event_notifier_t));
    notifier->dispatcher = dispatcher;
    notifier->source = dispatcher->source;
    notifier->name = evt_name;

    if (!ls_htable_put(dispatcher->events,
                       evt_name,
                       notifier,
                       NULL,
                       err))
    {
        retval = false;
        goto finish;
    }
    evt_name = NULL;

    *event = (ls_event)notifier;
    notifier = NULL;

    finish:
    if (evt_name != NULL)
    {
        ls_data_free((char *)evt_name);
        evt_name = NULL;
    }

    if (notifier != NULL)
    {
        ls_data_free(notifier);
        notifier = NULL;
    }

    return retval;
}

LS_API const char *ls_event_get_name(ls_event event)
{
    assert(event != NULL);

    return (EXPAND_NOTIFIER(event))->name;
}

LS_API const void *ls_event_get_source(ls_event event)
{
    assert(event != NULL);

    return EXPAND_NOTIFIER(event)->source;
}

LS_API bool ls_event_bind(ls_event event,
                          ls_event_notify_callback cb,
                          void *arg,
                          ls_err *err)
{
    ls_event_notifier_t     *notifier;
    ls_event_binding_t      *binding = NULL;
    ls_event_binding_t      *prev = NULL;

    assert(event != NULL);
    assert(cb != NULL);
    notifier = EXPAND_NOTIFIER(event);

    /* look for existing binding first */
    if (!_find_binding(notifier, cb, &binding, &prev))
    {
        /* no match found; allocate a new one */
        binding = (ls_event_binding_t *)ls_data_malloc(sizeof(ls_event_binding_t));
        if (binding == NULL)
        {
            LS_ERROR(err, LS_ERR_NO_MEMORY);
            return false;
        }
        memset(binding, 0, sizeof(ls_event_binding_t));
    }
    else
    {
        /* Keep previous binding status so that we know whether it is
         * bound before the current event or within the current event
         */
        bool normal_bound = binding->normal_bound;
        memset(binding, 0, sizeof(ls_event_binding_t));
        binding->normal_bound = normal_bound;
    }

    /* update binding properties */
    binding->cb = cb;
    binding->arg = arg;

    /* (re-)insert into list */
    if (notifier->bindings == NULL)
    {
        /* first binding; place on the front */
        notifier->bindings = binding;
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
            prev->next = binding;
        }
    }
    else if (prev == NULL)
    {
        binding->next = notifier->bindings;
        notifier->bindings = binding;
    }

    return true;
}
LS_API void ls_event_unbind(ls_event event,
                            ls_event_notify_callback cb)
{
    ls_event_notifier_t *notifier;
    ls_event_binding_t  *binding, *prev;

    assert(event != NULL);
    assert(cb != NULL);
    notifier = EXPAND_NOTIFIER(event);

    /* If we're currently running the event we're requesting the unbind from
     * we need to defer the operation until the event has finished its
     * trigger
     */
    if (notifier != notifier->dispatcher->running)
    {
        if (_find_binding(notifier, cb, &binding, &prev))
        {
            /* match found; lists already updated */
            ls_data_free(binding);
            binding = NULL;
        }
    }
    else
    {
        ls_event_binding_t *cur = notifier->bindings;
        while (cur != NULL)
        {
            if (cur->cb == cb)
            {
                cur->unbound = true;
                break;
            }
            cur = cur->next;
        }
    }

    return;
}

LS_API bool ls_event_trigger(ls_event event,
                             void *data,
                             ls_event_result_callback result_cb,
                             void *result_arg,
                             ls_err *err)
{
    ls_event_notifier_t     *notifier;
    ls_event_dispatch_t     *dispatcher;
    ls_event_data           evt;
    ls_pool                 pool;
    ls_event_moment_t       *moment;

    union
    {
        ls_event_moment_t *moment;
        void              *momentPtr;
    } momentUnion;

    assert(event != NULL);
    notifier = EXPAND_NOTIFIER(event);
    dispatcher = notifier->dispatcher;

    if (!ls_pool_create(MOMENT_POOLSIZE, &pool, err))
    {
        return false;
    }
    if (!ls_pool_malloc(pool,
                        sizeof(ls_event_moment_t),
                        &momentUnion.momentPtr,
                        err))
    {
        ls_pool_destroy(pool);
        return false;
    }
    moment = momentUnion.moment;

    /* setup the event moment */
    moment->result_cb = result_cb;
    moment->result_arg = result_arg;
    moment->bindings = notifier->bindings;
    moment->next = NULL;

    /* setup event data */
    evt = &moment->evt;
    evt->source = dispatcher->source;
    evt->name = notifier->name;
    evt->notifier = event;
    evt->data = data;
    evt->selected = NULL;
    evt->pool = pool;
    evt->handled = false;

    /* enque, and maybe run */
    _dispatch_trigger(dispatcher, moment);

    return true;
}
