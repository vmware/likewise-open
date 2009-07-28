AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_LTLIBRARIES = liblwadvapi_lwunistr.la

liblwadvapi_lwunistr_la_SOURCES = lwunistr.c
liblwadvapi_lwunistr_la_LIBADD = @LIBUNISTR_LIBS@
liblwadvapi_lwunistr_la_CPPFLAGS = @LIBUNISTR_INCLUDES@ $(AM_CPPFLAGS)
liblwadvapi_lwunistr_la_LDFLAGS = @LIBUNISTR_LDFLAGS@
