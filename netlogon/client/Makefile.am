lib_LTLIBRARIES = liblwnetclientapi.la

liblwnetclientapi_la_CPPFLAGS = \
    $(LWADVAPI_INCLUDES)

liblwnetclientapi_la_LIBADD =   \
    $(LWMSG_LIBS)               \
    $(LWADVAPI_NOTHR_LIBS)      \
    @top_builddir@/utils/liblwnetutils.la \
    @top_builddir@/ipc/liblwnetipc.la

liblwnetclientapi_la_LDFLAGS = \
    $(LWMSG_LDFLAGS)           \
    $(LWADVAPI_LDFLAGS)

liblwnetclientapi_la_SOURCES = \
    dcinfo.c     \
    ipc_client.c \
    krb5env.c
