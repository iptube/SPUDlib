#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([spudlib], [0.3pre], [palmarti@cisco.com])
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


dnl Check for check
AC_ARG_ENABLE([check],
  AS_HELP_STRING([--disable-check], [Do not build unit tests]),,
  [use_check=yes]
)

if test "$enable_check" = "yes" ; then
  PKG_CHECK_MODULES([CHECK], [check >= 0.9.8])

  dnl As of version 0.9.10, check does not include -pthread in its Cflags or
  dnl Libs even though it does depend on it. Assuming that check.pc may one day
  dnl be fixed for that, only add it here if it's not there already.  It really
  dnl belongs in LDFLAGS here (since check is a static lib and doesn't expose
  dnl pthread functions in its header), but it can't be added to CHECK_LIBS,
  dnl since automake 1.13 will barf about adding -pthread to *_LDADD rather
  dnl than *_LDFLAGS.  However libtool does also include CFLAGS when linking.
  case $CHECK_CFLAGS in
    *-pthread*)
      ;;
    *)
      CHECK_CFLAGS="-pthread $CHECK_CFLAGS"
      ;;
  esac
fi
AM_CONDITIONAL(USE_CHECK, [test $enable_check = "yes"])


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
AC_CONFIG_FILES([spudlib.pc])

AC_OUTPUT
