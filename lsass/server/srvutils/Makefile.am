lib_LTLIBRARIES = liblsasrvutils.la

liblsasrvutils_la_SOURCES = \
       lsaevent.c         \
       systemcache.c         \
       lsatime.c

liblsasrvutils_la_CPPFLAGS =      \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include

liblsasrvutils_la_LIBADD = \
	@top_builddir@/logging_r/liblsalog_r.la \
	@EVENTLOG_LIBS@ \
	@RT_LIBS@
