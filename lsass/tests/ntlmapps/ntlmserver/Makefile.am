noinst_PROGRAMS = ntlm-server

ntlm_server_CFLAGS =        \
    $(AM_CFLAGS)            \
    @LWBASE_INCLUDES@       \
    @LIBUNISTR_INCLUDES@

ntlm_server_SOURCES =                           \
    ntlm-server.c

ntlm_server_LDADD =                             \
    @top_builddir@/ntlmclient/libntlmclient.la  \
    @top_builddir@/ntlmipc/libntlmipc.la        \
    @LIBUNISTR_LIBS@                            \
    @LWMSG_NOTHR_LIBS@                          \
    @SOCKET_LIBS@

