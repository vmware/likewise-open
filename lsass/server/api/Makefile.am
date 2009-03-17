noinst_LTLIBRARIES = liblsaserverapi.la

liblsaserverapi_la_SOURCES = \
       artefacts.c     \
       auth.c          \
       auth_provider.c \
       config.c        \
       event.c         \
       globals.c       \
       groups.c        \
       loginfo.c       \
       metrics.c       \
       provider.c      \
       session.c       \
       state.c         \
       status.c        \
       traceinfo.c     \
       users.c         \
       rpc_server.c

liblsaserverapi_la_CPPFLAGS =         \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
        -I@top_srcdir@/server/rpc/svcctl

liblsaserverapi_la_LIBADD = \
	@top_builddir@/logging_r/liblsalog_r.la \
	@top_builddir@/server/srvutils/liblsasrvutils.la \
        @top_builddir@/server/rpc/svcctl/libsvcctl.la \
	@UUID_LIBS@ \
	@PTHREAD_LIBS@

liblsaserverapi_la_LDFLAGS = -static
