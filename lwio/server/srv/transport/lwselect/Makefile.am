noinst_LTLIBRARIES = libsrv.transport.select.la

libsrv_transport_select_la_SOURCES = \
    globals.c       \
    libmain.c       \
    listener.c      \
    reader.c        \
    srvconnection.c \
    srvsocket.c

libsrv_transport_select_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include \
    -I@top_srcdir@/server/srv/transport/include

libsrv_transport_select_la_LDFLAGS = -static

