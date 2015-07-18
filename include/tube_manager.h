/**
 * \file
 * \brief
 * SPUDlib facilities for working groups of tubes.
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#pragma once

#include <signal.h>

#include "spud.h"
#include "ls_error.h"
#include "ls_event.h"
#include "ls_timer.h"
#include "tube.h"

/**
 * Tube Event names
 *
 */
/**
 * Tube manager event loop started
 */
#define EV_LOOPSTART_NAME "loopstart"
/**
 * Tube started
 */
#define EV_RUNNING_NAME "running"
/**
 *Data arrived
 */
#define EV_DATA_NAME    "data"
/**
 * Tube closed
 */
#define EV_CLOSE_NAME   "close"
/**
 * A tube was added to the manager
 */
#define EV_ADD_NAME     "add"
/**
 * A tube was removed from the manager
 */
#define EV_REMOVE_NAME  "remove"

/** Tube policies.  Currently ony deals with handling OPEN.
 */
typedef enum {
  /** Ignore the source IP/port, and just match on tube ID's.   Not currently recommended. */
  TP_IGNORE_SOURCE = 1 << 0,
  /** If set, act as a responder, creating new tube when OPEN cmd is received. */
  TP_WILL_RESPOND  = 1 << 1
} tube_policies;

/**
 * Handle for a tube manager
 */
typedef struct _tube_manager tube_manager;

/**
 * Type of the function called to send a message on a tube.
 * See tube_set_socket_functions().
 */
typedef ssize_t (*tube_sendmsg_func)(int socket,
                                     const struct msghdr *message,
                                     int flags);
/**
 * Type of the function called on message arrival.
 * See in tube_set_socket_functions().
 */
typedef ssize_t (*tube_recvmsg_func)(int socket,
                                     struct msghdr *message,
                                     int flags);

/**
 * Type of the function called when iterating over all tubes under the manager's
 * control.  See tube_manager_foreach.
 *
 * \param user_data Optional data provided
 * \param tube_id The ID of the current tube.
 * \param tube The current tube.
 * \retval int Nonzero to continue iterating (continue), 0 to stop (break).
 */
typedef int (*tube_walker_func)(void *user_data,
                                const struct _spud_tube_id *tube_id,
                                struct _tube *tube);

/**
 * Type of tube event data; passed to event handler.
 */
typedef struct _tube_event_data {
    /** the tube the event applies to */
    struct _tube *t;
    /** the tube manager of the tube the event applies to */
    struct _tube_manager *tmgr;
    /** data in */
    const cn_cbor *cbor;
    /** address of tube's peer or source of incoming packet */
    const struct sockaddr* peer;
} tube_event_data;

/**
 * Create a tube manager and initialize: set up dispatcher, event handlers.
 *
 * \invariant m != NULL
 * \param[in] buckets Number of buckets in tube hash table. If 0,
 *    a value appropriate for a server is used.
 * \param[out] m  Where to put pointer to new tube manager
 * \param[out] err If non-NULL on input, describes error if false is returned
 * \return true: m points to the manager.  false: see err.
 */
LS_API bool tube_manager_create(int buckets,
                                tube_manager **m,
                                ls_err *err);
/**
 * Shut down and deallocate a tube manager.  All associated data structures are freed.
 *
 * \param mgr The previously-created manager to be terminated.
 * \warning mgr should not be dereferenced after return.
 */
LS_API void tube_manager_destroy(tube_manager *mgr);

/**
 * Interrupt the processing of the current loop by sending the given byte
 * down the self-pipe.
 *
 * \param[in]  mgr The tube manager to interrupt
 * \param[in]  b   The byte to send.  Small positive numbers are used for
 *                 signals.
 * \param[out] err If non-NULL on input, describes error if false is returned
 * \return     On error, false (see err)
 */
LS_API bool tube_manager_interrupt(tube_manager *mgr, char b, ls_err *err);

/**
 *  Create a (v6) socket for the tube manager to send/receive on.
 *  Bind the socket to an address and the given port, or a random port if 0.
 *  If port != 0, the manager will respond to incoming OPENs.
 *
 * \invariant m != NULL
 * \param[in] m  The tube manager to get the socket
 * \param[in] port The port to bind to (clients use 0)
 * \param[out] err If non-NULL on input, contains error if false is returned
 * \return true: socket created and bound.  false: see err.
 */
LS_API bool tube_manager_socket(tube_manager *m,
                                int port,
                                ls_err *err);

/**
 * Bind an event handler (callback) to an event.
 *
 * \invariant mgr != NULL, mgr->dispatcher != NULL
 * \invariant name != NULL
 * \invariant cb != NULL
 *
 * \param[in] mgr The tube manager for the event handler
 * \param[in] name The (string) name of the event
 * \param[in] cb  The event handler to be bound to the event
 * \param[out] err If non-NULL on input, contains error if false is returned
 * \return true: event bound. false: see err.
 */
LS_API bool tube_manager_bind_event(tube_manager *mgr,
                                    const char *name,
                                    ls_event_notify_callback cb,
                                    ls_err *err);

/**
 * Add the given tube to a tube manager's table.  Triggers the "add" event.
 * \invariant mgr != NULL
 * \param[in] mgr The tube manager to manage the tube
 * \param[in] t The tube to be managed
 * \param[out] err If non-NULL on input, contains error if false is returned
 * \return true: tube was added. false: see err.
 */
LS_API bool tube_manager_add(tube_manager *mgr,
                             struct _tube *t,
                             ls_err *err);

/**
 * Remove the given tube from the tube manager's control.
 *    Triggers the "remove" event.
 *
 * \invariant mgr != NULL, t != NULL
 * \param[in] mgr  The manager from which the tube will be removed
 * \param[in] t   The tube to remove
 */
LS_API void tube_manager_remove(tube_manager *mgr,
                                struct _tube *t);

/**
 * Start receiving SPUD packets and parsing them. Queue events (DATA/CLOSE/RUNNING)
 * as appropriate.
 *
 * \invariant mgr != NULL, t != NULL
 * \param[in] mgr The manager to start
 * \param[out] err If non-NULL on input, contains error if false is returned
 * \return true: manager is running, incoming packets will be processed.
 *         false: manager stopped (not receiving).
 */
LS_API bool tube_manager_loop(tube_manager *mgr, ls_err *err);

/**
 * Check whether tube manager is running, i.e., keepgoing is true.
 *
 * \invariant mgr != NULL
 * \param[in] mgr The manager to check
 * \return true: The manager is running. false: see err.
 */
LS_API bool tube_manager_running(tube_manager *mgr);

/**
 * Shut down tube manager.  Sets keepgoing to false.
 *
 * \invariant mgr != NULL
 * \param[in] mgr The manager to shut down
 * \param[out] err If non-NULL on input, contains error if false is returned
 * \return true: tube manager was stopped. false: see err.
 */
LS_API bool tube_manager_stop(tube_manager *mgr, ls_err *err);

/**
 * Returns the number of tubes managed by this manager.
 * \invariant mgr != NULL
 * \param[in] mgr The manager
 * \return Number of tubes under the manager.
 */
LS_API size_t tube_manager_size(tube_manager *mgr);

/**
 * Set the responding policy as indicated.
 * \invariant mgr != NULL
 * \param[in] mgr  The manager
 * \param[in] will_respond  The desired policy value.
 *    true = incoming OPENs will be acked. false = incoming tubes that
 *    were not opened will be ignored.
 */
LS_API void tube_manager_set_policy_responder(tube_manager *mgr, bool will_respond);

/**
 * Open a new tube to the given destination.  Allocates and sends network
 * traffic.
 *
 * \invariant  mgr != NULL
 * \param[in]  mgr  The tube manager that will manage the given tube
 * \param[in]  dest The destination of the tube
 * \param[out] t    The tube that was created
 * \param[out] err  If non-NULL on input, contains error if false is returned
 * \return     true: tube was initiated. false: see err.
 */
LS_API bool tube_manager_open_tube(tube_manager *mgr,
                                   const struct sockaddr *dest,
                                   tube  **t,
                                   ls_err *err);

/**
 * Return the current value of the response policy.
 * \invariant mgr != NULL
 * \param[in] mgr  The manager to check
 * \return true = the manager responds to incoming OPEN packets.
 *   false = incoming packets on unknown tubes are ignored.
 */
LS_API bool tube_manager_is_responder(tube_manager *mgr);

/**
 * Schedule a callback for some number of milliseconds from now.
 *
 * \param[in]  mgr     The manager to schedule on
 * \param[in]  ms      The number of milliseconds from now to call back
 * \param[in]  cb      The function to call when the timer expires
 * \param[in]  context A pointer to be passed back to context
 * \param[out] err     If non-NULL on input, contains error if false is returned
 * \return     true: callback was schedued.  false: see err.
 */
LS_API bool tube_manager_schedule_ms(tube_manager *mgr,
                                     unsigned long ms,
                                     ls_timer_func cb,
                                     void *context,
                                     ls_timer **tim,
                                     ls_err *err);

/**
 * Schedule a callback for a specific wall-clock time.
 *
 * \param[in]  mgr     The manager to schedule on
 * \param[in]  tv      The time (in seconds since the UTC epoch) to fire the callback
 * \param[in]  cb      The callback to call when the timer expires
 * \param[in]  context A pointer to be passed back to context
 * \param[out] err     If non-NULL on input, contains error if false is returned
 * \return     true: callback was schedued.  false: see err.
 */
LS_API bool tube_manager_schedule(tube_manager *mgr,
                                  struct timeval *tv,
                                  ls_timer_func cb,
                                  void *context,
                                  ls_timer **tim,
                                  ls_err *err);

/**
 * Register a callback to call when a signal is received.  The callback will
 * fire at a safe time in the tube_manager_loop, where it is safe to do
 * real work.
 *
 * \param[in]  mgr The manager to process the signal
 * \param[in]  sig The signal to wait for
 * \param[in]  cb  The callback to fire when the signal is received
 * \param[out] err If non-NULL on input, contains error if false is returned
 * \return     true: callback was registered.  false: see err.
 */
LS_API bool tube_manager_signal(tube_manager *mgr,
                                int sig,
                                sig_t cb,
                                ls_err *err);

/**
 * Send a UDP message.
 *
 * \invariant dest != NULL
 * \invariant iov != NULL
 * \invariant count > 0
 * \param[in]  sock   The socket to send.  Make sure to pick the correct v4/v6
 *                    socket for the destination address.
 * \param[in]  source The source address to send from
 * \param[in]  dest   The destination address to send to
 * \param[in]  iov    One or more iovec's to send
 * \param[in]  count  The number of iovec's to send
 * \param[out] err    err If non-NULL on input, contains error if false is returned
 * \return     true: callback was registered.  false: see err.
 */
LS_API bool tube_manager_sendmsg(int sock,
                                 ls_pktinfo *source,
                                 struct sockaddr *dest,
                                 struct iovec *iov,
                                 size_t count,
                                 ls_err *err);

/**
 * Set the default functions that handle sending and receiving for all tubes, i.e.,
 * this has global effect.
 * \param send Function for sending.  If NULL, sendmsg is used.
 * \param recv Function for receiving.  If NULL, recvmsg is used.
 */
LS_API void tube_manager_set_socket_functions(tube_sendmsg_func send,
                                              tube_recvmsg_func recv);

/**
 * Print out information about all of the tubes in the manager at the moment.
 *
 * \invariant mgr != NULL
 * \param[in] mgr
 */
LS_API void tube_manager_print_tubes(tube_manager *mgr);

/**
 * Iterate over every tube in mgr's control, performing some arbitrary action.
 *
 * \invariant mgr != NULL
 * \invariant walker != NULL
 * \param[in] mgr The tube manager to iterate over.
 * \param[in] walker The walker function to be called on each iteration.
 * \param[in] data Optional data supplied to the walker on each iteration.
 */
LS_API void tube_manager_foreach(tube_manager *mgr,
                                 tube_walker_func walker,
                                 void *data);
