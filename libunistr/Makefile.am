lib_LTLIBRARIES = libunistr.la

AM_CFLAGS=-fPIC -Werror -Wall
AM_CPPFLAGS=-Iinclude

libunistr_la_SOURCES = wc16str.c wc16printf.c
libunistr_la_LIBADD = @LIB_iconv@

include_HEADERS = include/tchar.h include/wc16str.h include/wc16printf.h include/wchar16.h include/winnttypes.h

check_PROGRAMS = wc16test wc16printftest
wc16test_SOURCES = test/wc16test.c
wc16test_LDADD = ./libunistr.la
wc16printftest_SOURCES = test/wc16printftest.c
wc16printftest_LDADD = ./libunistr.la

TESTS = wc16test
