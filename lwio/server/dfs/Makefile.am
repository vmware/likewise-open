if ENABLE_STATIC_DRIVERS
noinst_LTLIBRARIES = libdfs.sys.la
DRIVER_CFLAGS =
DRIVER_LDFLAGS =
else
lib_LTLIBRARIES = libdfs.sys.la
DRIVER_CFLAGS = -shared
DRIVER_LDFLAGS = -module -shared -avoid-version
endif

libdfs_sys_la_SOURCES = \
    driver.c    \
    close.c     \
    create.c    \
    fsctrl.c     \
    globals.c   \
    irpctx.c    \
    memory.c

libdfs_sys_la_CPPFLAGS = \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -D_FILE_OFFSET_BITS=64 \
    -DLW_ENABLE_THREADS=1

libdfs_sys_la_CFLAGS = $(AM_CFLAGS) $(DRIVER_CFLAGS)

libdfs_sys_la_LIBADD = \
    @top_builddir@/server/iomgr2/libiomgr2.la \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@

libdfs_sys_la_LDFLAGS = $(DRIVER_LDFLAGS)

