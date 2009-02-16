lib_LTLIBRARIES = libsrv.sys.la

libsrv_sys_la_SOURCES = \
    checkdir.c        \
    close.c           \
    creatdir.c        \
    createtemp.c      \
    createX.c         \
    deldir.c          \
    driver.c          \
    findclose2.c      \
    findfirst2.c      \
    findnext2.c       \
    flush.c           \
    globals.c         \
    hostinfo.c        \
    listener.c        \
    lockX.c           \
    prodcons.c        \
    reader.c          \
    rename.c          \
    seek.c            \
    sharedb.c         \
    srvccb.c          \
    srvconnection.c   \
    srvcontext.c      \
    srvdevice.c       \
    srvdeviceclose.c  \
    srvdevicecreate.c \
    srvdeviceio.c     \
    srvecho.c         \
    srvfile.c         \
    srvfinder.c       \
    srvgss.c          \
    srvlogoff.c       \
    srvnegotiate.c    \
    srvpath.c         \
    srvpipeinfo.c     \
    srvread.c         \
    srvreadx.c        \
    srvsessionsetup.c \
    srvsharelst.c     \
    srvsession.c      \
    srvsocket.c       \
    srvtcon.c         \
    srvtdiscon.c      \
    srvtree.c         \
    srvwrite.c        \
    srvwritex.c       \
    trans.c           \
    trans2.c          \
    trans2qfi.c       \
    trans2qfsi.c      \
    trans2qpi.c       \
    trans2sfi.c       \
    trans2spi.c       \
    transcreate.c     \
    worker.c

libsrv_sys_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

libsrv_sys_la_LIBADD = \
    @top_builddir@/server/iomgr2/libiomgr2.la \
    @top_builddir@/server/smbwire/liblwiosmbwire.la \
    @top_builddir@/utils/liblwioutils.la \
    @SQLITE_LIBS@ \
    @KRB5_LIBS@ \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@

libsrv_sys_la_LDFLAGS = -module -avoid-version -export-dynamic

