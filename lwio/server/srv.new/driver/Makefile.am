lib_LTLIBRARIES = libsrv.sys.la

libsrv_sys_la_SOURCES = \
    srvshares.c

libsrv_sys_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include

libsrv_sys_la_LIBADD = \
    @top_builddir@/server/iomgr2/libiomgr2.la \
    @top_builddir@/libshareinfo/liblwioshareinfo.la \
    @top_builddir@/utils/liblwioutils.la \
    @KRB5_LIBS@ \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@

libsrv_sys_la_LDFLAGS = -module -avoid-version -export-dynamic

