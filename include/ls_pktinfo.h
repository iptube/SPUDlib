/**
 * \file
 * \brief
 * Local address information, either v4 or v6
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#pragma once

#ifdef __APPLE__
#define __APPLE_USE_RFC_3542
#endif

#ifdef __linux__
/* needed for netinet/in.h */
#define _GNU_SOURCE 1
#define __USE_GNU 1
#endif

#include <netinet/in.h>
#include "ls_basics.h"
#include "ls_error.h"

/**
 * A structure that holds either a struct in_pktinfo or a struct in6_pktinfo
 * as needed for an IPv4 or IPv6 inbound destination address.
 */
typedef struct _ls_pktinfo ls_pktinfo;

/**
 * Create a pktinfo structure.  Allocates data that must be freed with
 * ls_pktinfo_destroy.
 *
 * @param[out]  p   Created pktinfo
 * @param[out]  err The error information (provide NULL to ignore)
 * @return      true on success
 */
LS_API bool
ls_pktinfo_create(ls_pktinfo** p,
                  ls_err*      err);

/**
 * Destroy a previously-created pktinfo.
 *
 * @param[in] p The pktinfo to free
 */
LS_API void
ls_pktinfo_destroy(ls_pktinfo* p);

/**
 * Clear the data stored in the pktinfo.
 *
 * @param[in] p The pktinfo to clear
 */
LS_API void
ls_pktinfo_clear(ls_pktinfo* p);

/**
 * Duplicate a pktinfo.  Allocates data that must be freed with
 * ls_pktinfo_destroy.
 *
 * @param[in]  source The pktinfo to copy
 * @param[out] dest   The new copy
 * @param[out] err    The error information (provide NULL to ignore)
 * @return     True on success
 */
LS_API bool
ls_pktinfo_dup(ls_pktinfo*  source,
               ls_pktinfo** dest,
               ls_err*      err);

/**
 * Set the IPv4 in_pktinfo.
 *
 * @param[in] p    The pktinfo to modify
 * @param[in] info The actual information about a packet that was received
 */
LS_API void
ls_pktinfo_set4(ls_pktinfo*        p,
                struct in_pktinfo* info);

/**
 * Set the IPv6 in_pktinfo.
 *
 * @param[in] p    The pktinfo to modify
 * @param[in] info The actual information about a packet that was received
 */
LS_API void
ls_pktinfo_set6(ls_pktinfo*         p,
                struct in6_pktinfo* info);

/**
 * Fill in a socket address from the pktinfo.  The port will be set to -1.
 *
 * @param[in]    p        The pktinfo to access
 * @param[out]   addr     The address to fill in
 * @param[inout] addr_len On input, the length of addr, in bytes.
 *                        On output, the number of bytes used.
 * @param[out]   err      The error information (provide NULL to ignore)
 * @return       True on success
 */
LS_API bool
ls_pktinfo_get_addr(ls_pktinfo*      p,
                    struct sockaddr* addr,
                    socklen_t*       addr_len,
                    ls_err*          err);

/**
 * Has the pktinfo had info assigned to it?
 *
 * @param[in]  p The pktinfo to access
 * @return     True if information has been set
 */
LS_API bool
ls_pktinfo_is_full(ls_pktinfo* p);

/**
 * Get the correct v4/v6 info back out.
 * @param[in]  p The pktinfo to access
 * @return     The info
 */
LS_API void*
ls_pktinfo_get_info(ls_pktinfo* p);

/**
 * Fill in a struct cmsg for an outbound packet, with the info received
 * from an inbound packet.
 *
 * @param[in]  p    The pktinfo to access
 * @param[out] cmsg The cmsg to fill in
 * @return     The number of bytes filled in to the cmsg, or a safe 0 on error.
 */
LS_API size_t
ls_pktinfo_cmsg(ls_pktinfo*     p,
                struct cmsghdr* cmsg);
