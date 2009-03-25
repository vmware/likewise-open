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
 *        tokencreateinfo.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Access Token Create Information
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include <lwmapsecurity/lwmapsecurity-plugin.h>
#include <lw/base.h>
#include <lsa/lsa.h>
#include <lw/rtlgoto.h>
#include <lw/safeint.h>
#include "lsautils.h"
#include <assert.h>

typedef struct _LW_MAP_SECURITY_PLUGIN_CONTEXT {
    HANDLE hConnection;
    // TODO-Add a mutex so we can serialize lookups or else
    // do some sort of connection caching stuff.  Need to make
    // sure to handle re-connection as needed.
} LW_MAP_SECURITY_PLUGIN_CONTEXT;

typedef struct _LSA_MAP_SECURITY_OBJECT_INFO {
    BOOLEAN IsUser;
    ULONG Id;
    ULONG Gid;
    PSID Sid;
} LSA_MAP_SECURITY_OBJECT_INFO, *PLSA_MAP_SECURITY_OBJECT_INFO;

#define IS_NOT_FOUND_ERROR(lsaError) ( \
    (LSA_ERROR_NO_SUCH_USER == (lsaError)) || \
    (LSA_ERROR_NO_SUCH_GROUP == (lsaError)) || \
    (LSA_ERROR_NO_SUCH_USER_OR_GROUP == (lsaError)) || \
    0 )

static
VOID
LsaMapSecurityFreeObjectInfo(
    IN OUT PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo
    )
{
    RTL_FREE(&pObjectInfo->Sid);
}

//
// Use a single "resolve" function so that we can centralize any code
// to (re)connect as well as special-case certain errors.
//

static
NTSTATUS
LsaMapSecurityResolveObjectInfo(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN BOOLEAN IsUser,
    IN OPTIONAL PCSTR pszName,
    IN OPTIONAL ULONG Id,
    OUT PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LSA_ERROR_SUCCESS;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };

    if (IsUser)
    {
        if (pszName)
        {
            dwError = LsaFindUserByName(Context->hConnection, pszName, 0, OUT_PPVOID(&pUserInfo));
        }
        else
        {
            dwError = LsaFindUserById(Context->hConnection, Id, 0, OUT_PPVOID(&pUserInfo));
        }
    }
    else
    {
        if (pszName)
        {
            dwError = LsaFindGroupByName(Context->hConnection, pszName, 0, 0, OUT_PPVOID(&pGroupInfo));
        }
        else
        {
            dwError = LsaFindGroupById(Context->hConnection, Id, 0, 0, OUT_PPVOID(&pGroupInfo));
        }
    }
    if (IS_NOT_FOUND_ERROR(dwError))
    {
        status = STATUS_SUCCESS;
        GOTO_CLEANUP();
    }
    status = LsaLsaErrorToNtStatus(dwError);
    GOTO_CLEANUP_ON_STATUS(status);

    objectInfo.IsUser = IsUser;
    if (IsUser)
    {
        objectInfo.Id = pUserInfo->uid;
        objectInfo.Gid = pUserInfo->gid;

        status = RtlAllocateSidFromCString(&objectInfo.Sid, pUserInfo->pszSid);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        objectInfo.Id = pGroupInfo->gid;
        objectInfo.Gid = pGroupInfo->gid;

        status = RtlAllocateSidFromCString(&objectInfo.Sid, pGroupInfo->pszSid);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:
    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeObjectInfo(&objectInfo);
    }

    if (pUserInfo)
    {
        LsaFreeUserInfo(0, pUserInfo);
    }
    if (pGroupInfo)
    {
        LsaFreeGroupInfo(0, pGroupInfo);
    }

    *pObjectInfo = objectInfo;

    return status;
}

static
NTSTATUS
LsaMapSecurityResolveObjectInfoBySid(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN PSID Sid,
    OUT PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LSA_ERROR_SUCCESS;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };
    PSTR pszSid = NULL;
    PLSA_SID_INFO sidInfoList = NULL;
    CHAR separator = 0;
    BOOLEAN isUser = FALSE;
    PSTR pszName = NULL;

    status = RtlAllocateCStringFromSid(&pszSid, Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    // TODO-Add LSA client API that allows lookup of user/group
    // info by SID directly as the provider can handle that internally.
    dwError = LsaGetNamesBySidList(
                    Context->hConnection,
                    1,
                    &pszSid,
                    &sidInfoList,
                    &separator);
    status = LsaLsaErrorToNtStatus(dwError);
    assert(STATUS_NOT_FOUND != status);
    GOTO_CLEANUP_ON_STATUS(status);

    // ISSUE-Does the code really distinguish error codes
    // properly?  Would LsaGetNamesBySidList() just
    // return AccountType_NotFound if there were a memory
    // error, for instance?

    switch (sidInfoList[0].accountType)
    {
        case AccountType_User:
            isUser = TRUE;
            break;
        case AccountType_Group:
            isUser = FALSE;
            break;
        case AccountType_NotFound:
            status = STATUS_NOT_FOUND;
            GOTO_CLEANUP();
        default:
            status = STATUS_NOT_FOUND;
            GOTO_CLEANUP();
    }

    dwError = LsaAllocateStringPrintf(
                    &pszName,
                    "%s%c%s",
                    sidInfoList[0].pszDomainName,
                    separator,
                    sidInfoList[0].pszSamAccountName);
    status = LsaLsaErrorToNtStatus(dwError);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsaMapSecurityResolveObjectInfo(
                    Context,
                    isUser,
                    pszName,
                    0,
                    &objectInfo);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeObjectInfo(&objectInfo);
    }

    if (sidInfoList)
    {
        LsaFreeSIDInfoList(sidInfoList, 1);
    }

    *pObjectInfo = objectInfo;

    return status;
}

static
VOID
LsaMapSecurityFreeSid(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN OUT PSID* Sid
    )
{
    RTL_FREE(Sid);
}

static
NTSTATUS
LsaMapSecurityGetIdFromSid(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PBOOLEAN IsUser,
    OUT PULONG Id,
    IN PSID Sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };
    BOOLEAN isUser = FALSE;
    ULONG id = 0;

    status = LsaMapSecurityResolveObjectInfoBySid(Context, Sid, &objectInfo);
    GOTO_CLEANUP_ON_STATUS(status);

    isUser = objectInfo.IsUser;
    id = objectInfo.Id;

cleanup:
    if (!NT_SUCCESS(status))
    {
        isUser = FALSE;
        id = 0;
    }

    LsaMapSecurityFreeObjectInfo(&objectInfo);

    *IsUser = isUser;
    *Id = id;

    return status;
}

static
NTSTATUS
LsaMapSecurityGetSidFromId(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PSID* Sid,
    IN BOOLEAN IsUser,
    IN ULONG Id
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };
    PSID sid = NULL;

    status = LsaMapSecurityResolveObjectInfo(Context, IsUser, NULL, Id, &objectInfo);
    GOTO_CLEANUP_ON_STATUS(status);

    sid = objectInfo.Sid;
    objectInfo.Sid = NULL;

cleanup:
    LsaMapSecurityFreeObjectInfo(&objectInfo);

    *Sid = sid;

    return status;
}

static
VOID
LsaMapSecurityFreeAccessTokenCreateInformation(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation
    )
{
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = *CreateInformation;

    if (createInformation)
    {
        ULONG i = 0;

        RTL_FREE(&createInformation->User->User.Sid);
        for (i = 0; i < createInformation->Groups->GroupCount; i++)
        {
            RTL_FREE(&createInformation->Groups->Groups[i].Sid);
        }
        RTL_FREE(&createInformation->Owner->Owner);
        RTL_FREE(&createInformation->PrimaryGroup->PrimaryGroup);
        RTL_FREE(&createInformation->DefaultDacl->DefaultDacl);
        RTL_FREE(&createInformation);
        *CreateInformation = NULL;
    }
}


static
NTSTATUS
LsaMapSecurityAllocateAccessTokenCreateInformation(
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN ULONG GroupCount
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;
    ULONG size = 0;
    PVOID location = NULL;

    //
    // Compute size for everything except the actual SIDs.  This includes
    // the flexible array using GroupCount.
    //

    status = RtlSafeMultiplyULONG(&size, GroupCount, sizeof(createInformation->Groups->Groups[0]));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation->User));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation->Groups));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(createInformation->Owner));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(createInformation->PrimaryGroup));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(createInformation->DefaultDacl));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(createInformation->Unix));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RTL_ALLOCATE(&createInformation, ACCESS_TOKEN_CREATE_INFORMATION, size);
    GOTO_CLEANUP_ON_STATUS(status);

    location = createInformation;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation));

    createInformation->User = location;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation->User));

    createInformation->Groups = location;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation->Groups));
    location = LwRtlOffsetToPointer(location, GroupCount * sizeof(createInformation->Groups->Groups[0]));

    createInformation->Owner = location;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation->Owner));

    createInformation->PrimaryGroup = location;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation->PrimaryGroup));

    createInformation->Unix = location;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation->Unix));

    assert(LwRtlOffsetToPointer(location, size) == location);

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&createInformation);
    }

    *CreateInformation = createInformation;

    return status;
}

static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformationFromUserInfo(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LSA_ERROR_SUCCESS;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;
    ULONG groupCount = 0;
    PLSA_GROUP_INFO_0* ppGroupInfoList = NULL;
    ULONG i = 0;

    dwError = LsaGetGroupsForUserById(
                    Context->hConnection,
                    pObjectInfo->Id,
                    0,
                    0,
                    &groupCount,
                    (PVOID**)OUT_PPVOID(&ppGroupInfoList));
    status = LsaLsaErrorToNtStatus(dwError);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsaMapSecurityAllocateAccessTokenCreateInformation(
                    &createInformation,
                    groupCount);
    GOTO_CLEANUP_ON_STATUS(status);

    createInformation->Unix->Uid = pObjectInfo->Id;
    createInformation->Unix->Gid = pObjectInfo->Gid;

    status = RtlDuplicateSid(&createInformation->User->User.Sid, pObjectInfo->Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    createInformation->Groups->GroupCount = groupCount;
    for (i = 0; i < groupCount; i++)
    {
        status = RtlAllocateSidFromCString(
                        &createInformation->Groups->Groups[i].Sid,
                        ppGroupInfoList[i]->pszSid);
        GOTO_CLEANUP_ON_STATUS(status);

        createInformation->Groups->Groups[i].Attributes = SE_GROUP_ENABLED;
    }

    status = RtlDuplicateSid(&createInformation->Owner->Owner, pObjectInfo->Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsaMapSecurityGetSidFromId(
                    Context,
                    &createInformation->PrimaryGroup->PrimaryGroup,
                    FALSE,
                    pObjectInfo->Gid);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeAccessTokenCreateInformation(Context, &createInformation);
    }

    LsaFreeGroupInfoList(0, (PVOID*)ppGroupInfoList, groupCount);

    *CreateInformation = createInformation;

    return status;
}

static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformationFromGroupInfo(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;

    status = LsaMapSecurityAllocateAccessTokenCreateInformation(&createInformation, 0);
    GOTO_CLEANUP_ON_STATUS(status);

    // TODO-Looking up a group directly is kind of wacky...
    // We probably should remove this codepath...
    status = RtlDuplicateSid(&createInformation->User->User.Sid, pObjectInfo->Sid);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeAccessTokenCreateInformation(Context, &createInformation);
    }

    *CreateInformation = createInformation;

    return status;
}

static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformationEx(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN BOOLEAN IsUser,
    IN OPTIONAL PUNICODE_STRING Name,
    IN OPTIONAL ULONG Id
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };
    PSTR pszName = NULL;

    if (Name)
    {
        status = LwRtlCStringAllocateFromUnicodeString(&pszName, Name);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = LsaMapSecurityResolveObjectInfo(Context, IsUser, pszName, Id, &objectInfo);
    GOTO_CLEANUP_ON_STATUS(status);

    if (IsUser)
    {
        status = LsaMapSecurityGetAccessTokenCreateInformationFromUserInfo(
                        Context,
                        &createInformation,
                        &objectInfo);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        status = LsaMapSecurityGetAccessTokenCreateInformationFromGroupInfo(
                        Context,
                        &createInformation,
                        &objectInfo);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:
    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeAccessTokenCreateInformation(Context, &createInformation);
    }

    LsaMapSecurityFreeObjectInfo(&objectInfo);

    *CreateInformation = createInformation;

    return status;
}

static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformationFromId(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN BOOLEAN IsUser,
    IN ULONG Id
    )
{
    return LsaMapSecurityGetAccessTokenCreateInformationEx(
                Context,
                CreateInformation,
                IsUser,
                NULL,
                Id);
}

static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformationFromName(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN BOOLEAN IsUser,
    IN PUNICODE_STRING Name
    )
{
    return LsaMapSecurityGetAccessTokenCreateInformationEx(
                Context,
                CreateInformation,
                IsUser,
                Name,
                0);
}

static
VOID
LsaMapSecurityFreeContext(
    IN OUT PLW_MAP_SECURITY_PLUGIN_CONTEXT* Context
    )
{
    PLW_MAP_SECURITY_PLUGIN_CONTEXT context = *Context;

    if (context)
    {
        if (context->hConnection)
        {
            LsaCloseServer(context->hConnection);
        }
        RTL_FREE(&context);
        *Context = NULL;
    }
}

static LW_MAP_SECURITY_PLUGIN_INTERFACE gLsaMapSecurityPluginInterface = {
    .FreeContext = LsaMapSecurityFreeContext,
    .GetIdFromSid = LsaMapSecurityGetIdFromSid,
    .GetSidFromId = LsaMapSecurityGetSidFromId,
    .FreeSid = LsaMapSecurityFreeSid,
    .GetAccessTokenCreateInformationFromId = LsaMapSecurityGetAccessTokenCreateInformationFromId,
    .GetAccessTokenCreateInformationFromName = LsaMapSecurityGetAccessTokenCreateInformationFromName,
    .FreeAccessTokenCreateInformation = LsaMapSecurityFreeAccessTokenCreateInformation,
};

NTSTATUS
LsaMapSecurityCreateContext(
    OUT PLW_MAP_SECURITY_PLUGIN_CONTEXT* Context,
    OUT PLW_MAP_SECURITY_PLUGIN_INTERFACE* Interface
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PLW_MAP_SECURITY_PLUGIN_CONTEXT context = NULL;
    // compiler type check for this function
    LWMSP_CREATE_CONTEXT_CALLBACK unused = LsaMapSecurityCreateContext;

    assert(unused);

    status = RTL_ALLOCATE(&context, LW_MAP_SECURITY_PLUGIN_CONTEXT, sizeof(*context));
    GOTO_CLEANUP_ON_STATUS(status);

    dwError = LsaOpenServer(&context->hConnection);
    status = LsaLsaErrorToNtStatus(dwError);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeContext(&context);
    }

    *Context = context;
    *Interface = NT_SUCCESS(status) ? &gLsaMapSecurityPluginInterface : NULL;

    return status;
}
