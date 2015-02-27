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
#pragma once

#include "basics.h"

/**
 * Enumeration of defined error codes.
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
    ls_errcode          code;
    /** The human readable message for the error code */
    const char *        message;
    /** The function where the error occured, or "<unknown>"
        if it cannot be determined */
    const char *        function;
    /** The file where the error occured */
    const char *        file;
    /** The line number in the file where the error occured */
    unsigned long       line;
} ls_err;


/**
 * Retrieves the error message for the given error code.
 *
 * \param code The error code to lookup
 * \retval const char * The message for {code}
 */
LS_API const char * ls_err_message(ls_errcode code);

/**
 * \def LS_ERROR(err, code)
 *
 * Macro to initialize an error context.
 *
 * \param err The pointer to the error context, or NULL if none
 * \param errcode The error code
 */
#define LS_ERROR(err, errcode) \
        if ((err) != NULL && (errcode) != LS_ERR_NONE) \
        { \
            (err)->code = (errcode); \
            (err)->message = ls_err_message((errcode)); \
            (err)->function = __func__; \
            (err)->file = __FILE__; \
            (err)->line = __LINE__; \
        }
