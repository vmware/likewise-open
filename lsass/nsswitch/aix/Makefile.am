nsslib_LTLIBRARIES = libnss_lsass.la

libnss_lsass_la_SOURCES = \
	nss-main.c

libnss_lsass_la_CPPFLAGS = -I@top_srcdir@/nsswitch/include
libnss_lsass_la_LIBADD = @top_builddir@/client/liblsaclient.la
libnss_lsass_la_LDFLAGS = -module -version-info 1:0:0 -export-dynamic
