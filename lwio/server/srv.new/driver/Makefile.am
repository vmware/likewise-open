lib_LTLIBRARIES = libsrv.sys.la

libsrv_sys_la_SOURCES = \
    ccb.c          \
    device.c       \
    deviceclose.c  \
    devicecreate.c \
    deviceio.c     \
    driver.c       \
    globals.c      \
    srvshares.c

libsrv_sys_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv.new/include

libsrv_sys_la_LIBADD = \
    @top_builddir@/server/srv.new/protocols/api/libsrv.protocols.api.la \
    @top_builddir@/server/srv.new/transport/api/libsrv.transport.api.la \
    @top_builddir@/server/srv.new/elements/libsrvelements.la \
    @top_builddir@/server/srv.new/shares/api/libshareapi.la \
    @top_builddir@/server/srv.new/utils/libsrvutils.la \
    @top_builddir@/server/iomgr2/libiomgr2.la \
    @top_builddir@/libshareinfo/liblwioshareinfo.la \
    @top_builddir@/utils/liblwioutils.la \
    @KRB5_LIBS@ \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@

libsrv_sys_la_LDFLAGS = -module -avoid-version -export-dynamic

