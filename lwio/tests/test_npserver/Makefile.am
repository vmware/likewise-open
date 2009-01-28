noinst_PROGRAMS = test_npserver

test_npserver_SOURCES = main.c

test_npserver_CPPFLAGS = \
	-DUNICODE

test_npserver_LDFLAGS = -static

test_npserver_LDADD = @top_builddir@/client/liblwioclient.la
