AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

lib_LTLIBRARIES = libntlmserver.la

libntlmserver_la_SOURCES =      \
    acceptsecctxt.c             \
    acquirecreds.c              \
    context.c                   \
    credentials.c               \
    decryptmsg.c                \
    delsecctxt.c                \
    encryptmsg.c                \
    exportsecctxt.c             \
    freecreds.c                 \
    globals.c                   \
    importsecctxt.c             \
    initsecctxt.c               \
    makesign.c                  \
    querycreds.c                \
    queryctxt.c                 \
    ntlmsrvipc.c                \
    verifysign.c

libntlmserver_la_CPPFLAGS =         \
    $(AM_CPPFLAGS)                  \
    $(LWADVAPI_INCLUDES)            \
    -I$(top_srcdir)/server/include  \
    @LWMSG_INCLUDES@                \
    @CRYPTO_INCLUDES@

libntlmserver_la_LDFLAGS =      \
    $(AM_LDFLAGS)               \
    @CRYPTO_LDFLAGS@

libntlmserver_la_LIBADD =                   \
    @top_builddir@/common/liblsacommon.la   \
    @LWMSG_NOTHR_LIBS@                      \
    @CRYPTO_LIBS@                           \
    @SOCKET_LIBS@
