sbin_PROGRAMS = lwiod

lwiod_SOURCES = \
    globals.c   \
    ipc.c       \
    main.c      \
    lwiocfg.c

lwiod_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

lwiod_LDADD = \
    @top_builddir@/server/iomgr/libiomgr.la \
    @top_builddir@/server/iomgr2/libiomgr2.la \
    @top_builddir@/logging_r/liblwiolog_r.la  \
    @NETLOGON_LIBS@ \
    @LWMSG_LIBS@

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(lwiocachedir)

