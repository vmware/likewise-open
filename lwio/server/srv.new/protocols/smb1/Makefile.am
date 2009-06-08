noinst_LTLIBRARIES = libsrv.protocols.smb1.la

libsrv_protocols_smb1_la_SOURCES = \
    checkdir.c       \
    close.c          \
    creatdir.c       \
    createx.c        \
    deldir.c         \
    delete.c         \
    echo.c           \
    findclose2.c     \
    findfirst2.c     \
    findnext2.c      \
    flush.c          \
    libmain.c        \
    lockx.c          \
    logoff.c         \
    negotiate.c      \
    nttransact.c     \
    openx.c          \
    read.c           \
    readx.c          \
    rename.c         \
    seek.c           \
    sessionsetup.c   \
    srvfinder.c      \
    srvpipeinfo.c    \
    trans2.c         \
    trans2qfi.c      \
    trans2qfsi.c     \
    trans2qpi.c      \
    trans2sfi.c      \
    trans2spi.c      \
    trans.c          \
    transcreate.c    \
    treeconnect.c    \
    treedisconnect.c \
    write.c          \
    writex.c

libsrv_protocols_smb1_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv.new/include \
    -I@top_srcdir@/server/srv.new/protocolscols/include

libsrv_protocols_smb1_la_LDFLAGS = -static

