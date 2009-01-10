lib_LTLIBRARIES = lwioipc_mu.la

TEST_SOURCES = test-marshal.c
BUILT_SOURCES = test-stub.c

lwioipc_mu_la_SOURCES = $(TEST_SOURCES) test-stub.c
lwioipc_mu_la_CFLAGS = $(AM_CFLAGS) -shared
lwioipc_mu_la_CPPFLAGS = $(AM_CPPFLAGS) -I@top_srcdir@/include @LWMSG_INCLUDES@
lwioipc_mu_la_LDFLAGS = $(AM_LDFLAGS) -export-dynamic -avoid-version -shared -module
lwioipc_mu_la_LIBADD = @top_builddir@/ipc/liblwioipc.la @LIB_moonunit@

test-stub.c: $(TEST_SOURCES)
	$(MOONUNIT_STUB) -o $@ CPP="$(CPP)" CPPFLAGS="$(CPPFLAGS) $(lwioipc_mu_la_CPPFLAGS)" $^
