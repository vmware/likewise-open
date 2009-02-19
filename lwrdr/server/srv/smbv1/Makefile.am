noinst_LTLIBRARIES = libsmbsrvsmbv1.la

libsmbsrvsmbv1_la_SOURCES = \
    libmain.c \
    receive.c

libsmbsrvsmbv1_la_CPPFLAGS = \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

libsmbsrvsmbv1_la_LIBADD = \
    @top_builddir@/server/smb/libsmb.la \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@

libsmbsrvsmbv1_la_LDFLAGS = -static

