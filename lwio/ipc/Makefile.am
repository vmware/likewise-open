SUBDIRS = .

if HAVE_MOONUNIT
SUBDIRS += test
endif

lib_LTLIBRARIES = liblwioipc.la

liblwioipc_la_SOURCES = \
    globals.c \
    libmain.c \
    ntipc.c

liblwioipc_la_CPPFLAGS = \
    -I@top_srcdir@/include  \
    @LWMSG_INCLUDES@

liblwioipc_la_LIBADD = \
    @top_builddir@/utils/liblwioutils.la \
    @LWMSG_NOTHR_LIBS@