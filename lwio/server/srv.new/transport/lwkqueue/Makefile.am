noinst_LTLIBRARIES = libsrv.transport.kqueue.la

libsrv_transport_kqueue_la_SOURCES = \
    libmain.c

libsrv_transport_kqueue_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv.new/include \
    -I@top_srcdir@/server/srv.new/transport/include

libsrv_transport_kqueue_la_LDFLAGS = -static

