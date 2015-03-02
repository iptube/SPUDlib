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
#include <netinet/in.h>

#include "../config.h"
#include "tube.h"
#include "ls_sockaddr.h"

LS_API bool tube_create(int sock, tube *t, ls_err *err)
{
    assert(t != NULL);
    *t = (tube)ls_data_malloc(sizeof(**t));
    if (*t == NULL) {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        return false;
    }
    memset(*t, 0, sizeof(**t));
    (*t)->sock = sock;
    (*t)->state = TS_UNKNOWN;
    return true;
}

LS_API void tube_destroy(tube t)
{
    if (t->state == TS_RUNNING) {
        tube_close(t, NULL); // ignore error for now
    }
    ls_data_free(t);
}

LS_API bool tube_print(const tube t, ls_err *err)
{
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];

    assert(t!=NULL);
    if (getsockname(t->sock, (struct sockaddr *)&addr, &addr_len) != 0) {
        LS_ERROR(err, -errno);
        return false;
    }

    if (getnameinfo((struct sockaddr *)&addr, addr_len,
                    host, sizeof(host),
                    serv, sizeof(serv),
                    NI_NUMERICHOST|NI_NUMERICSERV) != 0) {
        LS_ERROR(err, -errno);
        return false;
    }

    if (printf("[%s]:%s\n", host, serv) < 0) {
        LS_ERROR(err, -errno);
        return false;
    }
    return true;
}

LS_API bool tube_send(tube t,
                      spud_command_t cmd,
                      bool adec, bool pdec,
                      uint8_t **data, size_t *len,
                      int num,
                      ls_err *err)
{
    spud_header_t smh;
    struct msghdr msg;
    struct iovec iov[num+1];
    int i, count;

    assert(t!=NULL);
    if (!spud_init(&smh, &t->id, err)) {
        return false;
    }
    smh.flags = 0;
    smh.flags |= cmd;
    if (adec) {
        smh.flags |= SPUD_ADEC;
    }
    if (pdec) {
        smh.flags |= SPUD_PDEC;
    }

    count = 0;
    iov[count].iov_base = &smh;
    iov[count].iov_len  = sizeof(smh);
    for (i=0, count=1; i<num; i++) {
        if (len[i] > 0) {
            iov[count].iov_base = data[i];
            iov[count].iov_len = len[i];
            count++;
        }
    }

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &t->peer;
    msg.msg_namelen = ls_sockaddr_get_length((struct sockaddr *)&t->peer);
    msg.msg_iov = iov;
    msg.msg_iovlen = count;

    if (sendmsg(t->sock, &msg, 0) <= 0) {
        LS_ERROR(err, -errno)
        return false;
    }
    return true;
}

LS_API bool tube_open(tube t, const struct sockaddr *dest, ls_err *err)
{
    assert(t!=NULL);
    assert(dest!=NULL);
    memcpy(&t->peer, dest, ls_sockaddr_get_length(dest));
    if (!spud_createId(&t->id, err)) {
        return false;
    }
    t->state = TS_OPENING;
    return tube_send(t, SPUD_OPEN, false, false, NULL, 0, 0, err);
}

LS_API bool tube_ack(tube t,
                     const spud_tube_id_t *id,
                     const struct sockaddr *dest,
                     ls_err *err)
{
    assert(t!=NULL);
    assert(id!=NULL);
    assert(dest!=NULL);

    spud_copyId(id, &t->id, err);

    memcpy(&t->peer, dest, ls_sockaddr_get_length(dest));
    t->state = TS_RUNNING;
    return tube_send(t, SPUD_ACK, false, false, NULL, 0, 0, err);
}

LS_API bool tube_data(tube t, uint8_t *data, size_t len, ls_err *err)
{
    uint8_t *d[2];
    size_t l[2];
    if (len == 0) {
        return tube_send(t, SPUD_DATA, false, false, NULL, 0, 0, err);
    }
    if (len < 24) {
        uint8_t cbor[] = { 0xA1, 00, 0x40 | len };
        d[0] = cbor;
        l[0] = sizeof(cbor);
    } else if (len < 0x100) {
        uint8_t cbor[] = { 0xA1, 00, 0x58, len };
        d[0] = cbor;
        l[0] = sizeof(cbor);
    } else if (len < 0x10000) {
        uint8_t cbor[] = { 0xA1, 00, 0x59, (len & 0xFF00) >> 8, len&0xFF };
        d[0] = cbor;
        l[0] = sizeof(cbor);
    }
    d[1] = data;
    l[1] = len;
    return tube_send(t, SPUD_DATA, false, false, d, l, 2, err);
}

LS_API bool tube_close(tube t, ls_err *err)
{
    t->state = TS_UNKNOWN;
    return tube_send(t, SPUD_CLOSE, false, false, NULL, 0, 0, err);
}

LS_API bool tube_recv(tube t,
                      spud_message_t *msg,
                      const struct sockaddr* addr,
                      ls_err *err)
{
    spud_command_t cmd;
    assert(t!=NULL);
    assert(msg!=NULL);

    if (t->state == TS_START) {
        LS_ERROR(err, LS_ERR_INVALID_STATE);
        return false;
    }

    cmd = msg->header->flags & SPUD_COMMAND;
    switch(cmd) {
    case SPUD_DATA:
        if (t->state == TS_RUNNING) {
            if (t->data_cb) {
                t->data_cb(t, msg->cbor, addr);
            }
        }
        break;
    case SPUD_CLOSE:
        if (t->state != TS_UNKNOWN) {
            // double-close is a no-op
            t->state = TS_UNKNOWN;
            if (t->close_cb) {
                t->close_cb(t, addr);
            }
            memset(&t->peer, 0, sizeof(t->peer));
            // leave id in place to allow for reconnects later
        }
        break;
    case SPUD_OPEN:
        // TODO: check if we're in server policy
        return tube_ack(t,
                        &msg->header->tube_id,
                        addr,
                        err);
    case SPUD_ACK:
        if (t->state == TS_OPENING) {
            t->state = TS_RUNNING;
            if (t->running_cb) {
                t->running_cb(t, addr);
            }
        }
        break;
    }

    return true;
}
