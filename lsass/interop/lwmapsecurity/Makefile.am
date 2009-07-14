lib_LTLIBRARIES = liblwmapsecurity_lsass.la

liblwmapsecurity_lsass_la_SOURCES = \
	lwmapsecurity-lsass.c

liblwmapsecurity_lsass_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	@LWBASE_INCLUDES@

liblwmapsecurity_lsass_la_LIBADD = \
	@top_builddir@/client/liblsaclient.la \
	@LWBASE_NOTHR_LIBS@

liblwmapsecurity_lsass_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-dynamic
