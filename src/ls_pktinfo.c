/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include "ls_pktinfo.h"
#include "ls_mem.h"

#include <assert.h>
#include <string.h>

#define HAS_INFO_4 (1 << 0)
#define HAS_INFO_6 (1 << 1)

struct _ls_pktinfo {
  union {
    struct in6_pktinfo i6;
    struct in_pktinfo  i4;
  } info;
  char kind;
};

LS_API bool
ls_pktinfo_create(ls_pktinfo** p,
                  ls_err*      err)
{
  ls_pktinfo* ret;
  assert(p);
  ret = ls_data_calloc( 1, sizeof(*ret) );
  if (!ret)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    *p = NULL;
    return false;
  }
  *p = ret;
  return true;
}

LS_API bool
ls_pktinfo_dup(ls_pktinfo*  source,
               ls_pktinfo** dest,
               ls_err*      err)
{
  assert(source);
  assert(dest);
  if ( !ls_pktinfo_create(dest, err) )
  {
    return false;
  }
  memcpy( *dest, source, sizeof(ls_pktinfo) );
  return true;
}

LS_API void
ls_pktinfo_clear(ls_pktinfo* p)
{
  assert(p);
  p->kind = 0;
}

LS_API void
ls_pktinfo_destroy(ls_pktinfo* p)
{
  assert(p);
  ls_data_free(p);
}

LS_API void
ls_pktinfo_set4(ls_pktinfo*        p,
                struct in_pktinfo* info)
{
  assert(p);
  p->kind    = HAS_INFO_4;
  p->info.i4 = *info;
}

LS_API void
ls_pktinfo_set6(ls_pktinfo*         p,
                struct in6_pktinfo* info)
{
  assert(p);
  p->kind    = HAS_INFO_6;
  p->info.i6 = *info;
}

LS_API bool
ls_pktinfo_get_addr(ls_pktinfo*      p,
                    struct sockaddr* addr,
                    socklen_t*       addr_len,
                    ls_err*          err)
{
  assert(p);
  assert(addr);
  assert(addr_len);

  switch (p->kind)
  {
  case HAS_INFO_4:
    if ( *addr_len < sizeof(struct sockaddr_in) )
    {
      LS_ERROR(err, LS_ERR_OVERFLOW);
      return false;
    }
    {
      struct sockaddr_in* sa = (struct sockaddr_in*)addr;
      sa->sin_family = AF_INET;
      sa->sin_port   = htons(-1);
      sa->sin_addr   = p->info.i4.ipi_addr;
      *addr_len      = sizeof(struct sockaddr_in);
    }
    break;
  case HAS_INFO_6:
    if ( *addr_len < sizeof(struct sockaddr_in6) )
    {
      LS_ERROR(err, LS_ERR_OVERFLOW);
      return false;
    }
    {
      struct sockaddr_in6* sa = (struct sockaddr_in6*)addr;
      sa->sin6_family = AF_INET6;
      sa->sin6_port   = htons(-1);
      sa->sin6_addr   = p->info.i6.ipi6_addr;
      *addr_len       = sizeof(struct sockaddr_in6);
    }
    break;
  default:
    LS_ERROR(err, LS_ERR_INVALID_ARG);
    return false;
  }
  return true;
}

LS_API void*
ls_pktinfo_get_info(ls_pktinfo* p)
{
  assert(p);
  switch (p->kind)
  {
  case HAS_INFO_4:
    return &p->info.i4;
  case HAS_INFO_6:
    return &p->info.i6;
  default:
    return NULL;
  }
}

LS_API bool
ls_pktinfo_is_full(ls_pktinfo* p)
{
  assert(p);
  return p->kind != 0;
}

LS_API size_t
ls_pktinfo_cmsg(ls_pktinfo*     p,
                struct cmsghdr* cmsg)
{
  assert(p);
  assert(cmsg);

  switch (p->kind)
  {
  case HAS_INFO_4:
    cmsg->cmsg_level = IPPROTO_IP;
    cmsg->cmsg_type  = IP_PKTINFO;
    cmsg->cmsg_len   = CMSG_LEN( sizeof(struct in_pktinfo) );
    {
      struct in_pktinfo* ipi = (struct in_pktinfo*)CMSG_DATA(cmsg);
      *ipi = p->info.i4;
    }
    return CMSG_SPACE( sizeof(struct in_pktinfo) );
  case HAS_INFO_6:
    cmsg->cmsg_level = IPPROTO_IPV6;
    cmsg->cmsg_type  = IPV6_PKTINFO;
    cmsg->cmsg_len   = CMSG_LEN( sizeof(struct in6_pktinfo) );
    {
      struct in6_pktinfo* ipi = (struct in6_pktinfo*)CMSG_DATA(cmsg);
      ipi->ipi6_addr = p->info.i6.ipi6_addr;
    }
    return CMSG_SPACE( sizeof(struct in6_pktinfo) );
  default:
    return 0;
  }
}
