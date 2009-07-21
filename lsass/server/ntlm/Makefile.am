lib_LTLIBRARIES = libntlmserver_new.la

libntlmserver_new_la_SOURCES =  \
    acceptsecctxt.c             \
    acquirecreds.c              \
    context.c                   \
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

libntlmserver_new_la_CFLAGS =   \
    $(AM_CFLAGS)                \
    $(LWADVAPI_INCLUDES)        \
    @LWMSG_INCLUDES@            \
    @CRYPTO_INCLUDES@

libntlmserver_new_la_LDFLAGS =  \
    $(AM_LDFLAGS)               \
    $(LWADVAPI_LDFLAGS)         \
    @CRYPTO_LDFLAGS@

libntlmserver_new_la_LIBADD =               \
    @top_builddir@/common/liblsacommon.la   \
    $(LWADVAPI_LIBS)                        \
    @LWMSG_NOTHR_LIBS@                      \
    @CRYPTO_LIBS@                           \
    @SOCKET_LIBS@
