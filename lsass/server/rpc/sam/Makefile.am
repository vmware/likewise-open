srv_srcdir = $(top_srcdir)/server
rpc_srcdir = $(srv_srcdir)/rpc
idl_srcdir = $(rpc_srcdir)/idl

lib_LTLIBRARIES = liblsa_ss.la liblsa_srv.la

liblsa_srv_la_SOURCES = \
    globals.c \
    lsa.c \
    lsa_srv.c \
    LsaOpenPolicy2.c \
    LsaClose.c \
    LsaLookupNames.c \
    LsaLookupSids.c \
    LsaLookupNames2.c \
    LsaQueryInfoPolicy.c \
    LsaQueryInfoPolicy2.c
    globals.c
SamDBAddAliasMember.c
SamDBChangePasswordUser2.c
SamDBClose.c
SamDBConnect2.c
SamDBConnect4.c
SamDBCreateDomAlias.c
SamDBCreateUser2.c
SamDBCreateUser.c
SamDBDeleteAliasMember.c
SamDBDeleteDomAlias.c
SamDBDeleteUser.c
SamDBEnumDomainAliases.c
SamDBEnumDomains.c
SamDBEnumDomainUsers.c
SamDBGetAliasMembership.c
SamDBGetMembersInAlias.c
SamDBGetUserGroups.c
SamDBGetUserPwInfo.c
SamDBLookupDomain.c
SamDBLookupNames.c
SamDBLookupRids.c
SamDBOpenAlias.c
SamDBOpenDomain.c
SamDBOpenUser.c
SamDBQueryAliasInfo.c
SamDBQueryDomainInfo.c
SamDBQueryUserInfo.c
SamDBSetAliasInfo.c
SamDBSetUserInfo.c

LSASS_SRV_INCLUDES=-I$(srv_srcdir)/include
LSASS_INCLUDES=-I$(top_srcdir)/include

liblsa_srv_la_CFLAGS = \
    -DLIBRPC_BUILD \
    $(LSASS_SRV_INCLUDES) \
    $(LSASS_INCLUDES) \
    @DCERPC_INCLUDES@ \
    @LIBUNISTR_INCLUDES@ \
    @LIBSECDESC_INCLUDES@

liblsa_srv_la_LIBADD = \
    @DCERPC_LIBS@ \
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
