lib_LTLIBRARIES = libsrv.stat.la

libsrv_stat_la_SOURCES = \
    globals.c    \
    libmain.c    \
    statistics.c

libsrv_stat_la_CFLAGS  = $(AM_CFLAGS) -shared

libsrv_stat_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include

libsrv_stat_la_LIBADD = \
    @LWBASE_LIBS@ \
    @PTHREAD_LIBS@

libsrv_stat_la_LDFLAGS = -module -shared -avoid-version