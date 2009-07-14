lib_LTLIBRARIES = liblsalocalclient.la

liblsalocalclient_la_SOURCES = \
    groups.c

liblsalocalclient_la_CPPFLAGS = \
	@LWMSG_INCLUDES@

liblsalocalclient_la_LIBADD = \
	@top_builddir@/utils/liblsautils.la \
	@top_builddir@/ipc/liblsaipc.la \
	@top_builddir@/provider/local-provider/ipc/liblsalocalipc.la \
	@LWMSG_NOTHR_LIBS@ \
	@SOCKET_LIBS@
