lib_LTLIBRARIES = libsmblog_r.la

libsmblog_r_la_SOURCES = \
    globals.c \
    logging.c
	
libsmblog_r_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1 \
    -I@top_srcdir@/include    

libsmblog_r_la_LIBADD = \
    @top_builddir@/utils/libsmbutils.la \
    @PTHREAD_LIBS@

noinst_HEADERS = \
    includes.h   \
    externs.h
	
