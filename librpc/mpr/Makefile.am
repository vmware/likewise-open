librpc_srcdir=$(top_srcdir)
libmsrpc_srcdir=$(top_srcdir)/msrpc

lib_LTLIBRARIES = libmpr.la

libmpr_la_SOURCES =         \
    wnet_addconnection2.c   \
    wnet_cancelconnection2.c

LIBRPC_INCLUDES = -I$(librpc_srcdir)/include

libmpr_la_CFLAGS =        \
    $(AM_CFLAGS)          \
    $(INCLUDES)           \
    $(LWBASE_INCLUDES)    \
    $(LWADVAPI_INCLUDES)  \
    $(UNISTR_INCLUDES)    \
    $(LIBRPC_INCLUDES)

libmpr_la_LDFLAGS =       \
    $(LWBASE_LDFLAGS)     \
    $(LWADVAPI_LDFLAGS)   \
    $(UNISTR_LDFLAGS)

libmpr_la_LIBADD= \
    -ldcerpc -lunistr -llwbase
