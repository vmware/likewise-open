lib_LTLIBRARIES = libsrv.sys.la

libsrv_sys_la_SOURCES = \
    ntvfs.c		  \
    libmain.c             

libsrv_sys_la_CPPFLAGS = \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

libsrv_sys_la_LIBADD = \
    @top_builddir@/utils/libsmbutils.la \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@

libsrv_sys_la_LDFLAGS = \
    -module -avoid-version -export-dynamic

