/**
 * \file
 * \brief
 * Functions for simplified logging
 *
 * Copyrights
 *
 * Portions created or assigned to Cisco Systems, Inc. are
 * Copyright (c) 2015 Cisco Systems, Inc.  All Rights Reserved.
 */
#include <stdarg.h>
#include <stdio.h>
#include "ls_basics.h"

typedef enum
{
    LS_LOG_ERROR = 0,
    LS_LOG_WARN,
    LS_LOG_INFO,
    LS_LOG_VERBOSE,
    LS_LOG_DEBUG
} ls_loglevel;

/**
 * Retrieve the string version of the ls_loglevel enum.
 *
 * \param level The log level to lookup
 * \retval const char * The message for {level}
 */
LS_API const char *ls_log_level_name(ls_loglevel level);

/**
 * Function like vfprintf to be used for logging.
 *
 * Note: Supplied function will be called three times for each log
 * message; once for date/time/level preamble, once for the message and
 * once for a trailing newline.
 *
 * \param stream Output stream, always stderr.
 * \param format Format string like vfprintf.
 * \param ap Additional parameters to interpolate into {fmt}.
 * \retval Number of bytes written.
 */
typedef int (*ls_log_vararg_function)(FILE *stream, const char *format, va_list ap);

/**
 * Set the logging function.
 *
 * Log function defaults to vfprintf. A null parameter resets the
 * log function to its default.
 *
 * Note: Not thread-safe.
 *
 * \param fn The vfprintf-like function to use.
 */
LS_API void ls_log_set_function(ls_log_vararg_function fn);

/**
 * Set the current log level, defaults to LS_LOG_INFO.
 *
 * Everything at this level or less verbose than this level will be printed.
 *
 * Note: Not thread-safe.
 *
 * \param level The new log level.
 */
LS_API void ls_log_set_level(ls_loglevel level);

/**
 * Get the current log level.
 *
 * \retval The current log level.
 */
LS_API ls_loglevel ls_log_get_level(void);

/**
 * Log at the given level to stderr.
 *
 * All errors while logging are ignored (so this routine is not
 * guaranteed to log).  Extra parameters are injected into {fmt}
 * using the rules from vfprintf.
 *
 * Log messages are prepended with date/time and log level and
 * appended with a newline; YYYY-MM-DDTHH-MM-SS[level]: {fmt}\n
 *
 * Note: Not thread-safe with respect to the current log level.
 *
 * \invariant fmt != NULL
 * \param[in] level The log level for this message.
 * \param[in] fmt The printf-style format to log
 * \param[in] ... Extra parameters to interpolate into {fmt}.
 */
LS_API void ls_log(ls_loglevel level, const char *fmt, ...);

#define LS_LOG_ERR(err, what) ls_log(LS_LOG_ERROR, "%s:%d(%s) %d, %s", __FILE__, __LINE__, (what), (err).code, ls_err_message((err).code))
#define LS_LOG_PERROR(what) ls_log(LS_LOG_ERROR, "%s:%d(%s) %d, %s", __FILE__, __LINE__, (what), errno, strerror(errno))
