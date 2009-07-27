AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

lib_LTLIBRARIES = liblsaunistr.la

liblsaunistr_la_SOURCES = lsaunistr.c
liblsaunistr_la_LIBADD = @LIBUNISTR_LIBS@ \
	@top_builddir@/common/liblsacommon.la
liblsaunistr_la_CPPFLAGS = $(AM_CPPFLAGS) @LIBUNISTR_INCLUDES@
liblsaunistr_la_LDFLAGS = @LIBUNISTR_LDFLAGS@
