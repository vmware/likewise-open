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
SrvDevCtlAddShare(
    PBYTE lpInBuffer,
    ULONG ulInBufferSize,
    PBYTE lpOutBuffer,
    ULONG ulOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext = NULL;
    PSHARE_INFO_ADD_PARAMS pAddShareInfoParams = NULL;
    PSHARE_INFO_0 pShareInfo0 = NULL;
    PSHARE_INFO_1 pShareInfo1 = NULL;
    PSHARE_INFO_501 pShareInfo501 = NULL;
    PSHARE_INFO_502 pShareInfo502 = NULL;
    PWSTR pwszShareName = NULL;
    PWSTR pwszPath = NULL;
    PWSTR pwszComment = NULL;

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
	    pwszPath      = NULL;
        break;

    case 1:
	    pShareInfo1   = pAddShareInfoParams->info.p1;
	    pwszShareName = pShareInfo1->shi1_netname;
	    pwszPath      = NULL;
        break;

    case 501:
        pShareInfo501 = pAddShareInfoParams->info.p501;
	    pwszShareName = pShareInfo501->shi501_netname;
	    pwszPath      = NULL;
        break;

    case 502:
        pShareInfo502 = pAddShareInfoParams->info.p502;
	    pwszShareName = pShareInfo502->shi502_netname;
	    pwszPath      = pShareInfo502->shi502_path;
	    break;

    default:
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
        break;

    }

    pDbContext = &gSMBSrvGlobals.shareDBContext;

    ntStatus = SrvShareAddShare(
                        pDbContext,
                        pwszShareName,
                        pwszPath,
                        pwszComment
                        );

error:
    return ntStatus;
}


NTSTATUS
SrvDevCtlDeleteShare(
    PBYTE lpInBuffer,
    ULONG ulInBufferSize,
    PBYTE lpOutBuffer,
    ULONG ulOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext = NULL;
    PSHARE_INFO_DELETE_PARAMS pDeleteShareInfoParams = NULL;
    PWSTR pwszShareName = NULL;

    ntStatus = LwShareInfoUnmarshalDeleteParameters(
                        lpInBuffer,
                        ulInBufferSize,
                        &pDeleteShareInfoParams
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    pDbContext    = &gSMBSrvGlobals.shareDBContext;
    pwszShareName = pDeleteShareInfoParams->netname;

    ntStatus = SrvShareDeleteShare(
                        pDbContext,
                        pwszShareName
                        );

error:
    return ntStatus;
}


NTSTATUS
SrvDevCtlEnumShares(
    PBYTE pInBuffer,
    ULONG ulInBufferSize,
    PBYTE pOutBuffer,
    ULONG ulOutBufferSize,
    PULONG pulBytesTransferred
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulError = 0;
    ULONG ulLevel = 0;
    ULONG ulNumEntries = 0;
    ULONG i = 0;
    PBYTE pBuffer = NULL;
    ULONG pBufferSize = 0;
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext = NULL;
    PSHARE_INFO_ENUM_PARAMS pEnumShareInfoParamsIn = NULL;
    SHARE_INFO_ENUM_PARAMS EnumShareInfoParamsOut;
    PSHARE_DB_INFO pShares = NULL;
    PSHARE_INFO_1 p1 = NULL;
    PSHARE_INFO_2 p2 = NULL;
    PSHARE_INFO_502 p502 = NULL;

    ntStatus = LwShareInfoUnmarshalEnumParameters(
                        pInBuffer,
                        ulInBufferSize,
                        &pEnumShareInfoParamsIn
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    pDbContext = &gSMBSrvGlobals.shareDBContext;
    ulLevel    = pEnumShareInfoParamsIn->dwInfoLevel;

    ntStatus = SrvShareEnumShares(
                        pDbContext,
                        ulLevel,
                        &pShares,
                        &ulNumEntries
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    switch (ulLevel)
    {
    case 1:
        ulError = LW_RTL_ALLOCATE(
                      &p1,
                      SHARE_INFO_1,
                      sizeof(*p1) * ulNumEntries);
        ntStatus = LwUnixErrnoToNtStatus(ulError);
        BAIL_ON_NT_STATUS(ntStatus);

        for (i = 0; i < ulNumEntries; i++)
        {
            p1[i].shi1_netname             = pShares[i].pwszName;
            p1[i].shi1_type                = pShares[i].service;
            p1[i].shi1_remark              = pShares[i].pwszComment;
        }

        EnumShareInfoParamsOut.info.p1 = p1;
        break;

    case 2:
        ulError = LW_RTL_ALLOCATE(
                       &p2,
                       SHARE_INFO_2,
                       sizeof(*p2) * ulNumEntries);
        ntStatus = LwUnixErrnoToNtStatus(ulError);
        BAIL_ON_NT_STATUS(ntStatus);

        for (i = 0; i < ulNumEntries; i++)
        {
            p2[i].shi2_netname             = pShares[i].pwszName;
            p2[i].shi2_type                = pShares[i].service;
            p2[i].shi2_remark              = pShares[i].pwszComment;
            p2[i].shi2_permissions         = 0;
            p2[i].shi2_max_uses            = 0;
            p2[i].shi2_current_uses        = 0;
            p2[i].shi2_path                = pShares[i].pwszPath;
            p2[i].shi2_password            = NULL;
        }

        EnumShareInfoParamsOut.info.p1 = p1;
        break;

    case 502:
        ulError = LW_RTL_ALLOCATE(
                        &p502,
                        SHARE_INFO_502,
                        sizeof(*p502) * ulNumEntries);
        ntStatus = LwUnixErrnoToNtStatus(ulError);
        BAIL_ON_NT_STATUS(ntStatus);

        for (i = 0; i < ulNumEntries; i++)
        {
            p502[i].shi502_netname             = pShares[i].pwszName;
            p502[i].shi502_type                = pShares[i].service;
            p502[i].shi502_remark              = pShares[i].pwszComment;
            p502[i].shi502_permissions         = 0;
            p502[i].shi502_max_uses            = 0;
            p502[i].shi502_current_uses        = 0;
            p502[i].shi502_path                = pShares[i].pwszPath;
            p502[i].shi502_password            = NULL;
            p502[i].shi502_reserved            = 0;
            p502[i].shi502_security_descriptor = NULL;
        }

        EnumShareInfoParamsOut.info.p502 = p502;
        break;

    default:
        EnumShareInfoParamsOut.info.p0 = NULL;
    }

    EnumShareInfoParamsOut.dwInfoLevel  = ulLevel;
    EnumShareInfoParamsOut.dwNumEntries = ulNumEntries;

    ntStatus = LwShareInfoMarshalEnumParameters(
                        &EnumShareInfoParamsOut,
                        &pBuffer,
                        &pBufferSize
                        );

    if (pBufferSize <= ulOutBufferSize) {
        memcpy((void*)pOutBuffer, (void*)pBuffer, pBufferSize);

    } else {
        ntStatus = STATUS_MORE_ENTRIES;
        goto error;
    }

    *pulBytesTransferred = pBufferSize;

cleanup:

    if (pShares)
    {
        for (i = 0; i < ulNumEntries; i++)
        {
            if (pShares[i].pwszName)
            {
                LwRtlMemoryFree(pShares[i].pwszName);
            }
            if (pShares[i].pwszPath)
            {
                LwRtlMemoryFree(pShares[i].pwszPath);
            }
            if (pShares[i].pwszComment)
            {
                LwRtlMemoryFree(pShares[i].pwszComment);
            }
            if (pShares[i].pwszSID)
            {
                LwRtlMemoryFree (pShares[i].pwszSID);
            }
        }

        LwIoFreeMemory((void*)pShares);
    }

    if (p1)
    {
        LwRtlMemoryFree(p1);
    }
    if (p2)
    {
        LwRtlMemoryFree(p2);
    }
    if (p502)
    {
        LwRtlMemoryFree(p502);
    }
    if (pBuffer)
    {
        LwRtlMemoryFree(pBuffer);
    }
    if (pEnumShareInfoParamsIn)
    {
        LwRtlMemoryFree(pEnumShareInfoParamsIn);
    }

    return ntStatus;

error:
    memset((void*)pOutBuffer, 0, ulOutBufferSize);
    *pulBytesTransferred = 0;

    goto cleanup;
}


NTSTATUS
SrvDevCtlGetShareInfo(
    PBYTE lpInBuffer,
    ULONG ulInBufferSize,
    PBYTE lpOutBuffer,
    ULONG ulOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;
    ULONG Level = 0;

    switch (Level)
    {
        case 1:
            break;

        case 2:
            break;

        case 502:
            break;

        case 503:
            break;

        default:

            ntStatus = STATUS_INVALID_PARAMETER;

            break;

    }

    return ntStatus;
}

NTSTATUS
SrvDevCtlSetShareInfo(
    PBYTE lpInBuffer,
    ULONG ulInBufferSize,
    PBYTE lpOutBuffer,
    ULONG ulOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;
    ULONG Level = 0;

    switch(Level)
    {
        case 1:
            break;

        case 2:
            break;

        case 502:
            break;

        case 503:
            break;

        default:

            ntStatus = STATUS_INVALID_PARAMETER;

            break;

    }

    return ntStatus;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
