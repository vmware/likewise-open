lib_LTLIBRARIES = libiomgr2.la

libiomgr2_la_SOURCES = \
    ioinit.c \
    ioconfig.c \
    ioroot.c \
    iodriver.c \
    iodevice.c \
    ioirp.c \
    iofile.c \
    iomem.c \
    iostring.c \
    ioapi.c

libiomgr2_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1 \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

libiomgr2_la_LIBADD = \
    @top_builddir@/ipc/libsmbipc.la \
    @LWMSG_LIBS@ \
    @UUID_LIBS@  \
    @PTHREAD_LIBS@ \
    @SEM_LIBS@ \
    @SOCKET_LIBS@

libiomgr2_la_LDFLAGS = \
    -no-undefined
