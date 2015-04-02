/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#ifdef __APPLE__
#define __APPLE_USE_RFC_3542
#endif

#ifdef linux
/* needed for netinet/in.h */
#define _GNU_SOURCE 1
#endif

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "config.h"
#include "tube.h"
#include "ls_eventing.h"
#include "ls_htable.h"
#include "ls_log.h"
#include "ls_sockaddr.h"

#define DEFAULT_HASH_SIZE 65521
#define MAXBUFLEN 1500

static tube_sendmsg_func _sendmsg_func = sendmsg;
static tube_recvmsg_func _recvmsg_func = recvmsg;

struct _tube_manager
{
  int sock;
  ls_htable *tubes;
  ls_event_dispatcher *dispatcher;
  ls_event *e_running;
  ls_event *e_data;
  ls_event *e_close;
  ls_event *e_add;
  ls_event *e_remove;
  tube_policies policy;
  bool keep_going;
};

struct _tube
{
  tube_states_t state;
  struct sockaddr_storage peer;
  struct in6_pktinfo local;
  spud_tube_id id;
  void *data;
  tube_manager *mgr;
  bool has_local;
};

LS_API bool tube_create(tube_manager *mgr, tube **t, ls_err *err)
{
    tube *ret = NULL;
    assert(t != NULL);
    assert(mgr != NULL);

    ret = ls_data_calloc(1, sizeof(tube));
    if (ret == NULL) {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        *t = NULL;
        return false;
    }
    ret->state = TS_UNKNOWN;
    ret->mgr = mgr;
    *t = ret;
    return true;
}

LS_API void tube_destroy(tube *t)
{
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

LS_API void tube_set_local(tube *t, struct in6_addr *addr)
{
    assert(t);
    if (addr)
    {
        t->has_local = true;
        t->local.ipi6_ifindex = -1; // TODO: how do we find this??
        t->local.ipi6_addr = *addr;
    }
    else
    {
        t->has_local = false;
        t->local.ipi6_ifindex = -1;
        t->local.ipi6_addr = in6addr_any;
    }

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
    char msg_control[1024];

    assert(t!=NULL);
    iov = ls_data_calloc(num+1, sizeof(struct iovec));
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

    if (t->has_local) {
        struct cmsghdr* cmsg;
        struct in6_pktinfo* ipi;
        msg.msg_control = msg_control;

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = IPPROTO_IPV6;
        cmsg->cmsg_type = IPV6_PKTINFO;
        cmsg->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
        ipi = (struct in6_pktinfo*)CMSG_DATA(cmsg);
        *ipi = t->local;
        msg.msg_controllen = CMSG_SPACE(sizeof(struct in6_pktinfo));
    }

    if (_sendmsg_func(t->mgr->sock, &msg, 0) <= 0) {
        LS_ERROR(err, -errno)
        ls_data_free(iov);
        return false;
    }
    ls_data_free(iov);
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
                     spud_tube_id *id,
                     const struct sockaddr *peer,
                     ls_err *err)
{
    assert(t!=NULL);

    spud_copy_id(id, &t->id);
    ls_sockaddr_copy(peer, (struct sockaddr *)&t->peer);
    if (!tube_manager_add(t->mgr, t, err)) {
        return false;
    }

    t->state = TS_RUNNING;
    return tube_send(t, SPUD_ACK, false, false, NULL, 0, 0, err);
}

LS_API void path_create_mandatory_keys(cn_cbor **cbor, uint8_t *ipadress, size_t iplen, uint8_t *token, size_t tokenlen, char* url)
{
    //TODO: no error checking, use functions which fill these propoerly and do error checking
    // it works at the moment though
    //TODO: rewrite using newly imlemented CBOR functions
    cn_cbor *ret =NULL;
    const char * SPUD_IPADDR = "ipaddr";
    const char * SPUD_TOKEN = "token";
    const char * SPUD_URL = "url";


    ret=ls_data_malloc(sizeof(cn_cbor)*20); //TODO: free
    ret[0].type = CN_CBOR_MAP;
    ret[0].flags = CN_CBOR_FL_COUNT;
    ret[0].length = 6;
    ret[0].first_child=&(ret[1]);

   /*"ipaddr" (byte string, major type 2)  the IPv4 address or IPv6
      address of the sender, as a string of 4 or 16 bytes in network
      order.  This is necessary as the source IP address of the packet
      is spoofed    */


    ret[1].type = CN_CBOR_TEXT;
    ret[1].flags = CN_CBOR_FL_COUNT;
    ret[1].v.str = SPUD_IPADDR;
    ret[1].length = strlen(SPUD_IPADDR);
    ret[1].next=&(ret[2]);

    ret[2].type=CN_CBOR_BYTES;
    ret[2].flags = CN_CBOR_FL_COUNT;
    ret[2].v.str=(char*)ipadress;
    ret[2].length = iplen;
    ret[2].next=&(ret[3]);

    /*
       "token" (byte string, major type 2)  data that identifies the sending
      path element unambiguously

    */
    ret[3].type = CN_CBOR_TEXT;
    ret[3].flags = CN_CBOR_FL_COUNT;
    ret[3].v.str = SPUD_TOKEN;
    ret[3].length = strlen(SPUD_TOKEN);
    ret[3].next=&(ret[4]);

    ret[4].type=CN_CBOR_BYTES;
    ret[4].flags = CN_CBOR_FL_COUNT;
    ret[4].v.str=(char*)token;
    ret[4].length = tokenlen;
    ret[4].next=&(ret[5]);
    /*
   "url" (text string, major type 3)  a URL identifying some information
      about the path or its relationship with the tube.  The URL
      represents some path condition, and retrieval of content at the
      URL should include a human-readable description.
    */
    ret[5].type = CN_CBOR_TEXT;
    ret[5].flags = CN_CBOR_FL_COUNT;
    ret[5].v.str = SPUD_URL;
    ret[5].length = strlen(SPUD_URL);
    ret[5].next=&(ret[6]);

    ret[6].type=CN_CBOR_TEXT;
    ret[6].flags = CN_CBOR_FL_COUNT;
    ret[6].v.str= url;
    ret[6].length = strlen(url);
    ret[6].next=NULL;


    *cbor= ret;
}

LS_API bool tube_send_pdec(tube *t, cn_cbor *cbor, bool reflect, ls_err *err)
{
    return tube_send_cbor(t, SPUD_DATA, reflect, true, cbor, err);
}

LS_API bool tube_send_cbor(tube *t, spud_command cmd, bool adec, bool pdec, cn_cbor *cbor, ls_err *err)
{
    uint8_t buf[MAXBUFLEN];
    ssize_t sz = 0;
    uint8_t *d[1];
    size_t l[1];

    assert(t);

    sz=cbor_encoder_write(buf, 0, MAXBUFLEN, cbor);
    if (sz < 0) {
      LS_ERROR(err, LS_ERR_OVERFLOW);
      return false;
    }

    d[0] = buf;
    l[0] = sz;
    return tube_send(t, cmd, adec, pdec, d, l, 2, err);
}

static void *_tube_calloc(size_t count, size_t size, void *context)
{
    ls_pool *pool = context;
    ls_err err;
    void *ptr;

    if (!ls_pool_calloc(pool, count, size, &ptr, &err)) {
        LS_LOG_ERR(err, "ls_pool_calloc");
        return NULL;
    }
    return ptr;
}

static void _tube_free(void *ptr, void *context)
{
    UNUSED_PARAM(ptr);
    UNUSED_PARAM(context);
    return;
}

LS_API bool tube_data(tube *t, uint8_t *data, size_t len, ls_err *err)
{
    // max size for CBOR preamble 19 bytes:
    // 1(map|27) 8(length) 1(key:0) 1(bstr|27) 8(length)
    //uint8_t preamble[19];
    ls_pool *pool;
    cn_cbor *map;
    cn_cbor *cdata;
    bool ret = false;
    cn_cbor_context ctx = { _tube_calloc, _tube_free, NULL };

    assert(t);
    if (len == 0) {
        return tube_send(t, SPUD_DATA, false, false, NULL, 0, 0, err);
    }

    if (!ls_pool_create(128, &pool, err)) {
        return false;
    }
    ctx.context = pool;

    // TODO: the whole point of the iov system is so that we don't have to copy
    // the data here.  Which we just did.  Please fix.
    if (!(map = cn_cbor_map_create(&ctx, NULL)) ||
        !(cdata = cn_cbor_data_create(data, len, &ctx, NULL)) ||
        !cn_cbor_mapput_int(map, 0, cdata, &ctx, NULL))
    {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        goto cleanup;
    }
    ret = tube_send_cbor(t, SPUD_DATA, false, false, map, err);

cleanup:
    ls_pool_destroy(pool);
    return ret;
}

LS_API bool tube_close(tube *t, ls_err *err)
{
    assert(t);
    t->state = TS_UNKNOWN;
    if (!tube_send(t, SPUD_CLOSE, false, false, NULL, 0, 0, err)) {
        return false;
    }
    return true;
}

LS_API void tube_set_data(tube *t, void *data)
{
    assert(t);
    t->data = data;
}

LS_API void *tube_get_data(tube *t)
{
    assert(t);
    return t->data;
}

LS_API char *tube_id_to_string(tube *t, char* buf, size_t len)
{
    assert(t);
    return spud_id_to_string(buf, len, &t->id);
}

LS_API tube_states_t tube_get_state(tube *t)
{
    assert(t);
    return t->state;
}

LS_API void tube_get_id(tube *t, spud_tube_id *id)
{
    spud_copy_id(&t->id, id);
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
    ret = ls_data_calloc(1, sizeof(tube_manager));
    if (ret == NULL) {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        return false;
    }
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

    if (!ls_event_dispatcher_create_event(ret->dispatcher,
                                          EV_RUNNING_NAME,
                                          &ret->e_running,
                                          err) ||
        !ls_event_dispatcher_create_event(ret->dispatcher,
                                          EV_DATA_NAME,
                                          &ret->e_data,
                                          err) ||
        !ls_event_dispatcher_create_event(ret->dispatcher,
                                          EV_CLOSE_NAME,
                                          &ret->e_close,
                                          err) ||
        !ls_event_dispatcher_create_event(ret->dispatcher,
                                          EV_ADD_NAME,
                                          &ret->e_add,
                                          err) ||
        !ls_event_dispatcher_create_event(ret->dispatcher,
                                          EV_REMOVE_NAME,
                                          &ret->e_remove,
                                          err)) {
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

    tube_manager_set_policy_responder(m, port != 0);

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

#ifdef IPV6_RECVPKTINFO
    {
        const int on = 1;
        if (setsockopt(m->sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)) != 0) {
            LS_ERROR(err, -errno);
            return false;
        }
    }
#else
    #pragma message "No IPV6_RECVPKTINFO.  Destination addresses won't work."
#endif

    m->keep_going = true;
    return true;
}

LS_API void tube_manager_set_socket(tube_manager *m, int sock)
{
    assert(m);
    assert(m->sock < 0);
    m->sock = sock;
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

    if (t->state == TS_RUNNING) {
        if (!tube_close(t, &err)) {
            LS_LOG_ERR(err, "tube_close");
            // keep going!
        }
    }
    if (!ls_event_trigger(t->mgr->e_remove, t, NULL, NULL, &err)) {
        LS_LOG_ERR(err, "ls_event_trigger");
        // keep going!
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
    struct msghdr hdr;
    struct sockaddr_storage their_addr;
    struct iovec iov[1];
    ssize_t numbytes;
    uint8_t buf[MAXBUFLEN];
    char id_str[SPUD_ID_STRING_SIZE+1];
    spud_message msg = {NULL, NULL};
    spud_tube_id uid;
    spud_command cmd;
    tube_event_data d;
    struct cmsghdr* cmsg;
    uint8_t mctl[1024];
    struct in6_pktinfo *in6_pktinfo;

    assert(mgr);
    assert(mgr->sock >= 0);
    hdr.msg_name = &their_addr;
    hdr.msg_iov = iov;
    hdr.msg_iovlen = 1;
    hdr.msg_control = mctl;

    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf);
    d.peer = (const struct sockaddr *)&their_addr;

    while (mgr->keep_going) {
        hdr.msg_namelen = sizeof(their_addr);
        hdr.msg_controllen = sizeof(mctl);
        hdr.msg_flags = 0;
        if ((numbytes = _recvmsg_func(mgr->sock, &hdr, 0)) == -1) {
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

        spud_copy_id(&msg.header->tube_id, &uid);

        cmd    = msg.header->flags & SPUD_COMMAND;
        d.t    = ls_htable_get(mgr->tubes, &uid);
        d.cbor = msg.cbor;
        if (!d.t) {
            if (!tube_manager_is_responder(mgr) || (cmd != SPUD_OPEN)) {
              // Not for one of our tubes, and we're not a responder, so punt.
              // Even if we're a responder, if we get anything but an open
              // for an unknown tube, ignore it.
              ls_log(LS_LOG_WARN, "Invalid tube ID: %s",
                     spud_id_to_string(id_str, sizeof(id_str), &uid));
              goto cleanup;
            }

            // get started
            if (!tube_create(mgr, &d.t, err)) {
                // probably out of memory
                // TODO: replace with an unused queue
                goto error;
            }

            for (cmsg=CMSG_FIRSTHDR(&hdr); cmsg; cmsg=CMSG_NXTHDR(&hdr, cmsg)) {
                if ((cmsg->cmsg_level == IPPROTO_IPV6) && (cmsg->cmsg_type == IPV6_PKTINFO)) {
                    in6_pktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsg);
                    d.t->local = *in6_pktinfo;
                    d.t->has_local = true;
                }
            }

            if (!tube_ack(d.t, &uid, (const struct sockaddr *)&their_addr, err)) {
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

LS_API bool tube_manager_running(tube_manager *mgr)
{
    assert(mgr);
    return mgr->keep_going;
}

LS_API void tube_manager_stop(tube_manager *mgr)
{
    assert(mgr);
    mgr->keep_going = false;
    // TODO: interrupt?  That would mean knowing about threads.
}

LS_API size_t tube_manager_size(tube_manager *mgr)
{
    assert(mgr);
    return ls_htable_get_count(mgr->tubes);
}

LS_API void tube_manager_set_policy_responder(tube_manager *mgr, bool will_respond)
{
    assert(mgr);
    if (will_respond) {
        mgr->policy |= TP_WILL_RESPOND;
    } else {
        mgr->policy &= ~TP_WILL_RESPOND;
    }
}

LS_API bool tube_manager_is_responder(tube_manager *mgr)
{
    return (mgr->policy & TP_WILL_RESPOND) == TP_WILL_RESPOND;
}

LS_API void tube_set_socket_functions(tube_sendmsg_func send,
                                      tube_recvmsg_func recv)
{
    _sendmsg_func = (send == NULL) ? sendmsg : send;
    _recvmsg_func = (recv == NULL) ? recvmsg : recv;
}
