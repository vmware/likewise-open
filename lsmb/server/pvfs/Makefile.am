
lib_LTLIBRARIES = libpvfs.sys.la

libpvfs_sys_la_SOURCES = \
    driver.c

libpvfs_sys_la_CPPFLAGS = \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

libpvfs_sys_la_LIBADD = \
    @top_builddir@/utils/libsmbutils.la \
    @top_builddir@/server/iomgr2/libiomgr2.la \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@

libpvfs_sys_la_LDFLAGS = \
    -module -avoid-version -export-dynamic

