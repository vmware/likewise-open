lib_LTLIBRARIES = liblsadb.la

liblsadb_la_SOURCES = \
       db.c

liblsadb_la_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	@SQLITE_INCLUDES@

liblsadb_la_LIBADD = \
	@top_builddir@/utils/liblsautils.la \
	@top_builddir@/logging_r/liblsalog_r.la \
	@top_builddir@/server/srvutils/liblsasrvutils.la \
	@top_builddir@/server/sqlite/liblsasqlite.la
