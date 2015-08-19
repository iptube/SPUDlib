/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <netdb.h>
#include <stdio.h>
#include <string.h>

#include "ls_error.h"

/*****************************************************************************
 * Internal type definitions
 */

static const char* _ERR_MSG_TABLE[] = {
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
  "not implemented",
  "user-defined error"
};

#define GAI_OFFSET_POS -1000
#define GAI_OFFSET_NEG -1100

LS_API const char*
ls_err_message(ls_errcode code)
{
  int ic = (int)code;
  if (ic < 0)
  {
    if (ic < GAI_OFFSET_NEG)
    {
      return gai_strerror(ic - GAI_OFFSET_NEG);
    }
    else if (ic < GAI_OFFSET_POS)
    {
      return gai_strerror( -(ic - GAI_OFFSET_POS) );
    }
    return strerror(-ic);
  }
  if (ic > LS_ERR_USER)
  {
    return "Unknown code";
  }
  return _ERR_MSG_TABLE[code];
}

LS_API ls_errcode
ls_err_gai(int gai_error)
{
  return (gai_error < 0) ?
         (GAI_OFFSET_NEG + gai_error) :
         (GAI_OFFSET_POS - gai_error);
}
