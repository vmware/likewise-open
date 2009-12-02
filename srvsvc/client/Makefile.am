AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libsrvsvc_cs.la libsrvsvc.la

idl_srcdir = $(top_srcdir)/idl
include_srcdir = $(top_srcdir)/include

libsrvsvc_la_SOURCES =        \
    srvsvc_binding.c          \
    netr_connectionenum.c     \
    netr_fileenum.c           \
    netr_filegetinfo.c        \
    netr_fileclose.c          \
    netr_sessionenum.c        \
    netr_shareadd.c           \
    netr_shareenum.c          \
    netr_sharegetinfo.c       \
    netr_sharesetinfo.c       \
    netr_sharedel.c           \
    netr_servergetinfo.c      \
    netr_serversetinfo.c      \
    netr_remotetod.c          \
    net_shareadd.c            \
    net_sharedel.c            \
    net_shareenum.c           \
    srvsvc_memory.c           \
    srvsvc_stubmemory.c       \
    memptr.c                  \
    globals.c

libsrvsvc_cs_la_SOURCES = srvsvc_cstub.c

libsrvsvc_cs_la_CPPFLAGS = \
    $(AM_CPPFLAGS)         \
    -DDCERPC_STUB_BUILD    \
    $(DCERPC_INCLUDES)

libsrvsvc_cs_la_CPFLAGS = \
    -DDCERPC_STUB_BUILD   \
    $(DCERPC_INCLUDES)

libsrvsvc_la_CFLAGS =   \
    -DSRVSVC_BUILD      \
    -DLIBRPC_BUILD      \
    $(INCLUDES)

libsrvsvc_la_CPPFLAGS =   \
    $(AM_CPPFLAGS)        \
    $(DCERPC_INCLUDES)    \
    $(LIBUNISTR_INCLUDES)

libsrvsvc_la_LDFLAGS = \
    $(DCERPC_LDFLAGS)   \
    $(LIBUNISTR_LDFLAGS)

libsrvsvc_la_LIBADD = \
    $(DCERPC_LIBS)    \
    $(LIBUNISTR_LIBS) \
    libsrvsvc_cs.la

CLEANFILES = srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c

BUILT_SOURCES = srvsvc_h.h

srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c: $(idl_srcdir)/srvsvc.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header srvsvc_h.h \
		-I$(idl_srcdir) -I$(srcdir) -I$(include_srcdir) $<
