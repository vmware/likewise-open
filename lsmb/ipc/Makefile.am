SUBDIRS = .

if HAVE_MOONUNIT
SUBDIRS += test
endif

lib_LTLIBRARIES = libsmbipc.la

libsmbipc_la_SOURCES = \
    globals.c \
    libmain.c

libsmbipc_la_CPPFLAGS = \
    -I@top_srcdir@/include  \
    @LWMSG_INCLUDES@

libsmbipc_la_LIBADD = \
    @top_builddir@/utils/libsmbutils.la \
    @LWMSG_NOTHR_LIBS@