sbin_PROGRAMS = lsassd

lsassd_SOURCES = \
	globals.c    \
        listener.c   \
        main.c       \
        sighandler.c

lsassd_CPPFLAGS =                     \
        -DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include

lsassd_LDADD = \
	@top_builddir@/server/ipc/liblsaserveripc.la \
	@top_builddir@/server/api/liblsaserverapi.la \
	@top_builddir@/gssntlm/server/libntlmserver.la \
	@top_builddir@/client/liblsaclient.la \
	@top_builddir@/logging_r/liblsalog_r.la \
	@SOCKET_LIBS@ \
	@NETLOGON_LIBS@

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(lsasscachedir)
