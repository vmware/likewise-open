netsvclibdir=/usr/lib/netsvc/dynload

nsslib_LTLIBRARIES = LSASS.la
netsvclib_LTLIBRARIES = lsass.la

LSASS_la_SOURCES = \
	lam-main.c \
	lam-group.c \
	lam-user.c \
	lam-auth.c \
	globals.c

LSASS_la_CPPFLAGS = \
	-I@top_srcdir@/nsswitch/include \
	-I@top_srcdir@/auth/include

LSASS_la_CFLAGS = -shared

LSASS_la_LIBADD = \
	@top_builddir@/client/liblsaclient.la \
	@top_builddir@/nsswitch/common/libnsscommon.la \
	@top_builddir@/auth/common/liblsaauth.la

LSASS_la_LDFLAGS = -shared -module -avoid-version -export-dynamic -Wl,-eLsaNssInitialize -shrext "$(NSS_SHR_EXT)"

lsass_la_SOURCES = \
	netsvc-netgrp.c

lsass_la_CPPFLAGS = \
	-I@top_srcdir@/nsswitch/include \
	-I@top_srcdir@/auth/include

lsass_la_CFLAGS = -shared

lsass_la_LIBADD = \
	@top_builddir@/client/liblsaclient.la \
	@top_builddir@/nsswitch/common/libnsscommon.la \
	@top_builddir@/auth/common/liblsaauth.la

lsass_la_LDFLAGS = -shared -module -avoid-version -Wl,-brtl
