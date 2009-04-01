lib_LTLIBRARIES = libunistr.la

bin_SCRIPTS = parse_WC16STR.sh

AM_CFLAGS=-fPIC -Werror -Wall
AM_CPPFLAGS=-I$(top_srcdir)/include

libunistr_la_SOURCES = wc16str.c wc16printf.c
libunistr_la_LIBADD = @LIB_iconv@

include_HEADERS = $(top_srcdir)/include/tchar.h $(top_srcdir)/include/wc16str.h $(top_srcdir)/include/wc16printf.h $(top_srcdir)/include/wchar16.h
