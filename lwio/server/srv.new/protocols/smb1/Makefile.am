noinst_LTLIBRARIES = libsrv.protocols.smb1.la

libsrv_protocols_smb1_la_SOURCES = \
    libmain.c

libsrv_protocols_smb1_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv.new/include \
    -I@top_srcdir@/server/srv.new/protocolscols/include

libsrv_protocols_smb1_la_LDFLAGS = -static

