AM_CFLAGS = $(LWBASE_CFLAGS)
AM_CPPFLAGS = $(LWBASE_CPPFLAGS)
AM_LDFLAGS = $(LWBASE_LDFLAGS)

lib_LTLIBRARIES = liblwbase_nothr.la liblwbase.la

liblwbase_nothr_la_SOURCES = \
	ntstatus.c \
        rtlmemory.c \
        rtlstring.c \
        rtlstring_cstring.c \
        rtlstring_wc16string.c \
        rtlstring_unicode.c \
        rtlstring_ansi.c

liblwbase_nothr_la_LIBADD = \
    @LIBUNISTR_LIBS@

liblwbase_la_SOURCES = \
	atomic.c

liblwbase_la_LIBADD = liblwbase_nothr.la

