librpc_srcdir=$(top_srcdir)
idl_srcdir=$(top_srcdir)/Idl

noinst_LTLIBRARIES = libkrb5pac.la

libkrb5pac_la_SOURCES = \
	krb5pac_cstub.c \
	Krb5Pac.c

LIBRPC_INCDIR=$(librpc_srcdir)/Include
LIBRPC_INCLUDES=-I$(LIBRPC_INCDIR)
INCLUDES=$(DCERPC_INCLUDES) $(UNISTR_INCLUDES) $(LIBRPC_INCLUDES)

FEATURETEST_FLAGS="-D_XOPEN_SOURCE=500 -D__EXTENSIONS__ -D_REENTRANT"

libkrb5pac_la_CFLAGS=-g $(INCLUDES) -DLIBRPC_BUILD $(FEATURETEST_FLAGS) $(AM_CFLAGS)
libkrb5pac_la_LDFLAGS=$(DCERPC_LDFLAGS) $(UNISTR_LDFLAGS)
libkrb5pac_la_LIBADD=-ldcerpc -lunistr

BUILT_SOURCES = krb5pac_h.h
include_HEADERS = krb5pac_h.h

CLEANFILES = krb5pac_h.h krb5pac_cstub.c krb5pac_sstub.c

krb5pac_h.h krb5pac_cstub.c: $(idl_srcdir)/krb5pac.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header krb5pac_h.h -I$(idl_srcdir) \
		$(LIBRPC_INCLUDES) $<
