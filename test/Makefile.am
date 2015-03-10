AUTOMAKE_OPTIONS = subdir-objects

if CHECK
  MY_CFLAGS_1 = -I../include -g
  MY_LDFLAGS_1 = -lcheck -g
  TESTS = check_spudlib
  check_PROGRAMS = check_spudlib
  check_spudlib_SOURCES = spud_test.c testmain.c ../src/spud.c
  check_spudlib_LDADD = ../src/libspud.la

AM_CPPFLAGS = $(MY_CFLAGS_1)
check_spudlib_LDFLAGS = $(MY_LDFLAGS_1)


clean-local:
	rm -f *.g*


endif
