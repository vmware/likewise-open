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
 *        shareapi.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) Server subsystem
 *
 *        Server share list handling interface
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

static
NTSTATUS
SrvShareFindByName_inlock(
    IN  PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN  PWSTR                      pwszShareName,
    OUT PSRV_SHARE_INFO*           ppShareInfo
    );

static
NTSTATUS
SrvShareRemoveFromList_inlock(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName
    );

static
VOID
SrvShareFreeInfo(
    PSRV_SHARE_INFO pShareInfo
    );

NTSTATUS
SrvShareInitList(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList
    )
{
    NTSTATUS ntStatus = 0;
    HANDLE   hRepository = NULL;
    HANDLE   hResume = NULL;
    PSRV_SHARE_INFO* ppShareInfoList = NULL;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    ULONG            ulLimit  = 256;
    ULONG            ulNumSharesFound = 0;

    pthread_rwlock_init(&pShareList->mutex, NULL);
    pShareList->pMutex = &pShareList->mutex;

    pShareList->pShareEntry = NULL;

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryOpen(&hRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryBeginEnum(
                                            hRepository,
                                            ulLimit,
                                            &hResume);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        ULONG iShare = 0;

        if (ppShareInfoList)
        {
            SrvShareFreeInfoList(ppShareInfoList, ulNumSharesFound);
            ppShareInfoList = NULL;
            ulNumSharesFound = 0;
        }

        ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryEnum(
                                                hRepository,
                                                hResume,
                                                &ppShareInfoList,
                                                &ulNumSharesFound);
        BAIL_ON_NT_STATUS(ntStatus);

        for (; iShare < ulNumSharesFound; iShare++)
        {
            PSRV_SHARE_INFO pShareInfo = ppShareInfoList[iShare];

            ntStatus = SrvAllocateMemory(
                            sizeof(SRV_SHARE_ENTRY),
                            (PVOID*)&pShareEntry);
            BAIL_ON_NT_STATUS(ntStatus);

            pShareEntry->pInfo = pShareInfo;
            InterlockedIncrement(&pShareInfo->refcount);

            pShareEntry->pNext = pShareList->pShareEntry;
            pShareList->pShareEntry = pShareEntry;
            pShareEntry = NULL;
        }

    } while (ulNumSharesFound == ulLimit);

cleanup:

    if (hResume)
    {
        gSrvShareApi.pFnTable->pfnShareRepositoryEndEnum(
                                    hRepository,
                                    hResume);
    }

    if (hRepository)
    {
        gSrvShareApi.pFnTable->pfnShareRepositoryClose(hRepository);
    }

    if (ppShareInfoList)
    {
        SrvShareFreeInfoList(ppShareInfoList, ulNumSharesFound);
    }

    return ntStatus;

error:

    SrvShareFreeListContents(pShareList);

    goto cleanup;
}

NTSTATUS
SrvShareFindByName(
    IN  PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN  PWSTR                      pwszShareName,
    OUT PSRV_SHARE_INFO*           ppShareInfo
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareList->mutex);

    ntStatus = SrvShareFindByName_inlock(
                        pShareList,
                        pwszShareName,
                        ppShareInfo);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    return ntStatus;
}

static
NTSTATUS
SrvShareFindByName_inlock(
    IN  PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN  PWSTR                      pwszShareName,
    OUT PSRV_SHARE_INFO*           ppShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSRV_SHARE_INFO pShareInfo = NULL;

    pShareEntry = pShareList->pShareEntry;

    while (pShareEntry)
    {
        if (SMBWc16sCaseCmp(pwszShareName, pShareEntry->pInfo->pwszName) == 0)
        {
            pShareInfo = pShareEntry->pInfo;
            break;
        }

        pShareEntry = pShareEntry->pNext;
    }

    if (!pShareInfo)
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    InterlockedIncrement(&pShareInfo->refcount);

    *ppShareInfo = pShareInfo;

cleanup:

    return ntStatus;

error:

    *ppShareInfo = NULL;

    goto cleanup;
}


NTSTATUS
SrvShareAdd(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName,
    IN     PWSTR                      pwszSharePath,
    IN     PWSTR                      pwszShareComment,
    IN     PBYTE                      pSecDesc,
    IN     ULONG                      ulSecDescLen,
    IN     PWSTR                      pwszShareType
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSRV_SHARE_INFO pShareInfo = NULL;
    wchar16_t wszServiceType[] = LWIO_SRV_SHARE_STRING_ID_DISK_W;
    HANDLE hRepository = NULL;

    if (IsNullOrEmptyString(pwszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER_2;
    }
    if (IsNullOrEmptyString(pwszSharePath))
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pwszShareType)
    {
        pwszShareType = &wszServiceType[0];
    }

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pShareList->mutex);

    ntStatus = SrvShareFindByName_inlock(
                        pShareList,
                        pwszShareName,
                        &pShareInfo);
    if (!ntStatus)
    {
        ntStatus = STATUS_OBJECT_NAME_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_SHARE_INFO),
                    (PVOID*)&pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareInfo->refcount = 1;

    pthread_rwlock_init(&pShareInfo->mutex, NULL);
    pShareInfo->pMutex = &pShareInfo->mutex;

    ntStatus = SrvAllocateStringW(
                    pwszShareName,
                    &pShareInfo->pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateStringW(
                    pwszSharePath,
                    &pShareInfo->pwszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pwszShareComment)
    {
        ntStatus = SrvAllocateStringW(
                        pwszShareComment,
                        &pShareInfo->pwszComment);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        pShareInfo->pwszComment = NULL;
    }

    if (ulSecDescLen)
    {
        ntStatus = SrvAllocateMemory(
                        ulSecDescLen,
                        (PVOID*)&pShareInfo->pSecDesc);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pShareInfo->pSecDesc, pSecDesc, ulSecDescLen);

        pShareInfo->ulSecDescLen = ulSecDescLen;
    }
    else
    {
        pShareInfo->pSecDesc     = NULL;
        pShareInfo->ulSecDescLen = 0;
    }
    // pShareInfo->service     = ulShareType;

    pShareInfo->bMarkedForDeletion = FALSE;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_SHARE_ENTRY),
                    (PVOID*)&pShareEntry);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareEntry->pInfo = pShareInfo;
    InterlockedIncrement(&pShareInfo->refcount);

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryOpen(&hRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO: We only allow adding disk sharess
    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryAdd(
                                            hRepository,
                                            pShareInfo->pwszName,
                                            pShareInfo->pwszPath,
                                            pShareInfo->pwszComment,
                                            pShareInfo->pSecDesc,
                                            pShareInfo->ulSecDescLen,
                                            pwszShareType);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareEntry->pNext = pShareList->pShareEntry;
    pShareList->pShareEntry = pShareEntry;

cleanup:

    if (hRepository)
    {
        gSrvShareApi.pFnTable->pfnShareRepositoryClose(hRepository);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    if (pShareInfo)
    {
        SrvShareReleaseInfo(pShareInfo);
    }

    return ntStatus;

error:

    if (pShareEntry)
    {
        SrvShareFreeEntry(pShareEntry);
    }

    goto cleanup;
}

NTSTATUS
SrvShareUpdate(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName,
    IN     PSRV_SHARE_INFO            pShareInfo
    )
{
    NTSTATUS ntStatus = 0;

#if 0

    ntStatus = ValidateShareInfo(
                    dwLevel,
                    pShareInfo
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ENTER_WRITER_LOCK();

    ntStatus = SrvFindShareByName(
                    pszShareName,
                    &pShareEntry
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    switch (dwLevel) {

        case 0:
            break;

        case 1:
            break;

        case 2:
            break;

        case 501:
            break;

        case 502:
            break;

        case 503:
            break;

    }

    ntStatus =  SrvShareDbInsert(
                    pszShareName,
                    pszPathName,
                    pszComment,
                    dwFlags,
                    pSecurityDescriptor,
                    dwSDSize
                    );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    LEAVE_WRITER_LOCK();

#endif

    return(ntStatus);
}

NTSTATUS
SrvShareDelete(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    HANDLE   hRepository = NULL;

    if (IsNullOrEmptyString(pwszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pShareList->mutex);

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryOpen(&hRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryDelete(
                                            hRepository,
                                            pwszShareName);
    BAIL_ON_NT_STATUS(ntStatus);

    if (hRepository)
    {
        gSrvShareApi.pFnTable->pfnShareRepositoryClose(hRepository);
        hRepository = NULL;
    }

    ntStatus = SrvShareRemoveFromList_inlock(
                        pShareList,
                        pwszShareName);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (hRepository)
    {
        gSrvShareApi.pFnTable->pfnShareRepositoryClose(hRepository);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvShareRemoveFromList_inlock(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName
    )
{
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSRV_SHARE_ENTRY pPrevShareEntry = NULL;
    ULONG ulRemoved = 0;

    pShareEntry = pShareList->pShareEntry;

    while (pShareEntry)
    {
        if (SMBWc16sCaseCmp(pwszShareName, pShareEntry->pInfo->pwszName) == 0)
        {
            if (pPrevShareEntry)
            {
                pPrevShareEntry->pNext = pShareEntry->pNext;
            }
            else
            {
                pShareList->pShareEntry = pShareEntry->pNext;
            }

            pShareEntry->pNext = NULL;
            SrvShareFreeEntry(pShareEntry);

            ulRemoved++;

            break;
        }

        pPrevShareEntry = pShareEntry;
        pShareEntry     = pShareEntry->pNext;
    }

    return (ulRemoved ? STATUS_SUCCESS : STATUS_NOT_FOUND);
}

NTSTATUS
SrvShareEnum(
    IN      PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    OUT     PSRV_SHARE_INFO**          pppShareInfo,
    IN  OUT PULONG                     pulNumEntries
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulCount = 0;
    BOOLEAN bInLock = FALSE;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSRV_SHARE_INFO* ppShares = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareList->mutex);

    /* Count the number of share entries */
    pShareEntry = pShareList->pShareEntry;
    while (pShareEntry)
    {
        pShareEntry = pShareEntry->pNext;
        ulCount++;
    }

    if (ulCount)
    {
        ULONG i = 0;

        ntStatus = SrvAllocateMemory(
                        ulCount * sizeof(PSRV_SHARE_INFO),
                        (PVOID*)&ppShares);
        BAIL_ON_NT_STATUS(ntStatus);

        pShareEntry = pShareList->pShareEntry;
        for (; i < ulCount; i++)
        {
            InterlockedIncrement(&pShareEntry->pInfo->refcount);

            ppShares[i] = pShareEntry->pInfo;

            pShareEntry = pShareEntry->pNext;
        }
    }

    *pppShareInfo   = ppShares;
    *pulNumEntries  = ulCount;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    return ntStatus;

error:

    if (ppShares)
    {
        SrvShareFreeInfoList(ppShares, ulCount);
    }

    *pppShareInfo   = NULL;
    *pulNumEntries = 0;

    goto cleanup;
}

VOID
SrvShareFreeListContents(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList
    )
{
    if (pShareList->pShareEntry)
    {
        SrvShareFreeEntry(pShareList->pShareEntry);
        pShareList->pShareEntry = NULL;
    }

    if (pShareList->pMutex)
    {
        pthread_rwlock_destroy(&pShareList->mutex);
        pShareList->pMutex = NULL;
    }
}

VOID
SrvShareFreeEntry(
    IN PSRV_SHARE_ENTRY pShareEntry
    )
{
    while (pShareEntry)
    {
        PSRV_SHARE_ENTRY pTmpShareEntry = pShareEntry;

        pShareEntry = pShareEntry->pNext;

        if (pTmpShareEntry->pInfo)
        {
            SrvShareReleaseInfo(pTmpShareEntry->pInfo);
        }

        SrvFreeMemory(pTmpShareEntry);
    }
}

VOID
SrvShareFreeInfoList(
    PSRV_SHARE_INFO* ppInfoList,
    ULONG            ulNumInfos
    )
{
    ULONG iInfo = 0;

    for (; iInfo < ulNumInfos; iInfo++)
    {
        PSRV_SHARE_INFO pShareInfo = ppInfoList[iInfo];

        if (pShareInfo)
        {
            SrvShareReleaseInfo(pShareInfo);
        }
    }

    SrvFreeMemory(ppInfoList);
}

VOID
SrvShareReleaseInfo(
    IN PSRV_SHARE_INFO pShareInfo
    )
{
    if (InterlockedDecrement(&pShareInfo->refcount) == 0)
    {
        SrvShareFreeInfo(pShareInfo);
    }
}

static
VOID
SrvShareFreeInfo(
    IN PSRV_SHARE_INFO pShareInfo
    )
{
    if (pShareInfo->pMutex)
    {
        pthread_rwlock_destroy(&pShareInfo->mutex);
    }

    if (pShareInfo->pwszName)
    {
        SrvFreeMemory(pShareInfo->pwszName);
    }
    if (pShareInfo->pwszPath)
    {
        SrvFreeMemory(pShareInfo->pwszPath);
    }
    if (pShareInfo->pSecDesc)
    {
        SrvFreeMemory(pShareInfo->pSecDesc);
    }
    if (pShareInfo->pwszComment)
    {
        SrvFreeMemory(pShareInfo->pwszComment);
    }

    SrvFreeMemory(pShareInfo);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
