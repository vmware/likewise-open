librpc_srcdir=$(top_srcdir)
libmsrpc_srcdir=$(top_srcdir)/MsRpc
idl_srcdir=$(top_srcdir)/Idl

lib_LTLIBRARIES = libsamr_cs.la libsamr.la

libsamr_la_SOURCES = \
    globals.c \
    SamrBinding.c \
    SamrMemory.c \
    SamrStubMemory.c \
    SamrClose.c \
    SamrConnect2.c \
    SamrEnumDomains.c \
    SamrEnumDomainUsers.c \
    SamrLookupDomain.c \
    SamrLookupNames.c \
    SamrOpenDomain.c \
    SamrOpenUser.c \
    SamrQueryUserInfo.c \
    SamrSetUserInfo.c \
    SamrCreateUser.c \
    SamrDeleteUser.c \
    SamrCreateDomAlias.c \
    SamrDeleteDomAlias.c \
    SamrGetAliasMembership.c \
    SamrLookupRids.c \
    SamrQueryAliasInfo.c \
    SamrSetAliasInfo.c \
    SamrAddAliasMember.c \
    SamrDeleteAliasMember.c \
    SamrGetMembersInAlias.c \
    SamrQueryDomainInfo.c \
    SamrEnumDomainAliases.c \
    SamrOpenAlias.c \
    SamrConnect3.c \
    SamrConnect4.c \
    SamrConnect5.c \
    SamrCreateUser2.c \
    SamrGetUserPwInfo.c \
    SamrChangePasswordUser2.c \
    SamrGetUserGroups.c \
    SamrQueryDisplayInfo.c

libsamr_cs_la_SOURCES = samr_cstub.c

LIBRPC_INCDIR = $(librpc_srcdir)/Include
LIBRPC_INCLUDES = -I$(LIBRPC_INCDIR)
INCLUDES = $(DCERPC_INCLUDES) $(UNISTR_INCLUDES) $(LIBRPC_INCLUDES)

FEATURETEST_FLAGS="-D_XOPEN_SOURCE=500 -D__EXTENSIONS__ -D_REENTRANT"

libsamr_cs_la_CPPFLAGS = $(FEATURETEST_FLAGS)
libsamr_cs_la_CFLAGS = \
    -DDCERPC_STUB_BUILD \
    $(FEATURETEST_FLAGS)

libsamr_la_CFLAGS = -g $(INCLUDES) -DLIBRPC_BUILD $(FEATURETEST_FLAGS) $(AM_CFLAGS)
libsamr_la_LDFLAGS = $(DCERPC_LDFLAGS) $(UNISTR_LDFLAGS)
libsamr_la_LIBADD = \
    -ldcerpc -lunistr -lsecdesc \
    $(top_builddir)/MsRpc/libmsrpc.la \
    libsamr_cs.la

CLEANFILES = samr_h.h samr_cstub.c samr_sstub.c

BUILT_SOURCES = samr_h.h

samr_h.h samr_cstub.c: $(idl_srcdir)/samr.idl samr.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header samr_h.h -I$(idl_srcdir) \
		$(LIBRPC_INCLUDES) $<
