if ENABLE_STATIC_PROVIDERS
noinst_LTLIBRARIES = liblsass_auth_provider_ad.la
PROVIDER_CFLAGS =
PROVIDER_LDFLAGS =
else
provider_LTLIBRARIES = liblsass_auth_provider_ad.la
PROVIDER_CFLAGS = -shared
PROVIDER_LDFLAGS = -shared -module -avoid-version
endif

liblsass_auth_provider_ad_la_SOURCES = \
       adcache.c                 \
       adcfg.c                   \
       adldap.c                  \
       ad_marshal_user.c         \
       ad_marshal_group.c        \
       ad_marshal_nss_artefact.c \
       adnetapi.c                \
       state_store.c             \
       cache.c                   \
       cellldap.c                \
       sqlcache.c                \
       defldap.c                 \
       enumstate.c               \
       globals.c                 \
       machinepwd.c              \
       offline.c                 \
       offline-helper.c          \
       online.c                  \
       provider-main.c           \
       providerstate.c           \
       lsadm.c                   \
       lsadm_p.c                 \
       lsadmengine.c             \
       lsadmwrap.c               \
       lsasqlite.c               \
       media-sense.c             \
       batch.c                   \
       lsaum.c                   \
       lsaum_p.c                 \
       lsaumproc.c               \
       batch_build.c             \
       batch_gather.c            \
       batch_marshal.c           \
       batch_enum.c              \
       memcache.c              \
       unprov.c

liblsass_auth_provider_ad_la_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	@PSTORE_INCLUDES@             \
	@NETAPI_INCLUDES@             \
	@NETLOGON_INCLUDES@           \
	@SQLITE_INCLUDES@

liblsass_auth_provider_ad_la_CFLAGS = $(AM_CFLAGS) $(PROVIDER_CFLAGS)

# NOTE: Need to remove LSA client dependency.
liblsass_auth_provider_ad_la_LIBADD = \
	@top_builddir@/common/liblsacommon.la \
	@top_builddir@/client/liblsaclient.la \
	@top_builddir@/server/api/liblsaserverapi.la \
	@top_builddir@/join/liblsajoin.la \
	@PSTORE_LIBS@ \
	@NETAPI_LIBS@ \
	@NETLOGON_LIBS@ \
	@PROC_LIBS@ \
	@KVM_LIBS@  \
	@UUID_LIBS@ \
        @SQLITE_LIBS@ \
	@CRYPTO_LIBS@

liblsass_auth_provider_ad_la_LDFLAGS = \
	$(PROVIDER_LDFLAGS) \
	@PSTORE_LDFLAGS@ \
	@NETAPI_LDFLAGS@ \
	@NETLOGON_LDFLAGS@ \
	@CF_FRAMEWORK_LDFLAGS@ \
	@SC_FRAMEWORK_LDFLAGS@ \
	@CRYPTO_LDFLAGS@  \
        @SQLITE_LDFLAGS@
