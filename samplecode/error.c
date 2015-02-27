/**
 *
 * Copyrights
 *
 * Portions created or assigned to Cisco Systems, Inc. are
 * Copyright (c) 2015 Cisco Systems, Inc.  All Rights Reserved.
 */

#include "error.h"

/*****************************************************************************
 * Internal type definitions
 */

static const char *_ERR_MSG_TABLE[] = {
    "no error",
    "invalid argument",
    "invalid state",
    "out of memory",
    "buffer overflow",
    "socket connect failure",
    "bad data format",
    "protocol error",
    "timed out",
    "user-defined error"
};

/*****************************************************************************
 * External functions
 */

LS_API const char * ls_err_message(ls_errcode code)
{
    return _ERR_MSG_TABLE[code];
}
