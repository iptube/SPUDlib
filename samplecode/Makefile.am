bin_PROGRAMS = spudtest spudecho spudload

AM_CPPFLAGS = -I../include -Wall -Wextra -Werror -g -std=c++11

spudtest_LDADD = ../src/libspud.la
spudecho_LDADD = ../src/libspud.la
spudload_LDADD = ../src/libspud.la -lstdc++

spudtest_SOURCES = spudtest.c iphelper.c sockethelper.c sockaddr_util.c
spudecho_SOURCES = spudecho.c iphelper.c sockethelper.c sockaddr_util.c htable.c
spudload_SOURCES = spudload.c iphelper.c sockethelper.c sockaddr_util.c htable.c gauss.cc
