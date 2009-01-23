lib_LTLIBRARIES = libsrv.sys.la

libsrv_sys_la_SOURCES = \
    checkdir.c        \
    close.c           \
    creatdir.c        \
    createtemp.c      \
    createX.c         \
    deldir.c          \
    device.c          \
    driver.c          \
    findfirst2.c      \
    flush.c           \
    globals.c         \
    listener.c        \
    lockX.c           \
    ntrename.c        \
    prodcons.c        \
    readX.c           \
    receive.c         \
    reader.c          \
    rename.c          \
    seek.c            \
    sharedb.c         \
    smbv1.c           \
    srvconnection.c   \
    srvcontext.c      \
    srvecho.c         \
    srvfile.c         \
    srvgss.c          \
    srvidallocator.c  \
    srvlogoff.c       \
    srvnegotiate.c    \
    srvsessionsetup.c \
    srvshareutil.c    \
    srvsession.c      \
    srvsocket.c       \
    srvtcon.c         \
    srvtdiscon.c      \
    srvtree.c         \
    stubs.c           \
    trans2qfi.c       \
    trans2qpi.c       \
    trans2sfi.c       \
    trans2spi.c       \
    transcreate.c     \
    worker.c          \
    writeX.c

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

