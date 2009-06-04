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

libsrv_transport_kqueue_la_LIBADD = \
    @top_builddir@/server/srv.new/utils/libsrvutils.la \
    @top_builddir@/server/iomgr2/libiomgr2.la \
    @top_builddir@/utils/liblwioutils.la \
    @KRB5_LIBS@ \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@

libsrv_transport_kqueue_la_LDFLAGS = -static

