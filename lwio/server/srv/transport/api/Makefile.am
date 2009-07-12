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

if HAVE_SELECT

METHOD_LIB = \
    @top_builddir@/server/srv/transport/lwselect/libsrv.transport.select.la

endif

if HAVE_POLL

METHOD_LIB = \
    @top_builddir@/server/srv/transport/lwselect/libsrv.transport.poll.la

endif

if HAVE_KQUEUE

METHOD_LIB = \
    @top_builddir@/server/srv/transport/lwselect/libsrv.transport.kqueue.la

endif

if HAVE_EPOLL

METHOD_LIB = \
    @top_builddir@/server/srv/transport/lwselect/libsrv.transport.epoll.la

endif

libsrv_transport_api_la_LIBADD = $(METHOD_LIB)

libsrv_transport_api_la_LDFLAGS = -static
