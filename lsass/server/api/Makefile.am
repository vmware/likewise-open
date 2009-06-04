lib_LTLIBRARIES = liblsaserverapi.la

liblsaserverapi_la_SOURCES = \
       artefacts.c     \
       auth.c          \
       auth_provider.c \
       config.c        \
       event.c         \
       globals.c       \
       groups.c        \
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
       ipc_user.c      \
       loginfo.c       \
       metrics.c       \
       provider.c      \
       session.c       \
       state.c         \
       status.c        \
       traceinfo.c     \
       users.c         \
       rpc_server.c    \
       svc_register.c  \
       svc_unregister.c \
       svc_listen.c

liblsaserverapi_la_CPPFLAGS =         \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include

liblsaserverapi_la_LIBADD = \
	@top_builddir@/logging_r/liblsalog_r.la \
	@UUID_LIBS@ \
	@PTHREAD_LIBS@

