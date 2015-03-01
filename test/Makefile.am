AUTOMAKE_OPTIONS = subdir-objects
if CHECK
  MY_CFLAGS_1 = -I../include -g
  MY_LDFLAGS_1 = -lcheck -g
  TESTS = check_spudlib
  check_PROGRAMS = check_spudlib
  check_spudlib_SOURCES = spud_test.c testmain.c ../src/spudlib.c
  check_spudlib_LDADD = ../src/libspud.la
if GCOV
  MY_CFLAGS_3 =  -fprofile-arcs -ftest-coverage -g
  MY_LDFLAGS_3 = -lgcov
else

endif

if USE_VALGRIND
  TESTS_ENVIRONMENT = ../../valgrind-wrapper
endif

AM_CPPFLAGS = $(MY_CFLAGS_1) $(MY_CFLAGS_2) $(MY_CFLAGS_3)
check_spudlib_LDFLAGS = $(MY_LDFLAGS_1) $(MY_LDFLAGS_2) $(MY_LDFLAGS_3)


clean-local:
	rm -f *.g*


endif
