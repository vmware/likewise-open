noinst_LTLIBRARIES = libsrvoem.la

libsrvoem_la_SOURCES = \
    libmain.c       \
    srvoem.c

libsrvoem_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include

libsrvoem_la_LDFLAGS = -static