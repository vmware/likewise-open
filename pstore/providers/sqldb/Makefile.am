provider_LTLIBRARIES = liblwps-sqldb.la

liblwps_sqldb_la_SOURCES = \
	db.c               \
	globals.c          \
	provider-main.c

liblwps_sqldb_la_CFLAGS = @LIBUNISTR_INCLUDES@ @SQLITE_INCLUDES@
liblwps_sqldb_la_LIBADD = @LIBUNISTR_LIBS@ @SQLITE_LIBS@ @top_builddir@/utils/liblwps-utils.la
liblwps_sqldb_la_LDFLAGS = -module -avoid-version -export-dynamic @LIBUNISTR_LDFLAGS@ @SQLITE_LDFLAGS@

noinst_HEADERS = \
	 db_p.h \
	 dbquery.h \
	 externs.h \
	 provider-main_p.h