lib_LTLIBRARIES = liblsalog_r.la

liblsalog_r_la_SOURCES = \
	globals.c \
	logging.c \
	tracing.c
	
liblsalog_r_la_CPPFLAGS = \
        -DLW_ENABLE_THREADS=1

liblsalog_r_la_LIBADD = \
	@top_builddir@/utils/liblsautils.la \
	@PTHREAD_LIBS@

noinst_HEADERS =   \
	includes.h \
	externs.h
	
