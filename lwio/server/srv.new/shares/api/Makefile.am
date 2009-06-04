noinst_LTLIBRARIES = libshareapi.la

libshareapi_la_SOURCES = \
    globals.c  \
    libmain.c  \
    mapping.c  \
    shareapi.c

libshareapi_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv.new/include \
    -I@top_srcdir@/server/srv.new/shares/include

libshareapi_la_LDFLAGS = -static

