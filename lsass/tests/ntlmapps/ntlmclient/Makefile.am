noinst_PROGRAMS = ntlm-client

ntlm_client_CFLAGS =    \
    $(AM_CFLAGS)        \
    @LWBASE_INCLUDES@   \
    @LWMSG_INCLUDES@

ntlm_client_SOURCES =   \
    globals.c           \
    ntlmclient.c

ntlm_client_LDADD =                             \
    @top_builddir@/client/liblsaclient.la  \
    @top_builddir@/common/ipc/liblsaipc.la \
    @top_builddir@/common/utils/liblsautils.la  \
    @LWMSG_NOTHR_LIBS@                          \
    @SOCKET_LIBS@
