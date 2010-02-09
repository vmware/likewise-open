noinst_LTLIBRARIES = libsrvsvclog_r.la

libsrvsvclog_r_la_SOURCES = \
    globals.c \
    logging.c

libsrvsvclog_r_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1 \
    -I@top_srcdir@/include

libsrvsvclog_r_la_LIBADD = \
    @PTHREAD_LIBS@

noinst_HEADERS = \
    includes.h   \
    externs.h
