lib_LTLIBRARIES = libsrv.stat.la

libsrv_stat_la_SOURCES = \
    config.c     \
    filelog.c    \
    globals.c    \
    libmain.c    \
    logging.c    \
    statistics.c \
    syslog.c     \
    times.c

libsrv_stat_la_CFLAGS  = $(AM_CFLAGS) -shared

libsrv_stat_la_CPPFLAGS = \
    $(AM_CPPFLAGS)         \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include

libsrv_stat_la_LIBADD = \
    @LWREG_LIBS@ \
    @LWBASE_LIBS@ \
    @PTHREAD_LIBS@

libsrv_stat_la_LDFLAGS = \
	-module        \
	-shared        \
	-avoid-version \
	-export-symbols=srvstat.sym \
	@LWREG_LDFLAGS@ \
	@LWBASE_LDFLAGS@

