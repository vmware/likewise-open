noinst_LTLIBRARIES = libsrv.protocols.smb2.la

libsrv_protocols_smb2_la_SOURCES = \
    close.c          \
    create.c         \
    echo.c           \
    error.c          \
    flush.c          \
    getinfo.c        \
    ioctl.c          \
    libmain.c        \
    lock.c           \
    logoff.c         \
    negotiate.c      \
    read.c           \
    session_setup.c  \
    treeconnect.c    \
    treedisconnect.c \
    wire.c           \
    write.c

libsrv_protocols_smb2_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include \
    -I@top_srcdir@/server/srv/protocols/include

libsrv_protocols_smb2_la_LDFLAGS = -static

