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
       session.c       \
       state.c         \
       status.c        \
       users.c

liblsaserverapi_la_CPPFLAGS =         \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include

liblsaserverapi_la_LIBADD = \
	@top_builddir@/logging_r/liblsalog_r.la \
	@EVENTLOG_LIBS@ \
	@UUID_LIBS@ \
	@PTHREAD_LIBS@

liblsaserverapi_la_LDFLAGS = -static
