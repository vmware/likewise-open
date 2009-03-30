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
LwMapSecurityCreateAccessTokenFromUidGid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN ULONG Uid,
    IN ULONG Gid
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PUNICODE_STRING Username
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromAnsiStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PANSI_STRING Username
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromWC16StringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PCWSTR Username
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromCStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PCSTR Username
    );

#endif /* __LW_MAP_SECURITY_H__ */
