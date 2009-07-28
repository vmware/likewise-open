AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include -I$(top_srcdir)/gssntlm/include

noinst_LTLIBRARIES = libntlmserver.la

libntlmserver_la_SOURCES = \
       auth_ad.c      \
       authapi.c      \
       auth_db.c      \
       auth_inbound.c \
       processmsg.c   \
       server.c

if ENABLE_TESTGSSPROVIDER
libntlmserver_la_SOURCES += auth_test.c
endif

libntlmserver_la_LIBADD =\
	@top_builddir@/client/liblsaclient.la \
	@top_builddir@/gssntlm/common/libntlmcommon.la
