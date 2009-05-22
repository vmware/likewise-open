provider_LTLIBRARIES = liblsass_auth_provider_ad.la

liblsass_auth_provider_ad_la_SOURCES = \
       sid.c                     \
       adcfg.c                   \
       adldap.c                  \
       ad_marshal_user.c         \
       ad_marshal_group.c        \
       ad_marshal_nss_artefact.c \
       adnetapi.c                \
       state_store.c             \
       cache.c                   \
       cellldap.c                \
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
       media-sense.c             \
       batch.c                   \
       lsaum.c                   \
       lsaum_p.c                 \
       lsaumproc.c               \
       batch_build.c             \
       batch_gather.c            \
       batch_marshal.c           \
       batch_enum.c              \
       unprov.c

liblsass_auth_provider_ad_la_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	@PSTORE_INCLUDES@             \
	@NETAPI_INCLUDES@             \
	@NETLOGON_INCLUDES@           \
	@SQLITE_INCLUDES@

liblsass_auth_provider_ad_la_LIBADD = \
	@top_builddir@/utils/liblsautils.la \
	@top_builddir@/logging_r/liblsalog_r.la \
	@top_builddir@/server/krb5/liblsakrb5.la \
	@top_builddir@/server/ldap/liblsaldap.la \
	@top_builddir@/server/srvutils/liblsasrvutils.la \
	@top_builddir@/server/db/liblsadb.la \
	@top_builddir@/server/sqlite/liblsasqlite.la \
	@top_builddir@/ipc/liblsaipc.la \
	@top_builddir@/provider/ad-provider/ipc/liblsaadipc.la \
	@PSTORE_LIBS@ \
	@NETAPI_LIBS@ \
	@NETLOGON_LIBS@ \
	@PROC_LIBS@ \
	@KVM_LIBS@  \
	@UUID_LIBS@ \
	@CRYPTO_LIBS@

liblsass_auth_provider_ad_la_LDFLAGS = \
	-module -avoid-version -export-dynamic \
	@PSTORE_LDFLAGS@ \
	@NETAPI_LDFLAGS@ \
	@NETLOGON_LDFLAGS@ \
	@CF_FRAMEWORK_LDFLAGS@ \
	@SC_FRAMEWORK_LDFLAGS@ \
	@CRYPTO_LDFLAGS@
