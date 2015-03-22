/*
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

typedef enum {
  TS_START,
  TS_UNKNOWN,
  TS_OPENING,
  TS_RUNNING,
  TS_RESUMING
} tube_states_t;

typedef enum {
  TP_IGNORE_SOURCE = 1 << 0,
  TP_WILL_RESPOND  = 1 << 1  // If set, act as a responder, creating tubes
                             // when an OPEN command is received
} tube_policies;

#define EV_RUNNING_NAME "running"
#define EV_DATA_NAME    "data"
#define EV_CLOSE_NAME   "close"
#define EV_ADD_NAME     "add"
#define EV_REMOVE_NAME  "remove"

typedef struct _tube_manager tube_manager;

typedef struct _tube tube;

typedef ssize_t (*tube_sendmsg_func)(int socket,
                                     const struct msghdr *message,
                                     int flags);
typedef ssize_t (*tube_recvmsg_func)(int socket,
                                     struct msghdr *message,
                                     int flags);

typedef struct _tube_event_data {
    tube *t;
    const cn_cbor *cbor;
    const struct sockaddr* addr;
} tube_event_data;

LS_API bool tube_manager_create(int buckets,
                                tube_manager **m,
                                ls_err *err);
LS_API void tube_manager_destroy(tube_manager *mgr);

LS_API bool tube_manager_socket(tube_manager *m,
                                int port,
                                ls_err *err);

LS_API bool tube_manager_bind_event(tube_manager *mgr,
                                    const char *name,
                                    ls_event_notify_callback cb,
                                    ls_err *err);

LS_API bool tube_manager_add(tube_manager *mgr,
                             tube *t,
                             ls_err *err);

LS_API void tube_manager_remove(tube_manager *mgr,
                                tube *t);

LS_API bool tube_manager_loop(tube_manager *mgr, ls_err *err);
LS_API bool tube_manager_running(tube_manager *mgr);
LS_API void tube_manager_stop(tube_manager *mgr);
LS_API size_t tube_manager_size(tube_manager *mgr);
LS_API void tube_manager_set_policy_responder(tube_manager *mgr, bool will_respond);
LS_API bool tube_manager_is_responder(tube_manager *mgr);

LS_API bool tube_create(tube_manager *mgr, tube **t, ls_err *err);
LS_API void tube_destroy(tube *t);

/* print [local address]:port to stdout */
LS_API bool tube_print(const tube *t, ls_err *err);
LS_API bool tube_open(tube *t, const struct sockaddr *dest, ls_err *err);
LS_API bool tube_ack(tube *t,
                     spud_tube_id *id,
                     const struct sockaddr *peer,
                     ls_err *err);
LS_API bool tube_data(tube *t, uint8_t *data, size_t len, ls_err *err);

LS_API void path_create_mandatory_keys(cn_cbor **cbor, uint8_t *ipadress, 
								size_t iplen, uint8_t *token, size_t tokenlen, char* url);
LS_API bool tube_send_pdec(tube *t, cn_cbor *cbor, bool reflect, ls_err *err);
LS_API bool tube_send_pdec_example(tube* t, ls_err *err);

LS_API bool tube_close(tube *t, ls_err *err);

LS_API bool tube_send(tube *t,
                      spud_command cmd,
                      bool adec, bool pdec,
                      uint8_t **data, size_t *len,
                      int num,
                      ls_err *err);
LS_API bool tube_send_cbor(tube *t, spud_command cmd, bool adec, bool pdec, cn_cbor *cbor, ls_err *err);
LS_API void tube_set_data(tube *t, void *data);
LS_API void *tube_get_data(tube *t);
LS_API char *tube_id_to_string(tube *t, char* buf, size_t len);
LS_API tube_states_t tube_get_state(tube *t);
LS_API void tube_get_id(tube *t, spud_tube_id *id);

LS_API void tube_set_socket_functions(tube_sendmsg_func send,
                                      tube_recvmsg_func recv);
