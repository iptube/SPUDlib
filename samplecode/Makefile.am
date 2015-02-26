bin_PROGRAMS = spudtest spudecho

AM_CPPFLAGS = -I../include	-Wall -Wextra -Werror -g

spudtest_LDADD = ../src/libspud.la
spudecho_LDADD = ../src/libspud.la

spudtest_SOURCES = spudtest.c iphelper.c sockethelper.c sockaddr_util.c
spudecho_SOURCES = spudecho.c iphelper.c sockethelper.c sockaddr_util.c htable.c mem.c error.c str.c
