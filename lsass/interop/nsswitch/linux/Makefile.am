nsslib_LTLIBRARIES = libnss_lsass.la

libnss_lsass_la_SOURCES = \
	nss-group.c \
	nss-user.c \
	nss-netgrp.c \
	globals.c

libnss_lsass_la_CPPFLAGS = \
	-I@srcdir@/../include

libnss_lsass_la_LIBADD = \
	@builddir@/../common/libnsscommon.la \
	@top_builddir@/client/liblsaclient.la

libnss_lsass_la_LDFLAGS = -module -version-number 2 -export-dynamic
