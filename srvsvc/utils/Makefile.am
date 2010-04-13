AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libsrvsvcutils.la

libsrvsvcutils_la_SOURCES = \
    consolelog.c \
    filelog.c    \
    fileutils.c  \
    globals.c    \
    hostinfo.c   \
    logger.c     \
    loginfo.c    \
    memory.c     \
    strings.c    \
    sysfuncs.c   \
    syslog.c

libsrvsvcutils_la_LIBADD = \
    $(DCERPC_LIBS)         \
    $(LIBUNISTR_LIBS)

libsrvsvcutils_la_CFLAGS = \
    -DSRVSVC_BUILD

libsrvsvcutils_la_CPPFLAGS = \
    $(AM_CPPFLAGS)           \
    -DSRVSVC_BUILD           \
    $(DCERPC_INCLUDES)       \
    $(LIBUNISTR_INCLUDES)    \
    $(LWADVAPI_INCLUDES)

libsrvsvcutils_la_LDFLAGS = \
    $(DCERPC_LDFLAGS)       \
    $(LIBUNISTR_LDFLAGS)    \
    $(LWADVAPI_LDFLAGS)
