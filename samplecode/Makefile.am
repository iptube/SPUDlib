bin_PROGRAMS = spudtest spudecho spudload

AM_CPPFLAGS = -I$(top_srcdir)/include -Wall -Wextra -Werror -g

spudtest_LDADD = ../src/libspud.la
spudecho_LDADD = ../src/libspud.la
spudload_LDADD = ../src/libspud.la

spudtest_SOURCES = spudtest.c
spudecho_SOURCES = spudecho.c
spudload_SOURCES = spudload.c gauss.c gauss.h
