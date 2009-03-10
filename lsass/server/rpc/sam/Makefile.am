provider_LTLIBRARIES = libsamr_srv.la

libsamr_srv_la_SOURCES = \
    SamrChangePasswordUser2.c \
    SamrClose.c               \
    SamrConnect2.c            \
    SamrConnect4.c            \
    SamrCreateDomAlias.c      \
    SamrCreateUser2.c         \
    SamrCreateUser.c          \
    SamrDeleteAliasMember.c   \
    SamrDeleteDomAlias.c      \
    SamrDeleteUser.c          \
    SamrEnumDomainAliases.c   \
    SamrEnumDomains.c         \
    SamrEnumDomainUsers.c     \
    SamrGetAliasMembership.c  \
    SamrGetMembersInAlias.c   \
    SamrGetUserGroups.c       \
    SamrGetUserPwInfo.c       \
    SamrLookupDomain.c        \
    SamrLookupNames.c         \
    SamrLookupRids.c          \
    SamrOpenAlias.c           \
    SamrOpenDomain.c          \
    SamrOpenUser.c            \
    SamrQueryAliasInfo.c      \
    SamrQueryDomainInfo.c     \
    SamrQueryUserInfo.c       \
    SamrSetAliasInfo.c        \
    SamrSetUserInfo.c

libsamr_srv_la_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	-I@top_srcdir@/server/rpc/include

libsamr_srv_la_LDFLAGS = \
	-module -avoid-version -export-dynamic

