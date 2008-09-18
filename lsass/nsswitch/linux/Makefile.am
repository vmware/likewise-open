nsslib_LTLIBRARIES = libnss_lsass.la

libnss_lsass_la_SOURCES = \
	nss-group.c \
	nss-user.c

libnss_lsass_la_CPPFLAGS = -I@top_srcdir@/nsswitch/include
libnss_lsass_la_LIBADD = \
	@top_builddir@/client/liblsaclient.la \
	@top_builddir@/nsswitch/common/libnsscommon.la

libnss_lsass_la_LDFLAGS = -module -version-number 2 -export-dynamic
