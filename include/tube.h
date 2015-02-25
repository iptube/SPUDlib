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

#include "spudlib.h"

typedef enum {
  TS_START,
  TS_UNKNOWN,
  TS_OPENING,
  TS_RUNNING,
  TS_RESUMING
} tube_states_t;

struct _tube_t;

typedef void (*tube_data_cb)(struct _tube_t* tube,
                             const uint8_t *data,
                             ssize_t length,
                             const struct sockaddr* addr);

typedef void (*tube_state_cb)(struct _tube_t* tube,
                              const struct sockaddr* addr);

typedef struct _tube_t {
  tube_states_t state;
  int sock;
  struct sockaddr_storage peer;
  struct SpudMsgFlagsId id;
  void *data;
  tube_data_cb data_cb;
  tube_state_cb running_cb;
  tube_state_cb close_cb;
} tube_t;

// multiple tubes per socket
bool tube_init(tube_t *tube, int sock);

// print [local address]:port to stdout
bool tube_print(const tube_t *tube);
bool tube_open(tube_t *tube, const struct sockaddr *dest);
bool tube_ack(tube_t *tube,
              const struct SpudMsgFlagsId *id,
              const struct sockaddr *dest);
bool tube_data(tube_t *tube, uint8_t *data, size_t len);
bool tube_close(tube_t *tube);

bool tube_send(tube_t *tube,
               spud_command_t cmd,
               bool adec, bool pdec,
               uint8_t *data, size_t len);

bool tube_recv(tube_t *tube, struct SpudMsg *msg, const struct sockaddr* addr);
