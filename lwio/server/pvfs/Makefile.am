
lib_LTLIBRARIES = libpvfs.sys.la

libpvfs_sys_la_SOURCES = \
    driver.c \
    create.c \
    read.c   \
    write.c  \
    deviceio.c \
    fsctrl.c \
    queryinfo.c \
    setinfo.c \
    close.c

libpvfs_sys_la_CPPFLAGS = \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

libpvfs_sys_la_LIBADD = \
    @top_builddir@/utils/liblwioutils.la \
    @top_builddir@/server/iomgr2/libiomgr2.la \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@

libpvfs_sys_la_LDFLAGS = \
    -module -avoid-version -export-dynamic

