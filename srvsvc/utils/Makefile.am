AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libsrvsvcutils.la

libsrvsvcutils_la_SOURCES = \
    fileutils.c  \
    globals.c    \
    hostinfo.c   \
    memory.c     \
    strings.c    \
    sysfuncs.c

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
