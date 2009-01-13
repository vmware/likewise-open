AM_CFLAGS = $(LWBASE_CFLAGS)
AM_CPPFLAGS = $(LWBASE_CPPFLAGS)
AM_LDFLAGS = $(LWBASE_LDFLAGS)

lib_LTLIBRARIES = liblwbase_nothr.la liblwbase.la

liblwbase_nothr_la_SOURCES = \
	ntstatus.c \
        memory.c

liblwbase_la_SOURCES = \
	atomic.c

liblwbase_la_LIBADD = liblwbase_nothr.la

