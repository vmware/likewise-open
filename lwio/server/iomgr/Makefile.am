noinst_LTLIBRARIES = libiomgr.la

libiomgr_la_SOURCES = \
    globals.c \
    libmain.c \
    ntvfs.c   \
    vfs_provider.c

libiomgr_la_CPPFLAGS = \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

libiomgr_la_LIBADD = \
    @top_builddir@/ipc/libsmbipc.la \
    @LWMSG_LIBS@ \
    @UUID_LIBS@  \
    @PTHREAD_LIBS@ \
    @SEM_LIBS@ \
    @SOCKET_LIBS@

libiomgr_la_LDFLAGS = -static

