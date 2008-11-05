nsslib_LTLIBRARIES = nss_lsass.la

nss_lsass_la_SOURCES = \
	nss-group.c \
	nss-main.c \
	nss-user.c \
        nss-netgrp.c

nss_lsass_la_CPPFLAGS = -I@top_srcdir@/nsswitch/include
nss_lsass_la_LIBADD = \
	@top_builddir@/client/liblsaclient.la \
	@top_builddir@/nsswitch/common/libnsscommon.la

nss_lsass_la_LDFLAGS = -module -version-number 1 -export-dynamic

noinst_HEADERS = \
	nss-group.h \
	nss-user.h \
        nss-netgrp.h
