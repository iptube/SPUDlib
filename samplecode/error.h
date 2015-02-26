/**
 * \file
 * \brief
 * Datatypes and functions for error reporting.
 *
 * Copyrights
 *
 * Portions created or assigned to Cisco Systems, Inc. are
 * Copyright (c) 2010 Cisco Systems, Inc.  All Rights Reserved.
 */
#ifndef JABBERWERX_UTIL_ERROR_H
#define JABBERWERX_UTIL_ERROR_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "basics.h"

/**
 * Enumeration of defined error codes.
 */
typedef enum
{
    /** No error */
    JW_ERR_NONE = 0,
    /** argument was invalid (beyond invariants) */
    JW_ERR_INVALID_ARG,
    /** context is not in a valid state */
    JW_ERR_INVALID_STATE,
    /** out of memory */
    JW_ERR_NO_MEMORY,
    /** buffer would overflow */
    JW_ERR_OVERFLOW,
    /** error connecting to a remote endpoint */
    JW_ERR_SOCKET_CONNECT,
    /** provided data could not be parsed by consuming entity */
    JW_ERR_BAD_FORMAT,
    /** invalid protocol */
    JW_ERR_PROTOCOL,
    /** timed out */
    JW_ERR_TIMEOUT,
    /** user-defined errors */
    JW_ERR_USER
} jw_errcode;

/**
 * An instance of an error context. Unlike other structures, it
 * is the API user's responsibility to allocate the structure; however
 * the values provided are considered constants, and MUST NOT be
 * deallocated.
 */
typedef struct
{
    /** The error code */
    jw_errcode          code;
    /** The human readable message for the error code */
    const char *        message;
    /** The function where the error occured, or "<unknown>"
        if it cannot be determined */
    const char *        function;
    /** The file where the error occured */
    const char *        file;
    /** The line number in the file where the error occured */
    unsigned long       line;
} jw_err;


/**
 * Retrieves the error message for the given error code.
 *
 * \param code The error code to lookup
 * \retval const char * The message for {code}
 */
JABBERWERX_API const char * jw_err_message(jw_errcode code);

/**
 * \def JABBERWERX_ERROR(err, code)
 *
 * Macro to initialize an error context.
 *
 * \param err The pointer to the error context, or NULL if none
 * \param errcode The error code
 */
#define JABBERWERX_ERROR(err, errcode) \
        if ((err) != NULL && (errcode) != JW_ERR_NONE) \
        { \
            (err)->code = (errcode); \
            (err)->message = jw_err_message((errcode)); \
            (err)->function = __func__; \
            (err)->file = __FILE__; \
            (err)->line = __LINE__; \
        }

#ifdef __cplusplus
}
#endif

#endif  /* JABBERWERX_UTIL_ERROR_H */
