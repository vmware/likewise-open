AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

noinst_PROGRAMS = test_LsaParseConfigFile

test_LsaParseConfigFile_SOURCES = main.c
test_LsaParseConfigFile_LDFLAGS = -static
test_LsaParseConfigFile_LDADD = @top_builddir@/client/liblsaclient.la