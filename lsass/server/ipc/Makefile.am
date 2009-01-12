noinst_LTLIBRARIES = liblsaserveripc.la

liblsaserveripc_la_SOURCES = \
       globals.c       \
       ipc_artefact.c  \
       ipc_auth.c      \
       ipc_config.c    \
       ipc_error.c     \
       ipc_group.c     \
       ipc_gss.c       \
       ipc_log.c       \
       ipc_metrics.c   \
       ipc_provider.c  \
       ipc_session.c   \
       ipc_status.c    \
       ipc_state.c     \
       ipc_tracing.c   \
       ipc_user.c

liblsaserveripc_la_CPPFLAGS =         \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	@LWMSG_INCLUDES@


liblsaserveripc_la_LIBADD = \
	@top_builddir@/ipc/liblsaipc.la \
	@top_builddir@/server/api/liblsaserverapi.la \
	@top_builddir@/logging_r/liblsalog_r.la \
	@LWMSG_LIBS@ \
	@PTHREAD_LIBS@


liblsaserveripc_la_LDFLAGS = -static
