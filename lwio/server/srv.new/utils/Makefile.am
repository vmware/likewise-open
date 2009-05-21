noinst_LTLIBRARIES = libsrvutils.la

libsrvutils_la_SOURCES = \
    memory.c             \
    prodcons.c

libsrvutils_la_CPPFLAGS =  \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include

libsrvutils_la_LIBADD = \
    @top_builddir@/utils/liblwioutils.la \
    @PTHREAD_LIBS@

