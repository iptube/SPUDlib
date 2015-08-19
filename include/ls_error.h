/**
 * \file
 * \brief
 * Datatypes and functions for error reporting.
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */
#pragma once

#include "ls_basics.h"

/**
 * Enumeration of defined error codes.
 * errno errors are made negative
 */
typedef enum
{
  /** No error */
  LS_ERR_NONE = 0,
  /** argument was invalid (beyond invariants) */
  LS_ERR_INVALID_ARG,
  /** context is not in a valid state */
  LS_ERR_INVALID_STATE,
  /** out of memory */
  LS_ERR_NO_MEMORY,
  /** buffer would overflow */
  LS_ERR_OVERFLOW,
  /** error connecting to a remote endpoint */
  LS_ERR_SOCKET_CONNECT,
  /** provided data could not be parsed by consuming entity */
  LS_ERR_BAD_FORMAT,
  /** invalid protocol */
  LS_ERR_PROTOCOL,
  /** timed out */
  LS_ERR_TIMEOUT,
  /** can't find it */
  LS_ERR_NOT_FOUND,
  /** Function or behavior not implemented. */
  LS_ERR_NO_IMPL,
  /** user-defined errors */
  LS_ERR_USER
} ls_errcode;

/**
 * An instance of an error context. Unlike other structures, it
 * is the API user's responsibility to allocate the structure; however
 * the values provided are considered constants, and MUST NOT be
 * deallocated.
 */
typedef struct
{
  /** The error code */
  ls_errcode code;
  /** The human readable message for the error code */
  const char* message;
  /** The function where the error occured, or "<unknown>"
   *   if it cannot be determined */
  const char* function;
  /** The file where the error occured */
  const char* file;
  /** The line number in the file where the error occured */
  unsigned long line;
} ls_err;

/**
 * Retrieves the error message for the given error code.
 *
 * \param code The error code to lookup
 * \retval const char * The message for {code}
 */
LS_API const char*
ls_err_message(ls_errcode code);

/**
 * Translate a getaddressinfo error into an ls_errcode, such that
 * ls_err_message can return a string for it.
 */
LS_API ls_errcode
ls_err_gai(int gai_error);

/**
 * \def LS_ERROR(err, code)
 *
 * Macro to initialize an error context.
 *
 * \param err The pointer to the error context, or NULL if none
 * \param errcode The error code
 */
#define LS_ERROR(err, errcode) \
  if ( (err) != NULL && (errcode) != LS_ERR_NONE ) \
  { \
    (err)->code     = (errcode); \
    (err)->message  = ls_err_message( (errcode) ); \
    (err)->function = __func__; \
    (err)->file     = __FILE__; \
    (err)->line     = __LINE__; \
  }
