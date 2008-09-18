noinst_LTLIBRARIES = libnsscommon.la

libnsscommon_la_SOURCES = \
	nss-error.c \
	nss-user.c \
	nss-group.c

libnsscommon_la_CPPFLAGS = \
	-I@top_srcdir@/nsswitch/include

libnsscommon_la_LIBADD = \
	@top_builddir@/client/liblsaclient.la