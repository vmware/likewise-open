noinst_LTLIBRARIES = libsmbutils.la

libsmbutils_la_SOURCES = \
    bitvector.c   \
    consolelog.c  \
    dlinkedlist.c \
    filelog.c     \
    fileutils.c   \
    globals.c     \
    handlemgr.c   \
    hashtable.c   \
    libmain.c     \
    logger.c      \
    memory.c      \
    smbconfig.c   \
    smbloginfo.c  \
    smbxnet.c     \
    stack.c       \
    strerror.c    \
    strings.c     \
    sysfuncs.c    \
    syslog.c      \
    unistr.c

libsmbutils_la_CPPFLAGS = \
    -I@top_srcdir@/include

libsmbutils_la_LIBADD = \
    @LIBUNISTR_LIBS@ \
    @DL_LIBS@

