sbin_PROGRAMS = lsmbd

lsmbd_SOURCES = \
    globals.c   \
    ipc.c       \
    main.c      \
    smbcfg.c

lsmbd_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

lsmbd_LDADD = \
    @top_builddir@/server/iomgr/libiomgr.la \
    @top_builddir@/logging_r/libsmblog_r.la  \
    @NETLOGON_LIBS@ \
    @LWMSG_LIBS@

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(lsmbcachedir)

