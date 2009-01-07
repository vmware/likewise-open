lib_LTLIBRARIES = libsrv.sys.la

libsrv_sys_la_SOURCES = \
    checkdir.c     \
    close.c        \
    creatdir.c     \
    createtemp.c   \
    createX.c      \
    deldir.c       \
    driver.c       \
    echo.c         \
    findfirst2.c   \
    flush.c        \
    globals.c      \
    listener.c     \
    lockX.c        \
    logoffX.c      \
    negotiate.c    \
    ntrename.c     \
    readX.c        \
    receive.c      \
    rename.c       \
    seek.c         \
    sessionsetup.c \
    smbv1.c        \
    stubs.c        \
    tconX.c        \
    tdiscon.c      \
    trans2qfi.c    \
    trans2qpi.c    \
    trans2sfi.c    \
    trans2spi.c    \
    transcreate.c  \
    writeX.c

libsrv_sys_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

libsrv_sys_la_LIBADD = \
	@top_builddir@/server/iomgr2/libiomgr2.la \
    @top_builddir@/server/smb/libsmb.la \
    @top_builddir@/utils/libsmbutils.la \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@

libsrv_sys_la_LDFLAGS = -module -avoid-version -export-dynamic

