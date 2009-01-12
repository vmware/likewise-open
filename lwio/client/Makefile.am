lib_LTLIBRARIES = liblwioclient_nothr.la liblwioclient.la

liblwioclient_nothr_la_SOURCES = \
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
    libmain.c              \
    logging.c              \
    ntfileapiipc.c         \
    peeknp.c               \
    readfile.c             \
    transactnp.c           \
    waitnp.c               \
    writefile.c            \
    getsesskey.c 

liblwioclient_nothr_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -DSMB_NO_THREADS \
    -I@top_srcdir@/include  \
    @LWMSG_INCLUDES@

liblwioclient_nothr_la_CFLAGS = \
    $(AM_CFLAGS) \
    -Wmissing-prototypes

liblwioclient_nothr_la_LIBADD = \
    @top_builddir@/ipc/libsmbipc.la

liblwioclient_la_SOURCES = \
    context.c              \
    ntfileapi.c            \
    win32.c

liblwioclient_la_CPPFLAGS = \
    -I@top_srcdir@/include

liblwioclient_la_LIBADD = \
    liblwioclient_nothr.la \
    @LWMSG_LIBS@ \
    -lkrb5
