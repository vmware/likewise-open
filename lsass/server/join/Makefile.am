lib_LTLIBRARIES = liblsajoin.la

liblsajoin_la_SOURCES = \
       error.c    \
       globals.c  \
       join.c     \
       leave.c    \
       libmain.c  \
        krbtgt.c \
        lwkrb5.c \
        lwkrb5smb.c \
       lwldap.c   \
       common.c   \
       query.c


liblsajoin_la_CPPFLAGS =              \
        -DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	@NETLOGON_INCLUDES@           \
	@PSTORE_INCLUDES@             \
	@NETAPI_INCLUDES@      \
	@KRB5_INCLUDES@ \
	@KRB5PAC_INCLUDES@ \
	@LWIO_INCLUDES@


liblsajoin_la_LIBADD = \
	@top_builddir@/utils/liblsautils.la \
	@NETLOGON_LIBS@ \
	@PSTORE_LIBS@ \
	@NETAPI_LIBS@ \
	@top_builddir@/lsaunistr/liblsaunistr.la \
    @top_builddir@/logging_r/liblsalog_r.la \
	@KRB5_LIBS@ \
	@KRB5PAC_LIBS@ \
	@LWIO_LIBS@ \
	-llber

liblsajoin_la_LDFLAGS = \
        -module -avoid-version -export-dynamic \
	@NETLOGON_LDFLAGS@ \
	@PSTORE_LDFLAGS@ \
	@NETAPI_LDFLAGS@ \
	@OPENLDAP_LDFLAGS@ \
	@KRB5_LDFLAGS@ \
	@KRB5PAC_LDFLAGS@ \
	@LWIO_LDFLAGS@
