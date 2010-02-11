AM_CFLAGS = $(LWBASE_CFLAGS) -Wno-deprecated-declarations
AM_CPPFLAGS = $(LWBASE_CPPFLAGS) -I$(top_srcdir)/include
AM_LDFLAGS = $(LWBASE_LDFLAGS)

lib_LTLIBRARIES = liblwbase_nothr.la liblwbase.la

liblwbase_nothr_la_SOURCES = \
	error-conv.c \
	errno.c \
	ntstatus.c \
	dscache.c \
	rpcstatus.c \
        rtlmemory.c \
        rtlstring.c \
        rtlstring_cstring.c \
        rtlstring_wc16string.c \
        rtlstring_unicode.c \
        rtlstring_ansi.c \
        rbtree.c \
        security-sid.c \
        security-acl.c \
        security-sd.c

liblwbase_nothr_la_LIBADD = \
        @DS_FRAMEWORK_LDFLAGS@ \
        @LIBUNISTR_LIBS@ \
        @LIBUUID_LIBS@

liblwbase_la_SOURCES = \
	atomic.c \
        security-token.c

liblwbase_la_LIBADD = \
        @LIB_PTHREAD@ \
        liblwbase_nothr.la

if USE_EPOLL
liblwbase_la_SOURCES += threadpool-epoll.c
endif

if USE_KQUEUE
liblwbase_la_SOURCES += threadpool-kqueue.c
endif

if USE_SELECT
liblwbase_la_SOURCES += threadpool-select.c
endif
