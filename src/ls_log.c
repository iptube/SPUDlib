/**
 * \file
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */


#include "./ls_log_int.h"
#include "ls_log.h"
#include "ls_mem.h"

#include <time.h>
#include <string.h>
#include <assert.h>

/*****************************************************************************
 * Internal type definitions
 */

static const char* _LOG_MSG_TABLE[] = {
  "NONE",
  "ERROR",
  "WARN",
  "INFO",
  "VERBOSE",
  "DEBUG",
  "TRACE",
  "MEMTRACE"
};

static const int level_colors[] = {
  0,    /* NONE: reset */
  31,   /* ERROR: red */
  33,   /* WARN: yellow */
  35,   /* INFO: magenta */
  32,   /* VERBOSE: green */
  34,   /* DEBUG: blue */
  36,   /* TRACE: cyan */
  36,   /* MEMTRACE: cyan */
};

static ls_loglevel            _ls_loglevel            = LS_LOG_INFO;
static ls_log_vararg_function _ls_log_vararg_function = vfprintf;

typedef struct _ndc_node_int_t
{
  uint32_t                id;
  char*                   message;
  struct _ndc_node_int_t* next;
} _ndc_node_t;

static bool _ndc_enabled = true;
/* TODO: once we support threading, these will have to be thread-local
 * instead */
/* TODO:   of static */
static int          _ndc_depth = 0;
static _ndc_node_t* _ndc_head  = NULL;
static uint32_t     _ndc_count = 0;


static int
_ls_log_fixed_function(FILE*       stream,
                       const char* fmt,
                       ...)
__attribute__ ( ( __format__(__printf__, 2, 3) ) );
static int
_ls_log_fixed_function(FILE*       stream,
                       const char* fmt,
                       ...)
{
  va_list ap;
  int     ret;

  va_start(ap, fmt);
  ret = _ls_log_vararg_function(stream, fmt, ap);
  va_end(ap);

  return ret;
}

LS_API const char*
ls_log_level_name(ls_loglevel level)
{
  assert(LS_LOG_NONE <= (int)level);
  assert(LS_LOG_MEMTRACE >= level);

  return _LOG_MSG_TABLE[level];
}

LS_API void
ls_log_set_function(ls_log_vararg_function fn)
{
  if (!fn)
  {
    _ls_log_vararg_function = vfprintf;
  }
  else
  {
    _ls_log_vararg_function = fn;
  }
}

LS_API void
ls_log_set_level(ls_loglevel level)
{
  assert(LS_LOG_NONE <= (int)level);
  assert(LS_LOG_MEMTRACE >= level);

  _ls_loglevel = level;
}

LS_API void
ls_log_set_ndc_enabled(bool enabled)
{
  _ndc_enabled = enabled;
}

LS_API ls_loglevel
ls_log_get_level()
{
  return _ls_loglevel;
}

static void
_log_ndc_stack(_ndc_node_t* ndcNode)
{
  if (!ndcNode)
  {
    return;
  }

  if (ndcNode->next)
  {
    _log_ndc_stack(ndcNode->next);
  }

  _ls_log_fixed_function(stderr, "{ndcid=%u; %s} ",
                         ndcNode->id, ndcNode->message);
}

static bool
_log_prefix(ls_loglevel level)
{
  time_t    t;
  struct tm local;

  assert(LS_LOG_ERROR <= level);
  assert(LS_LOG_MEMTRACE >= level);

  if (level > _ls_loglevel)
  {
    return false;
  }

  /* TODO: cache time and update it asyncronously with a timer? */
  t = time(NULL);
  if ( (t == (time_t)-1) || !localtime_r(&t, &local) )
  {
    /* Note: both time() and localtime_r() only fail for */
    /* reasons that are difficult if impossible to create, */
    /* so don't worry about coverage over this return line. */
    return false;
  }

  _ls_log_fixed_function( stderr,
                          "\x1b[1m%d-%2.2d-%2.2dT%2.2d:%2.2d:%2.2d\x1b[0m [\x1b[%dm%-8s\x1b[0m]: ",
                          local.tm_year + 1900,
                          local.tm_mon + 1,
                          local.tm_mday,
                          local.tm_hour,
                          local.tm_min,
                          local.tm_sec,
                          level_colors[level],
                          ls_log_level_name(level) );

  if (_ndc_enabled)
  {
    _log_ndc_stack(_ndc_head);
  }

  return true;
}

LS_API int
ls_log_push_ndc(const char* fmt,
                ...)
{
  va_list      ap;
  int          messageLen;
  _ndc_node_t* newNode;
  assert(fmt);

  va_start(ap, fmt);
  if ( 0 > ( messageLen = vsnprintf(NULL, 0, fmt, ap) ) )
  {
    ls_log(LS_LOG_WARN,"invalid NDC format string: '%s'", fmt);
    return 0;
  }
  va_end(ap);

  newNode = ls_data_calloc( 1, sizeof(struct _ndc_node_int_t) );
  if (!newNode)
  {
    ls_log(LS_LOG_WARN, "could not push NDC: '%s' (out of memory)", fmt);
    return 0;
  }

  newNode->message = ls_data_malloc(messageLen + 1);
  if (!newNode->message)
  {
    ls_data_free(newNode);
    ls_log(LS_LOG_WARN, "could not push NDC: '%s' (out of memory)", fmt);
    return 0;
  }

  va_start(ap, fmt);
  messageLen = vsprintf(newNode->message, fmt, ap);
  assert(0 <= messageLen);
  va_end(ap);

  newNode->id   = _ndc_count++;
  newNode->next = _ndc_head;
  _ndc_head     = newNode;

  return ++_ndc_depth;
}

LS_API void
ls_log_pop_ndc(int ndc_depth)
{
  assert(0 <= ndc_depth);

  if (0 == ndc_depth)
  {
    return;
  }

  if (ndc_depth != _ndc_depth)
  {
    ls_log(LS_LOG_WARN, "ndc depth mismatch on pop (expected %d, got %d)",
           _ndc_depth, ndc_depth);
  }

  while (_ndc_head && _ndc_depth >= ndc_depth)
  {
    _ndc_node_t* prevHead;
    --_ndc_depth;
    prevHead  = _ndc_head;
    _ndc_head = prevHead->next;

    ls_data_free(prevHead->message);
    ls_data_free(prevHead);
  }
}

LS_API void
ls_log(ls_loglevel level,
       const char* fmt,
       ...)
{
  va_list ap;

  assert(fmt);

  if ( !_log_prefix(level) )
  {
    return;
  }

  va_start(ap, fmt);
  _ls_log_vararg_function(stderr, fmt, ap);
  va_end(ap);
  _ls_log_fixed_function(stderr, "\n");
}

LS_API void
ls_log_err(ls_loglevel level,
           ls_err*     err,
           const char* fmt,
           ...)
{
  va_list ap;

  assert(fmt);

  if ( !_log_prefix(level) )
  {
    return;
  }

  if (err && err->message)
  {
    /* err->code is almost always useless, since err->message is usually */
    /* already set with ls_err_message() in the LS_ERROR */
    /* macro. */
    _ls_log_fixed_function(stderr, "reason(%s): ", err->message);
  }

  va_start(ap, fmt);
  _ls_log_vararg_function(stderr, fmt, ap);
  va_end(ap);
  _ls_log_fixed_function(stderr, "\n");
}

LS_API void
ls_log_chunked(ls_loglevel         level,
               ls_log_generator_fn generator_fn,
               void*               arg,
               const char*         fmt,
               ...)
{
  va_list ap;

  assert(generator_fn);
  assert(fmt);

  if ( !_log_prefix(level) )
  {
    return;
  }

  va_start(ap, fmt);
  _ls_log_vararg_function(stderr, fmt, ap);
  va_end(ap);

  while (true)
  {
    const char*       chunk   = NULL;
    size_t            len     = 0;
    ls_data_free_func free_fn = NULL;

    generator_fn(&chunk, &len, &free_fn, arg);

    if (!chunk)
    {
      break;
    }

    if (0 == len)
    {
      _ls_log_fixed_function(stderr, "%s", chunk);
    }
    else
    {
      _ls_log_fixed_function(stderr, "%.*s", (int)len, chunk);
    }

    if (free_fn)
    {
      free_fn( (char*)chunk );
    }
  }

  _ls_log_fixed_function(stderr, "\n");
}

LS_API void
ls_log_format_timeval(struct timeval* tv,
                      const char*     tag)
{
  size_t     w_strftime;
  char       buf[28];
  struct tm* gm;

  if (!tv)
  {
    ls_log(LS_LOG_INFO, "%s: NULL timeval", tag);
    return;
  }
  gm = gmtime(&tv->tv_sec);
  if (!gm)
  {
    return;
  }
  w_strftime = strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", gm);
  if ( (w_strftime == 0) || (w_strftime > sizeof(buf) - 1) )
  {
    return;
  }
  if (snprintf(buf + w_strftime, sizeof(buf) - w_strftime, ".%06u",
               (unsigned)tv->tv_usec) < 0)
  {
    return;
  }
  ls_log(LS_LOG_INFO, "%s: %s", tag, buf);
}
