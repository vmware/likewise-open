AM_CFLAGS = $(LWBASE_CFLAGS)
AM_CPPFLAGS = $(LWBASE_CPPFLAGS) -I$(top_srcdir)/include
AM_LDFLAGS = $(LWBASE_LDFLAGS)

lib_LTLIBRARIES = liblwbase_nothr.la liblwbase.la

liblwbase_nothr_la_SOURCES = \
	errno.c \
	ntstatus.c \
	rpcstatus.c \
        rtlmemory.c \
        rtlstring.c \
        rtlstring_cstring.c \
        rtlstring_wc16string.c \
        rtlstring_unicode.c \
        rtlstring_ansi.c \
        rbtree.c \
        rtlqueue.c \
        rtlpolhnd.c \
        security-sid.c \
        security-acl.c \
        security-sd.c

liblwbase_nothr_la_LIBADD = \
        @LIBUNISTR_LIBS@ \
        @LIBUUID_LIBS@

liblwbase_la_SOURCES = \
	atomic.c \
        security-token.c

liblwbase_la_LIBADD = \
        @LIB_PTHREAD@ \
        liblwbase_nothr.la
