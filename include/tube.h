/*
Copyright 2015 Cisco. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY CISCO ''AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Cisco.
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
  spud_tube_id_t id;
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
                     const spud_tube_id_t *id,
                     const struct sockaddr *dest,
                     ls_err *err);
LS_API bool tube_data(tube *t, uint8_t *data, size_t len, ls_err *err);
LS_API bool tube_close(tube *t, ls_err *err);

LS_API bool tube_send(tube *t,
                      spud_command_t cmd,
                      bool adec, bool pdec,
                      uint8_t **data, size_t *len,
                      int num,
                      ls_err *err);

LS_API bool tube_recv(tube *t,
                      spud_message_t *msg,
                      const struct sockaddr* addr,
                      ls_err *err);
