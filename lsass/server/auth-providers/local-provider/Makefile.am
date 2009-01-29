provider_LTLIBRARIES = liblsass_auth_provider_local.la

liblsass_auth_provider_local_la_SOURCES = \
       crypt.c     \
       db.c        \
       enumstate.c \
       localcfg.c  \
       globals.c   \
       provider-main.c

liblsass_auth_provider_local_la_CPPFLAGS = \
        -DLW_ENABLE_THREADS=1              \
	-I@top_srcdir@/server/include      \
	@SQLITE_INCLUDES@

#netapi is needed for NTLMv1EncryptChallenge
liblsass_auth_provider_local_la_LIBADD = \
	@top_builddir@/utils/liblsautils.la \
	@top_builddir@/logging_r/liblsalog_r.la \
	@top_builddir@/server/srvutils/liblsasrvutils.la \
	@top_builddir@/lsaunistr/liblsaunistr.la \
	@SQLITE_LIBS@ \
	@NETAPI_LIBS@

liblsass_auth_provider_local_la_LDFLAGS = \
	-module -avoid-version -export-dynamic \
	@SQLITE_LDFLAGS@ \
	@NETAPI_LDFLAGS@
