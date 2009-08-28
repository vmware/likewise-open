AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

ADDITIONAL_LIBS =
NOINST_DEP_LIBS = \
	@top_builddir@/logging_r/liblsalog_r.la \
	@top_builddir@/server/store/dsapi/libdsapi.la

if ENABLE_MINIMAL
noinst_LTLIBRARIES = liblsaserverapi.la
ADDITIONAL_CPPFLAGS = "-DMINIMAL_LSASS=1"
if !ENABLE_SMALL_DAEMON
ADDITIONAL_LIBS += $(NOINST_DEP_LIBS)
endif
else
lib_LTLIBRARIES = liblsaserverapi.la
ADDITIONAL_CPPFLAGS =
ADDITIONAL_LIBS += @EVENTLOG_LIBS@
ADDITIONAL_LIBS += $(NOINST_DEP_LIBS)
endif

liblsaserverapi_la_SOURCES = \
       artefacts.c     \
       auth.c          \
       auth_provider.c \
       config.c        \
       credentials.c   \
       event.c         \
       globals.c       \
       groups.c        \
       ipc_dispatch.c  \
       ipc_error.c     \
       ipc_state.c     \
       lsaevent.c      \
       lsatime.c       \
       loginfo.c       \
       metrics.c       \
       provider.c      \
       session.c       \
       state.c         \
       status.c        \
       systemcache.c   \
       traceinfo.c     \
       users.c         \
       rpc_server.c    \
       svc_register.c  \
       svc_unregister.c \
       svc_listen.c

liblsaserverapi_la_CPPFLAGS =     \
	$(AM_CPPFLAGS)                \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	$(ADDITIONAL_CPPFLAGS)

liblsaserverapi_la_LIBADD = \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@ \
    $(ADDITIONAL_LIBS) \
    @RT_LIBS@
