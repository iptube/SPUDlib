/**
 * \file
 *
 * Copyrights
 *
 * Portions created or assigned to Cisco Systems, Inc. are
 * Copyright (c) 2015 Cisco Systems, Inc.  All Rights Reserved.
 */

#include <time.h>
#include <assert.h>
#include "ls_log.h"

/*****************************************************************************
 * Internal type definitions
 */

static const char *_LOG_MSG_TABLE[] = {
    "ERROR",
    "WARN",
    "INFO",
    "VERBOSE",
    "DEBUG"
};
static ls_loglevel _ls_loglevel = LS_LOG_INFO;
static ls_log_vararg_function _ls_log_vararg_function = vfprintf;

static int _ls_log_fixed_function(FILE *stream, const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = _ls_log_vararg_function(stream, fmt, ap);
    va_end(ap);

    return ret;
}

LS_API const char * ls_log_level_name(ls_loglevel level)
{
    return _LOG_MSG_TABLE[level];
}

LS_API void ls_log_set_function(ls_log_vararg_function fn)
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

LS_API void ls_log_set_level(ls_loglevel level)
{
    _ls_loglevel = level;
}

LS_API ls_loglevel ls_log_get_level(void)
{
    return _ls_loglevel;
}

static int level_colors[] = {
    31,
    33,
    32,
    34,
    30
};

LS_API void ls_log(ls_loglevel level, const char *fmt, ...)
{
    time_t t;
    struct tm local;
    va_list ap;

    assert(fmt);

    if (level > _ls_loglevel)
        return;

    t = time(NULL);
    if (t == (time_t)-1)
        return;
    if (!localtime_r(&t, &local))
        return;

    _ls_log_fixed_function(stderr,
            "\x1b[1m%d-%2.2d-%2.2dT%2.2d:%2.2d:%2.2d\x1b[0m [\x1b[%dm%7s\x1b[39m]: ",
            local.tm_year+1900,
            local.tm_mon+1,
            local.tm_mday,
            local.tm_hour,
            local.tm_min,
            local.tm_sec,
            level_colors[level],
            ls_log_level_name(level));

    va_start(ap, fmt);
    _ls_log_vararg_function(stderr, fmt, ap);
    va_end(ap);
    _ls_log_fixed_function(stderr, "\n");
}
