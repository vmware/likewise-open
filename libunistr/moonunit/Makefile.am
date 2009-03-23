AM_CFLAGS = -fPIC -Werror -Wall
AM_CPPFLAGS = -I$(top_srcdir)/include

TEST_SOURCES = test-printf.c \
	       test-convert.c

lib_LTLIBRARIES =

if HAVE_MOONUNIT
lib_LTLIBRARIES += libunistr_mu.la
endif

libunistr_mu_la_SOURCES = \
	test-stub.c \
	$(TEST_SOURCES)

libunistr_mu_la_LIBADD = \
	$(top_builddir)/lib/libunistr.la  \
	$(LIB_MOONUNIT)

libunistr_mu_la_LDFLAGS = -module -avoid-version

test-stub.c: $(TEST_SOURCES)
	$(MOONUNIT_STUB) -o $@ CPP="$(CPP)" CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS)" $^
