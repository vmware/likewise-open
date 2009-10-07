lib_LTLIBRARIES = libregclient.la

libregclient_la_SOURCES = \
    clientipc.c \
    regclient.c
        
libregclient_la_CPPFLAGS = \
    $(AM_CFLAGS)           \
    @LWADVAPI_INCLUDES@    \
    @LWMSG_INCLUDES@
    
libregclient_la_LIBADD = \
	@top_builddir@/utils/libregutils.la \
	@top_builddir@/ipc/libregipc.la \
	@LWADVAPI_NOTHR_LIBS@ \
	@LWMSG_NOTHR_LIBS@
