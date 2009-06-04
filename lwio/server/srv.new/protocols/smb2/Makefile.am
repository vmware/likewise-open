noinst_LTLIBRARIES = libsrv.proto.smb2.la

libsrv_proto_smb2_la_SOURCES = \
    libmain.c

libsrv_proto_smb2_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv.new/include \
    -I@top_srcdir@/server/srv.new/protocols/include

libsrv_proto_smb2_la_LIBADD = \
    @top_builddir@/server/srv.new/shares/api/libshareapi.la \
    @top_builddir@/server/srv.new/utils/libsrvutils.la \
    @top_builddir@/server/iomgr2/libiomgr2.la \
    @top_builddir@/libshareinfo/liblwioshareinfo.la \
    @top_builddir@/utils/liblwioutils.la \
    @KRB5_LIBS@ \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@

libsrv_proto_smb2_la_LDFLAGS = -static

