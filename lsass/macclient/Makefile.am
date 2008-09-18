lib_LTLIBRARIES = liblsaclient-mac.la

liblsaclient_mac_la_SOURCES = \
       api.c      \
       globals.c  \
       libmain.c

liblsaclient_mac_la_LIBADD = \
	@top_builddir@/client/liblsaclient.la

liblsaclient_mac_la_LDFLAGS = \
	-module -avoid-version -export-dynamic
