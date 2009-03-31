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
    // TODO-Add connection caching using TLS, a connection pool,
    // or somesuch.  It may be useful to change the calls to LSASS to
    // go through a LsaMapSecurityCallLsass() that is like
    // LsaDmConnectDomain() so we can automatically retry on
    // connection-type errors.
    HANDLE hUnusedConnection;
} LW_MAP_SECURITY_PLUGIN_CONTEXT;

typedef UCHAR LSA_MAP_SECURITY_OBJECT_INFO_FLAGS, *PLSA_MAP_SECURITY_OBJECT_INFO_FLAGS;

#define LSA_MAP_SECURITY_OBJECT_INFO_FLAG_IS_USER   0x01
#define LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_UID 0x02
#define LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID 0x04

typedef struct _LSA_MAP_SECURITY_OBJECT_INFO {
    LSA_MAP_SECURITY_OBJECT_INFO_FLAGS Flags;
    ULONG Uid;
    ULONG Gid;
    PSID Sid;
} LSA_MAP_SECURITY_OBJECT_INFO, *PLSA_MAP_SECURITY_OBJECT_INFO;

#define IS_NOT_FOUND_ERROR(lsaError) ( \
    (LSA_ERROR_NO_SUCH_USER == (lsaError)) || \
    (LSA_ERROR_NO_SUCH_GROUP == (lsaError)) || \
    (LSA_ERROR_NO_SUCH_USER_OR_GROUP == (lsaError)) || \
    0 )

static
NTSTATUS
LsaMapSecurityOpenConnection(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PHANDLE phConnection
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LSA_ERROR_SUCCESS;
    HANDLE hConnection = NULL;

    dwError = LsaOpenServer(&hConnection);
    status = LsaLsaErrorToNtStatus(dwError);

    *phConnection = hConnection;

    return status;
}

static
VOID
LsaMapSecurityCloseConnection(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN OUT PHANDLE phConnection
    )
{
    HANDLE hConnection = *phConnection;

    if (hConnection)
    {
        NTSTATUS status = STATUS_SUCCESS;
        DWORD dwError = LSA_ERROR_SUCCESS;

        dwError = LsaCloseServer(hConnection);
        status = LsaLsaErrorToNtStatus(dwError);

        *phConnection = NULL;
    }
}

static
VOID
LsaMapSecurityFreeObjectInfo(
    IN OUT PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo
    )
{
    RTL_FREE(&pObjectInfo->Sid);
    RtlZeroMemory(pObjectInfo, sizeof(*pObjectInfo));
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
    IN OPTIONAL PULONG Id,
    OUT PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LSA_ERROR_SUCCESS;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };
    ULONG id = Id ? *Id : (ULONG) -1;
    HANDLE hConnection = NULL;

    if (IS_BOTH_OR_NEITHER(pszName, Id))
    {
        assert(FALSE);
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    status = LsaMapSecurityOpenConnection(Context, &hConnection);
    if (NT_SUCCESS(status))
    {
        if (IsUser)
        {
            if (pszName)
            {
                dwError = LsaFindUserByName(hConnection, pszName, 0, OUT_PPVOID(&pUserInfo));
            }
            else
            {
                dwError = LsaFindUserById(hConnection, id, 0, OUT_PPVOID(&pUserInfo));
            }
        }
        else
        {
            if (pszName)
            {
                dwError = LsaFindGroupByName(hConnection, pszName, 0, 0, OUT_PPVOID(&pGroupInfo));
            }
            else
            {
                dwError = LsaFindGroupById(hConnection, id, 0, 0, OUT_PPVOID(&pGroupInfo));
            }
        }
        LsaMapSecurityCloseConnection(Context, &hConnection);
    }
    else
    {
        dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;
    }

    if (IS_NOT_FOUND_ERROR(dwError))
    {
        union {
            SID Sid;
            BYTE Buffer[SID_MAX_SIZE];
        } sidBuffer;

        if (pszName)
        {
            status = STATUS_NOT_FOUND;
            GOTO_CLEANUP();
        }

        status = LwMapSecurityInitializeSidFromUnmappedId(
                        sizeof(sidBuffer),
                        &sidBuffer.Sid,
                        IsUser,
                        id);
        GOTO_CLEANUP_ON_STATUS(status);

        if (IsUser)
        {
            SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_IS_USER);
            SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_UID);
            objectInfo.Uid = id;
        }
        else
        {
            SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID);
            objectInfo.Gid = id;
        }

        status = RtlDuplicateSid(&objectInfo.Sid, &sidBuffer.Sid);
        GOTO_CLEANUP_ON_STATUS(status);

        status = STATUS_SUCCESS;
        GOTO_CLEANUP();
    }

    status = LsaLsaErrorToNtStatus(dwError);
    GOTO_CLEANUP_ON_STATUS(status);

    if (IsUser)
    {
        assert(pszName || (id == pUserInfo->uid));

        SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_IS_USER);
        SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_UID);
        SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID);

        objectInfo.Uid = pUserInfo->uid;
        objectInfo.Gid = pUserInfo->gid;

        status = RtlAllocateSidFromCString(&objectInfo.Sid, pUserInfo->pszSid);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        assert(pszName || (id == pGroupInfo->gid));

        SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID);

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
    HANDLE hConnection = NULL;

    status = RtlAllocateCStringFromSid(&pszSid, Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsaMapSecurityOpenConnection(Context, &hConnection);
    GOTO_CLEANUP_ON_STATUS(status);

    // TODO-Add LSA client API that allows lookup of user/group
    // info by SID directly as the provider can handle that internally.
    dwError = LsaGetNamesBySidList(
                    hConnection,
                    1,
                    &pszSid,
                    &sidInfoList,
                    &separator);
    status = LsaLsaErrorToNtStatus(dwError);
    assert(STATUS_NOT_FOUND != status);
    GOTO_CLEANUP_ON_STATUS(status);

    LsaMapSecurityCloseConnection(Context, &hConnection);

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
                    NULL,
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
NTSTATUS
LsaMapSecurityDuplicateSid(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN OUT PSID* Sid,
    IN PSID OriginalSid
    )
{
    return RtlDuplicateSid(Sid, OriginalSid);
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

    isUser = IsSetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_IS_USER);
    id = isUser ? objectInfo.Uid : objectInfo.Gid;

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

    status = LsaMapSecurityResolveObjectInfo(Context, IsUser, NULL, &Id, &objectInfo);
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

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation->Owner));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation->PrimaryGroup));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation->DefaultDacl));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation->Unix));
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

    createInformation->DefaultDacl = location;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation->DefaultDacl));

    createInformation->Unix = location;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation->Unix));

    assert(LwRtlOffsetToPointer(createInformation, size) == location);

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&createInformation);
    }

    *CreateInformation = createInformation;

    return status;
}

static
BOOLEAN
LsaMapSecurityIsGidInGidList(
    IN ULONG Gid,
    IN ULONG GidCount,
    IN PULONG pGidList
    )
{
    BOOLEAN isFound = FALSE;
    ULONG i = 0;

    for (i = 0; i < GidCount; i++)
    {
        if (Gid == pGidList[i])
        {
            isFound = TRUE;
            break;
        }
    }

    return isFound;
}

static
BOOLEAN
LsaMapSecurityIsGidInGroupInfoList(
    IN ULONG Gid,
    IN ULONG GroupInfoCount,
    IN PLSA_GROUP_INFO_0* ppGroupInfoList
    )
{
    BOOLEAN isFound = FALSE;
    ULONG i = 0;

    for (i = 0; i < GroupInfoCount; i++)
    {
        if (Gid == ppGroupInfoList[i]->gid)
        {
            isFound = TRUE;
            break;
        }
    }

    return isFound;
}

static
VOID
LsaMapSecurityAddExtraGid(
    IN ULONG Gid,
    IN OUT PULONG ExtraGidCount,
    IN OUT PULONG ExtraGidList,
    IN ULONG ExtraGidMaximumCount,
    IN ULONG GroupInfoCount,
    IN PLSA_GROUP_INFO_0* ppGroupInfoList
    )
{
    ULONG extraGidCount = *ExtraGidCount;

    if (extraGidCount < ExtraGidMaximumCount)
    {
        if (!LsaMapSecurityIsGidInGidList(Gid, extraGidCount, ExtraGidList) &&
            !LsaMapSecurityIsGidInGroupInfoList(Gid, GroupInfoCount, ppGroupInfoList))
        {
            ExtraGidList[extraGidCount] = Gid;
            *ExtraGidCount = extraGidCount + 1;
        }
    }
}

static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformationFromObjectInfo(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo,
    IN OPTIONAL PULONG Gid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LSA_ERROR_SUCCESS;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;
    ULONG groupInfoCount = 0;
    PLSA_GROUP_INFO_0* ppGroupInfoList = NULL;
    ULONG i = 0;
    ULONG gid = Gid ? *Gid : 0;
    ULONG extraGidList[2] = { 0 };
    PSID extraGidSidList[LW_ARRAY_SIZE(extraGidList)] = { 0 };
    ULONG extraGidCount = 0;
    HANDLE hConnection = NULL;

    status = LsaMapSecurityOpenConnection(Context, &hConnection);
    GOTO_CLEANUP_ON_STATUS(status);

    dwError = LsaGetGroupsForUserById(
                    hConnection,
                    pObjectInfo->Uid,
                    0,
                    0,
                    &groupInfoCount,
                    (PVOID**)OUT_PPVOID(&ppGroupInfoList));
    status = LsaLsaErrorToNtStatus(dwError);
    GOTO_CLEANUP_ON_STATUS(status);

    LsaMapSecurityCloseConnection(Context, &hConnection);

    //
    // Take into account extra GIDs that came as primary GID from
    // object info or from passed in primary GID.
    //

    if (IsSetFlag(pObjectInfo->Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID))
    {
        LsaMapSecurityAddExtraGid(
                pObjectInfo->Gid,
                &extraGidCount,
                extraGidList,
                LW_ARRAY_SIZE(extraGidList),
                groupInfoCount,
                ppGroupInfoList);
    }

    if (Gid)
    {
        LsaMapSecurityAddExtraGid(
                gid,
                &extraGidCount,
                extraGidList,
                LW_ARRAY_SIZE(extraGidList),
                groupInfoCount,
                ppGroupInfoList);
    }

    //
    // Resolve extra GIDs into SIDs
    //

    for (i = 0; i < extraGidCount; i++)
    {
        status = LsaMapSecurityGetSidFromId(
                        Context,
                        &extraGidSidList[i],
                        FALSE,
                        extraGidList[i]);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    //
    // Allocate token create information with enough space
    // for any potential extra GIDs.
    //

    status = LsaMapSecurityAllocateAccessTokenCreateInformation(
                    &createInformation,
                    groupInfoCount + extraGidCount);
    GOTO_CLEANUP_ON_STATUS(status);

    //
    // TOKEN_UNIX
    //

    createInformation->Unix->Uid = pObjectInfo->Uid;

    // TODO-Should the passed in GID take precedence over the LSASS one?
    if (IsSetFlag(pObjectInfo->Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID))
    {
        createInformation->Unix->Gid = pObjectInfo->Gid;
    }
    else if (Gid)
    {
        createInformation->Unix->Gid = gid;
    }
    else
    {
        // TODO-Would need to put in some nobody-like gid.
        assert(FALSE);
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    // TODO-Should the token even have a umask, and if so,
    // where should it come from?
    createInformation->Unix->Umask = 0022;

    //
    // TOKEN_USER
    //

    status = RtlDuplicateSid(&createInformation->User->User.Sid, pObjectInfo->Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    //
    // TOKEN_GROUPS
    //

    for (i = 0; i < groupInfoCount; i++)
    {
        PSID_AND_ATTRIBUTES group = &createInformation->Groups->Groups[createInformation->Groups->GroupCount];

        status = RtlAllocateSidFromCString(
                        &group->Sid,
                        ppGroupInfoList[i]->pszSid);
        GOTO_CLEANUP_ON_STATUS(status);

        group->Attributes = SE_GROUP_ENABLED;

        createInformation->Groups->GroupCount++;
    }

    for (i = 0; i < extraGidCount; i++)
    {
        PSID_AND_ATTRIBUTES group = &createInformation->Groups->Groups[createInformation->Groups->GroupCount];

        group->Sid = extraGidSidList[i];
        extraGidSidList[i] = NULL;

        group->Attributes = SE_GROUP_ENABLED;

        createInformation->Groups->GroupCount++;
    }

    //
    // TOKEN_OWNER
    //

    status = RtlDuplicateSid(&createInformation->Owner->Owner, pObjectInfo->Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    //
    // TOKEN_PRIMARY_GROUP
    //

    status = LsaMapSecurityGetSidFromId(
                    Context,
                    &createInformation->PrimaryGroup->PrimaryGroup,
                    FALSE,
                    createInformation->Unix->Gid);
    GOTO_CLEANUP_ON_STATUS(status);

    //
    // TOKEN_DEFAULT_DACL
    //

    // TODO-Implement TOKEN_DEFAULT_DACL?

cleanup:
    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeAccessTokenCreateInformation(Context, &createInformation);
    }

    LsaFreeGroupInfoList(0, (PVOID*)ppGroupInfoList, groupInfoCount);

    for (i = 0; i < extraGidCount; i++)
    {
        RTL_FREE(&extraGidSidList[i]);
    }

    *CreateInformation = createInformation;

    return status;
}

static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformation(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN OPTIONAL PUNICODE_STRING Username,
    IN OPTIONAL PULONG Uid,
    IN OPTIONAL PULONG Gid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };
    PSTR pszUsername = NULL;

    if (IS_BOTH_OR_NEITHER(Username, Uid) ||
        (Gid && !Uid))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (Username)
    {
        status = LwRtlCStringAllocateFromUnicodeString(&pszUsername, Username);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = LsaMapSecurityResolveObjectInfo(
                    Context,
                    TRUE,
                    pszUsername,
                    Uid,
                    &objectInfo);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsaMapSecurityGetAccessTokenCreateInformationFromObjectInfo(
                    Context,
                    &createInformation,
                    &objectInfo,
                    Gid);
    GOTO_CLEANUP_ON_STATUS(status);

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
LsaMapSecurityGetAccessTokenCreateInformationFromUid(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN ULONG Uid,
    IN PULONG Gid
    )
{
    return LsaMapSecurityGetAccessTokenCreateInformation(
                Context,
                CreateInformation,
                NULL,
                &Uid,
                Gid);
}

static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformationFromUsername(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN PUNICODE_STRING Username
    )
{
    return LsaMapSecurityGetAccessTokenCreateInformation(
                Context,
                CreateInformation,
                Username,
                NULL,
                NULL);
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
        RTL_FREE(&context);
        *Context = NULL;
    }
}

static LW_MAP_SECURITY_PLUGIN_INTERFACE gLsaMapSecurityPluginInterface = {
    .FreeContext = LsaMapSecurityFreeContext,
    .GetIdFromSid = LsaMapSecurityGetIdFromSid,
    .GetSidFromId = LsaMapSecurityGetSidFromId,
    .DuplicateSid = LsaMapSecurityDuplicateSid,
    .FreeSid = LsaMapSecurityFreeSid,
    .GetAccessTokenCreateInformationFromUid = LsaMapSecurityGetAccessTokenCreateInformationFromUid,
    .GetAccessTokenCreateInformationFromUsername = LsaMapSecurityGetAccessTokenCreateInformationFromUsername,
    .FreeAccessTokenCreateInformation = LsaMapSecurityFreeAccessTokenCreateInformation,
};

NTSTATUS
MapSecurityPluginCreateContext(
    OUT PLW_MAP_SECURITY_PLUGIN_CONTEXT* Context,
    OUT PLW_MAP_SECURITY_PLUGIN_INTERFACE* Interface
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_MAP_SECURITY_PLUGIN_CONTEXT context = NULL;
    // compiler type check for this function
    LWMSP_CREATE_CONTEXT_CALLBACK unused = MapSecurityPluginCreateContext;

    assert(unused);

    status = RTL_ALLOCATE(&context, LW_MAP_SECURITY_PLUGIN_CONTEXT, sizeof(*context));
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
