lib_LTLIBRARIES = liblsatokencreateinfo.la

liblsatokencreateinfo_la_SOURCES = \
	tokencreateinfo.c

liblsatokencreateinfo_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	@LWBASE_INCLUDES@

liblsatokencreateinfo_la_LIBADD = \
	@top_builddir@/client/liblsaclient.la \
	@LWBASE_NOTHR_LIBS@

liblsatokencreateinfo_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-dynamic
