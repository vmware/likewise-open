AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS = testsrvsvc

testsrvsvc_SOURCES = \
    test.c           \
    params.c         \
    test_srvsvc.c

SRVSVC_INCLUDES= \
    -I$(SRVSVC_INCDIR) -I$(top_srcdir) \
    @DCERPC_INCLUDES@ \
    @LIBUNISTR_INCLUDES@

testsrvsvc_CFLAGS=-g $(SRVSVC_INCLUDES) -D_GNU_SOURCE -DSRVSVC_BUILD
testsrvsvc_LDFLAGS= \
    -rpath ${libdir} \
    @DCERPC_LDFLAGS@ \
    @LIBUNISTR_LDFLAGS@

testsrvsvc_LDADD= \
    @DCERPC_LIBS@ \
    @LIBUNISTR_LIBS@ \
    $(top_builddir)/client/libsrvsvc.la
