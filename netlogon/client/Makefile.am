AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = liblwnetclientapi.la

liblwnetclientapi_la_CPPFLAGS = \
    $(LWADVAPI_INCLUDES) \
    $(AM_CPPFLAGS)

liblwnetclientapi_la_LIBADD =   \
    $(LWMSG_LIBS)               \
    $(LWADVAPI_NOTHR_LIBS)      \
    @top_builddir@/common/liblwnetcommon.la

liblwnetclientapi_la_LDFLAGS = \
    $(LWMSG_LDFLAGS)           \
    $(LWADVAPI_LDFLAGS)

liblwnetclientapi_la_SOURCES = \
    dcinfo.c     \
    ipc_client.c \
    krb5env.c
