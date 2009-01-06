noinst_LTLIBRARIES = libiomgr2.la

libiomgr2_la_SOURCES = \
    ioinit.c \
    ioroot.c \
    ioconfig.c \
    iodriver.c \
    iomem.c \
    ioapi.c

libiomgr2_la_CPPFLAGS = \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

libiomgr2_la_LIBADD = \
    @top_builddir@/ipc/libsmbipc.la \
    @LWMSG_LIBS@ \
    @UUID_LIBS@  \
    @PTHREAD_LIBS@ \
    @SEM_LIBS@ \
    @SOCKET_LIBS@

libiomgr2_la_LDFLAGS = -static

