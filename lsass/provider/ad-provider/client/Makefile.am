lib_LTLIBRARIES = liblsaadclient.la

liblsaadclient_la_SOURCES = \
    cache.c     \
    groups.c    \
    users.c
    
liblsaadclient_la_CPPFLAGS = \
	@LWMSG_INCLUDES@

liblsaadclient_la_LIBADD = \
	@top_builddir@/utils/liblsautils.la \
	@top_builddir@/ipc/liblsaipc.la \
	@top_builddir@/provider/ad-provider/ipc/liblsaadipc.la \
	@LWMSG_NOTHR_LIBS@ \
	@SOCKET_LIBS@
