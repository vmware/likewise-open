noinst_LTLIBRARIES = libsrv.transport.poll.la

libsrv_transport_poll_la_SOURCES = \
    libmain.c

libsrv_transport_poll_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv.new/include \
    -I@top_srcdir@/server/srv.new/transport/include

libsrv_transport_poll_la_LDFLAGS = -static

