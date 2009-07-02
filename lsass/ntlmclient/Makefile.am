lib_LTLIBRARIES = libntlmclient.la

libntlmclient_la_SOURCES =  \
       acceptsecctxt.c      \
       acquirecreds.c       \
       clientipc.c          \
       decryptmsg.c         \
       delsecctxt.c         \
       encryptmsg.c         \
       exportsecctxt.c      \
       freecreds.c          \
       importsecctxt.c      \
       initsecctxt.c        \
       makesign.c           \
       querycreds.c         \
       queryctxt.c          \
       verifysign.c

libntlmclient_la_CPPFLAGS = \
    @LWMSG_INCLUDES@

libntlmclient_la_LIBADD =                   \
    @top_builddir@/ntlmipc/libntlmipc.la    \
    @top_builddir@/utils/liblsautils.la     \
    @LWMSG_NOTHR_LIBS@                      \
    @SOCKET_LIBS@
