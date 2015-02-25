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
  TS_INITIALIZED,
  TS_LISTENING,
  TS_SERVER,
  TS_CLIENT,
  TS_OPEN
} tube_states_t;

struct _tube_t;

typedef void (*tube_read_cb)(struct _tube_t* tube,
                             ssize_t nread,
                             uint8_t *buf,
                             const struct sockaddr* addr);

typedef struct _tube_t {
  tube_states_t state;
  int sock;
  tube_read_cb cb;
  struct sockaddr_storage peer;
  struct SpudMsgFlagsId id;
} tube_t;

// multiple tubes per socket
int tube_init(tube_t *tube, int sock);

// print [local address]:port to stdout
int tube_print(const tube_t *tube);
int tube_listen(tube_t *tube, tube_read_cb recv_cb);
int tube_open(tube_t *tube, const struct sockaddr *dest);
int tube_ack(tube_t *tube,
             const struct SpudMsgFlagsId *id,
             const struct sockaddr *dest);
int tube_data(tube_t *tube, uint8_t *data, size_t len);
int tube_close(tube_t *tube);

int tube_send(tube_t *tube,
              spud_command_t cmd,
              bool adec, bool pdec,
              uint8_t *data, size_t len);
