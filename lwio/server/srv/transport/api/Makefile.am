noinst_LTLIBRARIES = libsrv.transport.api.la

libsrv_transport_api_la_SOURCES = \
    globals.c    \
    libmain.c

libsrv_transport_api_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    @TRANSPORT_METHOD_DEFINES@ \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include \
    -I@top_srcdir@/server/srv/transport/include

METHOD_LIB = \
    @top_builddir@/server/srv/transport/threadpool/libsrv.transport.threadpool.la

libsrv_transport_api_la_LIBADD = $(METHOD_LIB)

libsrv_transport_api_la_LDFLAGS = -static
