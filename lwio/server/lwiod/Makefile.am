sbin_PROGRAMS = lwiod

lwiod_SOURCES = \
    globals.c   \
    main.c      \
    lwiocfg.c   \
    ipc.c       \
    proc.c

lwiod_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

lwiod_LDADD = \
    @top_builddir@/server/iomgr2/libiomgr2.la \
    @NETLOGON_LIBS@ \
    @LWMSG_LIBS@

if ENABLE_STATIC_DRIVERS

if ENABLE_RDR
lwiod_LDADD += $(top_builddir)/server/rdr/librdr.sys.la
endif

if ENABLE_SRV
lwiod_LDADD += $(top_builddir)/server/srv/libsrv.sys.la
endif

if ENABLE_PVFS
lwiod_LDADD += $(top_builddir)/server/pvfs/libpvfs.sys.la
endif

if ENABLE_NPFS
lwiod_LDADD += $(top_builddir)/server/npfs/libnpfs.sys.la
endif

endif

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(lwiocachedir)

