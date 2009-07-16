noinst_PROGRAMS = ntlm-client

ntlm_client_CFLAGS =    \
    $(AM_CFLAGS)        \
    @LWBASE_INCLUDES@   \
    @LWMSG_INCLUDES@

ntlm_client_SOURCES =   \
    ntlmclient.c

ntlm_client_LDADD =                                     \
    @top_builddir@/ntlmclient/liblsaclient_ntlm.la      \
    @top_builddir@/common/ipc/ntlm/liblsaipc_ntlm.la    \
    @top_builddir@/common/utils/liblsautils.la          \
    @LWMSG_NOTHR_LIBS@                                  \
    @SOCKET_LIBS@
