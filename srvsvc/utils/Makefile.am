AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libsrvsvcutils.la

libsrvsvcutils_la_SOURCES = \
    srvsvccfg.c      \
    srvsvcfutils.c   \
    srvsvchost.c     \
    srvsvclogger.c   \
    srvsvccfgutils.c \
    srvsvcmem.c      \
    srvsvcstack.c    \
    srvsvcstr.c      \
    globals.c        \
    sysfuncs.c

libsrvsvcutils_la_LIBADD = \
    @DCERPC_LIBS@    \
    @LIBUNISTR_LIBS@

libsrvsvcutils_la_CFLAGS = $(INCLUDES) -DSRVSVC_BUILD $(AM_CFLAGS)
libsrvsvcutils_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    @DCERPC_INCLUDES@   \
    @LIBUNISTR_INCLUDES@

libsrvsvcutils_la_LDFLAGS = \
    @DCERPC_LDFLAGS@   \
    @LIBUNISTR_LDFLAGS@
