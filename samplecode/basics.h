/**
 * \file
 * \brief
 * Basic defines, macros, and functions for JabberWerxC.
 *
 * Copyrights
 *
 * Portions created or assigned to Cisco Systems, Inc. are
 * Copyright (c) 2010 Cisco Systems, Inc.  All Rights Reserved.
 */

#ifndef JABBERWERX_BASICS_H
#define JABBERWERX_BASICS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * \def JABBERWERX_API
 * Marks a symbol as part of the public API.
 */
#if defined(_WIN32) || defined(_WIN64)
#   ifdef JABBERWERX_EXPORTS
#       define JABBERWERX_API __declspec(dllexport)
#   else
#       define JABBERWERX_API __declspec(dllimport)
#   endif
#else
#   define JABBERWERX_API
#endif

#ifndef UNUSED_PARAM
  /**
   * \def UNUSED_PARAM(p);
   *
   * A macro for quelling compiler warnings about unused variables.
   */
#  define UNUSED_PARAM(p) ((void)&(p))
#endif /* UNUSED_PARM */

#include <stdbool.h>

/**
 * The (compile-time) version of this library.
 *
 * For the runtime version of this library, call {@link jw_version()}.
 */
#define JABBERWERX_VERSION PROJECT_VERSION
/**
 * The (complie-time) version of this library, including build number.
 *
 * For the runtime version of this library, call {@link jw_version()}.
 */
#define JABBERWERX_VERSION_FULL PROJECT_VERSION_FULL

/**
 * Retrieves the (runtime) version string of this library.
 *
 * For the compile-time version of this library, use #JABBERWERX_VERSION
 * or #JABBERWERX_VERSION_FULL.
 *
 * \param full true to return the full version (with build number), false to
 *             return the common version (without build number)
 * \retval const char * The version string.
 */
JABBERWERX_API const char *jw_version(bool full);

#ifdef __cplusplus
}
#endif

#endif /* JABBERWERX_BASICS_H */
