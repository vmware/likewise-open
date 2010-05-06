noinst_LTLIBRARIES = libsrv.transport.threadpool.la

libsrv_transport_threadpool_la_SOURCES = \
    libmain.c       \
    listener.c      \
    srvsocket.c

libsrv_transport_threadpool_la_CPPFLAGS = \
    $(AM_CPPFLAGS)         \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include \
    -I@top_srcdir@/server/srv/transport/include

libsrv_transport_threadpool_la_LDFLAGS =
