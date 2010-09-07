/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise Distributed File System (DFS)
 *
 *        Registry Configuration
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "dfs.h"

static
NTSTATUS
DfsConfigInitializeRoot(
    HANDLE hRegistryServer,
    HKEY hParentKey,
    PWSTR pwszRootName
    );

static
NTSTATUS
DfsConfigRootAddReferrals(
    HANDLE hRegistryServer,
    HKEY hParentKey,
    PDFS_ROOT_CONTROL_BLOCK pRootCB
    );

/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsConfigReadStandaloneRoots(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HANDLE hRegConnection = (HANDLE)NULL;
    HKEY hStandaloneKey = (HKEY)NULL;
    PWSTR pwszStandaloneRootKeyPath = NULL;
    DWORD dwIndex = 0;
    DWORD dwReserved = 0;
    DWORD dwDfsRootNameSize = MAX_KEY_LENGTH;
    WCHAR pwszDfsRootName[MAX_KEY_LENGTH] = {0};
    BOOLEAN bFinished = FALSE;

    ntStatus = LwRtlWC16StringAllocateFromCString(
                   &pwszStandaloneRootKeyPath,
                   DFS_CONF_ROOT_STANDALONE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenServer(&hRegConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenKeyExW(
                   hRegConnection,
                   NULL,
                   pwszStandaloneRootKeyPath,
                   0,
                   KEY_READ,
                   &hStandaloneKey);
    BAIL_ON_NT_STATUS(ntStatus);

    for (dwIndex = 0; !bFinished; dwIndex++)
    {
        ntStatus = NtRegEnumKeyExW(
                       hRegConnection,
                       hStandaloneKey,
                       dwIndex,
                       pwszDfsRootName,
                       &dwDfsRootNameSize,
                       &dwReserved,
                       NULL,
                       NULL,
                       NULL);
        if (ntStatus == STATUS_NO_MORE_ENTRIES)
        {
            ntStatus = STATUS_SUCCESS;
            bFinished = TRUE;
            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = DfsConfigInitializeRoot(
                       hRegConnection,
                       hStandaloneKey,
                       pwszDfsRootName);
        BAIL_ON_NT_STATUS(ntStatus);
    }


cleanup:
    if (pwszStandaloneRootKeyPath)
    {
        LwRtlWC16StringFree(&pwszStandaloneRootKeyPath);
    }

    if (hStandaloneKey)
    {
        NtRegCloseKey(hRegConnection, hStandaloneKey);
    }

    if (hRegConnection)
    {
        NtRegCloseServer(hRegConnection);
    }

    return ntStatus;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
DfsConfigInitializeRoot(
    HANDLE hRegistryServer,
    HKEY hParentKey,
    PWSTR pwszDfsRootName
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDFS_ROOT_CONTROL_BLOCK pRootCB = NULL;

    ntStatus = DfsAllocateRootCB(&pRootCB, pwszDfsRootName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = DfsConfigRootAddReferrals(
                   hRegistryServer,
                   hParentKey,
                   pRootCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = DfsRootCtrlBlockTableAdd(
                   &gRootCtrlBlockTable,
                   pRootCB);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    if (pRootCB)
    {
        DfsReleaseRootCB(&pRootCB);
    }

    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

static
NTSTATUS
DfsConfigRootAddReferrals(
    HANDLE hRegistryServer,
    HKEY hParentKey,
    PDFS_ROOT_CONTROL_BLOCK pRootCB
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HKEY hDfsRootKey = (HKEY)NULL;
    PDFS_REFERRAL_CONTROL_BLOCK pReferralCB = NULL;
    PWSTR *ppwszTargets = NULL;
    DWORD i = 0;
    DWORD dwIndex = 0;
    BOOLEAN bFinished = FALSE;
    DWORD dwValueNameLength = MAX_VALUE_LENGTH;
    WCHAR pwszValueName[MAX_VALUE_LENGTH] = { 0 };
    DWORD dwType = REG_NONE;
    DWORD dwReserved = 0;
    BYTE pBuffer[4096] = { 0 };
    DWORD dwBufferSize = 0;

    ntStatus = NtRegOpenKeyExW(
                   hRegistryServer,
                   hParentKey,
                   pRootCB->pwszRootName,
                   0,
                   KEY_READ,
                   &hDfsRootKey);
    BAIL_ON_NT_STATUS(ntStatus);

    for (dwIndex=0; !bFinished; dwIndex++)
    {
        dwBufferSize = sizeof(pBuffer);
        LwRtlZeroMemory(pBuffer, dwBufferSize);

        ntStatus = NtRegEnumValueW(
                       hRegistryServer,
                       hDfsRootKey,
                       dwIndex,
                       pwszValueName,
                       &dwValueNameLength,
                       &dwReserved,
                       &dwType,
                       pBuffer,
                       &dwBufferSize);
        if (ntStatus == STATUS_NO_MORE_ENTRIES)
        {
            ntStatus = STATUS_SUCCESS;
            bFinished = TRUE;
            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (dwType != REG_MULTI_SZ)
        {
            continue;
        }

        ntStatus = NtRegByteArrayToMultiStrsW(
                       pBuffer,
                       dwBufferSize,
                       &ppwszTargets);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = DfsAllocateReferralCB(&pReferralCB, pwszValueName);
        BAIL_ON_NT_STATUS(ntStatus);

        for (i=0; ppwszTargets[i] != NULL; i++)
        {
            ntStatus = DfsReferralParseTarget(pReferralCB, ppwszTargets[i]);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        RegFreeMultiStrsW(ppwszTargets);
        ppwszTargets = NULL;
    }


cleanup:
    if (hDfsRootKey)
    {
        NtRegCloseKey(hRegistryServer, hDfsRootKey);
    }

    return ntStatus;

error:
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
