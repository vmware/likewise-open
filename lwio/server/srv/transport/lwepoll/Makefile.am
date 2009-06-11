noinst_LTLIBRARIES = libsrv.transport.epoll.la

libsrv_transport_epoll_la_SOURCES = \
    libmain.c

libsrv_transport_epoll_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include \
    -I@top_srcdir@/server/srv/transport/include

libsrv_transport_epoll_la_LDFLAGS = -static

