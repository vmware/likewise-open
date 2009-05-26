noinst_LTLIBRARIES = liblwshare.la

liblwshare_la_SOURCES = \
    dbcontext.c \
    globals.c   \
    libmain.c   \
    sharedb.c

liblwshare_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv.new/include \
    -I@top_srcdir@/server/srv.new/shares/include

liblwshare_la_LIBADD = \
    @top_builddir@/utils/liblwioutils.la

