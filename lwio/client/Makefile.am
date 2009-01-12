lib_LTLIBRARIES = liblsmbclient_nothr.la liblsmbclient.la

liblsmbclient_nothr_la_SOURCES = \
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

liblsmbclient_nothr_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -DSMB_NO_THREADS \
    -I@top_srcdir@/include  \
    @LWMSG_INCLUDES@

liblsmbclient_nothr_la_CFLAGS = \
    $(AM_CFLAGS) \
    -Wmissing-prototypes

liblsmbclient_nothr_la_LIBADD = \
    @top_builddir@/ipc/libsmbipc.la

liblsmbclient_la_SOURCES = \
    context.c              \
    ntfileapi.c            \
    win32.c

liblsmbclient_la_CPPFLAGS = \
    -I@top_srcdir@/include

liblsmbclient_la_LIBADD = \
    liblsmbclient_nothr.la \
    @LWMSG_LIBS@ \
    -lkrb5
