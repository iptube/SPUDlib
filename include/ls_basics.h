/**
 * \file
 * \brief
 * Basic defines, macros, and functions for libSpud.
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#pragma once

/**
 * \def LS_API
 * Marks a symbol as part of the public API.
 */
#if defined(_WIN32) || defined(_WIN64)
#   ifdef LS_EXPORTS
#       define LS_API __declspec(dllexport)
#   else
#       define LS_API __declspec(dllimport)
#   endif
#else
#   define LS_API
#endif

#ifndef UNUSED_PARAM
/**
 * \def UNUSED_PARAM(p);
 *
 * A macro for quelling compiler warnings about unused variables.
 */
#  define UNUSED_PARAM(p) ( (void)&(p) )
#endif /* UNUSED_PARM */

#include <stdbool.h>
