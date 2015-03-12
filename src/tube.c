/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include "../config.h"
#include "tube.h"
#include "ls_log.h"
#include "ls_sockaddr.h"
#include "cn-cbor/cn-encoder.h"

#define DEFAULT_HASH_SIZE 65521
#define MAXBUFLEN 1500

static ls_event *_get_or_create_event(ls_event_dispatcher *dispatcher,
                                      const char *name,
                                      ls_err *err)
{
    ls_event *ev = ls_event_dispatcher_get_event(dispatcher, name);
    if (ev) {
        return ev;
    }
    if (!ls_event_dispatcher_create_event(dispatcher, name, &ev, err)) {
        return NULL;
    }
    return ev;
}

LS_API bool tube_create(tube_manager *mgr, tube **t, ls_err *err)
{
    tube *ret = NULL;
    assert(t != NULL);
    assert(mgr != NULL);

    ret = ls_data_malloc(sizeof(tube));
    if (ret == NULL) {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        *t = NULL;
        return false;
    }
    memset(ret, 0, sizeof(tube));
    ret->state = TS_UNKNOWN;
    ret->mgr = mgr;
    *t = ret;
    return true;
}

LS_API void tube_destroy(tube *t)
{
    if (t->state == TS_RUNNING) {
        tube_close(t, NULL); /* ignore error for now */
    }
    ls_data_free(t);
}

LS_API bool tube_print(const tube *t, ls_err *err)
{
  // TODO: output peer and ID as well.
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];

    assert(t!=NULL);
    if (getsockname(t->mgr->sock, (struct sockaddr *)&addr, &addr_len) != 0) {
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

LS_API bool tube_send(tube *t,
                      spud_command cmd,
                      bool adec, bool pdec,
                      uint8_t **data, size_t *len,
                      int num,
                      ls_err *err)
{
    spud_header smh;
    struct msghdr msg;
    int i, count;
    struct iovec *iov;

    assert(t!=NULL);
    iov = calloc(num+1, sizeof(struct iovec));
    if (!iov) {
      LS_ERROR(err, LS_ERR_NO_MEMORY);
      return false;
    }
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

    if (sendmsg(t->mgr->sock, &msg, 0) <= 0) {
        LS_ERROR(err, -errno)
        free(iov);
        return false;
    }
    free(iov);
    return true;
}

LS_API bool tube_open(tube *t, const struct sockaddr *dest, ls_err *err)
{
    assert(t!=NULL);
    assert(dest!=NULL);
    memcpy(&t->peer, dest, ls_sockaddr_get_length(dest));
    if (!spud_create_id(&t->id, err)) {
        return false;
    }
    if (!tube_manager_add(t->mgr, t, err)) {
      return false;
    }
    t->state = TS_OPENING;
    return tube_send(t, SPUD_OPEN, false, false, NULL, 0, 0, err);
}

LS_API bool tube_ack(tube *t,
                     ls_err *err)
{
    assert(t!=NULL);

    t->state = TS_RUNNING;
    return tube_send(t, SPUD_ACK, false, false, NULL, 0, 0, err);
}

LS_API bool tube_data(tube *t, uint8_t *data, size_t len, ls_err *err)
{
    uint8_t preamble[13];
    uint8_t *d[2];
    size_t l[2];
    ssize_t sz = 0;
    ssize_t count = 0;

    if (len == 0) {
        return tube_send(t, SPUD_DATA, false, false, NULL, 0, 0, err);
    }

    /* Map with one pair */
    sz = cbor_encoder_write_head(preamble,
                                 count,
                                 sizeof(preamble),
                                 CN_CBOR_MAP,
                                 1);
    if (sz < 0) {
      LS_ERROR(err, LS_ERR_OVERFLOW);
      return false;
    }
    count += sz;

    /* Key 0 */
    sz = cbor_encoder_write_head(preamble,
                                 count,
                                 sizeof(preamble),
                                 CN_CBOR_UINT,
                                 0);
    if (sz < 0) {
      LS_ERROR(err, LS_ERR_OVERFLOW);
      return false;
    }
    count += sz;

    /* the data */
    sz = cbor_encoder_write_head(preamble,
                                 count,
                                 sizeof(preamble),
                                 CN_CBOR_BYTES,
                                 len);
    if (sz < 0) {
      LS_ERROR(err, LS_ERR_OVERFLOW);
      return false;
    }
    count += sz;

    d[0] = preamble;
    l[0] = count;
    d[1] = data;
    l[1] = len;
    return tube_send(t, SPUD_DATA, false, false, d, l, 2, err);
}

LS_API bool tube_close(tube *t, ls_err *err)
{
    t->state = TS_UNKNOWN;
    if (!tube_send(t, SPUD_CLOSE, false, false, NULL, 0, 0, err)) {
      return false;
    }
    tube_manager_remove(t->mgr, t);
    return true;
}

static unsigned int hash_id(const void *id) {
    // treat the 8 bytes of tube ID like a long long.
    uint64_t key = *(uint64_t *)id;

    // from
    // https://gist.github.com/badboy/6267743#64-bit-to-32-bit-hash-functions
    key = (~key) + (key << 18);
    key = key ^ (key >> 31);
    key = key * 21;
    key = key ^ (key >> 11);
    key = key + (key << 6);
    key = key ^ (key >> 22);
    return (unsigned int) key;
}

static int compare_id(const void *key1, const void *key2) {
    int ret = 0;
    uint64_t k1 = *(uint64_t *)key1;
    uint64_t k2 = *(uint64_t *)key2;
    if (k1<k2) {
        ret = -1;
    } else {
        ret = (k1==k2) ? 0 : 1;
    }
    return ret;
}

LS_API bool tube_manager_create(int buckets,
                                tube_manager **m,
                                ls_err *err)
{
    tube_manager *ret = NULL;
    assert(m != NULL);
    ret = ls_data_malloc(sizeof(tube_manager));
    if (ret == NULL) {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        return false;
    }
    memset(ret, 0, sizeof(tube_manager));
    ret->sock = -1;
    ret->keep_going = false;

    if (buckets <= 0) {
        buckets = DEFAULT_HASH_SIZE;
    }
    if (!ls_htable_create(buckets, hash_id, compare_id, &ret->tubes, err)) {
        goto cleanup;
    }

    if (!ls_event_dispatcher_create(ret, &ret->dispatcher, err)) {
        goto cleanup;
    }

    if ((ret->e_running = _get_or_create_event(ret->dispatcher, EV_RUNNING_NAME, err)) == NULL) {
        goto cleanup;
    }
    if ((ret->e_data = _get_or_create_event(ret->dispatcher, EV_DATA_NAME, err)) == NULL) {
        goto cleanup;
    }
    if ((ret->e_close = _get_or_create_event(ret->dispatcher, EV_CLOSE_NAME, err)) == NULL) {
        goto cleanup;
    }
    if ((ret->e_add = _get_or_create_event(ret->dispatcher, EV_ADD_NAME, err)) == NULL) {
        goto cleanup;
    }
    if ((ret->e_remove = _get_or_create_event(ret->dispatcher, EV_REMOVE_NAME, err)) == NULL) {
        goto cleanup;
    }

    *m = ret;
    return true;
cleanup:
    tube_manager_destroy(ret);
    *m = NULL;
    return false;
}

LS_API void tube_manager_destroy(tube_manager *mgr) {
    assert(mgr);

    mgr->keep_going = false;
    if (mgr->tubes) {
        ls_htable_destroy(mgr->tubes); // will clean
        mgr->tubes = NULL;
    }
    if (mgr->dispatcher) {
        ls_event_dispatcher_destroy(mgr->dispatcher);
        mgr->dispatcher = NULL;
    }
    ls_data_free(mgr);
}

LS_API bool tube_manager_socket(tube_manager *m,
                                int port,
                                ls_err *err)
{
    struct sockaddr_in6 addr;
    assert(m);
    assert(m->sock < 0);
    assert(port>=0);
    assert(port<=0xffff);

    if (port != 0) {
        m->policy |= TP_SERVER;
    }

    m->sock = socket(PF_INET6, SOCK_DGRAM, 0);
    if (m->sock < 0) {
        LS_ERROR(err, -errno);
        return false;
    }
    ls_sockaddr_v6_any(&addr, port);
    if (bind(m->sock,
             (struct sockaddr*)&addr,
             ls_sockaddr_get_length((struct sockaddr*)&addr)) != 0) {
        LS_ERROR(err, -errno);
        return false;
    }
    m->keep_going = true;
    return true;
}

LS_API bool tube_manager_bind_event(tube_manager *mgr,
                                    const char *name,
                                    ls_event_notify_callback cb,
                                    ls_err *err)
{
    ls_event *ev;
    assert(mgr);
    assert(mgr->dispatcher);
    assert(name);
    assert(cb);

    ev = ls_event_dispatcher_get_event(mgr->dispatcher, name);
    if (!ev) {
        LS_ERROR(err, LS_ERR_BAD_FORMAT);
        return false;
    }

    return ls_event_bind(ev, cb, mgr, err);
}

static void clean_tube(bool replace, bool destroy_key, void *key, void *data)
{
    tube *t = data;
    ls_err err;
    UNUSED_PARAM(replace);
    UNUSED_PARAM(destroy_key);
    UNUSED_PARAM(key);

    if (!ls_event_trigger(t->mgr->e_remove, t, NULL, NULL, &err)) {
        LS_LOG_ERR(err, "ls_event_trigger");
    }
    tube_destroy(t);
}

LS_API bool tube_manager_add(tube_manager *mgr,
                             tube *t,
                             ls_err *err)
{
    assert(mgr);
    assert(t);
    if (!ls_htable_put(mgr->tubes, &t->id, t, clean_tube, err)) {
      return false;
    }
    return ls_event_trigger(mgr->e_add, t, NULL, NULL, err);
}

LS_API void tube_manager_remove(tube_manager *mgr,
                                tube *t)
{
    assert(mgr);
    assert(t);
    /* Fires remove event as a side-effect */
    ls_htable_remove(mgr->tubes, &t->id);
}

LS_API bool tube_manager_loop(tube_manager *mgr, ls_err *err)
{
    struct sockaddr_storage their_addr;
    uint8_t buf[MAXBUFLEN];
    char id_str[SPUD_ID_STRING_SIZE+1];
    socklen_t addr_len = sizeof(their_addr);
    int numbytes;
    spud_message msg = {NULL, NULL};
    spud_tube_id uid;
    spud_command cmd;
    tube_event_data d;

    assert(mgr);
    assert(mgr->sock >= 0);

    while (mgr->keep_going) {
        addr_len = sizeof(their_addr);
        if ((numbytes = recvfrom(mgr->sock, buf,
                                 MAXBUFLEN , 0,
                                 (struct sockaddr *)&their_addr,
                                 &addr_len)) == -1) {
            if (errno == EINTR) {
                continue;
            }
            /* unrecoverable */
            LS_ERROR(err, -errno);
            goto error;
        }

        if (!spud_parse(buf, numbytes, &msg, err)) {
            // it's an attack.  Move along.
            LS_LOG_ERR(*err, "spud_parse");
            goto cleanup;
        }

        if (!spud_copy_id(&msg.header->tube_id, &uid, err)) {
            LS_LOG_ERR(*err, "spud_copy_id");
            goto cleanup;
        }

        cmd    = msg.header->flags & SPUD_COMMAND;
        d.t    = ls_htable_get(mgr->tubes, &uid);
        d.cbor = msg.cbor;
        d.addr = (const struct sockaddr *)&their_addr;
        if (!d.t) {
            if (!(mgr->policy & TP_SERVER) || (cmd != SPUD_OPEN)) {
              // Not for one of our tubes, and we're not a server, so punt.
              // Even if we're a server, if we get anything but an open
              // for an unknown tube, ignore it.
              ls_log(LS_LOG_WARN, "Invalid tube ID: %s",
                     spud_id_to_string(id_str, sizeof(id_str), &uid, NULL));
              goto cleanup;
            }

            // get started
            if (!tube_create(mgr, &d.t, err)) {
                // probably out of memory
                // TODO: replace with an unused queue
                goto error;
            }

            spud_copy_id(&uid, &d.t->id, err);
            ls_sockaddr_copy((const struct sockaddr *)&their_addr,
                             (struct sockaddr *)&d.t->peer);
            if (!tube_manager_add(mgr, d.t, err)) {
                goto error;
            }
            if (!tube_ack(d.t, err)) {
                goto cleanup;
            }
        }

        switch(cmd) {
        case SPUD_DATA:
            if (d.t->state == TS_RUNNING) {
                if (!ls_event_trigger(mgr->e_data, &d, NULL, NULL, err)) {
                    goto error;
                }
            }
            break;
        case SPUD_CLOSE:
            if (d.t->state != TS_UNKNOWN) {
                /* double-close is a no-op */
                d.t->state = TS_UNKNOWN;
                if (!ls_event_trigger(mgr->e_close, &d, NULL, NULL, err)) {
                    goto error;
                }
                tube_manager_remove(mgr, d.t);
            }
            break;
        case SPUD_OPEN:
            /* Double open.  no-op. */
            break;
        case SPUD_ACK:
            if (d.t->state == TS_OPENING) {
                d.t->state = TS_RUNNING;
                if (!ls_event_trigger(mgr->e_running, &d, NULL, NULL, err)) {
                    goto error;
                }
            }
            break;
        }
cleanup:
        spud_unparse(&msg);
    }
    return true;
error:
    return false;
}
