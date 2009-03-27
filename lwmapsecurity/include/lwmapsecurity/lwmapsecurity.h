#ifndef __LW_MAP_SECURITY_H__
#define __LW_MAP_SECURITY_H__

#include <lwmapsecurity/lwmapsecurity-types.h>
#include <lw/ntstatus.h>

typedef struct _LW_MAP_SECURITY_CONTEXT *PLW_MAP_SECURITY_CONTEXT;

NTSTATUS
LwMapSecurityCreateContext(
    OUT PLW_MAP_SECURITY_CONTEXT* Context
    );

VOID
LwMapSecurityFreeContext(
    IN OUT PLW_MAP_SECURITY_CONTEXT* Context
    );

NTSTATUS
LwMapSecurityGetIdFromSid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PBOOLEAN IsUser,
    OUT PULONG Id,
    IN PSID Sid
    );

NTSTATUS
LwMapSecurityGetSidFromId(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PSID* Sid,
    IN BOOLEAN IsUser,
    IN ULONG Id
    );

VOID
LwMapSecurityFreeSid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    IN OUT PSID* Sid
    );

NTSTATUS
LwMapSecurityGetAccessTokenCreateInformationFromUidGid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN ULONG Uid,
    IN ULONG Gid
    );

NTSTATUS
LwMapSecurityGetAccessTokenCreateInformationFromId(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN BOOLEAN IsUser,
    IN ULONG Id
    );

NTSTATUS
LwMapSecurityGetAccessTokenCreateInformationFromUnicodeStringName(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN BOOLEAN IsUser,
    IN PUNICODE_STRING Name
    );

NTSTATUS
LwMapSecurityGetAccessTokenCreateInformationFromAnsiStringName(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN BOOLEAN IsUser,
    IN PANSI_STRING Name
    );

NTSTATUS
LwMapSecurityGetAccessTokenCreateInformationFromWC16StringName(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN BOOLEAN IsUser,
    IN PCWSTR Name
    );

NTSTATUS
LwMapSecurityGetAccessTokenCreateInformationFromCStringName(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN BOOLEAN IsUser,
    IN PCSTR Name
    );

VOID
LwMapSecurityFreeAccessTokenCreateInformation(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    IN OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation
    );

#endif /* __LW_MAP_SECURITY_H__ */
