provider_LTLIBRARIES = liblsass_auth_provider_ad.la

liblsass_auth_provider_ad_la_SOURCES = \
       adcfg.c                   \
       adldap.c                  \
       ad_marshal_user.c         \
       ad_marshal_group.c        \
       ad_marshal_nss_artefact.c \
       adnetapi.c                \
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
       cachedb.c                 \
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
       batch_enum.c

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
	@top_builddir@/ipc/liblsaipc.la \
	@PSTORE_LIBS@ \
	@NETAPI_LIBS@ \
	@NETLOGON_LIBS@ \
	@PROC_LIBS@ \
	@KVM_LIBS@  \
	@SQLITE_LIBS@ \
	@UUID_LIBS@

liblsass_auth_provider_ad_la_LDFLAGS = \
	-module -avoid-version -export-dynamic \
	@PSTORE_LDFLAGS@ \
	@NETAPI_LDFLAGS@ \
	@NETLOGON_LDFLAGS@ \
	@SQLITE_LDFLAGS@ \
	@CF_FRAMEWORK_LDFLAGS@ \
	@SC_FRAMEWORK_LDFLAGS@
