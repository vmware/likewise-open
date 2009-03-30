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
#include <lw/base.h>
#include "config.h"
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif
//#include <lw/safeint.h>
//#include <assert.h>

#define LOG_ERROR(szFmt, ...)

#define LW_MAP_SECURITY_PLUGIN_PATH "/opt/likewise/" LIBDIR "/liblsatokencreateinfo" MOD_EXT

typedef struct _LW_MAP_SECURITY_CONTEXT {
    PVOID LibraryHandle;
    PLW_MAP_SECURITY_PLUGIN_CONTEXT PluginContext;
    PLW_MAP_SECURITY_PLUGIN_INTERFACE PluginInterface;
} LW_MAP_SECURITY_CONTEXT;

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

#define SECURITY_UNMAPPED_UNIX_UID_PREFIX "S-1-22-1-"
#define SECURITY_UNMAPPED_UNIX_GID_PREFIX "S-1-22-2-"

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
    NTSTATUS status = 0;
    int EE = 0;
    PCSTR pszPath = LW_MAP_SECURITY_PLUGIN_PATH;
    PCSTR pszError = NULL;
    PLW_MAP_SECURITY_CONTEXT pContext = NULL;
    LWMSP_CREATE_CONTEXT_CALLBACK pCreateContexCallback = NULL;

    status = RTL_ALLOCATE(&pContext, LW_MAP_SECURITY_CONTEXT, sizeof(*pContext));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    dlerror();

    pContext->LibraryHandle = dlopen(pszPath, RTLD_NOW | RTLD_GLOBAL);
    if (!pContext->LibraryHandle)
    {
        pszError = dlerror();

        LOG_ERROR("Failed to load %s (%s)", pszPath, SMB_SAFE_LOG_STRING(pszError));

        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    dlerror();
    pCreateContexCallback = (LWMSP_CREATE_CONTEXT_CALLBACK) dlsym(pContext->LibraryHandle, LWMSP_CREATE_CONTEXT_FUNCTION_NAME);
    if (!pCreateContexCallback)
    {
        pszError = dlerror();

        LOG_ERROR("Failed to load " LWMSP_CREATE_CONTEXT_FUNCTION_NAME " function from %s (%s)",
                  pszPath, SMB_SAFE_LOG_STRING(pszError));

        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    status = pCreateContexCallback(&pContext->PluginContext, &pContext->PluginInterface);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (!NT_SUCCESS(status))
    {
        LwMapSecurityFreeContext(&pContext);
    }

    *Context = pContext;

    return status;
}

VOID
LwMapSecurityFreeContext(
    IN OUT PLW_MAP_SECURITY_CONTEXT* Context
    )
{
    PLW_MAP_SECURITY_CONTEXT context = *Context;

    if (context)
    {
        if (context->PluginContext)
        {
            context->PluginInterface->FreeContext(&context->PluginContext);
        }
        if (context->LibraryHandle)
        {
            int err = dlclose(context->LibraryHandle);
            if (err)
            {
                LOG_ERROR("Failed to dlclose() %s", LW_MAP_SECURITY_PLUGIN_PATH);
            }
        }
        RtlMemoryFree(context);
        *Context = NULL;
    }
}

NTSTATUS
LwMapSecurityGetIdFromSid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PBOOLEAN IsUser,
    OUT PULONG Id,
    IN PSID Sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_UNMAPPED_UNIX_AUTHORITY };
    BOOLEAN isUser = FALSE;
    ULONG id = 0;

    if (RtlEqualMemory(&Sid->IdentifierAuthority, &identifierAuthority, sizeof(identifierAuthority)))
    {
        if (Sid->SubAuthorityCount != SECURITY_UNMAPPED_UNIX_RID_COUNT)
        {
            status = STATUS_INVALID_SID;
            GOTO_CLEANUP();
        }
        switch (Sid->SubAuthority[0])
        {
            case SECURITY_UNMAPPED_UNIX_UID_RID:
                isUser = TRUE;
                id = Sid->SubAuthority[1];
                break;
            case SECURITY_UNMAPPED_UNIX_GID_RID:
                isUser = FALSE;
                id = Sid->SubAuthority[1];
                break;
            default:
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
        }
    }
    else
    {
        status = Context->PluginInterface->GetIdFromSid(
                        Context->PluginContext,
                        &isUser,
                        &id,
                        Sid);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:
    if (!NT_SUCCESS(status))
    {
        isUser = FALSE;
        id = (ULONG) -1;
    }

    *IsUser = isUser;
    *Id = id;

    return status;
}

NTSTATUS
LwMapSecurityGetSidFromId(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PSID* Sid,
    IN BOOLEAN IsUser,
    IN ULONG Id
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;

    if (0 == Id)
    {
        union {
            SID Sid;
            BYTE Buffer[SID_MAX_SIZE];
        } sidBuffer;

        status = LwMapSecurityInitializeSidFromUnmappedId(
                        sizeof(sidBuffer),
                        &sidBuffer.Sid,
                        IsUser,
                        Id);
        GOTO_CLEANUP_ON_STATUS(status);

        status = Context->PluginInterface->DuplicateSid(
                        Context->PluginContext,
                        &sid,
                        &sidBuffer.Sid);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        status = Context->PluginInterface->GetSidFromId(
                        Context->PluginContext,
                        &sid,
                        IsUser,
                        Id);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:
    if (!NT_SUCCESS(sid))
    {
        Context->PluginInterface->FreeSid(
                        Context->PluginContext,
                        &sid);
    }

    *Sid = sid;

    return status;
}

VOID
LwMapSecurityFreeSid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    IN OUT PSID* Sid
    )
{
    if (*Sid)
    {
        Context->PluginInterface->FreeSid(
            Context->PluginContext,
            Sid);
    }
}

static
VOID
LwMapSecurityFreeAccessTokenCreateInformation(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    IN OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation
    )
{
    if (*CreateInformation)
    {
        Context->PluginInterface->FreeAccessTokenCreateInformation(
            Context->PluginContext,
            CreateInformation);
    }
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromUidGid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN ULONG Uid,
    IN ULONG Gid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN accessToken = NULL;
    PSID userSid = NULL;
    PSID groupSid = NULL;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;

    if (0 == Uid)
    {
        TOKEN_USER tokenUser = { { 0 } };
        union {
            TOKEN_GROUPS tokenGroups;
            struct {
                ULONG GroupCount;
                SID_AND_ATTRIBUTES Groups[1];
            };
        } tokenGroupsUnion = { .tokenGroups = { 0 } };
        TOKEN_OWNER tokenOwner = { 0 };
        TOKEN_PRIMARY_GROUP tokenPrimaryGroup = { 0 };
        TOKEN_DEFAULT_DACL tokenDefaultDacl = { 0 };
        TOKEN_UNIX tokenUnix = { 0 };

        status = LwMapSecurityGetSidFromId(Context, &userSid, TRUE, Uid);
        GOTO_CLEANUP_ON_STATUS(status);

        status = LwMapSecurityGetSidFromId(Context, &groupSid, FALSE, Gid);
        GOTO_CLEANUP_ON_STATUS(status);

        tokenUser.User.Sid = userSid;

        tokenGroupsUnion.tokenGroups.GroupCount = 1;
        tokenGroupsUnion.tokenGroups.Groups[1].Sid = groupSid;

        tokenOwner.Owner = userSid;
        tokenPrimaryGroup.PrimaryGroup = groupSid;

        tokenUnix.Uid = Uid;
        tokenUnix.Gid = Gid;
        tokenUnix.Umask = 0;

        status = RtlCreateAccessToken(
                        &accessToken,
                        &tokenUser,
                        &tokenGroupsUnion.tokenGroups,
                        &tokenOwner,
                        &tokenPrimaryGroup,
                        &tokenDefaultDacl,
                        &tokenUnix);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        status = Context->PluginInterface->GetAccessTokenCreateInformationFromUid(
                        Context->PluginContext,
                        &createInformation,
                        Uid,
                        &Gid);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlCreateAccessToken(
                        &accessToken,
                        createInformation->User,
                        createInformation->Groups,
                        createInformation->Owner,
                        createInformation->PrimaryGroup,
                        createInformation->DefaultDacl,
                        createInformation->Unix);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (accessToken)
        {
            RtlReleaseAccessToken(&accessToken);
        }
    }

    LwMapSecurityFreeSid(Context, &userSid);
    LwMapSecurityFreeSid(Context, &groupSid);
    LwMapSecurityFreeAccessTokenCreateInformation(Context, &createInformation);

    *AccessToken = accessToken;

    return status;
}


NTSTATUS
LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PUNICODE_STRING Username
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN accessToken = NULL;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;

    status = Context->PluginInterface->GetAccessTokenCreateInformationFromUsername(
                    Context->PluginContext,
                    &createInformation,
                    Username);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCreateAccessToken(
                    &accessToken,
                    createInformation->User,
                    createInformation->Groups,
                    createInformation->Owner,
                    createInformation->PrimaryGroup,
                    createInformation->DefaultDacl,
                    createInformation->Unix);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (accessToken)
        {
            RtlReleaseAccessToken(&accessToken);
        }
    }

    LwMapSecurityFreeAccessTokenCreateInformation(Context, &createInformation);

    *AccessToken = accessToken;

    return status;
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromAnsiStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PANSI_STRING Username
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN accessToken = NULL;
    UNICODE_STRING unicodeUsername = { 0 };

    status = LwRtlUnicodeStringAllocateFromAnsiString(
                    &unicodeUsername,
                    Username);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
                    Context,
                    &accessToken,
                    &unicodeUsername);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (accessToken)
        {
            RtlReleaseAccessToken(&accessToken);
        }
    }

    LwRtlUnicodeStringFree(&unicodeUsername);

    *AccessToken = accessToken;

    return status;
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromWC16StringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PCWSTR Username
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN accessToken = NULL;
    UNICODE_STRING unicodeUsername = { 0 };

    status = LwRtlUnicodeStringAllocateFromWC16String(
                    &unicodeUsername,
                    Username);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
                    Context,
                    &accessToken,
                    &unicodeUsername);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (accessToken)
        {
            RtlReleaseAccessToken(&accessToken);
        }
    }

    LwRtlUnicodeStringFree(&unicodeUsername);

    *AccessToken = accessToken;

    return status;
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromCStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PCSTR Username
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN accessToken = NULL;
    UNICODE_STRING unicodeUsername = { 0 };

    status = LwRtlUnicodeStringAllocateFromCString(
                    &unicodeUsername,
                    Username);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
                    Context,
                    &accessToken,
                    &unicodeUsername);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (accessToken)
        {
            RtlReleaseAccessToken(&accessToken);
        }
    }

    LwRtlUnicodeStringFree(&unicodeUsername);

    *AccessToken = accessToken;

    return status;
}
