lib_LTLIBRARIES = libntlmserver2.la

libntlmserver2_la_SOURCES = \
    acceptsecctxt.c         \
    acquirecreds.c          \
    context.c               \
    decryptmsg.c            \
    delsecctxt.c            \
    encryptmsg.c            \
    exportsecctxt.c         \
    freecreds.c             \
    globals.c               \
    importsecctxt.c         \
    initsecctxt.c           \
    makesign.c              \
    querycreds.c            \
    queryctxt.c             \
    ntlmsrvipc.c            \
    verifysign.c

libntlmserver2_la_CPPFLAGS =    \
    @LWMSG_INCLUDES@            \
    @CRYPTO_INCLUDES@

liblsass_auth_provider_ad_la_LDFLAGS =  \
    @CRYPTO_LDFLAGS@

libntlmserver2_la_LIBADD =                  \
    @top_builddir@/ntlmipc/libntlmipc.la    \
    @top_builddir@/utils/liblsautils.la     \
    @LWMSG_NOTHR_LIBS@                      \
    @CRYPTO_LIBS@                           \
    @SOCKET_LIBS@
