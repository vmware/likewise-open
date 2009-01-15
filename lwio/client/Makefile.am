lib_LTLIBRARIES = liblwioclient_nothr.la liblwioclient.la

liblwioclient_nothr_la_SOURCES = \
    access.c               \
    config.c               \
    connect.c              \
    globals.c              \
    libmain.c              \
    logging.c              \
    ntfileapictx.c         \
    smbfileapictx.c

liblwioclient_nothr_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -DLWIO_NO_THREADS \
    -I@top_srcdir@/include  \
    @LWMSG_INCLUDES@

liblwioclient_nothr_la_CFLAGS = \
    $(AM_CFLAGS) \
    -Wmissing-prototypes

liblwioclient_nothr_la_LIBADD = \
    @top_builddir@/ipc/liblwioipc.la

liblwioclient_la_SOURCES = \
    thread.c               \
    ntfileapi.c            \
    smbfileapi.c           \
    win32.c

liblwioclient_la_CPPFLAGS = \
    -I@top_srcdir@/include

liblwioclient_la_LIBADD = \
    liblwioclient_nothr.la \
    @LWMSG_LIBS@ \
    -lkrb5
