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
 *          Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
VOID
LwMapSecurityFreeContextInternal(
    IN OUT PLW_MAP_SECURITY_CONTEXT pContext
    );

static
NTSTATUS
LwMapSecurityCreateExtendedAccessToken(
    OUT         PACCESS_TOKEN*       ppAccessToken,
    IN          PTOKEN_USER          User,
    IN          PTOKEN_GROUPS        Groups,
    IN          PTOKEN_OWNER         Owner,
    IN          PTOKEN_PRIMARY_GROUP PrimaryGroup,
    IN          PTOKEN_DEFAULT_DACL  DefaultDacl,
    IN OPTIONAL PTOKEN_UNIX          Unix
    );

static
NTSTATUS
LwMapSecurityCreateExtendedGroups(
    OUT PTOKEN_GROUPS* ExtendedTokenGroups,
    IN  PTOKEN_GROUPS  OriginalTokenGroups,
    IN  ULONG          SidCount,
    IN  PSID*          ppSids
    );

static
VOID
LwMapSecurityFreeAccessTokenCreateInformation(
    IN     PLW_MAP_SECURITY_CONTEXT          pContext,
    IN OUT PACCESS_TOKEN_CREATE_INFORMATION* ppCreateInformation
    );

static
VOID
LwMapSecurityCloseLibrary(
    PVOID pLibHandle,
    PCSTR pszLibraryPath
    );

//
// Plugin Functions
//

NTSTATUS
LwMapSecurityInitializeSidFromUnmappedId(
    IN  ULONG   SidSize,
    OUT PSID    pSid,
    IN  BOOLEAN IsUser,
    IN  ULONG   Id
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
                    pSid,
                    &identifierAuthority,
                    SECURITY_UNMAPPED_UNIX_RID_COUNT);
    GOTO_CLEANUP_ON_STATUS(status);

    if (IsUser)
    {
        pSid->SubAuthority[0] = SECURITY_UNMAPPED_UNIX_UID_RID;
    }
    else
    {
        pSid->SubAuthority[0] = SECURITY_UNMAPPED_UNIX_GID_RID;
    }

    pSid->SubAuthority[1] = Id;

cleanup:

    return status;
}

//
// API Functions
//

NTSTATUS
LwMapSecurityInit(
    VOID
    )
{
    NTSTATUS                 status     = STATUS_SUCCESS;
    LW_MAP_SECURITY_CONTEXT* pContext   = NULL;
    PVOID                    pLibHandle = NULL;
    BOOLEAN                  bInLock    = FALSE;

    LW_MAP_SEC_LOCK_MUTEX(bInLock, &gLwMapSecurityGlobals.mutex);

    if (!gLwMapSecurityGlobals.pLibraryHandle)
    {
        int   EE       = 0;
        PCSTR pszError = NULL;
        LWMSP_CREATE_CONTEXT_CALLBACK pfnCreateContextCB = NULL;

        dlerror();

        pLibHandle = dlopen(gLwMapSecurityGlobals.pszLibraryPath,
                            RTLD_NOW | RTLD_GLOBAL);

        if (!pLibHandle)
        {
            pszError = dlerror();

            LOG_ERROR(  "Failed to load %s (%s (%d))",
                        gLwMapSecurityGlobals.pszLibraryPath,
                        SAFE_LOG_STRING(pszError), errno);

            status = STATUS_UNSUCCESSFUL;
            GOTO_CLEANUP_EE(EE);
        }

        dlerror();

        pfnCreateContextCB = (LWMSP_CREATE_CONTEXT_CALLBACK) dlsym(
                                    gLwMapSecurityGlobals.pLibraryHandle,
                                    LWMSP_CREATE_CONTEXT_FUNCTION_NAME);
        if (!pfnCreateContextCB)
        {
            pszError = dlerror();

            LOG_ERROR(  "Failed to load " LWMSP_CREATE_CONTEXT_FUNCTION_NAME " function from %s (%s)",
                        gLwMapSecurityGlobals.pszLibraryPath,
                        SAFE_LOG_STRING(pszError));

            status = STATUS_UNSUCCESSFUL;
            GOTO_CLEANUP_EE(EE);
        }

        status = RTL_ALLOCATE(
                        &pContext,
                        LW_MAP_SECURITY_CONTEXT,
                        sizeof(LW_MAP_SECURITY_CONTEXT));
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        pContext->refCount = 1;

        status = pfnCreateContextCB(
                            &pContext->pPluginContext,
                            &pContext->pPluginInterface);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        gLwMapSecurityGlobals.pLibraryHandle        = pLibHandle;
        pLibHandle = NULL;

        gLwMapSecurityGlobals.pLwMapSecurityContext = pContext;
        pContext = NULL;
    }

cleanup:

    LW_MAP_SEC_UNLOCK_MUTEX(bInLock, &gLwMapSecurityGlobals.mutex);

    if (pContext)
    {
        LwMapSecurityFreeContextInternal(pContext);
    }

    if (pLibHandle)
    {
        LwMapSecurityCloseLibrary(
                pLibHandle,
                gLwMapSecurityGlobals.pszLibraryPath);
    }

    return status;
}

NTSTATUS
LwMapSecurityCreateContext(
    OUT PLW_MAP_SECURITY_CONTEXT* ppContext
    )
{
    NTSTATUS status  = 0;
    int      EE      = 0;
    BOOLEAN  bInLock = FALSE;
    PLW_MAP_SECURITY_CONTEXT pContext = NULL;

    status = LwMapSecurityInit();
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LW_MAP_SEC_LOCK_MUTEX(bInLock, &gLwMapSecurityGlobals.mutex);

    pContext = gLwMapSecurityGlobals.pLwMapSecurityContext;
    InterlockedIncrement(&pContext->refCount);

    LW_MAP_SEC_UNLOCK_MUTEX(bInLock, &gLwMapSecurityGlobals.mutex);

cleanup:

    *ppContext = pContext;

    return status;
}

VOID
LwMapSecurityFreeContext(
    IN OUT PLW_MAP_SECURITY_CONTEXT* ppContext
    )
{
    PLW_MAP_SECURITY_CONTEXT pContext = *ppContext;

    if (pContext && (InterlockedDecrement(&pContext->refCount) == 0))
    {
        LwMapSecurityFreeContextInternal(pContext);
    }

    *ppContext = NULL;
}

static
VOID
LwMapSecurityFreeContextInternal(
    IN OUT PLW_MAP_SECURITY_CONTEXT pContext
    )
{
    if (pContext->pPluginContext)
    {
        pContext->pPluginInterface->FreeContext(&pContext->pPluginContext);
    }

    RtlMemoryFree(pContext);
}

NTSTATUS
LwMapSecurityGetIdFromSid(
    IN  PLW_MAP_SECURITY_CONTEXT pContext,
    OUT PBOOLEAN                 IsUser,
    OUT PULONG                   Id,
    IN  PSID                     pSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_UNMAPPED_UNIX_AUTHORITY };
    BOOLEAN isUser = FALSE;
    ULONG id = 0;

    if (RtlEqualMemory(&pSid->IdentifierAuthority, &identifierAuthority, sizeof(identifierAuthority)))
    {
        if (pSid->SubAuthorityCount != SECURITY_UNMAPPED_UNIX_RID_COUNT)
        {
            status = STATUS_INVALID_SID;
            GOTO_CLEANUP();
        }
        switch (pSid->SubAuthority[0])
        {
            case SECURITY_UNMAPPED_UNIX_UID_RID:
                isUser = TRUE;
                id = pSid->SubAuthority[1];
                break;
            case SECURITY_UNMAPPED_UNIX_GID_RID:
                isUser = FALSE;
                id = pSid->SubAuthority[1];
                break;
            default:
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
        }
    }
    else
    {
        status = pContext->pPluginInterface->GetIdFromSid(
                        pContext->pPluginContext,
                        &isUser,
                        &id,
                        pSid);
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
    IN  PLW_MAP_SECURITY_CONTEXT pContext,
    OUT PSID*                    ppSid,
    IN  BOOLEAN                  IsUser,
    IN  ULONG                    Id
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID pSid = NULL;

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

        status = pContext->pPluginInterface->DuplicateSid(
                        pContext->pPluginContext,
                        &pSid,
                        &sidBuffer.Sid);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        status = pContext->pPluginInterface->GetSidFromId(
                        pContext->pPluginContext,
                        &pSid,
                        IsUser,
                        Id);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:

    if (!NT_SUCCESS(status) && pSid)
    {
        pContext->pPluginInterface->FreeSid(
                        pContext->pPluginContext,
                        &pSid);
    }

    *ppSid = pSid;

    return status;
}

VOID
LwMapSecurityFreeSid(
    IN     PLW_MAP_SECURITY_CONTEXT pContext,
    IN OUT PSID*                    ppSid
    )
{
    if (*ppSid)
    {
        pContext->pPluginInterface->FreeSid(pContext->pPluginContext, ppSid);
    }
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromUidGid(
    IN  PLW_MAP_SECURITY_CONTEXT pContext,
    OUT PACCESS_TOKEN*           ppAccessToken,
    IN  ULONG                    Uid,
    IN  ULONG                    Gid
    )
{
    NTSTATUS      status = STATUS_SUCCESS;
    PACCESS_TOKEN pAccessToken = NULL;
    PSID          pUserSid = NULL;
    PSID          pGroupSid = NULL;
    PACCESS_TOKEN_CREATE_INFORMATION pCreateInformation = NULL;

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

        status = LwMapSecurityGetSidFromId(pContext, &pUserSid, TRUE, Uid);
        GOTO_CLEANUP_ON_STATUS(status);

        status = LwMapSecurityGetSidFromId(pContext, &pGroupSid, FALSE, Gid);
        GOTO_CLEANUP_ON_STATUS(status);

        tokenUser.User.Sid = pUserSid;

        tokenGroupsUnion.tokenGroups.GroupCount = 1;
        tokenGroupsUnion.tokenGroups.Groups[0].Sid = pGroupSid;

        tokenOwner.Owner = pUserSid;
        tokenPrimaryGroup.PrimaryGroup = pGroupSid;

        tokenUnix.Uid = Uid;
        tokenUnix.Gid = Gid;
        tokenUnix.Umask = 0;

        status = LwMapSecurityCreateExtendedAccessToken(
                        &pAccessToken,
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
        status = pContext->pPluginInterface->GetAccessTokenCreateInformationFromUid(
                        pContext->pPluginContext,
                        &pCreateInformation,
                        Uid,
                        &Gid);
        GOTO_CLEANUP_ON_STATUS(status);

        status = LwMapSecurityCreateExtendedAccessToken(
                        &pAccessToken,
                        pCreateInformation->User,
                        pCreateInformation->Groups,
                        pCreateInformation->Owner,
                        pCreateInformation->PrimaryGroup,
                        pCreateInformation->DefaultDacl,
                        pCreateInformation->Unix);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:

    if (!NT_SUCCESS(status))
    {
        if (pAccessToken)
        {
            RtlReleaseAccessToken(&pAccessToken);
        }
    }

    LwMapSecurityFreeSid(pContext, &pUserSid);
    LwMapSecurityFreeSid(pContext, &pGroupSid);
    LwMapSecurityFreeAccessTokenCreateInformation(pContext, &pCreateInformation);

    *ppAccessToken = pAccessToken;

    return status;
}

static
NTSTATUS
LwMapSecurityCreateExtendedAccessToken(
    OUT          PACCESS_TOKEN*       ppAccessToken,
    IN           PTOKEN_USER          User,
    IN           PTOKEN_GROUPS        Groups,
    IN           PTOKEN_OWNER         Owner,
    IN           PTOKEN_PRIMARY_GROUP PrimaryGroup,
    IN           PTOKEN_DEFAULT_DACL  DefaultDacl,
    IN  OPTIONAL PTOKEN_UNIX          Unix
    )
{
    NTSTATUS      status       = STATUS_SUCCESS;
    PACCESS_TOKEN pAccessToken = NULL;
    ULONG sidBuffer1[SID_MAX_SIZE / sizeof(ULONG) + 1] = { 0 };
    ULONG sidBuffer2[SID_MAX_SIZE / sizeof(ULONG) + 1] = { 0 };
    PSID sids[2] = { (PSID) sidBuffer1, (PSID) sidBuffer2 };
    ULONG sidCount = LW_ARRAY_SIZE(sids);
    ULONG size = 0;
    PTOKEN_GROUPS extendedGroups = NULL;

    size = sizeof(sidBuffer1);
    status = RtlCreateWellKnownSid(
                    WinWorldSid,
                    NULL,
                    (PSID) sidBuffer1,
                    &size);
    GOTO_CLEANUP_ON_STATUS(status);

    size = sizeof(sidBuffer2);
    status = RtlCreateWellKnownSid(
                    WinAuthenticatedUserSid,
                    NULL,
                    (PSID) sidBuffer2,
                    &size);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateExtendedGroups(
                    &extendedGroups,
                    Groups,
                    sidCount,
                    sids);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCreateAccessToken(
                    &pAccessToken,
                    User,
                    extendedGroups,
                    Owner,
                    PrimaryGroup,
                    DefaultDacl,
                    Unix);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (pAccessToken)
        {
            RtlReleaseAccessToken(&pAccessToken);
        }
    }

    RTL_FREE(&extendedGroups);

    *ppAccessToken = pAccessToken;

    return status;
}

static
NTSTATUS
LwMapSecurityCreateExtendedGroups(
    OUT PTOKEN_GROUPS* ExtendedTokenGroups,
    IN  PTOKEN_GROUPS  OriginalTokenGroups,
    IN  ULONG          SidCount,
    IN  PSID*          ppSids
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PTOKEN_GROUPS tokenGroups = NULL;
    ULONG groupCount = OriginalTokenGroups->GroupCount + SidCount;
    ULONG size = 0;
    ULONG i = 0;

    status = RtlSafeMultiplyULONG(&size, groupCount, sizeof(tokenGroups->Groups[0]));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*tokenGroups));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RTL_ALLOCATE(&tokenGroups, TOKEN_GROUPS, size);
    GOTO_CLEANUP_ON_STATUS(status);

    for (i = 0; i < SidCount; i++)
    {
        tokenGroups->Groups[tokenGroups->GroupCount].Attributes = SE_GROUP_ENABLED;
        tokenGroups->Groups[tokenGroups->GroupCount].Sid = ppSids[i];
        tokenGroups->GroupCount++;
    }

    for (i = 0; i < OriginalTokenGroups->GroupCount; i++)
    {
        tokenGroups->Groups[tokenGroups->GroupCount] = OriginalTokenGroups->Groups[i];
        tokenGroups->GroupCount++;
    }

cleanup:

    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&tokenGroups);
    }

    *ExtendedTokenGroups = tokenGroups;

    return status;
}

static
VOID
LwMapSecurityFreeAccessTokenCreateInformation(
    IN     PLW_MAP_SECURITY_CONTEXT          pContext,
    IN OUT PACCESS_TOKEN_CREATE_INFORMATION* ppCreateInformation
    )
{
    if (*ppCreateInformation)
    {
        pContext->pPluginInterface->FreeAccessTokenCreateInformation(
            pContext->pPluginContext,
            ppCreateInformation);
    }
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
    IN  PLW_MAP_SECURITY_CONTEXT pContext,
    OUT PACCESS_TOKEN*           ppAccessToken,
    IN  PUNICODE_STRING          Username
    )
{
    NTSTATUS      status       = STATUS_SUCCESS;
    PACCESS_TOKEN pAccessToken = NULL;
    PACCESS_TOKEN_CREATE_INFORMATION pCreateInformation = NULL;

    status = pContext->pPluginInterface->GetAccessTokenCreateInformationFromUsername(
                    pContext->pPluginContext,
                    &pCreateInformation,
                    Username);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateExtendedAccessToken(
                    &pAccessToken,
                    pCreateInformation->User,
                    pCreateInformation->Groups,
                    pCreateInformation->Owner,
                    pCreateInformation->PrimaryGroup,
                    pCreateInformation->DefaultDacl,
                    pCreateInformation->Unix);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (pAccessToken)
        {
            RtlReleaseAccessToken(&pAccessToken);
        }
    }

    LwMapSecurityFreeAccessTokenCreateInformation(pContext, &pCreateInformation);

    *ppAccessToken = pAccessToken;

    return status;
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromAnsiStringUsername(
    IN  PLW_MAP_SECURITY_CONTEXT pContext,
    OUT PACCESS_TOKEN*           ppAccessToken,
    IN  PANSI_STRING             Username
    )
{
    NTSTATUS       status          = STATUS_SUCCESS;
    PACCESS_TOKEN  pAccessToken    = NULL;
    UNICODE_STRING unicodeUsername = { 0 };

    status = LwRtlUnicodeStringAllocateFromAnsiString(
                    &unicodeUsername,
                    Username);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
                    pContext,
                    &pAccessToken,
                    &unicodeUsername);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (pAccessToken)
        {
            RtlReleaseAccessToken(&pAccessToken);
        }
    }

    LwRtlUnicodeStringFree(&unicodeUsername);

    *ppAccessToken = pAccessToken;

    return status;
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromWC16StringUsername(
    IN  PLW_MAP_SECURITY_CONTEXT pContext,
    OUT PACCESS_TOKEN*           ppAccessToken,
    IN  PCWSTR                   Username
    )
{
    NTSTATUS       status          = STATUS_SUCCESS;
    PACCESS_TOKEN  pAccessToken    = NULL;
    UNICODE_STRING unicodeUsername = { 0 };

    status = LwRtlUnicodeStringAllocateFromWC16String(
                    &unicodeUsername,
                    Username);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
                    pContext,
                    &pAccessToken,
                    &unicodeUsername);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (pAccessToken)
        {
            RtlReleaseAccessToken(&pAccessToken);
        }
    }

    LwRtlUnicodeStringFree(&unicodeUsername);

    *ppAccessToken = pAccessToken;

    return status;
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromCStringUsername(
    IN  PLW_MAP_SECURITY_CONTEXT pContext,
    OUT PACCESS_TOKEN*           ppAccessToken,
    IN  PCSTR                    Username
    )
{
    NTSTATUS       status = STATUS_SUCCESS;
    PACCESS_TOKEN  pAccessToken = NULL;
    UNICODE_STRING unicodeUsername = { 0 };

    status = LwRtlUnicodeStringAllocateFromCString(
                    &unicodeUsername,
                    Username);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
                    pContext,
                    &pAccessToken,
                    &unicodeUsername);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:

    if (!NT_SUCCESS(status))
    {
        if (pAccessToken)
        {
            RtlReleaseAccessToken(&pAccessToken);
        }
    }

    LwRtlUnicodeStringFree(&unicodeUsername);

    *ppAccessToken = pAccessToken;

    return status;
}

NTSTATUS
LwMapSecurityShutdown(
    VOID
    )
{
    NTSTATUS status       = STATUS_SUCCESS;
    BOOLEAN  bInLock      = FALSE;
    BOOLEAN bCloseLibrary = FALSE;

    LW_MAP_SEC_LOCK_MUTEX(bInLock, &gLwMapSecurityGlobals.mutex);

    bCloseLibrary = (gLwMapSecurityGlobals.pLibraryHandle != NULL);

    if (gLwMapSecurityGlobals.pLwMapSecurityContext)
    {
        if (LwInterlockedRead(&gLwMapSecurityGlobals.pLwMapSecurityContext->refCount) > 1)
        {
            LOG_ERROR("Warning: Cannot shut down module while still in use");

            bCloseLibrary = FALSE;
        }

        LwMapSecurityFreeContext(&gLwMapSecurityGlobals.pLwMapSecurityContext);
    }

    if (bCloseLibrary)
    {
        LwMapSecurityCloseLibrary(
                gLwMapSecurityGlobals.pLibraryHandle,
                gLwMapSecurityGlobals.pszLibraryPath);

        gLwMapSecurityGlobals.pLibraryHandle = NULL;
    }

    LW_MAP_SEC_UNLOCK_MUTEX(bInLock, &gLwMapSecurityGlobals.mutex);

    return status;
}

static
VOID
LwMapSecurityCloseLibrary(
    PVOID pLibHandle,
    PCSTR pszLibraryPath
    )
{
    int err = dlclose(pLibHandle);
    if (err)
    {
        LOG_ERROR("Failed to dlclose() %s", SAFE_LOG_STRING(pszLibraryPath));
    }
}
