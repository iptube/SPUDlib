/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "ls_sockaddr.h"
#include "ls_log.h"

static bool
get_first_addr(const char*            hostname,
               const char*            servname,
               const struct addrinfo* hints,
               struct sockaddr*       addr,
               size_t                 addr_len,
               ls_err*                err)
{
  int              status;
  struct addrinfo* res, * p, * found;
  bool             ret = false;

  if ( ( status = getaddrinfo(hostname, servname, hints, &res) ) != 0 )
  {
    if (err != NULL)
    {
      /* HACK.  GAI errors on linux are negative, positive on OSX */
      err->code     = ls_err_gai(status);
      err->message  = gai_strerror(status);
      err->function = __func__;
      err->file     = __FILE__;
      err->line     = __LINE__;
    }
    return false;
  }

  /* Take the first v6 address, otherwise take the first v4 address */
  found = NULL;
  for (p = res; p != NULL; p = p->ai_next)
  {
    if (p->ai_family == AF_INET6)
    {
      found = p;
      break;
    }
    else if ( !found && (p->ai_family == AF_INET) )
    {
      found = p;
    }
  }
  if (found)
  {
    if (addr_len < found->ai_addrlen)
    {
      LS_ERROR(err, LS_ERR_OVERFLOW);
    }
    else
    {
      memcpy(addr, found->ai_addr, found->ai_addrlen);
      ret = true;
    }
  }
  else
  {
    LS_ERROR(err, LS_ERR_NOT_FOUND);
  }

  freeaddrinfo(res);
  return ret;
}

LS_API size_t
ls_sockaddr_get_length(const struct sockaddr* addr)
{
  assert(addr);
  switch (addr->sa_family)
  {
  case AF_INET:
    return sizeof(struct sockaddr_in);
  case AF_INET6:
    return sizeof(struct sockaddr_in6);
  default:
    return -1;
  }
}

LS_API int
ls_sockaddr_get_port(const struct sockaddr* addr)
{
  assert(addr);
  switch (addr->sa_family)
  {
  case AF_INET:
    return ntohs( ( (struct sockaddr_in*)addr )->sin_port );
  case AF_INET6:
    return ntohs( ( (struct sockaddr_in6*)addr )->sin6_port );
  default:
    return -1;
  }

}

LS_API void
ls_sockaddr_set_port(const struct sockaddr* addr,
                     int                    port)
{
  assert(addr);
  switch (addr->sa_family)
  {
  case AF_INET:
    ( (struct sockaddr_in*)addr )->sin_port = htons(port);
    break;
  case AF_INET6:
    ( (struct sockaddr_in6*)addr )->sin6_port = htons(port);
    break;
  default:
    assert(false);     /* LCOV_EXCL_LINE */
    break;
  }
}

LS_API bool
ls_sockaddr_get_remote_ip_addr(const char*      fqdn,
                               const char*      port,
                               struct sockaddr* remoteAddr,
                               size_t           addr_len,
                               ls_err*          err)
{
  struct addrinfo hints;

  memset(&hints, 0, sizeof hints);
  hints.ai_protocol = IPPROTO_UDP;
  hints.ai_socktype = SOCK_DGRAM;

  return get_first_addr(fqdn, port, &hints, remoteAddr, addr_len, err);
}

LS_API void
ls_sockaddr_v6_any(struct sockaddr_in6* sa,
                   int                  port)
{
  memset( sa, 0, sizeof(*sa) );
  sa->sin6_family = AF_INET6;
  sa->sin6_addr   = in6addr_any;
  /* sa->sin6_len = sizeof(*sa); */
  sa->sin6_port = htons(port);
}

LS_API void
ls_sockaddr_v4_any(struct sockaddr_in* sa,
                   int                 port)
{
  memset( sa, 0, sizeof(*sa) );
  sa->sin_family      = AF_INET;
  sa->sin_addr.s_addr = htonl(INADDR_ANY);
  /* sa->sin6_len = sizeof(*sa); */
  sa->sin_port = htons(port);
}

LS_API void
ls_sockaddr_copy(const struct sockaddr* src,
                 struct sockaddr*       dest)
{
  memcpy( dest, src, ls_sockaddr_get_length(src) );
}

LS_API const char*
ls_sockaddr_to_string(const struct sockaddr* sa,
                      char*                  dest,
                      size_t                 destlen,
                      bool                   addport)
{
  const char* iret = NULL;
  size_t      r    = 0;
  assert(dest);
  assert(destlen > 0);

  if (!sa)
  {
    goto warn;
  }
  if (sa->sa_family == AF_INET)
  {
    if (destlen < INET_ADDRSTRLEN + 8)
    {
      /* 8 is enough for :port and termination addr:65535\n\0 */
      goto warn;
    }
    else
    {
      const struct sockaddr_in* sa4 = (const struct sockaddr_in*)sa;
      iret = inet_ntop(AF_INET, &(sa4->sin_addr), dest, destlen);
      if (iret == NULL)
      {
        assert(false);         /* LCOV_EXCL_LINE Should be impossible */
        goto warn;             /* LCOV_EXCL_LINE Should be impossible */
      }
      if (addport)
      {
        r = strnlen(dest, INET_ADDRSTRLEN + 1);
        snprintf( dest + r, destlen - r, ":%d", ntohs(sa4->sin_port) );
      }
      return dest;
    }
  }
  else if (sa->sa_family == AF_INET6)
  {
    if (destlen < INET6_ADDRSTRLEN + 10)
    {
      /* 10 is enough for brackets, :port and termination [addr]:65535\n\0 */
      goto warn;
    }
    else
    {
      const struct sockaddr_in6* sa6 = (const struct sockaddr_in6*)sa;
      if (addport)
      {
        dest[r++] = '[';
      }
      iret = inet_ntop(AF_INET6, &(sa6->sin6_addr), dest + r, destlen);
      if (iret == NULL)
      {
        assert(false);         /* LCOV_EXCL_LINE Should be impossible */
        goto warn;             /* LCOV_EXCL_LINE Should be impossible */
      }
      r = strnlen(dest, INET6_ADDRSTRLEN + 2);

      if (addport)
      {
        dest[r++] = ']';
        snprintf( dest + r, destlen - r, ":%d", ntohs(sa6->sin6_port) );
      }
      return dest;
    }
  }

warn:
  dest[0] = '\0';
  return dest;
}

LS_API bool
ls_sockaddr_parse(const char*      src,
                  struct sockaddr* addr,
                  size_t           addr_len,
                  ls_err*          err)
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_flags    = AI_NUMERICHOST;
  hints.ai_protocol = IPPROTO_UDP;
  hints.ai_socktype = SOCK_DGRAM;

  return get_first_addr(src, NULL, &hints, addr, addr_len, err);
}

LS_API int
ls_addr_cmp6(const struct in6_addr* a,
             const struct in6_addr* b)
{
  return memcmp( a, b, sizeof(struct in6_addr) );
}

LS_API int
ls_sockaddr_cmp(const struct sockaddr* sa,
                const struct sockaddr* sb)
{
  int                  diff;
  struct sockaddr_in*  a_in, * b_in;
  struct sockaddr_in6* a_in6, * b_in6;

  if (sa == NULL)
  {
    if (sb == NULL)
    {
      return 0;
    }
    return -1;
  }
  if (sb == NULL)
  {
    return 1;
  }

  diff = sa->sa_family - sb->sa_family;
  if (diff != 0)
  {
    return diff;
  }

  switch (sa->sa_family)
  {
  case AF_INET:
    a_in = (struct sockaddr_in*)sa;
    b_in = (struct sockaddr_in*)sb;
    diff = a_in->sin_addr.s_addr - b_in->sin_addr.s_addr;
    if (diff != 0)
    {
      return diff;
    }
    return ntohs(a_in->sin_port) - ntohs(b_in->sin_port);
    break;
  case AF_INET6:
    a_in6 = (struct sockaddr_in6*)sa;
    b_in6 = (struct sockaddr_in6*)sb;
    diff  = ls_addr_cmp6(&a_in6->sin6_addr, &b_in6->sin6_addr);
    if (diff != 0)
    {
      return diff;
    }
    return ntohs(a_in6->sin6_port) - ntohs(b_in6->sin6_port);
    break;
  default:
    break;
  }
  return 0;
}
