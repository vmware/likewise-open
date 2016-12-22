rpccli_srcdir = $(top_srcdir)/client/rpc
idl_srcdir = $(top_srcdir)/idl
lsass_srcdir = $(top_srcdir)

noinst_LTLIBRARIES = libvmpac.la

libvmpac_la_SOURCES = \
	vmpac_cstub.c \
	vmpac.c

DCEIDL_PASS_INCLUDES = \
    $(CPPFLAGS)        \
    $(LWBASE_INCLUDES) \
    -I$(lsass_srcdir)/include

libvmpac_la_CPPFLAGS =        \
    $(AM_CPPFLAGS)              \
    $(DCERPC_INCLUDES)          \
    -I$(librpc_srcdir)/include

libvmpac_la_CFLAGS =    \
    $(AM_CFLAGS)

libvmpac_la_LDFLAGS = \
    $(AM_LDFLAGS)       \
    $(DCERPC_LDFLAGS)

libvmpac_la_LIBADD =  \
    $(DCERPC_LIBS)

BUILT_SOURCES = vmpac_h.h
include_HEADERS = vmpac_h.h

CLEANFILES = vmpac_h.h vmpac_cstub.c vmpac_sstub.c

vmpac_h.h vmpac_cstub.c: $(idl_srcdir)/vmpac.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header vmpac_h.h \
                -I$(idl_srcdir) -I$(srcdir) $(DCEIDL_PASS_INCLUDES) $<
