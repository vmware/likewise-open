provider_LTLIBRARIES = liblwps-tdb.la

liblwps_tdb_la_SOURCES = \
	param.c \
	provider-main.c \
	util_sid.c \
	util_str.c \
	util_tdbkey.c

liblwps_tdb_la_CFLAGS = @LIBUNISTR_INCLUDES@ @TDB_INCLUDES@
liblwps_tdb_la_LIBADD = @LIBUNISTR_LIBS@ @TDB_LIBS@ @PTHREAD_LIBS@ @top_builddir@/utils/liblwps-utils.la
liblwps_tdb_la_LDFLAGS = -module -avoid-version -export-dynamic @LIBUNISTR_LDFLAGS@ @TDB_LDFLAGS@

noinst_HEADERS = \
	param.h \
	provider-main_p.h \
	util_sid.h \
	util_str.h \
	util_tdbkey.h