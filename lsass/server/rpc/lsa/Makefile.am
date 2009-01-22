rpc_srcdir = $(top_srcdir)/server/rpc
idl_srcdir = $(rpc_srcdir)/idl

lib_LTLIBRARIES = liblsa_ss.la liblsa_srv.la

liblsa_srv_la_SOURCES = \
    globals.c \
    lsa.c \
    lsa_srv.c

liblsa_srv_la_CFLAGS = \
    @DCERPC_INCLUDES@ \
    @LIBUNISTR_INCLUDES@ \
    @LIBSECDESC_INCLUDES@

liblsa_srv_la_LIBADD = \
    @DCEPC_LIBS@ \
    @LIBUNISTR_LIBS@ \
    @LIBSECDESC_LIBS@ \
    liblsa_ss.la

liblsa_ss_la_SOURCES = lsa_sstub.c

liblsa_ss_la_CFLAGS = \
    @DCERPC_INCLUDES@ \
    @LIBUNISTR_INCLUDES@ \
    @LIBSECDESC_INCLUDES@

CLEAN_FILES = lsa_cstub.c lsa_sstub.c lsa_h.h

BUILT_SOURCES = lsa_cstub.c lsa_sstub.c lsa_h.h

lsa_sstub.c lsa_cstub.c lsa_h.h: $(idl_srcdir)/lsa.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header lsa_h.h -I$(idl_srcdir) \
		$(LIRPC_INCLUDES) $<
