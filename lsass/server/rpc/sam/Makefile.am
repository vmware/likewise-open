provider_LTLIBRARIES = libsamr.la

libsamr_la_SOURCES = \
    SamDBChangePasswordUser2.c \
    SamDBClose.c               \
    SamDBConnect2.c            \
    SamDBConnect4.c            \
    SamDBCreateDomAlias.c      \
    SamDBCreateUser2.c         \
    SamDBCreateUser.c          \
    SamDBDeleteAliasMember.c   \
    SamDBDeleteDomAlias.c      \
    SamDBDeleteUser.c          \
    SamDBEnumDomainAliases.c   \
    SamDBEnumDomains.c         \
    SamDBEnumDomainUsers.c     \
    SamDBGetAliasMembership.c  \
    SamDBGetMembersInAlias.c   \
    SamDBGetUserGroups.c       \
    SamDBGetUserPwInfo.c       \
    SamDBLookupDomain.c        \
    SamDBLookupNames.c         \
    SamDBLookupRids.c          \
    SamDBOpenAlias.c           \
    SamDBOpenDomain.c          \
    SamDBOpenUser.c            \
    SamDBQueryAliasInfo.c      \
    SamDBQueryDomainInfo.c     \
    SamDBQueryUserInfo.c       \
    SamDBSetAliasInfo.c        \
    SamDBSetUserInfo.c         \
    SamrAddAliasMember.c

libsamr_la_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	-I@top_srcdir@/server/rpc/include

libsamr_la_LDFLAGS = \
	-module -avoid-version -export-dynamic

