nsslib_LTLIBRARIES = libnss_lsass.la

libnss_lsass_la_SOURCES = \
	nss-group.c \
	nss-main.c \
	nss-user.c \
	nss-pruser.c

libnss_lsass_la_CPPFLAGS = \
<<<<<<< .working
	$(AM_CPPFLAGS) \
	-I$(srcdir)/../include
=======
	-I$(srcdir)/../include \
	-I$(srcdir)/../../auth/include
>>>>>>> .merge-right.r48132

libnss_lsass_la_LIBADD = \
	./../common/libnsscommon.la \
	$(top_builddir)/client/liblsaclient.la \
	-lsec

libnss_lsass_la_LDFLAGS = -module -version-number 1 -export-dynamic -shrext "$(NSS_SHR_EXT)"

noinst_HEADERS = \
	nss-group.h \
	nss-user.h
