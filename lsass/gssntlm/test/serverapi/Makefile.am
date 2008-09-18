noinst_PROGRAMS = serverapi

serverapi_SOURCES = main.c
serverapi_CPPFLAGS = \
	-I@top_srcdir@/gssntlm/include \
	-I@top_srcdir@/gssntlm/test/include \
	-I@top_srcdir@/gssntlm/server
	
serverapi_LDFLAGS = -static
serverapi_LDADD = @top_builddir@/gssntlm/server/libntlmserver.la \
	@top_builddir@/gssntlm/client/libgssntlm.la \
	@top_builddir@/lsaunistr/liblsaunistr.la \
	@top_builddir@/client/liblsaclient.la
