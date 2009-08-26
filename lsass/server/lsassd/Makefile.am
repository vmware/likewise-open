AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

sbin_PROGRAMS = lsassd

lsassd_SOURCES = \
	globals.c \
    listener.c \
    main.c \
    sighandler.c

lsassd_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -DLW_ENABLE_THREADS=1 \
    -I@top_srcdir@/server/include

lsassd_LDADD = \
    @top_builddir@/server/api/liblsaserverapi.la \
    @top_builddir@/server/ntlm/libntlmserver.la \
	@top_builddir@/client/liblsaclient.la \
    @SOCKET_LIBS@ \
    @NETLOGON_LIBS@ \
    @LWIO_LIBS@

if ENABLE_STATIC_PROVIDERS
if ENABLE_AD
lsassd_LDADD += $(top_builddir)/server/auth-providers/ad-provider/liblsass_auth_provider_ad.la
endif
if ENABLE_LOCAL
lsassd_LDADD += $(top_builddir)/server/auth-providers/local-provider/liblsass_auth_provider_local.la
endif
endif

if ENABLE_STATIC_RPCSERVERS
if ENABLE_RPCSERVER
lsassd_LDADD += $(top_builddir)/server/rpc/lsa/liblsa_srv.la
lsassd_LDADD += $(top_builddir)/server/rpc/samr/libsamr_srv.la
lsassd_LDADD += $(top_builddir)/server/rpc/dssetup/libdssetup_srv.la
endif
endif

lsassd_LDFLAGS = \
    @NETLOGON_LDFLAGS@ \
    @LWIO_LDFLAGS@

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(lsasscachedir)
