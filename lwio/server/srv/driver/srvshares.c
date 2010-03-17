/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        srvshares.c
 *
 * Abstract:
 *
 *        Likewise File System Driver (Srv)
 *
 *        DeviceIo Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

NTSTATUS
SrvShareDevCtlAdd(
    IN     PBYTE lpInBuffer,
    IN     ULONG ulInBufferSize,
    IN OUT PBYTE lpOutBuffer,
    IN     ULONG ulOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList = NULL;
    PSHARE_INFO_ADD_PARAMS pAddShareInfoParams = NULL;
    PSHARE_INFO_0 pShareInfo0 = NULL;
    PSHARE_INFO_1 pShareInfo1 = NULL;
    PSHARE_INFO_2 pShareInfo2 = NULL;
    PSHARE_INFO_501 pShareInfo501 = NULL;
    PSHARE_INFO_502 pShareInfo502 = NULL;
    PWSTR pwszShareName = NULL;
    ULONG ulShareType = 0;
    PWSTR pwszComment = NULL;
    PWSTR pwszPath = NULL;
    PWSTR pwszPathLocal = NULL;
    PWSTR pwszShareType = NULL;
    ULONG ulSecDescLen = 0;
    PBYTE pSecDesc = NULL;

    ntStatus = LwShareInfoUnmarshalAddParameters(
                        lpInBuffer,
                        ulInBufferSize,
                        &pAddShareInfoParams
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pAddShareInfoParams->dwInfoLevel)
    {
    case 0:
        pShareInfo0   = pAddShareInfoParams->info.p0;
        pwszShareName = pShareInfo0->shi0_netname;
        ulShareType   = SHARE_SERVICE_DISK_SHARE;
        pwszComment   = NULL;
        pwszPath      = NULL;
        break;

    case 1:
        pShareInfo1   = pAddShareInfoParams->info.p1;
        pwszShareName = pShareInfo1->shi1_netname;
        ulShareType   = pShareInfo1->shi1_type;
        pwszComment   = pShareInfo1->shi1_remark;
        pwszPath      = NULL;
        break;

    case 2:
        pShareInfo2   = pAddShareInfoParams->info.p2;
        pwszShareName = pShareInfo2->shi2_netname;
        ulShareType   = pShareInfo2->shi2_type;
        pwszComment   = pShareInfo2->shi2_remark;
        pwszPath      = pShareInfo2->shi2_path;
        break;

    case 501:
        pShareInfo501 = pAddShareInfoParams->info.p501;
        pwszShareName = pShareInfo501->shi501_netname;
        ulShareType   = pShareInfo501->shi501_type;
        pwszComment   = pShareInfo501->shi501_remark;
        pwszPath      = NULL;
        break;

    case 502:
        pShareInfo502 = pAddShareInfoParams->info.p502;
        pwszShareName = pShareInfo502->shi502_netname;
        ulShareType   = pShareInfo502->shi502_type;
        pwszComment   = pShareInfo502->shi502_remark;
        pwszPath      = pShareInfo502->shi502_path;
        pSecDesc      = pShareInfo502->shi502_security_descriptor;
        ulSecDescLen  = pShareInfo502->shi502_reserved;
        break;

    default:
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
        break;

    }

    pShareList = &gSMBSrvGlobals.shareList;

    ntStatus = SrvShareMapFromWindowsPath(
                    pwszPath,
                    &pwszPathLocal);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareMapIdToServiceStringW(
                    ulShareType,
                    &pwszShareType);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareAdd(
                        pShareList,
                        pwszShareName,
                        pwszPathLocal,
                        pwszComment,
                        pSecDesc,
                        ulSecDescLen,
                        pwszShareType);

error:

    if (pAddShareInfoParams) {
        SrvFreeMemory(pAddShareInfoParams);
    }

    if (pwszPathLocal)
    {
        SrvFreeMemory(pwszPathLocal);
    }

    if (pwszShareType)
    {
        SrvFreeMemory(pwszShareType);
    }

    return ntStatus;
}


NTSTATUS
SrvShareDevCtlDelete(
    IN     PBYTE lpInBuffer,
    IN     ULONG ulInBufferSize,
    IN OUT PBYTE lpOutBuffer,
    IN     ULONG ulOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList = NULL;
    PSHARE_INFO_DELETE_PARAMS pDeleteShareInfoParams = NULL;
    PWSTR pwszShareName = NULL;

    ntStatus = LwShareInfoUnmarshalDeleteParameters(
                        lpInBuffer,
                        ulInBufferSize,
                        &pDeleteShareInfoParams
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    pShareList    = &gSMBSrvGlobals.shareList;
    pwszShareName = pDeleteShareInfoParams->netname;

    ntStatus = SrvShareDelete(pShareList, pwszShareName);

error:

    if (pDeleteShareInfoParams) {
        SrvFreeMemory(pDeleteShareInfoParams);
    }

    return ntStatus;
}


NTSTATUS
SrvShareDevCtlEnum(
    IN     PBYTE  lpInBuffer,
    IN     ULONG  ulInBufferSize,
    IN OUT PBYTE  lpOutBuffer,
    IN     ULONG  ulOutBufferSize,
    IN OUT PULONG pulBytesTransferred
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulLevel = 0;
    ULONG ulNumEntries = 0;
    ULONG i = 0;
    PBYTE pBuffer = NULL;
    ULONG ulBufferSize = 0;
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList = NULL;
    PSHARE_INFO_ENUM_PARAMS pEnumShareInfoParamsIn = NULL;
    SHARE_INFO_ENUM_PARAMS EnumShareInfoParamsOut;
    PSRV_SHARE_INFO* ppShares = NULL;
    PSHARE_INFO_0 p0 = NULL;
    PSHARE_INFO_1 p1 = NULL;
    PSHARE_INFO_2 p2 = NULL;
    PSHARE_INFO_501 p501 = NULL;
    PSHARE_INFO_502 p502 = NULL;

    memset(&EnumShareInfoParamsOut, 0, sizeof(EnumShareInfoParamsOut));

    ntStatus = LwShareInfoUnmarshalEnumParameters(
                        lpInBuffer,
                        ulInBufferSize,
                        &pEnumShareInfoParamsIn
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    pShareList = &gSMBSrvGlobals.shareList;
    ulLevel    = pEnumShareInfoParamsIn->dwInfoLevel;

    ntStatus = SrvShareEnum(
                        pShareList,
                        &ppShares,
                        &ulNumEntries);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (ulLevel)
    {
        case 0:

            ntStatus = SrvAllocateMemory(
                            sizeof(*p0) * ulNumEntries,
                            (PVOID*)&p0);
            BAIL_ON_NT_STATUS(ntStatus);

            for (i = 0; i < ulNumEntries; i++)
            {
                PSRV_SHARE_INFO pShareInfo = ppShares[i];

                p0[i].shi0_netname             = pShareInfo->pwszName;
            }

            EnumShareInfoParamsOut.info.p0 = p0;

            break;

        case 1:

            ntStatus = SrvAllocateMemory(
                            sizeof(*p1) * ulNumEntries,
                            (PVOID*)&p1);
            BAIL_ON_NT_STATUS(ntStatus);

            for (i = 0; i < ulNumEntries; i++)
            {
                PSRV_SHARE_INFO pShareInfo = ppShares[i];

                p1[i].shi1_netname             = pShareInfo->pwszName;
                p1[i].shi1_type                = pShareInfo->service;
                p1[i].shi1_remark              = pShareInfo->pwszComment;
            }

            EnumShareInfoParamsOut.info.p1 = p1;

            break;

        case 2:

            ntStatus = SrvAllocateMemory(
                            sizeof(*p2) * ulNumEntries,
                            (PVOID*)&p2);
            BAIL_ON_NT_STATUS(ntStatus);

            for (i = 0; i < ulNumEntries; i++)
            {
                PSRV_SHARE_INFO pShareInfo = ppShares[i];

                p2[i].shi2_netname             = pShareInfo->pwszName;
                p2[i].shi2_type                = pShareInfo->service;
                p2[i].shi2_remark              = pShareInfo->pwszComment;
                p2[i].shi2_permissions         = 0;
                p2[i].shi2_max_uses            = (UINT32)-1;
                p2[i].shi2_current_uses        = 0;

                ntStatus = SrvShareMapToWindowsPath(
                                pShareInfo->pwszPath,
                                &p2[i].shi2_path);
                BAIL_ON_NT_STATUS(ntStatus);

                p2[i].shi2_password            = NULL;
            }

            EnumShareInfoParamsOut.info.p2 = p2;

            break;

        case 501:

            ntStatus = SrvAllocateMemory(
                            sizeof(*p501) * ulNumEntries,
                            (PVOID*)&p501);
            BAIL_ON_NT_STATUS(ntStatus);

            for (i = 0; i < ulNumEntries; i++)
            {
                PSRV_SHARE_INFO pShareInfo = ppShares[i];

                p501[i].shi501_netname         = pShareInfo->pwszName;
                p501[i].shi501_type            = pShareInfo->service;
                p501[i].shi501_remark          = pShareInfo->pwszComment;
                p501[i].shi501_flags           = 0;
            }

            EnumShareInfoParamsOut.info.p501 = p501;

            break;

        case 502:

            ntStatus = SrvAllocateMemory(
                            sizeof(*p502) * ulNumEntries,
                            (PVOID*)&p502);
            BAIL_ON_NT_STATUS(ntStatus);

            for (i = 0; i < ulNumEntries; i++)
            {
                PSRV_SHARE_INFO pShareInfo = ppShares[i];

                p502[i].shi502_netname             = pShareInfo->pwszName;
                p502[i].shi502_type                = pShareInfo->service;
                p502[i].shi502_remark              = pShareInfo->pwszComment;
                p502[i].shi502_permissions         = 0;
                p502[i].shi502_max_uses            = (UINT32)-1;
                p502[i].shi502_current_uses        = 0;

                ntStatus = SrvShareMapToWindowsPath(
                                pShareInfo->pwszPath,
                                &p502[i].shi502_path);
                BAIL_ON_NT_STATUS(ntStatus);

                p502[i].shi502_password            = NULL;
                p502[i].shi502_reserved            = pShareInfo->ulSecDescLen;
                p502[i].shi502_security_descriptor = (PBYTE) pShareInfo->pSecDesc;
            }

            EnumShareInfoParamsOut.info.p502 = p502;

            break;

        default:

            ntStatus = STATUS_INVALID_LEVEL;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    EnumShareInfoParamsOut.dwInfoLevel  = ulLevel;
    EnumShareInfoParamsOut.dwNumEntries = ulNumEntries;

    ntStatus = LwShareInfoMarshalEnumParameters(
                        &EnumShareInfoParamsOut,
                        &pBuffer,
                        &ulBufferSize);
    BAIL_ON_NT_STATUS(ntStatus);

    if (ulBufferSize <= ulOutBufferSize)
    {
        memcpy((void*)lpOutBuffer, (void*)pBuffer, ulBufferSize);
    }
    else
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pulBytesTransferred = ulBufferSize;

cleanup:

    if (ppShares)
    {
        for (i = 0; i < ulNumEntries; i++)
        {
            PSRV_SHARE_INFO pShareInfo = ppShares[i];

            if (pShareInfo)
            {
                SrvShareReleaseInfo(pShareInfo);
            }
        }

        LwIoFreeMemory(ppShares);
    }

    if (p0)
    {
        SrvFreeMemory(p0);
    }
    if (p1)
    {
        SrvFreeMemory(p1);
    }
    if (p2)
    {
        for (i = 0; i < ulNumEntries; i++)
        {
            if (p2[i].shi2_path)
            {
                SrvFreeMemory(p2[i].shi2_path);
            }
        }
        SrvFreeMemory(p2);
    }
    if (p501)
    {
        SrvFreeMemory(p501);
    }
    if (p502)
    {
        for (i = 0; i < ulNumEntries; i++)
        {
            if (p502[i].shi502_path)
            {
                SrvFreeMemory(p502[i].shi502_path);
            }
        }

        SrvFreeMemory(p502);
    }
    if (pBuffer)
    {
        SrvFreeMemory(pBuffer);
    }
    if (pEnumShareInfoParamsIn)
    {
        SrvFreeMemory(pEnumShareInfoParamsIn);
    }

    return ntStatus;

error:

    memset((void*)lpOutBuffer, 0, ulOutBufferSize);
    *pulBytesTransferred = 0;

    goto cleanup;
}


NTSTATUS
SrvShareDevCtlGetInfo(
    IN     PBYTE  lpInBuffer,
    IN     ULONG  ulInBufferSize,
    IN OUT PBYTE  lpOutBuffer,
    IN     ULONG  ulOutBufferSize,
    IN OUT PULONG pulBytesTransferred
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulLevel = 0;
    PBYTE pBuffer = NULL;
    ULONG ulBufferSize = 0;
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList = NULL;
    PSHARE_INFO_GETINFO_PARAMS pGetShareInfoParamsIn = NULL;
    SHARE_INFO_GETINFO_PARAMS GetShareInfoParamsOut;
    PWSTR pwszShareName = NULL;
    PSRV_SHARE_INFO pShareInfo = NULL;
    BOOLEAN         bInLock = FALSE;
    PSHARE_INFO_0 p0 = NULL;
    PSHARE_INFO_1 p1 = NULL;
    PSHARE_INFO_2 p2 = NULL;
    PSHARE_INFO_501 p501 = NULL;
    PSHARE_INFO_502 p502 = NULL;

    memset(&GetShareInfoParamsOut, 0, sizeof(GetShareInfoParamsOut));

    ntStatus = LwShareInfoUnmarshalGetParameters(
                        lpInBuffer,
                        ulInBufferSize,
                        &pGetShareInfoParamsIn
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    pShareList    = &gSMBSrvGlobals.shareList;
    pwszShareName = pGetShareInfoParamsIn->pwszNetname;
    ulLevel       = pGetShareInfoParamsIn->dwInfoLevel;

    ntStatus = SrvShareFindByName(
                        pShareList,
                        pwszShareName,
                        &pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareInfo->mutex);

    switch (ulLevel)
    {
        case 0:

            ntStatus = SrvAllocateMemory(
                            sizeof(*p0),
                            (PVOID*)&p0);
            BAIL_ON_NT_STATUS(ntStatus);

            p0->shi0_netname             = pShareInfo->pwszName;

            GetShareInfoParamsOut.Info.p0 = p0;

            break;

        case 1:

            ntStatus = SrvAllocateMemory(
                        sizeof(*p1),
                        (PVOID*)&p1);
            BAIL_ON_NT_STATUS(ntStatus);

            p1->shi1_netname             = pShareInfo->pwszName;
            p1->shi1_type                = pShareInfo->service;
            p1->shi1_remark              = pShareInfo->pwszComment;

            GetShareInfoParamsOut.Info.p1 = p1;

            break;

        case 2:

            ntStatus = SrvAllocateMemory(
                            sizeof(*p2),
                            (PVOID*)&p2);
            BAIL_ON_NT_STATUS(ntStatus);

            p2->shi2_netname             = pShareInfo->pwszName;
            p2->shi2_type                = pShareInfo->service;
            p2->shi2_remark              = pShareInfo->pwszComment;
            p2->shi2_permissions         = 0;
            p2->shi2_max_uses            = (UINT32)-1;
            p2->shi2_current_uses        = 0;

            ntStatus = SrvShareMapToWindowsPath(
                            pShareInfo->pwszPath,
                            &p2->shi2_path);
            BAIL_ON_NT_STATUS(ntStatus);

            p2->shi2_password            = NULL;

            GetShareInfoParamsOut.Info.p2 = p2;

            break;

        case 501:

            ntStatus = SrvAllocateMemory(
                            sizeof(*p501),
                            (PVOID*)&p501);
            BAIL_ON_NT_STATUS(ntStatus);

            p501->shi501_netname         = pShareInfo->pwszName;
            p501->shi501_type            = pShareInfo->service;
            p501->shi501_remark          = pShareInfo->pwszComment;
            p501->shi501_flags           = 0;

            GetShareInfoParamsOut.Info.p501 = p501;

            break;

        case 502:

            ntStatus = SrvAllocateMemory(
                            sizeof(*p502),
                            (PVOID*)&p502);
            BAIL_ON_NT_STATUS(ntStatus);

            p502->shi502_netname             = pShareInfo->pwszName;
            p502->shi502_type                = pShareInfo->service;
            p502->shi502_remark              = pShareInfo->pwszComment;
            p502->shi502_permissions         = 0;
            p502->shi502_max_uses            = (UINT32)-1;
            p502->shi502_current_uses        = 0;

            ntStatus = SrvShareMapToWindowsPath(
                            pShareInfo->pwszPath,
                            &p502->shi502_path);
            BAIL_ON_NT_STATUS(ntStatus);

            p502->shi502_password            = NULL;
            p502->shi502_reserved            = pShareInfo->ulSecDescLen;
            p502->shi502_security_descriptor = (PBYTE)pShareInfo->pSecDesc;

            GetShareInfoParamsOut.Info.p502 = p502;

            break;

        default:

            ntStatus = STATUS_INVALID_LEVEL;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    GetShareInfoParamsOut.dwInfoLevel = ulLevel;

    ntStatus = LwShareInfoMarshalGetParameters(
                        &GetShareInfoParamsOut,
                        &pBuffer,
                        &ulBufferSize);
    BAIL_ON_NT_STATUS(ntStatus);

    if (ulBufferSize <= ulOutBufferSize)
    {
        memcpy((void*)lpOutBuffer, (void*)pBuffer, ulBufferSize);
    }
    else
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pulBytesTransferred = ulBufferSize;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    if (pShareInfo) {
        SrvShareReleaseInfo(pShareInfo);
    }

    if (pGetShareInfoParamsIn) {
        SrvFreeMemory(pGetShareInfoParamsIn);
    }

    if (p0)
    {
        SrvFreeMemory(p0);
    }
    if (p1)
    {
        SrvFreeMemory(p1);
    }
    if (p2)
    {
        if (p2->shi2_path)
        {
            SrvFreeMemory(p2->shi2_path);
        }
        SrvFreeMemory(p2);
    }
    if (p501)
    {
        SrvFreeMemory(p501);
    }
    if (p502)
    {
        if (p502->shi502_path)
        {
            SrvFreeMemory(p502->shi502_path);
        }

        SrvFreeMemory(p502);
    }
    if (pBuffer)
    {
        SrvFreeMemory(pBuffer);
    }

    return ntStatus;

error:

    memset((void*)lpOutBuffer, 0, ulOutBufferSize);
    *pulBytesTransferred = 0;

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
SrvValidateShareNameChange(
    PSRV_SHARE_INFO pShareInfo,
    PWSTR pwszNewName
    );

static
NTSTATUS
SrvShareUpdateInfo0(
    PSRV_SHARE_INFO pShareInfo,
    PSHARE_INFO_0 pInfo0
    );

static
NTSTATUS
SrvShareUpdateInfo1(
    PSRV_SHARE_INFO pShareInfo,
    PSHARE_INFO_1 pInfo1
    );

static
NTSTATUS
SrvShareUpdateInfo2(
    PSRV_SHARE_INFO pShareInfo,
    PSHARE_INFO_2 pInfo2
    );

static
NTSTATUS
SrvShareUpdateInfo501(
    PSRV_SHARE_INFO pShareInfo,
    PSHARE_INFO_501 pInfo501
    );

static
NTSTATUS
SrvShareUpdateInfo502(
    PSRV_SHARE_INFO pShareInfo,
    PSHARE_INFO_502 pInfo502
    );

NTSTATUS
SrvShareDevCtlSetInfo(
    IN     PBYTE lpInBuffer,
    IN     ULONG ulInBufferSize,
    IN OUT PBYTE lpOutBuffer,
    IN     ULONG ulOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList = &gSMBSrvGlobals.shareList;
    PSHARE_INFO_SETINFO_PARAMS pSetShareInfoParamsIn = NULL;
    PSRV_SHARE_INFO pShareInfo = NULL;
    BOOLEAN bShareLocked = FALSE;

    ntStatus = LwShareInfoUnmarshalSetParameters(
                   lpInBuffer,
                   ulInBufferSize,
                   &pSetShareInfoParamsIn);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareFindByName(
                   pShareList,
                   pSetShareInfoParamsIn->pwszNetname,
                   &pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bShareLocked, &pShareInfo->mutex);

    switch (pSetShareInfoParamsIn->dwInfoLevel)
    {
    case 0:
        ntStatus = SrvShareUpdateInfo0(
                       pShareInfo,
                       pSetShareInfoParamsIn->Info.p0);
        break;

    case 1:
        ntStatus = SrvShareUpdateInfo1(
                       pShareInfo,
                       pSetShareInfoParamsIn->Info.p1);
        break;

    case 2:
        ntStatus = SrvShareUpdateInfo2(
                       pShareInfo,
                       pSetShareInfoParamsIn->Info.p2);
        break;

    case 501:
        ntStatus = SrvShareUpdateInfo501(
                       pShareInfo,
                       pSetShareInfoParamsIn->Info.p501);
        break;

    case 502:
        ntStatus = SrvShareUpdateInfo502(
                       pShareInfo,
                       pSetShareInfoParamsIn->Info.p502);
        break;

    default:
        ntStatus = STATUS_INVALID_LEVEL;
        break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bShareLocked, &pShareInfo->mutex);

    ntStatus = SrvShareUpdate(pShareList, pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    LWIO_UNLOCK_RWMUTEX(bShareLocked, &pShareInfo->mutex);

    if (pSetShareInfoParamsIn) {
        SrvFreeMemory(pSetShareInfoParamsIn);
    }

    if (pShareInfo) {
        SrvShareReleaseInfo(pShareInfo);
    }

    return ntStatus;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
SrvValidateShareNameChange(
    PSRV_SHARE_INFO pShareInfo,
    PWSTR pwszNewName
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SHARE_INFO pNewShareInfo = NULL;

    if (pwszNewName == NULL)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* This needs better locking to ensure that there is no race
       window between the new name check and the update */

    ntStatus = SrvShareFindByName(
                   &gSMBSrvGlobals.shareList,
                   pwszNewName,
                   &pNewShareInfo);
    if ((ntStatus == STATUS_SUCCESS) && (pShareInfo != pNewShareInfo))
    {
        ntStatus = STATUS_DUPLICATE_NAME;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    ntStatus = STATUS_SUCCESS;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
SrvShareUpdateInfo0(
    PSRV_SHARE_INFO pShareInfo,
    PSHARE_INFO_0 pInfo0
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_SHARE_INFO NewShareInfo = *pShareInfo;

    /* Sanity Check */

    ntStatus = SrvValidateShareNameChange(pShareInfo, pInfo0->shi0_netname);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Setup new share info */

    ntStatus = SrvAllocateStringW(pInfo0->shi0_netname, &NewShareInfo.pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Struct copy to original share */

    SrvFreeMemory(pShareInfo->pwszName);
    pShareInfo->pwszName = NULL;

    *pShareInfo = NewShareInfo;

    ntStatus = STATUS_SUCCESS;

cleanup:

    return ntStatus;

error:

    if (NewShareInfo.pwszName)
    {
        SrvFreeMemory(NewShareInfo.pwszName);
    }

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
SrvShareUpdateInfo1(
    PSRV_SHARE_INFO pShareInfo,
    PSHARE_INFO_1 pInfo1
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_SHARE_INFO NewShareInfo = *pShareInfo;

    /* Sanity Check */

    if (pShareInfo->service != pInfo1->shi1_type)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvValidateShareNameChange(pShareInfo, pInfo1->shi1_netname);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Setup New Share */

    ntStatus = SrvAllocateStringW(pInfo1->shi1_netname, &NewShareInfo.pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pInfo1->shi1_remark)
    {
        ntStatus = SrvAllocateStringW(pInfo1->shi1_remark, &NewShareInfo.pwszComment);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Copy to the original share using struct copy */

    SrvFreeMemory(pShareInfo->pwszName);
    pShareInfo->pwszName = NULL;

    if (pShareInfo->pwszComment)
    {
        SrvFreeMemory(pShareInfo->pwszComment);
        pShareInfo->pwszComment = NULL;
    }

    *pShareInfo = NewShareInfo;

    ntStatus = STATUS_SUCCESS;

cleanup:

    return ntStatus;

error:

    if (NewShareInfo.pwszName)
    {
        SrvFreeMemory(NewShareInfo.pwszName);
    }

    if (NewShareInfo.pwszComment)
    {
        SrvFreeMemory(NewShareInfo.pwszComment);
    }

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
SrvShareUpdateInfo2(
    PSRV_SHARE_INFO pShareInfo,
    PSHARE_INFO_2 pInfo2
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_SHARE_INFO NewShareInfo = *pShareInfo;

    /* Sanity Check */

    if (pShareInfo->service != pInfo2->shi2_type)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if ((pInfo2->shi2_path == NULL) ||
        (LwRtlWC16StringNumChars(pInfo2->shi2_path) == 0))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvValidateShareNameChange(pShareInfo, pInfo2->shi2_netname);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Setup New Share */

    ntStatus = SrvAllocateStringW(pInfo2->shi2_netname, &NewShareInfo.pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareMapFromWindowsPath(pInfo2->shi2_path, &NewShareInfo.pwszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pInfo2->shi2_remark)
    {
        ntStatus = SrvAllocateStringW(pInfo2->shi2_remark, &NewShareInfo.pwszComment);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Copy to the original share using struct copy */

    SrvFreeMemory(pShareInfo->pwszName);
    pShareInfo->pwszName = NULL;

    SrvFreeMemory(pShareInfo->pwszPath);
    pShareInfo->pwszPath = NULL;

    if (pShareInfo->pwszComment)
    {
        SrvFreeMemory(pShareInfo->pwszComment);
        pShareInfo->pwszComment = NULL;
    }

    *pShareInfo = NewShareInfo;

    ntStatus = STATUS_SUCCESS;

cleanup:

    return ntStatus;

error:

    if (NewShareInfo.pwszName)
    {
        SrvFreeMemory(NewShareInfo.pwszName);
    }

    if (NewShareInfo.pwszPath)
    {
        SrvFreeMemory(NewShareInfo.pwszPath);
    }

    if (NewShareInfo.pwszComment)
    {
        SrvFreeMemory(NewShareInfo.pwszComment);
    }

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
SrvShareUpdateInfo501(
    PSRV_SHARE_INFO pShareInfo,
    PSHARE_INFO_501 pInfo501
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_SHARE_INFO NewShareInfo = *pShareInfo;

    /* Sanity Check */

    if (pShareInfo->service != pInfo501->shi501_type)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvValidateShareNameChange(pShareInfo, pInfo501->shi501_netname);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Setup New Share */

    ntStatus = SrvAllocateStringW(pInfo501->shi501_netname, &NewShareInfo.pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pInfo501->shi501_remark)
    {
        ntStatus = SrvAllocateStringW(pInfo501->shi501_remark, &NewShareInfo.pwszComment);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Copy to the original share using struct copy */

    SrvFreeMemory(pShareInfo->pwszName);
    pShareInfo->pwszName = NULL;

    if (pShareInfo->pwszComment)
    {
        SrvFreeMemory(pShareInfo->pwszComment);
        pShareInfo->pwszComment = NULL;
    }

    *pShareInfo = NewShareInfo;

    ntStatus = STATUS_SUCCESS;

cleanup:

    return ntStatus;

error:

    if (NewShareInfo.pwszName)
    {
        SrvFreeMemory(NewShareInfo.pwszName);
    }

    if (NewShareInfo.pwszComment)
    {
        SrvFreeMemory(NewShareInfo.pwszComment);
    }

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
SrvShareUpdateInfo502(
    PSRV_SHARE_INFO pShareInfo,
    PSHARE_INFO_502 pInfo502
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_SHARE_INFO NewShareInfo = *pShareInfo;

    /* Sanity Check */

    if (pShareInfo->service != pInfo502->shi502_type)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if ((pInfo502->shi502_path == NULL) ||
        (LwRtlWC16StringNumChars(pInfo502->shi502_path) == 0))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvValidateShareNameChange(pShareInfo, pInfo502->shi502_netname);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Setup New Share */

    ntStatus = SrvAllocateStringW(pInfo502->shi502_netname, &NewShareInfo.pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareMapFromWindowsPath(pInfo502->shi502_path, &NewShareInfo.pwszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pInfo502->shi502_remark)
    {
        ntStatus = SrvAllocateStringW(pInfo502->shi502_remark, &NewShareInfo.pwszComment);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pInfo502->shi502_security_descriptor)
    {
        /* NULL the SDs since we don't own them to prevent side effecting
           the pShareInfo */

        NewShareInfo.pSecDesc = NULL;
        NewShareInfo.ulSecDescLen = 0;
        NewShareInfo.pAbsSecDesc = NULL;

        ntStatus = SrvShareSetSecurity(
                       &NewShareInfo,
                       (PSECURITY_DESCRIPTOR_RELATIVE)pInfo502->shi502_security_descriptor,
                       pInfo502->shi502_reserved);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Copy to the original share using struct copy */

    SrvFreeMemory(pShareInfo->pwszName);
    pShareInfo->pwszName = NULL;

    SrvFreeMemory(pShareInfo->pwszPath);
    pShareInfo->pwszPath = NULL;

    if (pShareInfo->pwszComment)
    {
        SrvFreeMemory(pShareInfo->pwszComment);
        pShareInfo->pwszComment = NULL;
    }

    SrvShareFreeSecurity(pShareInfo);

    *pShareInfo = NewShareInfo;

    ntStatus = STATUS_SUCCESS;

cleanup:

    return ntStatus;

error:

    if (NewShareInfo.pwszName)
    {
        SrvFreeMemory(NewShareInfo.pwszName);
    }

    if (NewShareInfo.pwszPath)
    {
        SrvFreeMemory(NewShareInfo.pwszPath);
    }

    if (NewShareInfo.pwszComment)
    {
        SrvFreeMemory(NewShareInfo.pwszComment);
    }

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
