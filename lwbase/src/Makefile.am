AM_CFLAGS = $(LWBASE_CFLAGS)
AM_CPPFLAGS = $(LWBASE_CPPFLAGS)
AM_LDFLAGS = $(LWBASE_LDFLAGS)

lib_LTLIBRARIES = liblwbase_nothr.la liblwbase.la libsecdesc.la 

liblwbase_nothr_la_SOURCES = \
	errno.c \
	ntstatus.c \
        rtlmemory.c \
        rtlstring.c \
        rtlstring_cstring.c \
        rtlstring_wc16string.c \
        rtlstring_unicode.c \
        rtlstring_ansi.c \
        rbtree.c \
        security/security.c

liblwbase_nothr_la_LIBADD = \
        @LIBUNISTR_LIBS@

liblwbase_la_SOURCES = \
	atomic.c

liblwbase_la_LIBADD = liblwbase_nothr.la

libsecdesc_la_SOURCES = \
        rtlsid.c \
        sid.c \
        sidstr.c \
        rtlpolhnd.c

libsecdesc_la_LIBADD = \
        @LIBUUID_LIBS@ \
        liblwbase_nothr.la
