lib_LTLIBRARIES = libwbclient.la

libwbclient_la_SOURCES = \
	lsa_wbc_auth.c \
	lsa_wbc_domain.c \
	lsa_wbc_group.c \
	lsa_wbc_idmap.c \
	lsa_wbc_nss_user.c \
	lsa_wbc_nss_group.c \
	lsa_wbc_sid.c \
	lsa_wbc_user.c \
	lsa_wbc_util.c \
	lsa_wbc_wins.c \
	lsa_wbc_error.c \
	lsa_wbc_memory.c \
	util_str.c

libwbclient_la_CPPFLAGS = -I@srcdir@
libwbclient_la_LIBADD = @top_builddir@/client/liblsaclient.la \
	@NETLOGON_LIBS@
libwbclient_la_LDFLAGS = -module -version-info 2:0:0 -export-dynamic \
	@NETLOGON_LDFLAGS@
