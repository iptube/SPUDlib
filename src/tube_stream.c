/**
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "spud.h"
#include "tube_manager_int.h"
#include "tube_stream.h"

// ### TUBE STREAM IMPLEMENTATION ### //

struct _tube_stream
{
    // TODO: fill this out
    bool initialized;
};

LS_API bool tube_stream_create(tube_stream **s,
                               ls_err *err)
{
    assert(s);
    
    tube_stream *ret = ls_data_calloc(1, sizeof(tube_stream));
    if (NULL == ret) {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        *s = ret = NULL;
        return false;
    }

    // TODO: finish initializing
    ret->initialized = true;
    *s = ret;
    return true;
}

LS_API void tube_stream_destroy(tube_stream *s)
{
    if (NULL == s) {
        return;
    }
    
    // TODO: finish destroying
    s->initialized = false;
    ls_data_free(s);
    s = NULL;
}

LS_API bool tube_stream_bind(tube_stream *s,
                             tube *t,
                             ls_err *err)
{
    assert(s);
    
    UNUSED_PARAM(t);
    LS_ERROR(err, LS_ERR_NO_IMPL);
    return false;
}

LS_API ssize_t tube_stream_read(tube_stream *s,
                                uint8_t *data, size_t len)
{
    assert(s);
    assert(data);
    UNUSED_PARAM(len);
    
    return -1;
}

LS_API bool tube_stream_write(tube_stream *s,
                              uint8_t *data, size_t len,
                              ls_err *err)
{
    assert(s);
    UNUSED_PARAM(data);
    UNUSED_PARAM(len);
    
    LS_ERROR(err, LS_ERR_NO_IMPL);
    return false;
}

LS_API bool tube_stream_close(tube_stream *s,
                              ls_err *err)
{
    assert(s);
    
    LS_ERROR(err, LS_ERR_NO_IMPL);
    return false;
}


// ### TUBE STREAM MANAGER IMPLEMENTATION ### //

struct _tube_stream_manager
{
    tube_manager    tm;
    ls_event        *e_open;
    ls_event        *e_close;
    ls_event        *e_data;
    // TODO: fill this out more
};


LS_API bool tube_stream_manager_create(int buckets,
                                       tube_stream_manager **sm,
                                       ls_err *err)
{
    assert(sm);
    
    tube_stream_manager *ret;
    ret = ls_data_calloc(1, sizeof(tube_stream_manager));
    if (NULL == ret) {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        *sm = ret = NULL;
        return false;
    }
    if (!_tube_manager_init((tube_manager *)ret, buckets, err)) {
        ls_data_free(ret);
        *sm = ret = NULL;
        return false;
    }
    
    // setup events
    ls_event_dispatcher *dispatcher = _tube_manager_get_dispatcher((tube_manager *)ret);
    if (!ls_event_dispatcher_create_event(dispatcher,
                                          EV_STREAM_OPEN_NAME,
                                          &ret->e_open,
                                          err) ||
        !ls_event_dispatcher_create_event(dispatcher,
                                          EV_STREAM_CLOSE_NAME,
                                          &ret->e_close,
                                          err) ||
        !ls_event_dispatcher_create_event(dispatcher,
                                          EV_STREAM_DATA_NAME,
                                          &ret->e_data,
                                          err)) {
        goto cleanup;
    }
    
    ((tube_manager *)ret)->initialized = true;
    *sm = ret;
    return true;
cleanup:
    tube_stream_manager_destroy(ret);
    *sm = ret = NULL;
    return false;
}

LS_API void tube_stream_manager_destroy(tube_stream_manager *sm)
{
    if (!sm) {
        return;
    }

    sm->e_open = NULL;
    sm->e_close = NULL;
    sm->e_data = NULL;
    // TODO: finish destroying

    _tube_manager_finalize((tube_manager *)sm);

    ls_data_free(sm);
    sm = NULL;
}

LS_API tube_manager *tube_stream_manager_get_manager(tube_stream_manager *sm)
{
    return (tube_manager *)(sm);
}

LS_API bool tube_stream_manager_connect(tube_stream_manager *sm,
                                        struct sockaddr *dest,
                                        tube_stream **s,
                                        ls_err *err)
{
    assert(sm);
    assert(s);
    UNUSED_PARAM(dest);
    
    LS_ERROR(err, LS_ERR_NO_IMPL);
    return false;
}

LS_API bool tube_stream_manager_listen(tube_stream_manager *sm,
                                       ls_err *err)
{
    assert(sm);
    
    LS_ERROR(err, LS_ERR_NO_IMPL);
    return false;
}

LS_API bool tube_stream_manager_bind_event(tube_stream_manager *sm,
                                           const char *name,
                                           ls_event_notify_callback *cb,
                                           ls_err *err)
{
    assert(sm);
    assert(name);
    assert(cb);
    
    LS_ERROR(err, LS_ERR_NO_IMPL);
    return false;
}
