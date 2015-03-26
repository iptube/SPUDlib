/**
 * \file
 * \brief
 * SPUDlib facilities for working with tubes.
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#pragma once

#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>

#include "spud.h"
#include "ls_error.h"
#include "ls_event.h"
#include "cn-cbor/cn-cbor.h"

/** State of the tube.  See draft-
 */
typedef enum {
  /** just init'd */
  TS_START,
  /** saw something in the middle */
  TS_UNKNOWN,
  /** OPEN cmd sent, awaiting ACK */
  TS_OPENING,
  /** Tube is known and running */
  TS_RUNNING,
  /** ???? Not in the code... */
  TS_RESUMING
} tube_states_t;

/** Tube policies.  Currently ony deals with handling OPEN.
 */
typedef enum {
  /** Ignore the source.  */
  TP_IGNORE_SOURCE = 1 << 0,
  /** If set, act as a responder, creating new tube when OPEN cmd is received. */
  TP_WILL_RESPOND  = 1 << 1
} tube_policies;

/**
 * Tube Event names
 *
 */
/**
 * Tube manager starts
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
 * Added a tube
 */
#define EV_ADD_NAME     "add"
/**
 * A tube was removed
 */
#define EV_REMOVE_NAME  "remove"

/** 
 * Handle for a tube manager
 */
typedef struct _tube_manager tube_manager;

/** 
 * Handle for an individual tube
 */
typedef struct _tube tube;

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
 * Type of tube event data; passed to event handler.
 */
typedef struct _tube_event_data {
  /** the tube the event applies to */
    tube *t;
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
 * \param[in,out] m  Where to put pointer to new tube manager
 * \param[in,out] err If non-NULL on input, describes error if false is returned
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
 *  Create a (v6) socket for the tube manager to send/receive on.
 *  Bind the socket to an address and the given port, or a random port if 0.
 *  If port != 0, the manager will respond to incoming OPENs.
 *
 * \invariant m != NULL
 * \param[in] m  The tube manager to get the socket
 * \param[in] port The port to bind to (clients use 0)
 * \param[in,out] err If non-NULL on input, contains error if false is returned
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
 * \param[in,out] err If non-NULL on input, contains error if false is returned
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
 * \param[in,out] err If non-NULL on input, contains error if false is returned
 * \return true: tube was added. false: see err.
 */
LS_API bool tube_manager_add(tube_manager *mgr,
                             tube *t,
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
                                tube *t);

/**
 * Start receiving SPUD packets and parsing them. Queue events (DATA/CLOSE/RUNNING)
 * as appropriate.
 *
 * \invariant mgr != NULL, t != NULL
 * \param[in] mgr The manager to start
 * \param[in,out] err If non-NULL on input, contains error if false is returned
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
 */
LS_API void tube_manager_stop(tube_manager *mgr);

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
 * Associate a socket with the tube manager.  Manager must not already have
 * an associated socket.
 * \invariant mgr != NULL
 * \param[in] m  The manager
 * \param[in] sock  The socket to be associated with the manager
 */
LS_API void tube_manager_set_socket(tube_manager *m, int sock);

/**
 * Return the current value of the response policy.
 * \invariant mgr != NULL
 * \param[in] mgr  The manager to check
 * \return true = the manager responds to incoming OPEN packets.
 *   false = incoming packets on unknown tubes are ignored.
 */
LS_API bool tube_manager_is_responder(tube_manager *mgr);

/**
 * Allocate a new tube to be managed by this manager.
 * The new tube has no associated socket or ID and is in state UNKNOWN.
 * \invariant mgr != NULL
 * \invariant t != NULL
 * \param[in] mgr  The manager
 * \param[in,out] t  Where to put the new tube
 * \param[in,out] err If non-NULL on input, points to error when false is returned
 * \return  true: *t points to the new tube.
 *       false: see err
 */
LS_API bool tube_create(tube_manager *mgr, tube **t, ls_err *err);

/**
 * Deallocate an existing tube.
 * \invariant t != NULL
 * \param[in] t  The tube to be reclaimed.
 */
LS_API void tube_destroy(tube *t);

 

/**
 * Print local address of tube to stdout in the format "[address]:port".
 * \invariant t != NULL
 * \param[in] t  The tube to be printed
 * \param[in,out] err  If non-NULL on input, points to error when false is returned
 * \return true: print succeeded. false: getsockname, getnameinfo or printf failed.
 */
LS_API bool tube_print(const tube *t, ls_err *err);

/**
 * Open a tube to the given destination.
 * The tube must already have been added to a manager.
 * \invariant t != NULL
 * \param[in] t  The tube to be connected to the destination
 * \param[in] dest  Address and port of destination
 * \param[in,out] err  If non-NULL on input, points to error when false is returned
 * \return true: OPEN has been sent to the destination.  false: see err.
 */
LS_API bool tube_open(tube *t, const struct sockaddr *dest, ls_err *err);

/**
 * Respond to an incoming OPEN. (Called from tube_manager_loop;
 * application programs should use with care.)
 * The tube ID is taken from the incoming OPEN. The peer address is set to the
 * source of the OPEN.  The ACK is sent in a datagram with no payload.
 *
 * \invariant t != NULL
 * \invariant dest != NULL
 * \param[in] t  The tube to be activated.
 * \param[in] id  Tube ID from incoming message.
 * \param[in] peer  Source of the received OPEN message.
 * \param[in,out] err  If non-NULL on input, points to error when false is returned
 * \return true: ACK has been sent, tube added to mgr's hash table. false: see err.
 */
LS_API bool tube_ack(tube *t,
                     spud_tube_id *id,
                     const struct sockaddr *peer,
                     ls_err *err);

/**
 * Send a DATA packet.  Adds fixed header and encodes payload into CBOR map.
 * If invoked with len==0, will send a SPUD packet with no (data).
 *
 * \invariant t != NULL
 *
 * \param[in] t  The tube to send on
 * \param[in] data  Bytes to send
 * \param[in] len  Number of bytes to send
 * \param[in,out] err  If non-NULL on input, points to error when false is returned
 * \return true: packet successfully sent. false: see err.
 */
LS_API bool tube_data(tube *t, uint8_t *data, size_t len, ls_err *err);

/**
 * create keys for the CBOR map
 */
LS_API void path_create_mandatory_keys(cn_cbor **cbor,
				       uint8_t *ipadress, 
				       size_t iplen,
				       uint8_t *token,
				       size_t tokenlen,
				       char* url);
/**
 * Send a path declaration on the given tube.
 * Sent as a DATA packet with PDEC flag set.
 *
 * \invariant t != NULL
 * \param[in] t The tube to send on
 * \param[in] cbor  The encoded map to send
 * \param[in] reflect  The value to place in the ADEC flag
 * \param[in,out] err  If non-NULL on input, points to error when false is returned
 */
LS_API bool tube_send_pdec(tube *t, cn_cbor *cbor, bool reflect, ls_err *err);

/**
 * Close a tube.  Sends an (empty) CLOSE packet and sets tube state to UNKNOWN.
 *
 * \param[in] t  The tube to send on (must have peer address set)
 * \param[in,out] err  If non-NULL on input, points to error when false is returned
 * \return true: tube closed.  false: see err.
 */
LS_API bool tube_close(tube *t, ls_err *err);

/**
 * Construct and send a fully-specified SPUD packet.
 *
 * \invariant t != NULL
 * \param[in] t  The tube to send on (peer address must have been set)
 * \param[in] cmd SPUD command to send
 * \param[in] adec App-to-path declaration bit
 * \param[in] pdec Path-to-app declaration bit
 * \param[in] data Payload - sent as-is (must be CBOR-encoded)
 * \param[in] len  Size of the payload
 * \param[in] num  Number of items in the data array
 * \param[in,out] err If non-NULL on input, points to error when false is returned
 * \return true: success.  false: see err.
 */
LS_API bool tube_send(tube *t,
                      spud_command cmd,
                      bool adec, bool pdec,
                      uint8_t **data, size_t *len,
                      int num,
                      ls_err *err);
<<<<<<< HEAD
LS_API bool tube_send_cbor(tube *t, spud_command cmd, bool adec, bool pdec, cn_cbor *cbor, ls_err *err);
=======


/**
 * Set the data associated with a tube.  (Unused.)
 */
>>>>>>> docs
LS_API void tube_set_data(tube *t, void *data);

/**
 * Returns the data associated with a tube. (Unused.)
 */
LS_API void *tube_get_data(tube *t);
<<<<<<< HEAD
LS_API void tube_set_local(tube *t, struct in6_addr *addr);
=======

/**
 * Print the ID of a tube (on stdout) as a 16-digit hex string.
 *
 * \param[in] t  The tube whose ID is to be printed
 * \param[in] buf  Where to put the string
 * \param[in] len  Size of the buffer
 * \return buf on success, else NULL.
 */
>>>>>>> docs
LS_API char *tube_id_to_string(tube *t, char* buf, size_t len);

/**
 * Return the state of a tube.
 *
 * \param[in] t  The tube whose state is to be retrieved.
 * \return The state of the tube (see definition)
 */
LS_API tube_states_t tube_get_state(tube *t);

/**
 * Return the ID associated with a tube.
 *
 * \invariant t != NULL
 * \invariant id != NULL
 * \param[in] t  The tube whose id is returned
 * \param[in,out] id  Space for the ID.  On return, contains the tube ID.
 */
LS_API void tube_get_id(tube *t, spud_tube_id *id);

/**
 * Set the default functions that handle sending and receiving for all tubes, i.e.,
 * this has global effect.
 * \param send Function for sending.  If NULL, sendmsg is used.
 * \param recv Function for receiving.  If NULL, recvmsg is used.
 */
LS_API void tube_set_socket_functions(tube_sendmsg_func send,
                                      tube_recvmsg_func recv);
