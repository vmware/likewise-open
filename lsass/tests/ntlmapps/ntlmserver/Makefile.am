noinst_PROGRAMS = ntlm-server

ntlm_server_CFLAGS =        \
    $(AM_CFLAGS)            \
    @LWBASE_INCLUDES@       \
    @LIBUNISTR_INCLUDES@

ntlm_server_SOURCES =                           \
    ntlm-server.c

ntlm_server_LDADD =                                     \
    @top_builddir@/ntlmclient/liblsaclient_ntlm.la      \
    @top_builddir@/common/ipc/ntlm/liblsaipc_ntlm.la    \
    @top_builddir@/common/utils/liblsautils.la          \
    @LIBUNISTR_LIBS@                                    \
    @LWMSG_NOTHR_LIBS@                                  \
    @SOCKET_LIBS@

