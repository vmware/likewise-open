AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include -I$(top_srcdir)/gssntlm/include

noinst_PROGRAMS = gss

gss_CPPFLAGS = $(AM_CPPFLAGS) -I@top_srcdir@/gssntlm/server

gss_LDFLAGS = -static
gss_LDADD = \
	@top_builddir@/gssntlm/client/libgssntlm.la \
	@SOCKET_LIBS@
