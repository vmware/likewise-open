AM_CPPFLAGS = -I$(top_srcdir)/include @LWMSG_CPPFLAGS@
AM_CFLAGS = -Wall -Werror

lib_LTLIBRARIES = lwmsg_mu.la

TEST_SOURCES = test-convert.c test-marshal.c test-assoc.c test-session.c test-client-server.c
BUILT_SOURCES = stub.c

lwmsg_mu_la_SOURCES = $(TEST_SOURCES) stub.c
lwmsg_mu_la_CFLAGS = -shared -Wall -Werror
lwmsg_mu_la_LIBADD = @LIB_moonunit@ $(top_builddir)/src/liblwmsg.la
lwmsg_mu_la_LDFLAGS = -shared -module -export-dynamic -avoid-version

stub.c: $(TEST_SOURCES)
	$(MOONUNIT_STUB) -o $@ CPP="$(CPP)" CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS) $(lwmsg_mu_la_CPPFLAGS)" $^
