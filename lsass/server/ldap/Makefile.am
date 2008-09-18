lib_LTLIBRARIES = liblsaldap.la

liblsaldap_la_SOURCES = \
       lsaldap.c

liblsaldap_la_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	@OPENLDAP_INCLUDES@

liblsaldap_la_LIBADD = \
	@top_builddir@/logging_r/liblsalog_r.la \
	@top_builddir@/utils/liblsautils.la \
	@OPENLDAP_LIBS@

liblsaldap_la_LDFLAGS = \
	@OPENLDAP_LDFLAGS@

