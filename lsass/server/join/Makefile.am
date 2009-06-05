lib_LTLIBRARIES = liblsajoin.la

liblsajoin_la_SOURCES = \
       error.c    \
       globals.c  \
       join.c     \
       leave.c    \
       libmain.c  \
       common.c   \
       query.c


liblsajoin_la_CPPFLAGS =              \
        -DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	@NETLOGON_INCLUDES@           \
	@PSTORE_INCLUDES@             \
	@NETAPI_INCLUDES@

liblsajoin_la_LIBADD = \
	@top_builddir@/utils/liblsautils.la \
	@top_builddir@/server/krb5/liblsakrb5.la \
	@top_builddir@/server/ldap/liblsaldap.la \
	@top_builddir@/server/api/liblsaserverapi.la \
	@NETLOGON_LIBS@ \
	@PSTORE_LIBS@ \
	@NETAPI_LIBS@

liblsajoin_la_LDFLAGS = \
        -module -avoid-version -export-dynamic \
	@NETLOGON_LDFLAGS@ \
	@PSTORE_LDFLAGS@ \
	@NETAPI_LDFLAGS@

