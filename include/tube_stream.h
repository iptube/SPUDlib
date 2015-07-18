/**
 * \file
 * \brief
 * Streaming data in terms of SPUD.
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#pragma once

#include <unistd.h>
#include <stdint.h>

#include "ls_basics.h"
#include "ls_error.h"
#include "ls_event.h"
#include "tube_manager.h"

// ### TUBE STREAM MANAGER INTERFACE ### //
/**
 * Handle for a tube stream manager.
 */
typedef struct _tube_stream_manager tube_stream_manager;

// #### TUBE STREAM EVENTS #### //
/**
 * Type of tube stream event data; passed to event handler.
 */
typedef struct _tube_stream_event_data {
    /** The TCP the event applies to */
    struct _tcp_tube *s;
    /** the TCP manager of the TCP the event applies to */
    struct _tube_stream_manager *s_mgr;
} tube_stream_event_data;

#define EV_STREAM_READABLE_NAME "tcp:data"

#define EV_STREAM_OPEN_NAME "tcp:open"

#define EV_STREAM_CLOSE_NAME "tcp:close"

/**
 * Creates a new tube stream manager.  Set up dispatcher, event handlers.
 *
 * \invariant tcpm != NULL
 * \param[in] mgr Underlying tube manager. If NULL, a new tube manager is
 *            created using appropriate default values
 * \param[out] sm Where to put pointer to new tube stream manager
 * \param[out] err If non-NULL on input, describes error if false is returned
 * \return true: New tube stream manager created.  false: see err.
 */
LS_API bool tube_stream_manager_create(tube_manager *mgr,
                                       tube_stream_manager **sm,
                                       ls_error *err);
/**
 * Destroys a tube stream manager. All tube streams are closed and released.
 *
 * \param[in] sm The previously-created tube stream manager to be terminated.
 * \warning sm should not be dereferenced after return.
 */
LS_API void tube_stream_manager_destroy(tube_stream_manager *sm);

/**
 * Connects a tube stream to the given socket address.
 *
 * \invariant sm != NULL
 * \invariant sockaddr != NULL
 * \param[in] sm The tube stream manager
 * \param[in] dest The socket address to connect to
 * \param[out] s The newly created tube stream connected to dest
 * \param[out] err If non-NULL on input, describes error if false is returned
 * \return true: stream tube connected.  false: see err.
 */
LS_API bool tube_stream_connect(tube_stream_manager *sm
                                sockaddr *dest,
                                tube_stream **s,
                                ls_error *err);

/**
 * Retrieves the tube manager for this tube stream manager.
 *
 * \invariant sm != NULL
 * \param[in] sm The tube stream
 * \return The tube manager for sm
 */
LS_API tube_manager *tube_stream_get_manager(tube_stream_manager *sm);

/**
 * Start listening for incoming tube stream connections.
 *
 * \invariant sm != NULL
 * \param[in] sm The tube stream manager
 */
LS_API bool tube_stream_listen(tube_stream_manager *sm
                               ls_error *err);

/**
 * Bind an event handler (callback) to an event.
 *
 * \invariant sm != NULL
 * \invariant name != NULL
 * \invariant cb != NULL
 * \param[in] sm The tube stream manager to bind to
 * \param[in] name The (string) name of the event
 * \param[in] cb The event handler to be bound to the event
 * \param[in] arg The argument for cb
 * \param[out] err If non-NULL on input, describes error if false is returned
 * \return true: event bound.  false: see err.
 */
LS_API bool tube_stream_manager_bind_event(tube_stream_manager *sm,
                                           const char *name,
                                           ls_event_callback *cb,
                                           void *arg,
                                           ls_error *err);


// ### TUBE STREAM INTERFACE ### //
/**
 * Handle for a tube stream.
 */
typedef struct _tube_stream tube_stream;

/**
 * Allocate a new tube stream to be managed.
 *
 * The new tube stream has not associated tube, and its state is UNKNOWN.
 *
 * \invariant tcp != NULL
 * \param[out] s Where to put the new tube stream.
 * \param[out] err If non-NULL on input, describes error if false is returned
 * \return true: m points to the manager.  false: see err.
 */
LS_API bool tube_stream_create(tube_stream **s,
                               ls_error *err);
/**
 * Deallocate an existing tube stream.
 *
 * \invariant s != NULL
 * \param[in] s  The tube stream to be reclaimed.
 */
LS_API void tube_stream_destroy(tube_stream *s);

/**
 * Binds this stream to a tube.
 *
 * \invariant s != NULL
 * \invariant t != NULL
 * \param[in] s The stream to bind to
 * \param[in] t The tube to bind to the stream
 * \param[out] err If non-NULL on input, describes error if false is returned
 * \return true: tube bound to this stream.  false: see err.
 */
LS_API bool tube_stream_bind(tube_stream *s,
                             tube *t,
                             ls_error *err);

// QUESTION: amount readable now?
/**
 * Reads data from the tube stream.
 *
 * \invariant s != NULL
 * \invariant data != NULL
 * \invariant sizeof(data) >= len
 * \param[in] s The tube stream to read data from
 * \param[in] data The buffer to place read data
 * \param[in] len The size of data
 * \result The amount of data read and placed into data
 */
LS_API ssize_t tube_stream_read(tube_stream *s,
                                uint8_t *data, size_t len);
/**
 * Writes data to the tube stream.
 *
 * \invariant s != NULL
 * \invariant sizeof(data) >= len
 * \param[in] s the tube stream to write data to.
 * \param[in] data The buffer to extract data to write
 * \param[in] len The amount of data to write from data
 * \param[out] err If non-NULL on input, describes error if false is returned
 * \return true: data is written.  false: see err.
 */
LS_API bool tube_stream_write(tube_stream *s,
                              uint8_t *data, size_t len,
                              ls_error *err);

/**
 * Close the tube stream.
 *
 * \param[in] s The tube stream to close.
 * \param[out] err If non-NULL on input, describes error if false is returned
 * \return true: tube stream is closed.  false: see err.
 */
LS_API bool tube_stream_close(tube_stream *s,
                              ls_error *err);


