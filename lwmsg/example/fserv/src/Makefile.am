lib_LTLIBRARIES = libfservcommon.la libfservclient.la
sbin_PROGRAMS = fserver
bin_PROGRAMS = fcat fsink

AM_CPPFLAGS = @FSERV_CPPFLAGS@
AM_CFLAGS = @FSERV_CFLAGS@ -Wall -Werror
AM_LDFLAGS = @FSERV_LDFLAGS@ -static

libfservcommon_la_SOURCES = protocol.c
libfservcommon_la_CPPFLAGS = $(AM_CPPFLAGS) $(LWMSG_CPPFLAGS) -DLWMSG_NO_THREADS
libfservcommon_la_LIBADD = $(LIB_LWMSG_NOTHR)

libfservclient_la_SOURCES = fserv.c
libfservclient_la_CPPFLAGS = $(AM_CPPFLAGS) $(LWMSG_CPPFLAGS) -DLWMSG_NO_THREADS
libfservclient_la_LIBADD = libfservcommon.la

fserver_SOURCES = fserver.c fserver-dispatch.c
fserver_LDADD = libfservcommon.la $(LIB_LWMSG)

fcat_SOURCES = fcat.c
fcat_LDADD = libfservclient.la

fsink_SOURCES = fsink.c
fsink_LDADD = libfservclient.la