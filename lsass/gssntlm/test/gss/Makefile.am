noinst_PROGRAMS = gss

gss_CPPFLAGS = \
	-I@top_srcdir@/gssntlm/include \
	-I@top_srcdir@/gssntlm/server

gss_LDFLAGS = -static
gss_LDADD = \
	@top_builddir@/gssntlm/client/libgssntlm.la \
	@SOCKET_LIBS@
