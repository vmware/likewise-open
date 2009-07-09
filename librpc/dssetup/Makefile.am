librpc_srcdir=$(top_srcdir)
libmsrpc_srcdir=$(top_srcdir)/MsRpc
idl_srcdir=$(top_srcdir)/idl

noinst_LTLIBRARIES = libdssetup_cs.la libdssetup.la

libdssetup_la_SOURCES = \
    dsr_rolegetprimarydomaininfo.c \
    dsr_memory.c \
    dsr_stubmemory.c \
    dsr_binding.c

libdssetup_cs_la_SOURCES = dssetup_cstub.c

LIBRPC_INCDIR = $(librpc_srcdir)/Include
LIBRPC_INCLUDES = -I$(LIBRPC_INCDIR)
INCLUDES = $(DCERPC_INCLUDES) $(UNISTR_INCLUDES) $(LIBRPC_INCLUDES)

FEATURETEST_FLAGS="-D_XOPEN_SOURCE=500 -D__EXTENSIONS__ -D_REENTRANT"

libdssetup_cs_la_CPPFLAGS = $(FEATURETEST_FLAGS)
libdssetup_cs_la_CFLAGS = \
    -DDCERPC_STUB_BUILD \
    $(FEATURETEST_FLAGS)

libdssetup_la_CFLAGS=-g $(INCLUDES) -DLIBRPC_BUILD $(FEATURETEST_FLAGS) $(AM_CFLAGS)
libdssetup_la_LDFLAGS=$(DCERPC_LDFLAGS) $(UNISTR_LDFLAGS)
libdssetup_la_LIBADD= \
    -ldcerpc -lunistr -llwbase \
    libdssetup_cs.la

CLEANFILES = dssetup_h.h dssetup_cstub.c dssetup_sstub.c

BUILT_SOURCES = dssetup_h.h

dssetup_cstub.c dssetup_h.h: $(idl_srcdir)/dssetup.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header dssetup_h.h -I$(idl_srcdir) \
		$(LIBRPC_INCLUDES) $<
