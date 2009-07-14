AM_CFLAGS = $(LWBASE_CFLAGS)
AM_CPPFLAGS = $(LWBASE_CPPFLAGS) -I$(top_srcdir)/include
AM_LDFLAGS = $(LWBASE_LDFLAGS)

TEST_SOURCES = test-security.c

lib_LTLIBRARIES =

if HAVE_MOONUNIT
lib_LTLIBRARIES += lwbase_mu.la
endif

lwbase_mu_la_SOURCES = \
	test-stub.c \
	$(TEST_SOURCES)

lwbase_mu_la_LIBADD = \
	$(top_builddir)/src/liblwbase.la  \
	$(LIB_MOONUNIT)

lwbase_mu_la_LDFLAGS = -module -avoid-version

test-stub.c: $(TEST_SOURCES)
	$(MOONUNIT_STUB) -o $@ CPP="$(CPP)" CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS)" $^
