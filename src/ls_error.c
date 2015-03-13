/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <stdio.h>
#include <netdb.h>

#include "ls_error.h"

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
    "not found",
    "user-defined error"
};

/*****************************************************************************
 * External functions
 */

LS_API const char * ls_err_message(ls_errcode code)
{
    int ic = (int)code;
    if (ic < 0) {
        if (ic < -1000) {
            return gai_strerror(-(ic+1000));
        }
        return sys_errlist[-ic];
    }
    if (ic > LS_ERR_USER) {
        return "Unknown code";
    }
    return _ERR_MSG_TABLE[code];
}
