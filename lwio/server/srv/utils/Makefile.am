noinst_LTLIBRARIES = libsrvutils.la

libsrvutils_la_SOURCES = \
    globals.c            \
    libmain.c            \
    logfilter.c          \
    memory.c             \
    path.c               \
    socket.c             \
    string.c

libsrvutils_la_CPPFLAGS =  \
    $(AM_CPPFLAGS)         \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include

libsrvutils_la_LDFLAGS =

