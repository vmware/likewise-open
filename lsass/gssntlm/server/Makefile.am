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

libntlmserver_la_CPPFLAGS = -I@top_srcdir@/gssntlm/include
libntlmserver_la_LIBADD =\
	@top_builddir@/client/liblsaclient.la \
	@top_builddir@/gssntlm/common/libntlmcommon.la
