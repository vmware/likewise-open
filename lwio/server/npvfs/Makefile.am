lib_LTLIBRARIES = libnpvfs.sys.la

libnpvfs_sys_la_SOURCES = \
    close.c  \
    create.c \
    deviceio.c \
    driver.c \
    fsctl.c \
    globals.c \
    queryinfo.c \
    read.c \
    setinfo.c \
    write.c

libnpvfs_sys_la_CPPFLAGS = \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

libnpvfs_sys_la_LIBADD = \
    @top_builddir@/utils/liblwioutils.la \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@
    
libnpvfs_sys_la_LDFLAGS = \
    -module -avoid-version -export-dynamic

