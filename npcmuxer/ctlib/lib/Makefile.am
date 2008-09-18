noinst_LTLIBRARIES = libct.la
libct_la_SOURCES = ctstatus.c ctmemory.c ctlock.c ctstring.c ctlogger.c ctdaemon.c ctfileutils.c ctserver.c ctlist.c
libct_la_CPPFLAGS = -I$(srcdir)/../include -D_GNU_SOURCE
libct_la_CFLAGS=-Wall -Werror -g -fno-strict-aliasing -fPIC
libct_la_LIBADD=-lpthread