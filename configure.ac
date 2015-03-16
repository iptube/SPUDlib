#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([spudlib], [0.1], [palmarti@cisco.com])
AC_CONFIG_SRCDIR([include/spud.h])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR
LT_INIT

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_C99

AC_ARG_ENABLE(gcov,
AS_HELP_STRING([--enable-gcov],
               [enable test coverage with gcov @<:@default=no@:>@]),
              [case "${enableval}" in
                 yes) gcov=true ;;
                 no)  gcov=false ;;
                 *)   AC_MSG_ERROR([bad value ${enableval} for --enable-gcov]) ;;
               esac],
              [gcov=false])

AM_CONDITIONAL(GCOV, test x"$gcov" = "xtrue")

# Support for running test cases using valgrind:
use_valgrind=false
AC_ARG_ENABLE(valgrind,
[  --enable-valgrind       Use valgrind when running unit tests. ],
[ use_valgrind=true ])

if [[ "$use_valgrind" = "true" ]]; then
        AC_CHECK_PROG(HAVE_VALGRIND, valgrind, yes, no)

        if [[ "$HAVE_VALGRIND" = "no" ]]; then
                AC_MSG_ERROR([Valgrind not found in PATH. ])
        fi
fi
AM_CONDITIONAL(USE_VALGRIND, $use_valgrind)


use_check=false
AC_ARG_WITH(check,
  [  --with-check       Use check when running unit tests. ],
  [ use_check=true ])

if [[ "$use_check" = "true" ]]; then

  PKG_CHECK_MODULES([CHECK], [check >= 0.9.14])
  AC_SUBST(CHECK_CFLAGS)
  AC_SUBST(CHECK_LIBS)
  if [[ "$HAVE_CHECK" = "no" ]]; then
          AC_MSG_ERROR([check not found])
  fi
fi
AM_CONDITIONAL(CHECK, test -n "$CHECK_LIBS")
AM_CONDITIONAL(USE_CHECK, $use_check)


# Checks for header files.
AC_CHECK_HEADERS([netinet/in.h stddef.h stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([memset arc4random])
AC_CHECK_FILES([/dev/urandom])

AC_CHECK_LIB(pthread, pthread_create,,
             [AC_MSG_ERROR([required library pthread missing])])

AC_CHECK_LIB(m, tan,,
             [AC_MSG_ERROR([required library m missing])])


AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
                 samplecode/Makefile])

AC_OUTPUT
