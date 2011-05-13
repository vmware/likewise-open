/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
int
SrvShareCompareInfo(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SrvShareReleaseInfoHandle(
    PVOID hShareInfo
    );

static
VOID
SrvShareFreeInfo(
    PSRV_SHARE_INFO pShareInfo
    );

static
NTSTATUS
SrvShareCollectionDuplicate(
    IN PLWRTL_RB_TREE pShareCollection,
    OUT PLWRTL_RB_TREE* ppShareCollectionCopy
    );

static
NTSTATUS
SrvShareCollectionDuplicateVisit(
    IN PVOID pKey,
    IN PVOID pData,
    IN PVOID pUserData,
    OUT PBOOLEAN pContinue
    );

static
NTSTATUS
SrvShareReloadConfigurationVisitDeleteShare_inlock(
    IN PVOID pKey,
    IN PVOID hShareInfo,
    IN PVOID hShareList,
    OUT PBOOLEAN pContinue
    );

static
NTSTATUS
SrvShareAddShareInfoInMemory_inlock(
    IN PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN PSRV_SHARE_INFO pShareInfo
    );

static
NTSTATUS
SrvShareDeleteShareInfoInMemory_inlock(
    IN PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN PSRV_SHARE_INFO pShareInfo,
    IN BOOLEAN DisconnectClients
    );

static
NTSTATUS
SrvShareUpdateShareInfoInMemory_inlock(
    IN OUT PSRV_SHARE_INFO pShareInfoDst,
    IN PSRV_SHARE_INFO pShareInfoSrc
    );

static
NTSTATUS
SrvShareDuplicateInfo_inlock(
    IN PSRV_SHARE_INFO pShareInfo,
    OUT PSRV_SHARE_INFO* ppShareInfo
    );

static
NTSTATUS
SrvShareDisconnectClients_inlock(
    IN PSRV_SHARE_INFO pShareInfo
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
    ULONG            ulBatchLimit  = 256;
    ULONG            ulNumSharesFound = 0;

    pthread_rwlock_init(&pShareList->mutex, NULL);
    pShareList->pMutex = &pShareList->mutex;

    pShareList->pShareEntry = NULL;

    ntStatus = LwRtlRBTreeCreate(
                    &SrvShareCompareInfo,
                    NULL,
                    &SrvShareReleaseInfoHandle,
                    &pShareList->pShareCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryOpen(&hRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryBeginEnum(
                                            hRepository,
                                            ulBatchLimit,
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
            ntStatus = SrvShareAddShareInfoInMemory_inlock(
                            pShareList,
                            ppShareInfoList[iShare]);
            BAIL_ON_NT_STATUS(ntStatus);
        }

    } while (ulNumSharesFound == ulBatchLimit);

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

/*
 * The flow is as following:
 *
 * 1) Duplicate in-memory share list
 * 2) Enum shares from DB
 *   a) If share does not exist in in-memory list, add it to the in-memory list
 *   b) Else remove it from the copy (created in 1) and update the in-memory
 *      info
 * 3) Remove any shares from the in-memory list that are still in the copy
 */
NTSTATUS
SrvShareReloadConfiguration(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN inLock = FALSE;
    HANDLE hRepository = NULL;
    HANDLE hResume = NULL;
    ULONG batchLimit = 256;
    ULONG numSharesFound = 0;
    PSRV_SHARE_INFO* ppShareInfoList = NULL;
    PLWRTL_RB_TREE pShareCollectionCopy = NULL;

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryOpen(&hRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(inLock, &pShareList->mutex);

    // In the end of the enumeration this copy will hold a list of deleted
    // shares
    ntStatus = SrvShareCollectionDuplicate(pShareList->pShareCollection,
                                           &pShareCollectionCopy);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryBeginEnum(
                                            hRepository,
                                            batchLimit,
                                            &hResume);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        ULONG shareIndex = 0;

        if (ppShareInfoList)
        {
            SrvShareFreeInfoList(ppShareInfoList, numSharesFound);
            ppShareInfoList = NULL;
            numSharesFound = 0;
        }

        ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryEnum(
                                                hRepository,
                                                hResume,
                                                &ppShareInfoList,
                                                &numSharesFound);
        BAIL_ON_NT_STATUS(ntStatus);

        for (; shareIndex < numSharesFound; shareIndex++)
        {
            PSRV_SHARE_INFO pDbShareInfo = ppShareInfoList[shareIndex];
            PSRV_SHARE_INFO pMemoryShareInfo = NULL;
            BOOLEAN inMemoryShareInfoLock = FALSE;

            ntStatus = LwRtlRBTreeFind(
                            pShareList->pShareCollection,
                            pDbShareInfo->pwszName,
                            OUT_PPVOID(&pMemoryShareInfo));
            if (ntStatus == STATUS_NOT_FOUND)
            {
                ntStatus = STATUS_SUCCESS;

                // new entry case - add pDbShareInfo to the in-memory list
                ntStatus = SrvShareAddShareInfoInMemory_inlock(
                                pShareList,
                                pDbShareInfo);
                BAIL_ON_NT_STATUS(ntStatus);

                continue;
            }
            BAIL_ON_NT_STATUS(ntStatus);


            ntStatus = LwRtlRBTreeRemove(pShareCollectionCopy,
                                         pDbShareInfo->pwszName);
            BAIL_ON_NT_STATUS(ntStatus);


            LWIO_LOCK_RWMUTEX_EXCLUSIVE(inMemoryShareInfoLock,
                                        &pMemoryShareInfo->mutex);

            ntStatus = SrvShareUpdateShareInfoInMemory_inlock(
                            pMemoryShareInfo,
                            pDbShareInfo);

            LWIO_UNLOCK_RWMUTEX(inMemoryShareInfoLock,
                                &pMemoryShareInfo->mutex);

            BAIL_ON_NT_STATUS(ntStatus);
        }

    } while (numSharesFound == batchLimit);

    // Take care of all the deleted shares
    ntStatus = LwRtlRBTreeTraverse(
                    pShareCollectionCopy,
                    LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    SrvShareReloadConfigurationVisitDeleteShare_inlock,
                    pShareList);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (hResume)
    {
        gSrvShareApi.pFnTable->pfnShareRepositoryEndEnum(hRepository, hResume);
    }

    if (ppShareInfoList)
    {
        SrvShareFreeInfoList(ppShareInfoList, numSharesFound);
    }

    LwRtlRBTreeFree(pShareCollectionCopy);

    LWIO_UNLOCK_RWMUTEX(inLock, &pShareList->mutex);

    if (hRepository)
    {
        gSrvShareApi.pFnTable->pfnShareRepositoryClose(hRepository);
    }

    return ntStatus;

error:

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
    PSRV_SHARE_INFO pShareInfo = NULL;

    ntStatus = LwRtlRBTreeFind(
                    pShareList->pShareCollection,
                    pwszShareName,
                    (PVOID*)&pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppShareInfo = SrvShareAcquireInfo(pShareInfo);

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
    IN     PWSTR                      pwszShareType,
    IN     ULONG                      ulShareFlags
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSRV_SHARE_INFO pShareInfo = NULL;
    wchar16_t wszServiceType[] = LWIO_SRV_SHARE_STRING_ID_DISK_W;
    wchar16_t wszShareComment[] = {0};
    PWSTR     pwszShareCommentRef =
                    (pwszShareComment ? pwszShareComment : &wszShareComment[0]);
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

    ntStatus = SrvAllocateStringW(
                        pwszShareCommentRef,
                        &pShareInfo->pwszComment);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareMapServiceStringToIdW(
                    pwszShareType,
                    &pShareInfo->service);
    BAIL_ON_NT_STATUS(ntStatus);

    if (ulSecDescLen)
    {
        ntStatus = SrvShareSetSecurity(
                       pShareInfo,
                       (PSECURITY_DESCRIPTOR_RELATIVE)pSecDesc,
                       ulSecDescLen);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = SrvShareSetDefaultSecurity(pShareInfo);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pShareInfo->ulFlags = ulShareFlags;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_SHARE_ENTRY),
                    (PVOID*)&pShareEntry);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareEntry->pInfo = SrvShareAcquireInfo(pShareInfo);

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryOpen(&hRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO: We only allow adding disk sharess
    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryAdd(
                                            hRepository,
                                            pShareInfo->pwszName,
                                            pShareInfo->pwszPath,
                                            pShareInfo->pwszComment,
                                            (PBYTE)pShareInfo->pSecDesc,
                                            pShareInfo->ulSecDescLen,
                                            pwszShareType,
                                            ulShareFlags);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeAdd(
                    pShareList->pShareCollection,
                    pShareInfo->pwszName,
                    pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareInfo = NULL;

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
    IN     PSRV_SHARE_INFO pShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WCHAR wszServiceType[] = LWIO_SRV_SHARE_STRING_ID_DISK_W;
    BOOLEAN  bInLock = FALSE;
    BOOLEAN  bShareInLock = FALSE;
    HANDLE   hRepository = NULL;

    if (!pShareInfo || IsNullOrEmptyString(pShareInfo->pwszName))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pShareList->mutex);

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryOpen(&hRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryDelete(
                                            hRepository,
                                            pShareInfo->pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_SHARED(bShareInLock, &pShareInfo->mutex);

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryAdd(
                                            hRepository,
                                            pShareInfo->pwszName,
                                            pShareInfo->pwszPath,
                                            pShareInfo->pwszComment,
                                            (PBYTE)pShareInfo->pSecDesc,
                                            pShareInfo->ulSecDescLen,
                                            wszServiceType,
                                            pShareInfo->ulFlags);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bShareInLock, &pShareInfo->mutex);

    gSrvShareApi.pFnTable->pfnShareRepositoryClose(hRepository);
    hRepository = NULL;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bShareInLock, &pShareInfo->mutex);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    return ntStatus;

error:

    if (hRepository)
    {
        gSrvShareApi.pFnTable->pfnShareRepositoryClose(hRepository);
    }

    goto cleanup;
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

    ntStatus = LwRtlRBTreeRemove(
                        pShareList->pShareCollection,
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
            ntStatus = SrvShareDuplicateInfo(pShareEntry->pInfo, &ppShares[i]);
            BAIL_ON_NT_STATUS(ntStatus);

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

NTSTATUS
SrvShareDuplicateInfo(
    PSRV_SHARE_INFO  pShareInfo,
    PSRV_SHARE_INFO* ppShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareInfo->mutex);

    ntStatus = SrvShareDuplicateInfo_inlock(pShareInfo, ppShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
int
SrvShareCompareInfo(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PWSTR pwszShareName1 = (PWSTR)pKey1;
    PWSTR pwszShareName2 = (PWSTR)pKey2;

    return SMBWc16sCaseCmp(pwszShareName1, pwszShareName2);
}

static
VOID
SrvShareReleaseInfoHandle(
    PVOID hShareInfo
    )
{
    SrvShareReleaseInfo((PSRV_SHARE_INFO)hShareInfo);
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

    if (pShareList->pShareCollection)
    {
        LwRtlRBTreeFree(pShareList->pShareCollection);
        pShareList->pShareCollection = NULL;
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

PSRV_SHARE_INFO
SrvShareAcquireInfo(
    IN PSRV_SHARE_INFO pShareInfo
    )
{
    InterlockedIncrement(&pShareInfo->refcount);

    return pShareInfo;
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
    if (pShareInfo->pwszComment)
    {
        SrvFreeMemory(pShareInfo->pwszComment);
    }

    SrvShareFreeSecurity(pShareInfo);

    SrvFreeMemory(pShareInfo);
}

static
NTSTATUS
SrvShareCollectionDuplicate(
    IN PLWRTL_RB_TREE pShareCollection,
    OUT PLWRTL_RB_TREE* ppShareCollectionCopy
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWRTL_RB_TREE pShareCollectionCopy = NULL;

    ntStatus = LwRtlRBTreeCreate(
                    &SrvShareCompareInfo,
                    NULL,
                    &SrvShareReleaseInfoHandle,
                    &pShareCollectionCopy);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeTraverse(
                    pShareCollection,
                    LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    SrvShareCollectionDuplicateVisit,
                    pShareCollectionCopy);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    *ppShareCollectionCopy = pShareCollectionCopy;

    return ntStatus;

error:

    if (pShareCollectionCopy)
    {
        LwRtlRBTreeFree(pShareCollectionCopy);
        pShareCollectionCopy = NULL;
    }

    goto cleanup;
}

static
NTSTATUS
SrvShareCollectionDuplicateVisit(
    IN PVOID pKey,
    IN PVOID pData,
    IN PVOID pUserData,
    OUT PBOOLEAN pContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWRTL_RB_TREE pShareCollectionCopy = (PLWRTL_RB_TREE)pUserData;
    PSRV_SHARE_INFO pShareInfo = (PSRV_SHARE_INFO)pData;

    SrvShareAcquireInfo(pShareInfo);

    ntStatus = LwRtlRBTreeAdd(pShareCollectionCopy, pKey, pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    *pContinue = TRUE;

cleanup:

    return ntStatus;

error:

    SrvShareReleaseInfo(pShareInfo);

    *pContinue = FALSE;

    goto cleanup;
}

static
NTSTATUS
SrvShareReloadConfigurationVisitDeleteShare_inlock(
    IN PVOID pKey,
    IN PVOID hShareInfo,
    IN PVOID hShareList,
    OUT PBOOLEAN pContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList =
                                        (PLWIO_SRV_SHARE_ENTRY_LIST)hShareList;
    PSRV_SHARE_INFO pShareInfo = (PSRV_SHARE_INFO)hShareInfo;

    ntStatus = SrvShareDeleteShareInfoInMemory_inlock(
                    pShareList,
                    pShareInfo,
                    TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    *pContinue = TRUE;

cleanup:

    return ntStatus;

error:

    *pContinue = FALSE;

    goto cleanup;
}

static
NTSTATUS
SrvShareAddShareInfoInMemory_inlock(
    IN PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN PSRV_SHARE_INFO pShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SHARE_ENTRY pShareEntry = NULL;

    ntStatus = SrvAllocateMemory(sizeof(*pShareEntry),
                               OUT_PPVOID(&pShareEntry));
    BAIL_ON_NT_STATUS(ntStatus);

    pShareEntry->pInfo = SrvShareAcquireInfo(pShareInfo);

    pShareEntry->pNext = pShareList->pShareEntry;
    pShareList->pShareEntry = pShareEntry;

    ntStatus = LwRtlRBTreeAdd(
                pShareList->pShareCollection,
                pShareInfo->pwszName,
                pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvShareAcquireInfo(pShareInfo);

cleanup:

    return ntStatus;

error:

    if (pShareEntry)
    {
        SrvShareRemoveFromList_inlock(pShareList, pShareInfo->pwszName);
        pShareEntry = NULL;
    }

    goto cleanup;
}

static
NTSTATUS
SrvShareDeleteShareInfoInMemory_inlock(
    IN PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN PSRV_SHARE_INFO pShareInfo,
    IN BOOLEAN DisconnectClients
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN inLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(inLock, pShareInfo->pMutex);

    SrvShareAcquireInfo(pShareInfo);

    ntStatus = SrvShareRemoveFromList_inlock(pShareList, pShareInfo->pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeRemove(pShareList->pShareCollection,
                                 pShareInfo->pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    if (DisconnectClients)
    {
        ntStatus = SrvShareDisconnectClients_inlock(pShareInfo);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    SrvShareReleaseInfo(pShareInfo);

    LWIO_UNLOCK_RWMUTEX(inLock, pShareInfo->pMutex);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvShareUpdateShareInfoInMemory_inlock(
    IN OUT PSRV_SHARE_INFO pShareInfoDst,
    IN PSRV_SHARE_INFO pShareInfoSrc
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SHARE_INFO pShareInfoSrcCopy = NULL;

    LWIO_ASSERT(LwRtlWC16StringIsEqual(pShareInfoSrc->pwszName,
                                       pShareInfoDst->pwszName, FALSE));

    if (LwRtlWC16StringIsEqual(pShareInfoSrc->pwszPath,
                               pShareInfoDst->pwszPath, FALSE) == FALSE ||
        pShareInfoSrc->service != pShareInfoDst->service)
    {
        ntStatus = SrvShareDisconnectClients_inlock(pShareInfoDst);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvShareDuplicateInfo_inlock(pShareInfoSrc, &pShareInfoSrcCopy);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvFreeMemory(pShareInfoDst->pwszPath);
    pShareInfoDst->pwszPath = pShareInfoSrcCopy->pwszPath;
    pShareInfoSrcCopy->pwszPath = NULL;

    SrvFreeMemory(pShareInfoDst->pwszComment);
    pShareInfoDst->pwszComment = pShareInfoSrcCopy->pwszComment;
    pShareInfoSrcCopy->pwszComment = NULL;


    SrvShareFreeSecurity(pShareInfoDst);

    pShareInfoDst->pSecDesc = pShareInfoSrcCopy->pSecDesc;
    pShareInfoDst->ulSecDescLen = pShareInfoSrcCopy->ulSecDescLen;
    pShareInfoSrcCopy->pSecDesc = NULL;
    pShareInfoSrcCopy->ulSecDescLen = 0;

    pShareInfoDst->pAbsSecDesc = pShareInfoSrcCopy->pAbsSecDesc;
    pShareInfoSrcCopy->pAbsSecDesc = NULL;


    pShareInfoDst->service = pShareInfoSrcCopy->service;
    pShareInfoDst->ulFlags = pShareInfoSrcCopy->ulFlags;

cleanup:

    if (pShareInfoSrcCopy)
    {
        SrvShareReleaseInfo(pShareInfoSrcCopy);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvShareDuplicateInfo_inlock(
    IN PSRV_SHARE_INFO pShareInfo,
    OUT PSRV_SHARE_INFO* ppShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SHARE_INFO pShareInfoCopy = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_SHARE_INFO),
                    (PVOID*)&pShareInfoCopy);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareInfoCopy->refcount = 1;

    pthread_rwlock_init(&pShareInfoCopy->mutex, NULL);
    pShareInfoCopy->pMutex = &pShareInfoCopy->mutex;

    if (pShareInfo->pwszName)
    {
        ntStatus = SrvAllocateStringW(
                        pShareInfo->pwszName,
                        &pShareInfoCopy->pwszName);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pShareInfo->pwszPath)
    {
        ntStatus = SrvAllocateStringW(
                        pShareInfo->pwszPath,
                        &pShareInfoCopy->pwszPath);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pShareInfo->pwszComment)
    {
        ntStatus = SrvAllocateStringW(
                        pShareInfo->pwszComment,
                        &pShareInfoCopy->pwszComment);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pShareInfo->ulSecDescLen)
    {
        ntStatus = SrvShareSetSecurity(
                        pShareInfoCopy,
                        pShareInfo->pSecDesc,
                        pShareInfo->ulSecDescLen);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pShareInfoCopy->service = pShareInfo->service;
    pShareInfoCopy->ulFlags = pShareInfo->ulFlags;

    *ppShareInfo = pShareInfoCopy;

cleanup:

    return ntStatus;

error:

    *ppShareInfo = NULL;

    if (pShareInfoCopy)
    {
        SrvShareFreeInfo(pShareInfoCopy);
    }

    goto cleanup;
}

static
NTSTATUS
SrvShareDisconnectClients_inlock(
    IN PSRV_SHARE_INFO pShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    // TODO - implement this function.
    // Disconnect all clients from the share.
    // The pShareInfo is already locked here (at least SHARED).

    return ntStatus;
}
