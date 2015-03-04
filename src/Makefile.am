## Process this file with automake to produce Makefile.in
if GCOV
  MY_CFLAGS_GCOV =  -fprofile-arcs -ftest-coverage -g
  MY_LDFLAGS_GCOV = -lgcov
endif

AM_CPPFLAGS = $(MY_AM_CPPFLAGS) $(MY_CFLAGS_GCOV) -I../include -O2 -g -Wall -Wextra -Wno-unknown-pragmas -Werror-implicit-function-declaration -Werror -Wno-unused-parameter -Wdeclaration-after-statement -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes

include_HEADERS = ../include/spudlib.h

lib_LTLIBRARIES = libspud.la
libspud_la_SOURCES = spudlib.c tube.c ls_error.c ls_log.c ls_str.c ls_mem.c ls_sockaddr.c ls_htable.c cn-cbor.c
libspud_la_LDFLAGS = $(MY_LDFLAGS_GCOV) -lgcov -version-info 1:0:0

clean-local:
	rm -f *.g* *.final *.shorten *.ce* *.regmove *.split *.dfinish *.dwarf* *.nothrow *.mach *.vartrack *.compgotos *.stack *.sched* *.split* *.bbro *.rtl_dce *.cprop_hardreg  *.peephole* *.jump2 *.barriers *.alignments *.csa *.dse* *.pro* *.ree *.split* *.postreload *.reload *.ira *.asmcons *.mode_sw *.subreg* *.split1 *.outof* *.combine *.ud_dce *.init-regs* *.fwprop* *.loop* *.cse* *.cprop* *.reginfo* *.cprop* *.pre* *.jump* *.dfinit* *.into* *.vregs* *.expand*
