noinst_LTLIBRARIES = libsrvutils.la

libsrvutils_la_SOURCES = \
    hostinfo.c           \
    memory.c             \
    path.c               \
    prodcons.c           \
    string.c

libsrvutils_la_CPPFLAGS =  \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv.new/include

libsrvutils_la_LDFLAGS = -static

