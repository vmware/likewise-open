SUBDIRS = open enterprise

provider_LTLIBRARIES = liblsass_auth_provider_new.la

liblsass_auth_provider_new_la_SOURCES = \
       globals.c                      \
       provider-main.c

liblsass_auth_provider_new_la_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	@PSTORE_INCLUDES@             \
	@NETAPI_INCLUDES@             \
	@NETLOGON_INCLUDES@           \
	@SQLITE_INCLUDES@

liblsass_auth_provider_new_la_LIBADD = \
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

liblsass_auth_provider_new_la_LDFLAGS = \
	-module -avoid-version -export-dynamic \
	@PSTORE_LDFLAGS@ \
	@NETAPI_LDFLAGS@ \
	@NETLOGON_LDFLAGS@ \
	@CF_FRAMEWORK_LDFLAGS@ \
	@SC_FRAMEWORK_LDFLAGS@ \
	@CRYPTO_LDFLAGS@
