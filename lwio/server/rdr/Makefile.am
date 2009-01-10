lib_LTLIBRARIES = librdr.sys.la

librdr_sys_la_SOURCES = \
    callnp.c              \
    client_reaper.c       \
    client_session.c      \
    client_socket.c       \
    client_tree.c         \
    closehandle.c         \
    createfile.c          \
    getnpinfo.c           \
    getnpserverprocid.c   \
    getsesskey.c          \
    globals.c             \
    handlestate.c         \
    libmain.c             \
    packet.c              \
    peeknp.c              \
    readfile.c            \
    response.c            \
    session.c             \
    smb_logoff.c          \
    smb_negotiate.c       \
    smb_npclose.c         \
    smb_npopen.c          \
    smb_nptransact.c      \
    smb_read.c            \
    smb_session_setup.c   \
    smb_tree_connect.c    \
    smb_tree_disconnect.c \
    smb_write.c           \
    socket.c              \
    transactnp.c          \
    tree.c                \
    waitnp.c              \
    writefile.c           \
    rdrdriver.c           \
    rdrcreate.c           \
    rdrread.c             \
    rdrwrite.c            \
    rdrsetinfo.c          \
    rdrqueryinfo.c        \
    rdrdeviceio.c         \
    rdrfsctrl.c           \
    rdrclose.c

librdr_sys_la_CPPFLAGS = \
    -I@top_srcdir@  	   \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

librdr_sys_la_LIBADD = \
    @top_builddir@/server/smb/liblwiosmb.la \
    @top_builddir@/smbkrb5/liblwiosmbkrb5.la \
    @top_builddir@/utils/liblwioutils.la \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@ \
    @SOCKET_LIBS@

librdr_sys_la_LDFLAGS = \
    -module -avoid-version -export-dynamic


