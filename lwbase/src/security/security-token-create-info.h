#ifndef __LW_SECURITY_TOKEN_CREATE_INFO_H__
#define __LW_SECURITY_TOKEN_CREATE_INFO_H__

#include <lw/security-types.h>
#include <lw/ntstatus.h>
#include <lw/rtlstring.h>

// TODO-These need to be in a different component that can talk to lsass.

typedef struct _ACCESS_TOKEN_CREATE_INFORMATION {
    TOKEN_USER User;
    TOKEN_GROUPS Groups;
#if 0
    TOKEN_PRIVILEGES Privileges;
#endif
    TOKEN_OWNER Owner;
    TOKEN_PRIMARY_GROUP PrimaryGroup;
    TOKEN_DEFAULT_DACL DefaultDacl;
    TOKEN_UNIX Unix;
} ACCESS_TOKEN_CREATE_INFORMATION, *PACCESS_TOKEN_CREATE_INFORMATION;

NTSTATUS
RtlGetAccessTokenCreateInformationFromUid(
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN ULONG Uid
    );

NTSTATUS
RtlGetAccessTokenCreateInformationFromUnicodeStringUsername(
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN PUNICODE_STRING UserName
    );

NTSTATUS
RtlGetAccessTokenCreateInformationFromAnsiStringUsername(
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN PANSI_STRING Username
    );

NTSTATUS
RtlGetAccessTokenCreateInformationFromWC16StringUsername(
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN PCWSTR Username
    );

NTSTATUS
RtlGetAccessTokenCreateInformationFromCStringUsername(
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN PCSTR Username
    );

VOID
RtlFreeAccessTokenCreateInformation(
    IN OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation
    );

#endif /* __LW_SECURITY_TOKEN_CREATE_INFO_H__ */
