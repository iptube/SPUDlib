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
#include "cn-encoder.h"

static ls_event _get_or_create_event(ls_event_dispatcher dispatcher,
                                     const char *name,
                                     ls_err *err)
{
    ls_event ev = ls_event_dispatcher_get_event(dispatcher, "open");
    if (ev) {
        return ev;
    }
    if (!ls_event_dispatcher_create_event(dispatcher, name, &ev, err)) {
        return NULL;
    }
    return ev;
}

LS_API bool tube_bind_events(ls_event_dispatcher dispatcher,
                             ls_event_notify_callback running_cb,
                             ls_event_notify_callback data_cb,
                             ls_event_notify_callback close_cb,
                             void *arg,
                             ls_err *err)
{
    ls_event ev;
    if ((ev = _get_or_create_event(dispatcher, "running", err)) == NULL) {
        return false;
    }
    if (running_cb) {
      if (!ls_event_bind(ev, running_cb, arg, err)) {
        return false;
      }
    }
    if ((ev = _get_or_create_event(dispatcher, "data", err)) == NULL) {
        return false;
    }
    if (data_cb) {
      if (!ls_event_bind(ev, data_cb, arg, err)) {
        return false;
      }
    }
    if ((ev = _get_or_create_event(dispatcher, "close", err)) == NULL) {
        return false;
    }
    if (close_cb) {
      if (!ls_event_bind(ev, close_cb, arg, err)) {
        return false;
      }
    }
    return true;
}

LS_API bool tube_create(int sock, ls_event_dispatcher dispatcher, tube *t, ls_err *err)
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
    if (dispatcher) {
        (*t)->my_dispatcher = false;
        (*t)->dispatcher = dispatcher;
    } else {
        (*t)->my_dispatcher = true;
        if (!ls_event_dispatcher_create(*t, &(*t)->dispatcher, err)) {
            goto error;
        }
        if (!tube_bind_events((*t)->dispatcher, NULL, NULL, NULL, NULL, err)) {
          goto error;
        }
    }
    return true;
error:
    tube_destroy(*t);
    *t = NULL;
    return false;
}

LS_API void tube_destroy(tube t)
{
    if (t->state == TS_RUNNING) {
        tube_close(t, NULL); // ignore error for now
    }
    if (t->my_dispatcher && t->dispatcher) {
        ls_event_dispatcher_destroy(t->dispatcher);
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
    uint8_t preamble[11];
    uint8_t *d[] = {preamble, data};
    size_t l[2];
    ssize_t sz;

    if (len == 0) {
        return tube_send(t, SPUD_DATA, false, false, NULL, 0, 0, err);
    }

    d[0] = preamble;
    sz = cbor_encoder_write_head(preamble,
                                 0,
                                 sizeof(preamble),
                                 CN_CBOR_BYTES,
                                 len);
    if (sz < 0) {
      LS_ERROR(err, LS_ERR_OVERFLOW);
      return false;
    }
    l[0] = sz;
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
            tubeData d = {t, msg->cbor, addr};
            if (!ls_event_trigger(t->e_data, &d, NULL, NULL, err)) {
                return false;
            }
        }
        break;
    case SPUD_CLOSE:
        if (t->state != TS_UNKNOWN) {
            // double-close is a no-op
            t->state = TS_UNKNOWN;
            tubeData d = {t, msg->cbor, addr};
            if (!ls_event_trigger(t->e_close, &d, NULL, NULL, err)) {
                return false;
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
            tubeData d = {t, msg->cbor, addr};
            if (!ls_event_trigger(t->e_running, &d, NULL, NULL, err)) {
                return false;
            }
        }
        break;
    }

    return true;
}
