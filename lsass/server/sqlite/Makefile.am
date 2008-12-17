lib_LTLIBRARIES = liblsasqlite.la

liblsasqlite_la_SOURCES = \
       sqlite.c

liblsasqlite_la_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	@SQLITE_INCLUDES@

liblsasqlite_la_LIBADD = \
	@top_builddir@/utils/liblsautils.la \
	@top_builddir@/logging_r/liblsalog_r.la \
	@top_builddir@/server/srvutils/liblsasrvutils.la \
	@SQLITE_LIBS@ \
	@NETAPI_LIBS@ \
	@UUID_LIBS@

liblsasqlite_la_LDFLAGS = \
	@SQLITE_LDFLAGS@ \
	@NETAPI_LDFLAGS@
