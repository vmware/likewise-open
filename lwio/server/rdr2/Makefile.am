ADDITIONAL_LIBS =
NOINST_DEP_LIBS = \
    @top_builddir@/server/iomgr/libiomgr.la

if ENABLE_STATIC_DRIVERS
noinst_LTLIBRARIES = librdr2.sys.la
DRIVER_CFLAGS =
DRIVER_LDFLAGS =
if !ENABLE_SMALL_DAEMON
ADDITIONAL_LIBS += $(NOINST_DEP_LIBS)
endif
else
lib_LTLIBRARIES = librdr2.sys.la
DRIVER_CFLAGS = -shared
DRIVER_LDFLAGS = -module -shared -avoid-version -export-symbols $(srcdir)/rdr.sym
ADDITIONAL_LIBS += $(NOINST_DEP_LIBS)
endif

librdr2_sys_la_SOURCES =   \
    client_session.c      \
    client_socket.c       \
    client_tree.c         \
    getsesskey.c          \
    globals.c             \
    queryinfofile.c       \
    setinfofile.c         \
    renamefile.c          \
    querysecurity.c       \
    queryfsinfo.c         \
    findfirst2.c          \
    findnext2.c           \
    response.c            \
    session.c             \
    smb_logoff.c          \
    smb_negotiate.c       \
    smb_tree_disconnect.c \
    smb_write.c           \
    socket.c              \
    tree.c                \
    writefile.c           \
    rdrdriver.c           \
    rdrcreate.c           \
    rdrread.c             \
    rdrwrite.c            \
    rdrsetinfo.c          \
    rdrqueryinfo.c        \
    rdrquerydir.c         \
    rdrdeviceio.c         \
    rdrfsctl.c            \
    rdrsecurity.c         \
    rdrqueryfs.c          \
    rdrclose.c

librdr2_sys_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@ \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

librdr2_sys_la_CFLAGS = $(AM_CFLAGS) $(DRIVER_CFLAGS) -Wmissing-prototypes

librdr2_sys_la_LIBADD = \
    @top_builddir@/server/smbcommon/liblwiosmbcommon.la \
    $(ADDITIONAL_LIBS) \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@ \
    @SOCKET_LIBS@

librdr2_sys_la_LDFLAGS = $(DRIVER_LDFLAGS)
