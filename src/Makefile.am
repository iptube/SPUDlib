## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = $(MY_AM_CPPFLAGS) -I../include -O2 -g -Wall -Wextra -Wno-unknown-pragmas -Werror-implicit-function-declaration -Werror -Wno-unused-parameter -Wdeclaration-after-statement -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes


include_HEADERS = ../include/spudlib.h


lib_LTLIBRARIES = libspud.la
libspud_la_SOURCES = spudlib.c tube.c ls_error.c ls_log.c ls_str.c ls_mem.c ls_sockaddr.c ls_eventing.c ls_htable.c cn-cbor.c
libspud_la_LDFLAGS = -version-info 1:0:0
