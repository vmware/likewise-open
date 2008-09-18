LIBCT_PREFIX=$(top_srcdir)/ctlib
LIBCT_INCDIR=$(LIBCT_PREFIX)/include

lib_LTLIBRARIES = libnpc.la

libnpc_la_CFLAGS=-fPIC
libnpc_la_CPPFLAGS=-I$(LIBCT_INCDIR) -I$(top_srcdir)/include -I$(srcdir)/../include $(KRB5_CPPFLAGS) -D_GNU_SOURCE

libnpc_la_SOURCES = npclib.c
libnpc_la_LIBADD = $(top_builddir)/ctlib/lib/libct.la $(KRB5_LDFLAGS) $(LTLDFLAGS)
