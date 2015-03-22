## Process this file with automake to produce Makefile.in
if GCOV
  MY_CFLAGS_GCOV =  -fprofile-arcs -ftest-coverage -g
  MY_LDFLAGS_GCOV = -lgcov
endif

AUTOMAKE_OPTIONS = subdir-objects
AM_CPPFLAGS = $(MY_AM_CPPFLAGS) $(MY_CFLAGS_GCOV) -I$(top_srcdir)/include -O2 -g -pedantic -Wall -Wextra -Wno-unknown-pragmas -Werror-implicit-function-declaration -Werror -Wno-unused-parameter -Wdeclaration-after-statement -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes

include_HEADERS = ../include/ls_basics.h ../include/ls_error.h ../include/ls_event.h ../include/ls_htable.h ../include/ls_log.h ../include/ls_mem.h ../include/ls_sockaddr.h ../include/spud.h ../include/tube.h
cncbordir = $(includedir)/cn-cbor
cncbor_HEADERS = ../include/cn-cbor/cn-cbor.h ../include/cn-cbor/cn-create.h

lib_LTLIBRARIES = libspud.la
libspud_la_SOURCES = spud.c tube.c ls_error.c ls_log.c ls_str.c ls_mem.c ls_sockaddr.c ls_htable.c ls_eventing.c cn-cbor/cn-cbor.c cn-cbor/cn-encoder.c cn-cbor/cn-create.c cn-cbor/cn-error.c ls_eventing.h ls_eventing_int.h ls_log_int.h ls_pool_types.h ls_str.h cn-cbor/cbor.h cn-cbor/cn-encoder.h
libspud_la_LDFLAGS = $(MY_LDFLAGS_GCOV) -version-info 1:0:0

clean-local:
	rm -f *.g* *.final *.shorten *.ce* *.regmove *.split *.dfinish *.dwarf* *.nothrow *.mach *.vartrack *.compgotos *.stack *.sched* *.split* *.bbro *.rtl_dce *.cprop_hardreg  *.peephole* *.jump2 *.barriers *.alignments *.csa *.dse* *.pro* *.ree *.split* *.postreload *.reload *.ira *.asmcons *.mode_sw *.subreg* *.split1 *.outof* *.combine *.ud_dce *.init-regs* *.fwprop* *.loop* *.cse* *.cprop* *.reginfo* *.cprop* *.pre* *.jump* *.dfinit* *.into* *.vregs* *.expand*
