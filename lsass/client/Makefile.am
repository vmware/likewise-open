lib_LTLIBRARIES = liblsaclient.la

liblsaclient_la_SOURCES = \
    artefacts.c \
    auth.c      \
    clientipc.c \
    config.c    \
    groups.c    \
    gss.c       \
    logging.c   \
    metrics.c   \
    provider.c  \
    session.c   \
    status.c    \
    tracing.c   \
    users.c
    
liblsaclient_la_CPPFLAGS = \
	@LWMSG_INCLUDES@

liblsaclient_la_LIBADD = \
	@top_builddir@/utils/liblsautils.la \
	@top_builddir@/ipc/liblsaipc.la \
	@LWMSG_NOTHR_LIBS@ \
	@SOCKET_LIBS@
