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
#include "ls_pktinfo.h"
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
  /** Tube might be resumed later.  Not yet used. */
  TS_RESUMING
} tube_states_t;

/**
 * Handle for an individual tube
 */
typedef struct _tube tube;

/**
 * Allocate a new tube to be managed by this manager.
 * The new tube has no associated socket or ID and is in state UNKNOWN.
 * \invariant t != NULL
 * \param[out] t  Where to put the new tube
 * \param[out] err If non-NULL on input, points to error when false is returned
 * \return  true: *t points to the new tube.
 *       false: see err
 */
LS_API bool tube_create(tube **t, ls_err *err);

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
 * \param[out] err  If non-NULL on input, points to error when false is returned
 * \return true: print succeeded. false: getsockname, getnameinfo or printf failed.
 */
LS_API bool tube_print(const tube *t, ls_err *err);

/**
 * Send a DATA packet.  Adds fixed header and encodes payload into CBOR map.
 * If invoked with len==0, will send a SPUD packet with no (data).
 *
 * \invariant t != NULL
 *
 * \param[in] t  The tube to send on
 * \param[in] data  Bytes to send
 * \param[in] len  Number of bytes to send
 * \param[out] err  If non-NULL on input, points to error when false is returned
 * \return true: packet successfully sent. false: see err.
 */
LS_API bool tube_data(tube *t, uint8_t *data, size_t len, ls_err *err);

/**
 * Create a CBOR map containing the mandatory pdec keys
 */
LS_API bool path_mandatory_keys_create(uint8_t *ipadress,
                                       size_t iplen,
                                       uint8_t *token,
                                       size_t tokenlen,
                                       char* url,
                                       cn_cbor_context *ctx,
                                       cn_cbor **map,
                                       ls_err *err);

/**
 * Send a path declaration on the given tube.
 * Sent as a DATA packet with PDEC flag set.
 *
 * \invariant t != NULL
 * \param[in] t The tube to send on
 * \param[in] cbor  The encoded map to send
 * \param[in] reflect  The value to place in the ADEC flag
 * \param[out] err  If non-NULL on input, points to error when false is returned
 */
LS_API bool tube_send_pdec(tube *t, cn_cbor *cbor, bool reflect, ls_err *err);

/**
 * Close a tube.  Sends an (empty) CLOSE packet and sets tube state to UNKNOWN.
 *
 * \param[in] t  The tube to send on (must have peer address set)
 * \param[out] err  If non-NULL on input, points to error when false is returned
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
 * \param[out] err If non-NULL on input, points to error when false is returned
 * \return true: success.  false: see err.
 */
LS_API bool tube_send(tube *t,
                      spud_command cmd,
                      bool adec, bool pdec,
                      uint8_t **data, size_t *len,
                      int num,
                      ls_err *err);

/**
 * Serialize the given CBOR, and send it to the tube's peer.
 *
 * \invariant t != NULL
 * @param[in]  t    The tube to send on
 * @param[in]  cmd  Open, close, ack, error
 * @param[in]  adec Is this an application declaration?
 * @param[in]  pdec Is this a path declaration?
 * @param[in]  cbor The CBOR to send
 * @param[out] err  If non-NULL on input, points to error when false is returned
 * @return     true: success.  false: see err.
 */
LS_API bool tube_send_cbor(tube *t,
                           spud_command cmd,
                           bool adec,
                           bool pdec,
                           cn_cbor *cbor,
                           ls_err *err);

/**
 * Set the data associated with a tube.  (Unused.)
 */
LS_API void tube_set_data(tube *t, void *data);

/**
 * Returns the data associated with a tube. (Unused.)
 */
LS_API void *tube_get_data(tube *t);

/**
 * Set the local address of a tube.
 *
 * \param[in] t The tube whose address is to be set
 * \param[in] info  The local address to set
 * \param[out] err If non-NULL on input, points to error when false is returned
 * \return true: success.  false: see err.
 */
LS_API bool tube_set_local(tube *t, ls_pktinfo *info, ls_err *err);

/**
 * Print the ID of a tube (on stdout) as a 16-digit hex string.
 *
 * \param[in] t  The tube whose ID is to be printed
 * \param[in] buf  Where to put the string
 * \param[in] len  Size of the buffer
 * \return buf on success, else NULL.
 */
LS_API char *tube_id_to_string(tube *t, char* buf, size_t len);

/**
 * Return the state of a tube.
 *
 * \param[in] t  The tube whose state is to be retrieved.
 * \return The state of the tube (see definition)
 */
LS_API tube_states_t tube_get_state(tube *t);

/**
 * Set the current state of the tube.
 *
 * @param[in] t     The tube to modify
 * @param[in] state The new tube state
 */
LS_API void tube_set_state(tube *t, tube_states_t state);

/**
 * Return the ID associated with a tube.
 *
 * \invariant t != NULL
 * \invariant id != NULL
 * \param[in] t  The tube whose id is returned
 * \param[out] id  On return, contains the internal tube ID pointer.
 */
LS_API void tube_get_id(tube *t, spud_tube_id **id);

/**
 * Set the current manager of the tube.
 *
 * \invariant t != NULL
 * @param[in] t     The tube to modify.
 * @param[in] state The new tube manager.
 */
LS_API void tube_set_manager(tube *t, tube_manager *tmgr);

/**
 * Return the manager a tube is attached to.
 *
 * \invariant t != NULL
 * \param[in] t  The tube whose manager is returned.
 * \param[out] id  On return, contains a pointer to the tube manager. This may
 * be NULL if the tube belongs to no manager.
 */
LS_API void tube_get_manager(tube *t, tube_manager **tmgr);

/**
 * Set various pieces of information about the tube.
 *
 * \invariant t != NULL
 * @param[in] t      The tube to modify
 * @param[in] socket The tube's new socket (if >= 0)
 * @param[in] peer   The tube's new peer address (if != NULL)
 * @param[in] id     The tube id (if != NULL)
 */
LS_API void tube_set_info(tube *t,
                          int socket,
                          const struct sockaddr *peer,
                          spud_tube_id *id);
