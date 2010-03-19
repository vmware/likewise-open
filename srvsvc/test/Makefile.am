AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS = testsrvsvc

testsrvsvc_SOURCES = \
    net_servergetinfo.c \
    net_serversetinfo.c \
    net_shareadd.c     \
    net_sharedel.c     \
    net_shareenum.c    \
    net_sharegetinfo.c \
    net_sharesetinfo.c \
    params.c           \
    test.c             \
    test_srvsvc.c

SRVSVC_INCLUDES= \
    -I$(SRVSVC_INCDIR)    \
    -I$(top_srcdir)       \
    $(DCERPC_INCLUDES)    \
    $(LIBUNISTR_INCLUDES)

testsrvsvc_CFLAGS =       \
    $(AM_CPPFLAGS)        \
    $(SRVSVC_INCLUDES)    \
    -D_GNU_SOURCE         \
    -DLIBRPC_BUILD

testsrvsvc_LDFLAGS=       \
    -rpath ${libdir}      \
    $(DCERPC_LDFLAGS)     \
    $(LIBUNISTR_LDFLAGS)

testsrvsvc_LDADD =        \
    $(DCERPC_LIBS)        \
    $(LIBUNISTR_LIBS)     \
    $(LIBRPC_LIBS)        \
    $(top_builddir)/client/libsrvsvc.la
