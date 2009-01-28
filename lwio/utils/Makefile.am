noinst_LTLIBRARIES = liblwioutils.la

liblwioutils_la_SOURCES = \
    bitvector.c   \
    consolelog.c  \
    ecp.c         \
    dlinkedlist.c \
    filelog.c     \
    fileutils.c   \
    globals.c     \
    handlemgr.c   \
    hashtable.c   \
    libmain.c     \
    logger.c      \
    lwlist.c      \
    memory.c      \
    queue.c       \
    lwioconfig.c  \
    lwiologinfo.c \
    lwioxnet.c    \
    path.c        \
    rbtree.c      \
    stack.c       \
    strerror.c    \
    strings.c     \
    sysfuncs.c    \
    syslog.c      \
    unistr.c

liblwioutils_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/include

liblwioutils_la_LIBADD = \
    @LIBUNISTR_LIBS@ \
    @LWBASE_LIBS@ \
    @DL_LIBS@

liblwioutils_la_LDFLAGS = \
   $(AM_LDFLAGS) \
   @LWBASE_LDFLAGS@
