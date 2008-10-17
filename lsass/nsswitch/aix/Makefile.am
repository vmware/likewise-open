nsslib_LTLIBRARIES = LSASS.la

LSASS_la_SOURCES = \
	nss-main.c \
	nss-group.c \
	nss-user.c \
	nss-auth.c \
	globals.c

LSASS_la_CPPFLAGS = -I@top_srcdir@/nsswitch/include \
		    -I@top_srcdir@/auth/include
LSASS_la_LIBADD = @top_builddir@/client/liblsaclient.la \
	@top_builddir@/nsswitch/common/libnsscommon.la \
	@top_builddir@/auth/common/liblsaauth.la
LSASS_la_LDFLAGS = -module -version-info 1:0:0 -export-dynamic -Wl,-eLsaNssInitialize -shrext "$(NSS_SHR_EXT)"
