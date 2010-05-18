noinst_LTLIBRARIES = libstatisticsapi.la

libstatisticsapi_la_SOURCES = \
    globals.c  \
    libmain.c

libstatisticsapi_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include

libstatisticsapi_la_LDFLAGS = -static