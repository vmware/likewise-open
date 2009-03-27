AM_CFLAGS = $(LWMAPSECURITY_CFLAGS)
AM_CPPFLAGS = $(LWMAPSECURITY_CPPFLAGS) -I$(top_srcdir)/include
AM_LDFLAGS = $(LWMAPSECURITY_LDFLAGS)

lib_LTLIBRARIES = liblwmapsecurity.la

liblwmapsecurity_la_SOURCES = \
	lwmapsecurity.c

liblwmapsecurity_la_LIBADD = \
        @LWBASE_LIBS@
