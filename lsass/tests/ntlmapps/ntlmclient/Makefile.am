noinst_PROGRAMS = ntlm-client

ntlm_client_CFLAGS =    \
    $(AM_CFLAGS)        \
    @LWBASE_INCLUDES@   \
    @LWMSG_INCLUDES@

ntlm_client_SOURCES =                           \
    ntlmclient.c

ntlm_client_LDADD =                             \
    @top_builddir@/ntlmclient/libntlmclient.la  \
    @top_builddir@/ntlmipc/libntlmipc.la     \
    @LWMSG_NOTHR_LIBS@                          \
    @SOCKET_LIBS@
