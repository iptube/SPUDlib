/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#pragma once

#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>

#include "spud.h"
#include "ls_error.h"
#include "ls_eventing.h"
#include "ls_mem.h"
#include "cn-cbor.h"

typedef enum {
  TS_START,
  TS_UNKNOWN,
  TS_OPENING,
  TS_RUNNING,
  TS_RESUMING
} tube_states_t;

struct _tube;


typedef void (*tube_state_cb)(struct _tube* t,
                              const struct sockaddr* addr);

typedef struct _tube {
  tube_states_t state;
  int sock;
  struct sockaddr_storage peer;
  spud_tube_id id;
  void *data;
  ls_event_dispatcher *dispatcher;
  ls_event *e_running;
  ls_event *e_data;
  ls_event *e_close;
  bool my_dispatcher;
} tube;

typedef struct _tubeData {
    tube *t;
    const cn_cbor *cbor;
    const struct sockaddr* addr;
} tubeData;

/* When you want to create a single dispatcher for a lot of tubes */
LS_API bool tube_bind_events(ls_event_dispatcher *dispatcher,
                             ls_event_notify_callback running_cb,
                             ls_event_notify_callback data_cb,
                             ls_event_notify_callback close_cb,
                             void *arg,
                             ls_err *err);

/* multiple tubes per socket */
LS_API bool tube_create(int sock, ls_event_dispatcher *dispatcher, tube **t, ls_err *err);
LS_API void tube_destroy(tube *t);

/* print [local address]:port to stdout */
LS_API bool tube_print(const tube *t, ls_err *err);
LS_API bool tube_open(tube *t, const struct sockaddr *dest, ls_err *err);
LS_API bool tube_ack(tube *t,
                     const spud_tube_id *id,
                     const struct sockaddr *dest,
                     ls_err *err);
LS_API bool tube_data(tube *t, uint8_t *data, size_t len, ls_err *err);
LS_API bool tube_close(tube *t, ls_err *err);

LS_API bool tube_send(tube *t,
                      spud_command cmd,
                      bool adec, bool pdec,
                      uint8_t **data, size_t *len,
                      int num,
                      ls_err *err);

LS_API bool tube_recv(tube *t,
                      spud_message *msg,
                      const struct sockaddr* addr,
                      ls_err *err);
