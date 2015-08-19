/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <assert.h>
#include <string.h>
#include "ls_log.h"
#include "test_utils.h"

#undef LS_ERROR
inline static void
LS_ERROR(ls_err* err,
         int     errcode)
{
  if ( ( (err) != NULL ) && ( (errcode) != LS_ERR_NONE ) )
  {
    (err)->code     = (errcode);
    (err)->message  = ls_err_message( (errcode) );
    (err)->function = __func__;
    (err)->file     = __FILE__;
    (err)->line     = __LINE__;
  }
}


typedef struct _log_chunk_int
{
  const char*            chunk;
  size_t                 len;
  ls_data_free_func      free_fn;
  struct _log_chunk_int* cur;
  struct _log_chunk_int* next;
} _log_chunk;

static char _log_output[1024];
static int  _log_offset = 0;

static int
_myvfprintf(FILE*       stream,
            const char* format,
            va_list     ap)
{
  UNUSED_PARAM(stream);
  int written;
  written      = vsprintf(_log_output + _log_offset, format, ap);
  _log_offset += written;
  return written;
}

static void
_normalizeLogOutput()
{
  char* start = &_log_output[0];

  /* remove variable strings so we can compare deterministically in the tests */
  int startlen = strlen(start);
  /* printf("orig start='%s'\n", start); */

  /* replace final newline with terminating null */
  start[startlen - 1] = '\0';

  /* remove date header */
  int len = startlen - 28;
  memmove(start, start + 28, len);

  /* remove "ndcid=#####; " tokens */
  char* starttok = start;
  while ( ( starttok = strstr(starttok, "ndcid=") ) )
  {
    /* find end of id token */
    char* endtok = strstr(starttok + 7, " ");
    /* printf("starttok='%s'; endtok='%s'\n", starttok, endtok); */
    if (!endtok)
    {
      /* something's wrong, but we can't do anything about it */
      assert(false);
    }

    /* remove id token */
    len -= (endtok - starttok + 1);
    memmove(starttok, endtok + 1, len);
    /* printf("interim start='%s'\n", start); */
  }

  /* printf("final start='%s'\n", start); */
  _log_offset -= (startlen - len);
}

static void
_test_log_generator_fn(const char**       chunk,
                       size_t*            len,
                       ls_data_free_func* free_fn,
                       void*              arg)
{
  _log_chunk* chunk_info = arg;
  _log_chunk* cur        = chunk_info ? chunk_info->cur : NULL;

  if (cur)
  {
    *chunk   = cur->chunk;
    *len     = cur->len;
    *free_fn = cur->free_fn;

    chunk_info->cur = cur->next;
  }
}

CTEST_DATA(ls_log)
{
  char* ignored;
};

static ls_loglevel _initlevel;
CTEST_SETUP(ls_log)
{
  UNUSED_PARAM(data);
  _initlevel = ls_log_get_level();
  ls_log_set_function(_myvfprintf);
}

CTEST_TEARDOWN(ls_log)
{
  UNUSED_PARAM(data);
  ls_log_set_function(NULL);
  ls_log_set_level(_initlevel);
}

CTEST(ls_log, message)
{
  const char* msg;
  msg = ls_log_level_name(LS_LOG_ERROR);
  ASSERT_STR(msg, "ERROR");
  msg = ls_log_level_name(LS_LOG_WARN);
  ASSERT_STR(msg, "WARN");
  msg = ls_log_level_name(LS_LOG_INFO);
  ASSERT_STR(msg, "INFO");
  msg = ls_log_level_name(LS_LOG_VERBOSE);
  ASSERT_STR(msg, "VERBOSE");
  msg = ls_log_level_name(LS_LOG_DEBUG);
  ASSERT_STR(msg, "DEBUG");
  msg = ls_log_level_name(LS_LOG_TRACE);
  ASSERT_STR(msg, "TRACE");
  msg = ls_log_level_name(LS_LOG_MEMTRACE);
  ASSERT_STR(msg, "MEMTRACE");
}

CTEST2(ls_log, levels)
{
  UNUSED_PARAM(data);
  ls_log_set_level(LS_LOG_DEBUG);

  _log_offset = 0;
  ls_log(LS_LOG_ERROR, "This is a test error");
  _normalizeLogOutput();
  ASSERT_STR(_log_output, "[\x1b[31mERROR   \x1b[0m]: This is a test error");

  _log_offset = 0;
  ls_log(LS_LOG_WARN, "This is a test warning: %s", "with string");
  _normalizeLogOutput();
  ASSERT_STR(_log_output,
             "[\x1b[33mWARN    \x1b[0m]: This is a test warning: with string");

  _log_offset = 0;
  ls_log(LS_LOG_INFO, "Information: %d", 4);
  _normalizeLogOutput();
  ASSERT_STR(_log_output, "[\x1b[35mINFO    \x1b[0m]: Information: 4");

  _log_offset = 0;
  ls_log(LS_LOG_VERBOSE, "Verbose");
  _normalizeLogOutput();
  ASSERT_STR(_log_output, "[\x1b[32mVERBOSE \x1b[0m]: Verbose");

  _log_offset = 0;
  ls_log(LS_LOG_DEBUG, "%s", "Debug");
  _normalizeLogOutput();
  ASSERT_STR(_log_output, "[\x1b[34mDEBUG   \x1b[0m]: Debug");
}

CTEST2(ls_log, ndc)
{
  UNUSED_PARAM(data);
  ls_log_set_level(LS_LOG_DEBUG);

  _log_offset = 0;
  ls_log(LS_LOG_DEBUG, "test");
  _normalizeLogOutput();
  ASSERT_STR(_log_output, "[\x1b[34mDEBUG   \x1b[0m]: test");

  int depth = ls_log_push_ndc("jid=%s", "user1@dom.com/res");
  ASSERT_EQUAL(depth, 1);

  _log_offset = 0;
  ls_log(LS_LOG_DEBUG, "test");
  _normalizeLogOutput();
  ASSERT_STR(_log_output,
             "[\x1b[34mDEBUG   \x1b[0m]: {jid=user1@dom.com/res} test");

  ls_log_pop_ndc(depth);

  depth = ls_log_push_ndc("a");
  /* int depth2 = */ ls_log_push_ndc("b");
  int depth3 = ls_log_push_ndc("c");
  ASSERT_EQUAL(depth3, 3);

  _log_offset = 0;
  ls_log(LS_LOG_DEBUG, "test");
  _normalizeLogOutput();
  ASSERT_STR(_log_output, "[\x1b[34mDEBUG   \x1b[0m]: {a} {b} {c} test");

  ls_log_pop_ndc(depth3);

  _log_offset = 0;
  ls_log(LS_LOG_DEBUG, "test");
  _normalizeLogOutput();
  ASSERT_STR(_log_output, "[\x1b[34mDEBUG   \x1b[0m]: {a} {b} test");

  /* skip popping depth2 ("b") */
  _log_offset = 0;
  ls_log_pop_ndc(depth);
  _normalizeLogOutput();
  ASSERT_STR(_log_output,
             "[\x1b[33mWARN    \x1b[0m]: {a} {b} ndc depth mismatch on pop (expected 2, got 1)");

  _log_offset = 0;
  ls_log(LS_LOG_DEBUG, "test");
  _normalizeLogOutput();
  ASSERT_STR(_log_output, "[\x1b[34mDEBUG   \x1b[0m]: test");

  /* noop */
  _log_offset = 0;
  ls_log_pop_ndc(0);
  ASSERT_EQUAL(_log_offset, 0);
}

CTEST2(ls_log, err)
{
  UNUSED_PARAM(data);
  ls_err err;
  ls_log_set_level(LS_LOG_ERROR);

  _log_offset = 0;
  ls_log_err(LS_LOG_ERROR, NULL, "This is a test error");
  _normalizeLogOutput();
  ASSERT_STR(_log_output, "[\x1b[31mERROR   \x1b[0m]: This is a test error");

  _log_offset = 0;
  LS_ERROR(&err, LS_ERR_INVALID_ARG);
  ls_log_err(LS_LOG_ERROR, &err, "foo");
  _normalizeLogOutput();
  ASSERT_STR(_log_output,
             "[\x1b[31mERROR   \x1b[0m]: reason(invalid argument): foo");

  _log_offset = 0;
  ls_log_err(LS_LOG_DEBUG, &err, "foo");
  ASSERT_EQUAL(_log_offset, 0);

  _log_offset = 0;
  ls_log_set_level(LS_LOG_WARN);
  LS_ERROR(&err, LS_ERR_TIMEOUT);
  ls_log_err(LS_LOG_WARN, NULL, "This is a test warning: %s", "timeout");
  _normalizeLogOutput();
  ASSERT_STR(_log_output,
             "[\x1b[33mWARN    \x1b[0m]: This is a test warning: timeout");

  _log_offset = 0;
  ls_log_set_level(LS_LOG_INFO);
  LS_ERROR(&err, LS_ERR_USER);
  ls_log_err(LS_LOG_INFO, &err, "Information: %d", 4);
  _normalizeLogOutput();
  ASSERT_STR(_log_output,
             "[\x1b[35mINFO    \x1b[0m]: reason(user-defined error): Information: 4");

  _log_offset = 0;
  ls_log_set_level(LS_LOG_VERBOSE);
  ls_log_err(LS_LOG_VERBOSE, NULL, "Verbose");
  _normalizeLogOutput();
  ASSERT_STR(_log_output, "[\x1b[32mVERBOSE \x1b[0m]: Verbose");

  ls_log_set_level(0);
  ASSERT_EQUAL(ls_log_get_level(), LS_LOG_NONE);
}

CTEST2(ls_log, chunked)
{
  UNUSED_PARAM(data);
  ls_log_set_level(LS_LOG_ERROR);

  char expected[256];
  sprintf(expected, "%s", "[\x1b[31mERROR   \x1b[0m]: empty");
  _log_offset = 0;
  ls_log_chunked(LS_LOG_ERROR, _test_log_generator_fn, NULL, "empty");
  _normalizeLogOutput();
  ASSERT_STR(_log_output, expected);

  struct _log_chunk_int chunk1 = { .chunk = "first" };
  chunk1.cur = &chunk1;
  sprintf(expected, "%s", "[\x1b[31mERROR   \x1b[0m]: onefirst");
  _log_offset = 0;
  ls_log_chunked(LS_LOG_ERROR, _test_log_generator_fn, &chunk1, "one");
  _normalizeLogOutput();
  ASSERT_STR(_log_output, expected);

  struct _log_chunk_int chunk2 = { .chunk = "second", .len = 3 };
  chunk1.cur  = &chunk1;
  chunk1.next = &chunk2;
  sprintf(expected, "%s", "[\x1b[31mERROR   \x1b[0m]: twofirstsec");
  _log_offset = 0;
  ls_log_chunked(LS_LOG_ERROR, _test_log_generator_fn, &chunk1, "two");
  _normalizeLogOutput();
  ASSERT_STR(_log_output, expected);

  struct _log_chunk_int chunk3 =
  { .chunk = ls_data_strdup("third"), .free_fn = ls_data_free };
  chunk1.cur  = &chunk1;
  chunk2.next = &chunk3;
  sprintf(expected, "%s", "[\x1b[31mERROR   \x1b[0m]: threefirstsecthird");
  _log_offset = 0;
  ls_log_chunked(LS_LOG_ERROR, _test_log_generator_fn, &chunk1, "three");
  _normalizeLogOutput();
  ASSERT_STR(_log_output, expected);
}

CTEST2(ls_log, set_level)
{
  UNUSED_PARAM(data);
  ls_log_set_level(LS_LOG_ERROR);
  _log_offset = 0;
  ls_log(LS_LOG_MEMTRACE, "MemTrace");
  ASSERT_EQUAL(_log_offset, 0);
  _log_offset = 0;
  ls_log(LS_LOG_TRACE, "Trace");
  ASSERT_EQUAL(_log_offset, 0);
  _log_offset = 0;
  ls_log(LS_LOG_DEBUG, "Debug");
  ASSERT_EQUAL(_log_offset, 0);
  _log_offset = 0;
  ls_log(LS_LOG_DEBUG, "Verbose");
  ASSERT_EQUAL(_log_offset, 0);
  _log_offset = 0;
  ls_log(LS_LOG_DEBUG, "Information");
  ASSERT_EQUAL(_log_offset, 0);
  _log_offset = 0;
  ls_log(LS_LOG_DEBUG, "Warn");
  ASSERT_EQUAL(_log_offset, 0);
  _log_offset = 0;
  ls_log(LS_LOG_ERROR, "Error");
  ASSERT_NOT_EQUAL(_log_offset, 0);

  ls_log_set_level(LS_LOG_NONE);
  _log_offset = 0;
  ls_log(LS_LOG_ERROR, "Error");
  ASSERT_EQUAL(_log_offset, 0);
}

CTEST2(ls_log, format_timeval)
{
  UNUSED_PARAM(data);

  struct timeval tv = {0, 0};
  ls_log_format_timeval(NULL, NULL);
  ls_log_format_timeval(NULL, "foo");
  ls_log_format_timeval(&tv,  "foo");
}
