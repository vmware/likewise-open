AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

noinst_LTLIBRARIES = liblsaclient_vmdir.la

liblsaclient_vmdir_la_SOURCES = \
    signal.c

liblsaclient_vmdir_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	@LWMSG_INCLUDES@

liblsaclient_vmdir_la_LIBADD = \
	@top_builddir@/common/liblsacommon.la \
	@LWMSG_NOTHR_LIBS@ \
	@SOCKET_LIBS@
