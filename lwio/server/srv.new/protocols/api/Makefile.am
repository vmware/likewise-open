noinst_LTLIBRARIES = libsrv.protocols.api.la

libsrv_protocols_api_la_SOURCES = \
    libmain.c

libsrv_protocols_api_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv.new/include \
    -I@top_srcdir@/server/srv.new/protocols/include

libsrv_protocols_api_la_LDFLAGS = -static

