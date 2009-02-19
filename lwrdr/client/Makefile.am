lib_LTLIBRARIES = liblwrdrclient_nothr.la liblwrdrclient.la

liblwrdrclient_nothr_la_SOURCES = \
    access.c               \
    callnp.c               \
    closehandle.c          \
    config.c               \
    connect.c              \
    connectnp.c            \
    createfile.c           \
    createnp.c             \
    disconnectnp.c         \
    getnpclientcompname.c  \
    getnpclientprocid.c    \
    getnpclientsessid.c    \
    getnpinfo.c            \
    getnpserverprocid.c    \
    globals.c              \
    handle.c               \
    handlestate.c          \
    ipc.c                  \
    libmain.c              \
    logging.c              \
    peeknp.c               \
    readfile.c             \
    transactnp.c           \
    waitnp.c               \
    writefile.c            \
    getsesskey.c 

liblwrdrclient_nothr_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -DSMB_NO_THREADS \
    -I@top_srcdir@/include  \
    @LWMSG_INCLUDES@

liblwrdrclient_nothr_la_CFLAGS = \
    $(AM_CFLAGS) \
    -Wmissing-prototypes

liblwrdrclient_nothr_la_LIBADD = \
    @top_builddir@/ipc/libsmbipc.la

liblwrdrclient_la_SOURCES = \
    context.c              \
    win32.c

liblwrdrclient_la_CPPFLAGS = \
    -I@top_srcdir@/include

liblwrdrclient_la_LIBADD = \
    liblwrdrclient_nothr.la \
    @LWMSG_LIBS@ \
    -lkrb5
