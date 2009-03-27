/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwmapsecurity.c
 *
 * Abstract:
 *
 *        Likewise Map Security - Access Token Create Information
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include <lwmapsecurity/lwmapsecurity-plugin.h>
#include <lwmapsecurity/lwmapsecurity.h>
#include <lw/security-api.h>
#include <lw/rtlgoto.h>
//#include <lw/safeint.h>
//#include <assert.h>

//
// Unmapped Unix User and Group SIDs
//
// S-1-22-1-UID for users
// S-1-22-2-GID for groups
//

#define SECURITY_UNMAPPED_UNIX_AUTHORITY    { 0, 0, 0, 0, 0, 22 }
#define SECURITY_UNMAPPED_UNIX_UID_RID      1
#define SECURITY_UNMAPPED_UNIX_GID_RID      2
#define SECURITY_UNMAPPED_UNIX_RID_COUNT    2

//
// Plugin Functions
//

NTSTATUS
LwMapSecurityInitializeSidFromUnmappedId(
    IN ULONG SidSize,
    OUT PSID Sid,
    IN BOOLEAN IsUser,
    IN ULONG Id
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_UNMAPPED_UNIX_AUTHORITY };

    if (SidSize < RtlLengthRequiredSid(SECURITY_UNMAPPED_UNIX_RID_COUNT))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    status = RtlInitializeSid(
                    Sid,
                    &identifierAuthority,
                    SECURITY_UNMAPPED_UNIX_RID_COUNT);
    GOTO_CLEANUP_ON_STATUS(status);

    if (IsUser)
    {
        Sid->SubAuthority[0] = SECURITY_UNMAPPED_UNIX_UID_RID;
    }
    else
    {
        Sid->SubAuthority[0] = SECURITY_UNMAPPED_UNIX_GID_RID;
    }

    Sid->SubAuthority[1] = Id;

cleanup:
    return status;
}

//
// API Functions
//

NTSTATUS
LwMapSecurityCreateContext(
    OUT PLW_MAP_SECURITY_CONTEXT* Context
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
LwMapSecurityFreeContext(
    IN OUT PLW_MAP_SECURITY_CONTEXT* Context
    )
{
}

NTSTATUS
LwMapSecurityGetIdFromSid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PBOOLEAN IsUser,
    OUT PULONG Id,
    IN PSID Sid
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
LwMapSecurityGetSidFromId(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PSID* Sid,
    IN BOOLEAN IsUser,
    IN ULONG Id
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
LwMapSecurityFreeSid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    IN OUT PSID* Sid
    )
{
}

NTSTATUS
LwMapSecurityGetAccessTokenCreateInformationFromUidGid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN ULONG Uid,
    IN ULONG Gid
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
LwMapSecurityGetAccessTokenCreateInformationFromId(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN BOOLEAN IsUser,
    IN ULONG Id
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
LwMapSecurityGetAccessTokenCreateInformationFromUnicodeStringName(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN BOOLEAN IsUser,
    IN PUNICODE_STRING Name
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
LwMapSecurityGetAccessTokenCreateInformationFromAnsiStringName(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN BOOLEAN IsUser,
    IN PANSI_STRING Name
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
LwMapSecurityGetAccessTokenCreateInformationFromWC16StringName(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN BOOLEAN IsUser,
    IN PCWSTR Name
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
LwMapSecurityGetAccessTokenCreateInformationFromCStringName(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN BOOLEAN IsUser,
    IN PCSTR Name
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
LwMapSecurityFreeAccessTokenCreateInformation(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    IN OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation
    )
{
}
