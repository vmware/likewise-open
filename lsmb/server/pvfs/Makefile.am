
lib_LTLIBRARIES = libpvfs.sys.la

libpvfs_sys_la_SOURCES = \
    globals.c \
    pvfs.c

libpvfs_sys_la_CPPFLAGS = \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

libpvfs_sys_la_LIBADD = \
    @top_builddir@/utils/libsmbutils.la \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@
    
libpvfs_sys_la_LDFLAGS = \
    -module -avoid-version -export-dynamic

