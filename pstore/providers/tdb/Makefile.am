if ENABLE_STATIC_PROVIDERS
noinst_LTLIBRARIES = liblwps-tdb.la
PROVIDER_CFLAGS =
PROVIDER_LDFLAGS =
PROVIDER_LIBADD =
else
provider_LTLIBRARIES = liblwps-tdb.la
PROVIDER_CFLAGS = -shared
PROVIDER_LDFLAGS = -shared -module -avoid-version
PROVIDER_LIBADD = @top_builddir@/utils/liblwps-utils.la
endif

liblwps_tdb_la_SOURCES = \
	param.c \
	provider-main.c \
	util_sid.c \
	util_str.c \
	util_tdbkey.c

liblwps_tdb_la_CFLAGS = @LIBUNISTR_INCLUDES@ @TDB_INCLUDES@ $(PROVIDER_CFLAGS)

liblwps_tdb_la_LIBADD = @LIBUNISTR_LIBS@ @TDB_LIBS@ @PTHREAD_LIBS@ $(PROVIDER_LIBADD)

liblwps_tdb_la_LDFLAGS = $(PROVIDER_LDFLAGS) @LIBUNISTR_LDFLAGS@ @TDB_LDFLAGS@

noinst_HEADERS = \
	param.h \
	provider-main_p.h \
	util_sid.h \
	util_str.h \
	util_tdbkey.h
