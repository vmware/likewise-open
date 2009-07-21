librpc_srcdir=$(top_srcdir)
liballoc_builddir=$(top_builddir)/alloc
libnetapi_builddir=$(top_builddir)/netlib

bin_PROGRAMS = testrpc

testrpc_SOURCES = \
	TestRpc.c \
	Params.c \
	Lsa.c \
	Samr.c \
	Netlogon.c \
	NetApi.c \
        dssetup.c \
	Mpr.c

INCLUDES = -I$(librpc_srcdir)/include

testrpc_CPPFLAGS = $(AM_CPPFLAGS) @LWBASE_INCLUDES@
testrpc_CFLAGS = -g $(INCLUDES) -D_GNU_SOURCE -DLIBRPC_BUILD $(AM_CFLAGS)
testrpc_LDFLAGS = $(PSTORE_LDFLAGS) $(KEYTAB_LDFLAGS) -rpath ${libdir}
testrpc_LDADD = \
	$(liballoc_builddir)/liballoc.la \
	$(libnetapi_builddir)/libnetapi.la \
	@LWBASE_LIBS@
