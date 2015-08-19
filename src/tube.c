/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include "tube.h"

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "config.h"
#include "ls_log.h"
#include "ls_sockaddr.h"
#include "tube_manager.h"

#define MAXBUFLEN 1500

#define HAS_LOCAL_4 (1 << 0)
#define HAS_LOCAL_6 (1 << 1)

struct _tube
{
  tube_states_t           state;
  struct sockaddr_storage peer;
  spud_tube_id            id;
  void*                   data;
  ls_pktinfo*             pktinfo;
  int                     sock;
};

LS_API bool
tube_create(tube**  t,
            ls_err* err)
{
  tube* ret = NULL;
  assert(t != NULL);

  ret = ls_data_calloc( 1, sizeof(tube) );
  if (ret == NULL)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    *t = NULL;
    return false;
  }
  ret->sock  = -1;
  ret->state = TS_UNKNOWN;
  *t         = ret;
  return true;
}

LS_API void
tube_destroy(tube* t)
{
  if (t->pktinfo)
  {
    ls_pktinfo_destroy(t->pktinfo);
  }
  ls_data_free(t);
}

LS_API bool
tube_print(const tube* t,
           ls_err*     err)
{
  struct sockaddr_storage addr;
  socklen_t               addr_len                            = sizeof(addr);
  char                    local[NI_MAXHOST + NI_MAXSERV + 2]  = {0};
  char                    remote[NI_MAXHOST + NI_MAXSERV + 2] = {0};
  char                    id_buf[SPUD_ID_STRING_SIZE + 1]     = {0};

  assert(t != NULL);
  if (t->sock == -1)
  {
    printf("No socket yet\n");
    return true;
  }

  if (getsockname(t->sock, (struct sockaddr*)&addr, &addr_len) != 0)
  {
    LS_ERROR(err, -errno);
    return false;
  }

  if (t->pktinfo)
  {
    int port = ls_sockaddr_get_port( (struct sockaddr*)&addr );

    /* ignore errors. */
    /* TODO: set pktinfo when receiving ACK */
    addr_len = sizeof(addr);
    if ( ls_pktinfo_get_addr(t->pktinfo, (struct sockaddr*)&addr, &addr_len,
                             NULL) )
    {
      ls_sockaddr_set_port( (struct sockaddr*)&addr, port );
    }
  }

  if ( !ls_sockaddr_to_string( (struct sockaddr*)&addr, local, sizeof(local),
                               true ) ||
       !ls_sockaddr_to_string( (struct sockaddr*)&t->peer, remote,
                               sizeof(remote), true ) )
  {
    return false;
  }

  ls_log(LS_LOG_INFO,
         "TUBE %s: %s->%s",
         spud_id_to_string(id_buf, sizeof(id_buf), &t->id),
         local, remote);
  return true;
}

LS_API bool
tube_set_local(tube*       t,
               ls_pktinfo* info,
               ls_err*     err)
{
  assert(t);
  if (!info)
  {
    t->pktinfo = NULL;
    return true;
  }
  return ls_pktinfo_dup(info, &t->pktinfo, err);
}

LS_API bool
tube_send(tube*        t,
          spud_command cmd,
          bool         adec,
          bool         pdec,
          uint8_t**    data,
          size_t*      len,
          int          num,
          ls_err*      err)
{
  spud_header   smh;
  int           i, count;
  struct iovec* iov;
  bool          ret;

  assert(t != NULL);
  iov = ls_data_calloc( num + 1, sizeof(struct iovec) );
  if (!iov)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    return false;
  }
  if ( !spud_init(&smh, &t->id, err) )
  {
    return false;
  }
  smh.flags  = 0;
  smh.flags |= cmd;
  if (adec)
  {
    smh.flags |= SPUD_ADEC;
  }
  if (pdec)
  {
    smh.flags |= SPUD_PDEC;
  }

  count               = 0;
  iov[count].iov_base = &smh;
  iov[count].iov_len  = sizeof(smh);
  count++;
  for (i = 0; i < num; i++)
  {
    if (len[i] > 0)
    {
      iov[count].iov_base = data[i];
      iov[count].iov_len  = len[i];
      count++;
    }
  }
  ret = tube_manager_sendmsg(t->sock,
                             t->pktinfo, (struct sockaddr*)&t->peer,
                             iov, count,
                             err);
  ls_data_free(iov);
  return ret;
}

static void*
_pool_calloc(size_t count,
             size_t size,
             void*  context)
{
  ls_pool* pool = context;
  ls_err   err;
  void*    ptr;

  if ( !ls_pool_calloc(pool, count, size, &ptr, &err) )
  {
    LS_LOG_ERR(err, "ls_pool_calloc");
    return NULL;
  }
  return ptr;
}

static void
_pool_free(void* ptr,
           void* context)
{
  UNUSED_PARAM(ptr);
  UNUSED_PARAM(context);
}

static bool
_map_create(cn_cbor_context* ctx,
            cn_cbor**        map,
            ls_err*          err)
{
  ls_pool* pool;
  cn_cbor* m;

  ctx->calloc_func = _pool_calloc;
  ctx->free_func   = _pool_free;

  /* TODO: add pool High-water mark, and test to see if this is enough */
  if ( !ls_pool_create(128, &pool, err) )
  {
    return false;
  }
  ctx->context = pool;
  m            = cn_cbor_map_create(ctx, NULL);
  if (!m)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    ls_pool_destroy(pool);
    return false;
  }

  *map = m;
  return true;
}

static bool
_mapput_string_data(cn_cbor*         map,
                    const char*      key,
                    uint8_t*         data,
                    size_t           sz,
                    cn_cbor_context* ctx,
                    ls_err*          err)
{
  cn_cbor* cdata = cn_cbor_data_create(data, sz, ctx, NULL);
  if ( !cdata || !cn_cbor_mapput_string(map, key, cdata, ctx, NULL) )
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    return false;
  }
  return true;
}

static bool
_mapput_string_string(cn_cbor*         map,
                      const char*      key,
                      const char*      data,
                      cn_cbor_context* ctx,
                      ls_err*          err)
{
  cn_cbor* cdata = cn_cbor_string_create(data, ctx, NULL);
  if ( !cdata || !cn_cbor_mapput_string(map, key, cdata, ctx, NULL) )
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    return false;
  }
  return true;
}

LS_API bool
path_mandatory_keys_create(uint8_t*         ipadress,
                           size_t           iplen,
                           uint8_t*         token,
                           size_t           tokenlen,
                           char*            url,
                           cn_cbor_context* ctx,
                           cn_cbor**        map,
                           ls_err*          err)
{
  const char* SPUD_IPADDR = "ipaddr";
  const char* SPUD_TOKEN  = "token";
  const char* SPUD_URL    = "url";

  if ( !_map_create(ctx, map, err) )
  {
    /* no need to destroy pool */
    return false;
  }

  /*
   *  "ipaddr" (byte string, major type 2)  the IPv4 address or IPv6
   *  address of the sender, as a string of 4 or 16 bytes in network
   *  order.  This is necessary as the source IP address of the packet
   *  is spoofed
   *
   *   "token" (byte string, major type 2)  data that identifies the sending
   *   path element unambiguously
   *
   *  "url" (text string, major type 3)  a URL identifying some information
   *  about the path or its relationship with the tube.  The URL
   *  represents some path condition, and retrieval of content at the
   *  URL should include a human-readable description.
   */

  if ( !_mapput_string_data(*map, SPUD_IPADDR, ipadress, iplen, ctx, err) ||
       !_mapput_string_data(*map, SPUD_TOKEN, token, tokenlen, ctx, err) ||
       !_mapput_string_string(*map, SPUD_URL, url, ctx, err) )
  {
    ls_pool_destroy( (ls_pool*)ctx->context );
    return false;
  }

  return true;
}

LS_API bool
tube_send_pdec(tube*    t,
               cn_cbor* cbor,
               bool     reflect,
               ls_err*  err)
{
  return tube_send_cbor(t, SPUD_DATA, reflect, true, cbor, err);
}

LS_API bool
tube_send_cbor(tube*        t,
               spud_command cmd,
               bool         adec,
               bool         pdec,
               cn_cbor*     cbor,
               ls_err*      err)
{
  uint8_t  buf[MAXBUFLEN];
  ssize_t  sz = 0;
  uint8_t* d[1];
  size_t   l[1];

  assert(t);

  sz = cbor_encoder_write(buf, 0, MAXBUFLEN, cbor);
  if (sz < 0)
  {
    LS_ERROR(err, LS_ERR_OVERFLOW);
    return false;
  }

  d[0] = buf;
  l[0] = sz;
  return tube_send(t, cmd, adec, pdec, d, l, 1, err);
}

LS_API bool
tube_data(tube*    t,
          uint8_t* data,
          size_t   len,
          ls_err*  err)
{
  /* max size for CBOR preamble 19 bytes: */
  /* 1(map|27) 8(length) 1(key:0) 1(bstr|27) 8(length) */
  /* uint8_t preamble[19]; */
  cn_cbor*        map;
  cn_cbor*        cdata;
  bool            ret = false;
  cn_cbor_context ctx;

  assert(t);
  if (len == 0)
  {
    return tube_send(t, SPUD_DATA, false, false, NULL, 0, 0, err);
  }

  if ( !_map_create(&ctx, &map, err) )
  {
    return false;
  }

  /* TODO: the whole point of the iov system is so that we don't have to copy */
  /* the data here.  Which we just did.  Please fix. */
  if ( !( cdata = cn_cbor_data_create(data, len, &ctx, NULL) ) ||
       !cn_cbor_mapput_int(map, 0, cdata, &ctx, NULL) )
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    goto cleanup;
  }
  ret = tube_send_cbor(t, SPUD_DATA, false, false, map, err);

cleanup:
  ls_pool_destroy( (ls_pool*)ctx.context );
  return ret;
}

LS_API bool
tube_close(tube*   t,
           ls_err* err)
{
  assert(t);
  t->state = TS_UNKNOWN;
  if ( !tube_send(t, SPUD_CLOSE, false, false, NULL, 0, 0, err) )
  {
    return false;
  }
  return true;
}

LS_API void
tube_set_data(tube* t,
              void* data)
{
  assert(t);
  t->data = data;
}

LS_API void*
tube_get_data(tube* t)
{
  assert(t);
  return t->data;
}

LS_API char*
tube_id_to_string(tube*  t,
                  char*  buf,
                  size_t len)
{
  assert(t);
  return spud_id_to_string(buf, len, &t->id);
}

LS_API tube_states_t
tube_get_state(tube* t)
{
  assert(t);
  return t->state;
}

LS_API void
tube_set_state(tube*         t,
               tube_states_t state)
{
  assert(t);
  t->state = state;
}

LS_API void
tube_get_id(tube*          t,
            spud_tube_id** id)
{
  assert(t);
  *id = &t->id;
}

LS_API void
tube_set_info(tube*                  t,
              int                    socket,
              const struct sockaddr* peer,
              spud_tube_id*          id)
{
  assert(t);
  if (socket >= 0)
  {
    t->sock = socket;
  }
  if (peer)
  {
    ls_sockaddr_copy(peer, (struct sockaddr*)&t->peer);
  }
  if (id)
  {
    spud_copy_id(id, &t->id);
  }
}
