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

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#include "../config.h"
#include "tube.h"

bool tube_init(tube_t *tube, int sock)
{
    assert(tube!=NULL);
    memset(tube, 0, sizeof(tube_t));
    tube->sock = sock;
    tube->state = TS_UNKNOWN;
    return true;
}

bool tube_print(const tube_t *tube)
{
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];

    assert(tube!=NULL);
    if (getsockname(tube->sock, (struct sockaddr *)&addr, &addr_len) != 0) {
        perror("getsockname");
        return false;
    }

    if (getnameinfo((struct sockaddr *)&addr, addr_len,
                    host, sizeof(host),
                    serv, sizeof(serv),
                    NI_NUMERICHOST|NI_NUMERICSERV) != 0) {
        perror("getnameinfo");
        return false;
    }

    printf("[%s]:%s\n", host, serv);
    return true;
}

bool tube_send(tube_t *tube,
               spud_command_t cmd,
               bool adec, bool pdec,
               uint8_t *data, size_t len)
{
    spud_header_t smh;
    uint8_t flags = 0;
    struct msghdr msg;
    struct iovec iov[2];

    assert(tube!=NULL);
    if (!spud_init(&smh, &tube->id)) {
        return false;
    }
    flags |= cmd;
    if (adec) {
        flags |= SPUD_ADEC;
    }
    if (pdec) {
        flags |= SPUD_PDEC;
    }
    smh.flags_id.octet[0] |= flags;

    iov[0].iov_base = &smh;
    iov[0].iov_len  = sizeof(smh);
    iov[1].iov_base = data;
    iov[1].iov_len  = len;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &tube->peer;
    msg.msg_namelen = tube->peer.ss_len;
    msg.msg_iov = iov;
    msg.msg_iovlen = (data==NULL) ? 1 : 2;

    int ret = sendmsg(tube->sock, &msg, 0);
    if (ret <= 0) {
        perror("sendmsg");
        printf("ret: %d\n", ret);
        return false;
    }
    return true;
}

bool tube_open(tube_t *tube, const struct sockaddr *dest)
{
    assert(tube!=NULL);
    assert(dest!=NULL);
    memcpy(&tube->peer, dest, dest->sa_len);
    if (!spud_createId(&tube->id)) {
        return false;
    }
    tube->state = TS_OPENING;
    return tube_send(tube, SPUD_OPEN, false, false, NULL, 0);
}

bool tube_ack(tube_t *tube,
              const spud_flags_id_t *id,
              const struct sockaddr *dest)
{
    assert(tube!=NULL);
    assert(id!=NULL);
    assert(dest!=NULL);

    memcpy(&tube->id, id, sizeof(spud_flags_id_t));
    tube->id.octet[0] &= SPUD_FLAGS_EXCLUDE_MASK;

    memcpy(&tube->peer, dest, dest->sa_len);
    tube->state = TS_RUNNING;
    return tube_send(tube, SPUD_ACK, false, false, NULL, 0);
}

bool tube_data(tube_t *tube, uint8_t *data, size_t len)
{
    return tube_send(tube, SPUD_DATA, false, false, data, len);
}

bool tube_close(tube_t *tube)
{
    tube->state = TS_UNKNOWN;
    return tube_send(tube, SPUD_CLOSE, false, false, NULL, 0);
}

bool tube_recv(tube_t *tube, spud_message_t *msg, const struct sockaddr* addr)
{
    spud_command_t cmd;
    assert(tube!=NULL);
    assert(msg!=NULL);

    if (tube->state == TS_START) {
        fprintf(stderr, "Invalid state\n");
        return false;
    }

    cmd = msg->header->flags_id.octet[0] & SPUD_COMMAND;
    switch(cmd) {
    case SPUD_DATA:
        if (tube->state == TS_RUNNING) {
            if (tube->data_cb) {
                tube->data_cb(tube, msg->data, msg->length, addr);
            }
        }
        break;
    case SPUD_CLOSE:
        if (tube->state != TS_UNKNOWN) {
            // double-close is a no-op
            if (tube->close_cb) {
                tube->close_cb(tube, addr);
            }
            tube->state = TS_UNKNOWN;
            memset(&tube->peer, 0, sizeof(tube->peer));
            // leave id in place to allow for reconnects later
        }
        break;
    case SPUD_OPEN:
        // TODO: check if we're in server policy
        return tube_ack(tube,
                        &msg->header->flags_id,
                        addr);
    case SPUD_ACK:
        if (tube->state == TS_OPENING) {
            tube->state = TS_RUNNING;
            if (tube->running_cb) {
                tube->running_cb(tube, addr);
            }
        }
        break;
    }

    return true;
}
