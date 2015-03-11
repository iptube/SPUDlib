#include <stdlib.h>
#include <check.h>

#include "ls_error.h"

Suite * ls_error_suite (void);

START_TEST (ls_error_message_test)
{
    const char *msg;
    msg = ls_err_message(LS_ERR_NONE);
    ck_assert_str_eq(msg, "no error");
    msg = ls_err_message(LS_ERR_INVALID_ARG);
    ck_assert_str_eq(msg, "invalid argument");
    msg = ls_err_message(LS_ERR_INVALID_STATE);
    ck_assert_str_eq(msg, "invalid state");
    msg = ls_err_message(LS_ERR_NO_MEMORY);
    ck_assert_str_eq(msg, "out of memory");
    msg = ls_err_message(LS_ERR_OVERFLOW);
    ck_assert_str_eq(msg, "buffer overflow");
    msg = ls_err_message(LS_ERR_SOCKET_CONNECT);
    ck_assert_str_eq(msg, "socket connect failure");
    msg = ls_err_message(LS_ERR_BAD_FORMAT);
    ck_assert_str_eq(msg, "bad data format");
    msg = ls_err_message(LS_ERR_PROTOCOL);
    ck_assert_str_eq(msg, "protocol error");
    msg = ls_err_message(LS_ERR_TIMEOUT);
    ck_assert_str_eq(msg, "timed out");
    msg = ls_err_message(LS_ERR_USER);
    ck_assert_str_eq(msg, "user-defined error");
}
END_TEST

START_TEST (ls_error_macro_test)
{
    ls_err  *err_ctx;

    err_ctx = (ls_err *)malloc(sizeof(ls_err));
    ck_assert(err_ctx);
    LS_ERROR(err_ctx, LS_ERR_INVALID_ARG);
    ck_assert_int_eq(err_ctx->code, LS_ERR_INVALID_ARG);
    ck_assert_str_eq(err_ctx->message, "invalid argument");
    ck_assert(err_ctx->function != NULL);
    ck_assert(err_ctx->file != NULL);
    ck_assert(err_ctx->line != 0);

    memset(err_ctx, 0, sizeof(ls_err));
    LS_ERROR(err_ctx, LS_ERR_INVALID_STATE);
    ck_assert_int_eq(err_ctx->code, LS_ERR_INVALID_STATE);
    ck_assert_str_eq(err_ctx->message, "invalid state");
    ck_assert(err_ctx->function != NULL);
    ck_assert(err_ctx->file != NULL);
    ck_assert(err_ctx->line != 0);

    memset(err_ctx, 0, sizeof(ls_err));
    LS_ERROR(err_ctx, LS_ERR_NO_MEMORY);
    ck_assert_int_eq(err_ctx->code, LS_ERR_NO_MEMORY);
    ck_assert_str_eq(err_ctx->message, "out of memory");
    ck_assert(err_ctx->function != NULL);
    ck_assert(err_ctx->file != NULL);
    ck_assert(err_ctx->line != 0);

    memset(err_ctx, 0, sizeof(ls_err));
    LS_ERROR(err_ctx, LS_ERR_OVERFLOW);
    ck_assert_int_eq(err_ctx->code, LS_ERR_OVERFLOW);
    ck_assert_str_eq(err_ctx->message, "buffer overflow");
    ck_assert(err_ctx->function != NULL);
    ck_assert(err_ctx->file != NULL);
    ck_assert(err_ctx->line != 0);

    memset(err_ctx, 0, sizeof(ls_err));
    LS_ERROR(err_ctx, LS_ERR_SOCKET_CONNECT);
    ck_assert_int_eq(err_ctx->code, LS_ERR_SOCKET_CONNECT);
    ck_assert_str_eq(err_ctx->message, "socket connect failure");
    ck_assert(err_ctx->function != NULL);
    ck_assert(err_ctx->file != NULL);
    ck_assert(err_ctx->line != 0);

    memset(err_ctx, 0, sizeof(ls_err));
    LS_ERROR(err_ctx, LS_ERR_BAD_FORMAT);
    ck_assert_int_eq(err_ctx->code, LS_ERR_BAD_FORMAT);
    ck_assert_str_eq(err_ctx->message, "bad data format");
    ck_assert(err_ctx->function != NULL);
    ck_assert(err_ctx->file != NULL);
    ck_assert(err_ctx->line != 0);

    memset(err_ctx, 0, sizeof(ls_err));
    LS_ERROR(err_ctx, LS_ERR_PROTOCOL);
    ck_assert_int_eq(err_ctx->code, LS_ERR_PROTOCOL);
    ck_assert_str_eq(err_ctx->message, "protocol error");
    ck_assert(err_ctx->function != NULL);
    ck_assert(err_ctx->file != NULL);
    ck_assert(err_ctx->line != 0);

    memset(err_ctx, 0, sizeof(ls_err));
    LS_ERROR(err_ctx, LS_ERR_TIMEOUT);
    ck_assert_int_eq(err_ctx->code, LS_ERR_TIMEOUT);
    ck_assert_str_eq(err_ctx->message, "timed out");
    ck_assert(err_ctx->function != NULL);
    ck_assert(err_ctx->file != NULL);
    ck_assert(err_ctx->line != 0);

    memset(err_ctx, 0, sizeof(ls_err));
    LS_ERROR(err_ctx, LS_ERR_USER);
    ck_assert_int_eq(err_ctx->code, LS_ERR_USER);
    ck_assert_str_eq(err_ctx->message, "user-defined error");
    ck_assert(err_ctx->function != NULL);
    ck_assert(err_ctx->file != NULL);
    ck_assert(err_ctx->line != 0);

    memset(err_ctx, 0, sizeof(ls_err));
    LS_ERROR(err_ctx, LS_ERR_NONE);
    ck_assert_int_eq(err_ctx->code, LS_ERR_NONE);
    ck_assert(err_ctx->message == NULL);
    ck_assert(err_ctx->function == NULL);
    ck_assert(err_ctx->file == NULL);
    ck_assert(err_ctx->line == 0);

    free(err_ctx);
    err_ctx = NULL;
    LS_ERROR(err_ctx, LS_ERR_NONE);
    ck_assert_msg(1, "successful NULL-check");
    LS_ERROR(err_ctx, LS_ERR_INVALID_ARG);
    ck_assert_msg(1, "successful NULL-check");
}
END_TEST

Suite * ls_error_suite (void)
{
  Suite *s = suite_create ("ls_error");
  {/* Error test case */
      TCase *tc_ls_error = tcase_create ("Error");
      tcase_add_test (tc_ls_error, ls_error_message_test);
      tcase_add_test (tc_ls_error, ls_error_macro_test);

      suite_add_tcase (s, tc_ls_error);
  }

  return s;
}
