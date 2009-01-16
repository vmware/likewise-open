if HAVE_MOONUNIT
bin_PROGRAMS = jenny
jenny_SOURCES = jenny.c

lib_LTLIBRARIES = libsmbtest.la

SOURCE_FILES = \
	jenny_test.c

libsmbtest_la_SOURCES = $(SOURCE_FILES)
libsmbtest_la_CPPFLAGS = -I@top_srcdir@ -I@top_srcdir@/include
libsmbtest_la_LDFLAGS = -static
endif
