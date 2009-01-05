lib_LTLIBRARIES = libsrv.sys.la

libsrv_sys_la_SOURCES = \
    globals.c  \
    libmain.c  \
    listener.c

libsrv_sys_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include

libsrv_sys_la_LIBADD = \
    @top_builddir@/server/srv/smbv1/libsmbsrvsmbv1.la \
    @top_builddir@/server/smb/libsmb.la \
    @top_builddir@/utils/libsmbutils.la \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@

libsrv_sys_la_LDFLAGS = -module -avoid-version -export-dynamic

