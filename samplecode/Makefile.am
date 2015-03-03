bin_PROGRAMS = spudtest spudecho spudload

AM_CPPFLAGS = -I../include -Wall -Wextra -Werror -g

spudtest_LDADD = ../src/libspud.la
spudecho_LDADD = ../src/libspud.la
spudload_LDADD = ../src/libspud.la -lm

spudtest_SOURCES = spudtest.c iphelper.c sockethelper.c sockaddr_util.c
spudecho_SOURCES = spudecho.c iphelper.c sockethelper.c sockaddr_util.c
spudload_SOURCES = spudload.c iphelper.c sockethelper.c sockaddr_util.c gauss.c
