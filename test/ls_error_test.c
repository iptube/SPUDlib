/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>

#include "ls_error.h"
#include "test_utils.h"

CTEST(ls_error, message_test)
{
  const char* msg;
  msg = ls_err_message(LS_ERR_NONE);
  ASSERT_STR(msg, "no error");
  msg = ls_err_message(LS_ERR_INVALID_ARG);
  ASSERT_STR(msg, "invalid argument");
  msg = ls_err_message(LS_ERR_INVALID_STATE);
  ASSERT_STR(msg, "invalid state");
  msg = ls_err_message(LS_ERR_NO_MEMORY);
  ASSERT_STR(msg, "out of memory");
  msg = ls_err_message(LS_ERR_OVERFLOW);
  ASSERT_STR(msg, "buffer overflow");
  msg = ls_err_message(LS_ERR_SOCKET_CONNECT);
  ASSERT_STR(msg, "socket connect failure");
  msg = ls_err_message(LS_ERR_BAD_FORMAT);
  ASSERT_STR(msg, "bad data format");
  msg = ls_err_message(LS_ERR_PROTOCOL);
  ASSERT_STR(msg, "protocol error");
  msg = ls_err_message(LS_ERR_TIMEOUT);
  ASSERT_STR(msg, "timed out");
  msg = ls_err_message(LS_ERR_NO_IMPL);
  ASSERT_STR(msg, "not implemented");
  msg = ls_err_message(LS_ERR_USER);
  ASSERT_STR(msg, "user-defined error");
}

CTEST(ls_error, macro_test)
{
  ls_err* err_ctx;

  err_ctx = malloc( sizeof(ls_err) );
  ASSERT_NOT_NULL(err_ctx);
  LS_ERROR(err_ctx, LS_ERR_INVALID_ARG);
  ASSERT_EQUAL(err_ctx->code, LS_ERR_INVALID_ARG);
  ASSERT_STR(err_ctx->message, "invalid argument");
  ASSERT_NOT_NULL(err_ctx->function);
  ASSERT_NOT_NULL(err_ctx->file);
  ASSERT_NOT_EQUAL(err_ctx->line, 0);

  memset( err_ctx, 0, sizeof(ls_err) );
  LS_ERROR(err_ctx, LS_ERR_INVALID_STATE);
  ASSERT_EQUAL(err_ctx->code, LS_ERR_INVALID_STATE);
  ASSERT_STR(err_ctx->message, "invalid state");
  ASSERT_NOT_NULL(err_ctx->function);
  ASSERT_NOT_NULL(err_ctx->file);
  ASSERT_NOT_EQUAL(err_ctx->line, 0);

  memset( err_ctx, 0, sizeof(ls_err) );
  LS_ERROR(err_ctx, LS_ERR_NO_MEMORY);
  ASSERT_EQUAL(err_ctx->code, LS_ERR_NO_MEMORY);
  ASSERT_STR(err_ctx->message, "out of memory");
  ASSERT_NOT_NULL(err_ctx->function);
  ASSERT_NOT_NULL(err_ctx->file);
  ASSERT_NOT_EQUAL(err_ctx->line, 0);

  memset( err_ctx, 0, sizeof(ls_err) );
  LS_ERROR(err_ctx, LS_ERR_OVERFLOW);
  ASSERT_EQUAL(err_ctx->code, LS_ERR_OVERFLOW);
  ASSERT_STR(err_ctx->message, "buffer overflow");
  ASSERT_NOT_NULL(err_ctx->function);
  ASSERT_NOT_NULL(err_ctx->file);
  ASSERT_NOT_EQUAL(err_ctx->line, 0);

  memset( err_ctx, 0, sizeof(ls_err) );
  LS_ERROR(err_ctx, LS_ERR_SOCKET_CONNECT);
  ASSERT_EQUAL(err_ctx->code, LS_ERR_SOCKET_CONNECT);
  ASSERT_STR(err_ctx->message, "socket connect failure");
  ASSERT_NOT_NULL(err_ctx->function);
  ASSERT_NOT_NULL(err_ctx->file);
  ASSERT_NOT_EQUAL(err_ctx->line, 0);

  memset( err_ctx, 0, sizeof(ls_err) );
  LS_ERROR(err_ctx, LS_ERR_BAD_FORMAT);
  ASSERT_EQUAL(err_ctx->code, LS_ERR_BAD_FORMAT);
  ASSERT_STR(err_ctx->message, "bad data format");
  ASSERT_NOT_NULL(err_ctx->function);
  ASSERT_NOT_NULL(err_ctx->file);
  ASSERT_NOT_EQUAL(err_ctx->line, 0);

  memset( err_ctx, 0, sizeof(ls_err) );
  LS_ERROR(err_ctx, LS_ERR_PROTOCOL);
  ASSERT_EQUAL(err_ctx->code, LS_ERR_PROTOCOL);
  ASSERT_STR(err_ctx->message, "protocol error");
  ASSERT_NOT_NULL(err_ctx->function);
  ASSERT_NOT_NULL(err_ctx->file);
  ASSERT_NOT_EQUAL(err_ctx->line, 0);

  memset( err_ctx, 0, sizeof(ls_err) );
  LS_ERROR(err_ctx, LS_ERR_TIMEOUT);
  ASSERT_EQUAL(err_ctx->code, LS_ERR_TIMEOUT);
  ASSERT_STR(err_ctx->message, "timed out");
  ASSERT_NOT_NULL(err_ctx->function);
  ASSERT_NOT_NULL(err_ctx->file);
  ASSERT_NOT_EQUAL(err_ctx->line, 0);

  memset( err_ctx, 0, sizeof(ls_err) );
  LS_ERROR(err_ctx, LS_ERR_NO_IMPL);
  ASSERT_EQUAL(err_ctx->code, LS_ERR_NO_IMPL);
  ASSERT_STR(err_ctx->message, "not implemented");
  ASSERT_NOT_NULL(err_ctx->function);
  ASSERT_NOT_NULL(err_ctx->file);
  ASSERT_NOT_EQUAL(err_ctx->line, 0);

  memset( err_ctx, 0, sizeof(ls_err) );
  LS_ERROR(err_ctx, LS_ERR_USER);
  ASSERT_EQUAL(err_ctx->code, LS_ERR_USER);
  ASSERT_STR(err_ctx->message, "user-defined error");
  ASSERT_NOT_NULL(err_ctx->function);
  ASSERT_NOT_NULL(err_ctx->file);
  ASSERT_NOT_EQUAL(err_ctx->line, 0);

  memset( err_ctx, 0, sizeof(ls_err) );
  LS_ERROR(err_ctx, LS_ERR_NONE);
  ASSERT_EQUAL(err_ctx->code, LS_ERR_NONE);
  ASSERT_NULL(err_ctx->message);
  ASSERT_NULL(err_ctx->function);
  ASSERT_NULL(err_ctx->file);
  ASSERT_EQUAL(err_ctx->line, 0);

  free(err_ctx);
  err_ctx = NULL;
  LS_ERROR(err_ctx, LS_ERR_NONE);
  LS_ERROR(err_ctx, LS_ERR_INVALID_ARG);
}

CTEST(ls_error, perror_test)
{
  const char* msg = ls_err_message(-EINTR);
  ASSERT_STR(msg, "Interrupted system call");
}

CTEST(ls_error, gai_test)
{
  ls_errcode  c   = ls_err_gai(EAI_FAIL);
  const char* msg = ls_err_message(c);
  ASSERT_STR(msg, "Non-recoverable failure in name resolution");

  c = ls_err_gai(4);
  ASSERT_EQUAL( (int)c, -1004 );
  c = ls_err_gai(-4);
  ASSERT_EQUAL( (int)c, -1104 );
  ASSERT_NOT_EQUAL(strlen( ls_err_message(-1004) ), 0);
  ASSERT_NOT_EQUAL(strlen( ls_err_message(-1104) ), 0);
}

CTEST(ls_error, invalid_error)
{
  ASSERT_STR(ls_err_message(LS_ERR_USER + 1), "Unknown code");
}
