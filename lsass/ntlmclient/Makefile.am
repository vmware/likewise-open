lib_LTLIBRARIES = libntlmclient.la

libntlmclient_la_SOURCES = \
       acceptsecctxt.c \
       acquirecreds.c \
       clientipc.c \
       decryptmsg.c \
       encryptmsg.c \
       exportsecctxt.c \
       freecreds.c \
       importsecctxt.c \
       initsecctxt.c \
       makesign.c \
       querycreds.c \
       queryctxt.c \
       verifysign.c

libntlmclient_la_CPPFLAGS = -I@top_srcdir@/gssntlm2/include @LWMSG_INCLUDES@

libntlmclient_la_LIBADD = \
    @top_builddir@/gssntlm2/ipc/libntlmipc.la \
    @LWMSG_NOTHR_LIBS@ \
    @SOCKET_LIBS@
