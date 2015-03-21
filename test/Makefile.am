AUTOMAKE_OPTIONS = subdir-objects

if USE_CHECK
  MY_CFLAGS_1 = -I$(top_srcdir)/include -g
  MY_LDFLAGS_1 = -g
  TESTS = check_spudlib
  check_PROGRAMS = check_spudlib
  check_spudlib_SOURCES = ls_str_test.c ls_sockaddr_test.c ls_error_test.c ls_mem_test.c ls_log_test.c ls_htable_test.c ls_eventing_test.c spud_test.c tube_test.c cbor_test.c test_utils.c test_utils.h testmain.c
  check_spudlib_LDADD = ../src/libspud.la

AM_CPPFLAGS = $(MY_CFLAGS_1) $(CHECK_CFLAGS)
check_spudlib_LDFLAGS = $(MY_LDFLAGS_1) $(CHECK_LIBS)


clean-local:
	rm -f *.g*


endif
