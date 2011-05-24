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
    IN PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN PWSTR pwszShareName,
    OUT PSRV_SHARE_INFO* ppShareInfo
    );

static
NTSTATUS
SrvShareRemoveFromList_inlock(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN PWSTR pwszShareName
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

static
VOID
SrvShareFreeEntry(
    IN PSRV_SHARE_ENTRY pShareEntry
    );

static
NTSTATUS
SrvShareInfoCreate(
    IN OPTIONAL PWSTR pwszShareName,
    IN OPTIONAL PWSTR pwszSharePath,
    IN OPTIONAL PWSTR pwszShareComment,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    IN ULONG SecDescLength,
    IN SHARE_SERVICE Service,
    IN ULONG Flags,
    OUT PSRV_SHARE_INFO* ppShareInfo
    );


NTSTATUS
SrvShareInitList(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList
    )
{
    NTSTATUS ntStatus = 0;
    HANDLE hRepository = NULL;
    HANDLE hResume = NULL;
    PSRV_SHARE_INFO* ppShareInfoList = NULL;
    ULONG batchLimit  = 256;
    ULONG numSharesFound = 0;

    pthread_rwlock_init(&pShareList->Mutex, NULL);
    pShareList->pMutex = &pShareList->Mutex;

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
                                            batchLimit,
                                            &hResume);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        ULONG iShare = 0;

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

        for (; iShare < numSharesFound; iShare++)
        {
            ntStatus = SrvShareAddShareInfoInMemory_inlock(
                            pShareList,
                            ppShareInfoList[iShare]);
            BAIL_ON_NT_STATUS(ntStatus);
        }

    } while (numSharesFound == batchLimit);

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
        SrvShareFreeInfoList(ppShareInfoList, numSharesFound);
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

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(inLock, &pShareList->Mutex);

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
                                        &pMemoryShareInfo->Mutex);

            ntStatus = SrvShareUpdateShareInfoInMemory_inlock(
                            pMemoryShareInfo,
                            pDbShareInfo);

            LWIO_UNLOCK_RWMUTEX(inMemoryShareInfoLock,
                                &pMemoryShareInfo->Mutex);

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

    LWIO_UNLOCK_RWMUTEX(inLock, &pShareList->Mutex);

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
    IN PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN PWSTR pwszShareName,
    OUT PSRV_SHARE_INFO* ppShareInfo
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN inLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(inLock, &pShareList->Mutex);

    ntStatus = SrvShareFindByName_inlock(
                        pShareList,
                        pwszShareName,
                        ppShareInfo);

    LWIO_UNLOCK_RWMUTEX(inLock, &pShareList->Mutex);

    return ntStatus;
}

static
NTSTATUS
SrvShareFindByName_inlock(
    IN PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN PWSTR pwszShareName,
    OUT PSRV_SHARE_INFO* ppShareInfo
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
    IN PWSTR pwszShareName,
    IN PWSTR pwszSharePath,
    IN PWSTR pwszShareComment,
    IN PBYTE pSecDesc,
    IN ULONG SecDescLen,
    IN PWSTR pwszShareType,
    IN ULONG ShareFlags
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN inLock = FALSE;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSRV_SHARE_INFO pShareInfo = NULL;
    wchar16_t wszServiceType[] = LWIO_SRV_SHARE_STRING_ID_DISK_W;
    wchar16_t wszShareComment[] = {0};
    PWSTR pwszShareCommentRef =
                    (pwszShareComment ? pwszShareComment : &wszShareComment[0]);
    HANDLE hRepository = NULL;
    SHARE_SERVICE shareService = SHARE_SERVICE_UNKNOWN;

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

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(inLock, &pShareList->Mutex);

    ntStatus = SrvShareFindByName_inlock(
                        pShareList,
                        pwszShareName,
                        &pShareInfo);
    if (!ntStatus)
    {
        ntStatus = STATUS_OBJECT_NAME_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvShareMapServiceStringToIdW(pwszShareType, &shareService);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareInfoCreate(
                    pwszShareName,
                    pwszSharePath,
                    pwszShareCommentRef,
                    (PSECURITY_DESCRIPTOR_RELATIVE)pSecDesc,
                    SecDescLen,
                    shareService,
                    ShareFlags,
                    &pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!SecDescLen)
    {
        ntStatus = SrvShareSetDefaultSecurity(pShareInfo);
        BAIL_ON_NT_STATUS(ntStatus);
    }

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
                                            pShareInfo->SecDescLen,
                                            pwszShareType,
                                            ShareFlags);
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

    LWIO_UNLOCK_RWMUTEX(inLock, &pShareList->Mutex);

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
    IN PSRV_SHARE_INFO pShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WCHAR wszServiceType[] = LWIO_SRV_SHARE_STRING_ID_DISK_W;
    BOOLEAN inLock = FALSE;
    BOOLEAN inShareLock = FALSE;
    HANDLE hRepository = NULL;

    if (!pShareInfo || IsNullOrEmptyString(pShareInfo->pwszName))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(inLock, &pShareList->Mutex);

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryOpen(&hRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryDelete(
                                            hRepository,
                                            pShareInfo->pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_SHARED(inShareLock, &pShareInfo->Mutex);

    ntStatus = gSrvShareApi.pFnTable->pfnShareRepositoryAdd(
                                            hRepository,
                                            pShareInfo->pwszName,
                                            pShareInfo->pwszPath,
                                            pShareInfo->pwszComment,
                                            (PBYTE)pShareInfo->pSecDesc,
                                            pShareInfo->SecDescLen,
                                            wszServiceType,
                                            pShareInfo->Flags);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(inShareLock, &pShareInfo->Mutex);

    gSrvShareApi.pFnTable->pfnShareRepositoryClose(hRepository);
    hRepository = NULL;

cleanup:

    LWIO_UNLOCK_RWMUTEX(inShareLock, &pShareInfo->Mutex);

    LWIO_UNLOCK_RWMUTEX(inLock, &pShareList->Mutex);

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
    IN PWSTR pwszShareName
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN inLock = FALSE;
    HANDLE hRepository = NULL;

    if (IsNullOrEmptyString(pwszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(inLock, &pShareList->Mutex);

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

    LWIO_UNLOCK_RWMUTEX(inLock, &pShareList->Mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvShareRemoveFromList_inlock(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN PWSTR pwszShareName
    )
{
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSRV_SHARE_ENTRY pPrevShareEntry = NULL;
    ULONG sharesRemoved = 0;

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

            sharesRemoved++;

            break;
        }

        pPrevShareEntry = pShareEntry;
        pShareEntry     = pShareEntry->pNext;
    }

    return (sharesRemoved ? STATUS_SUCCESS : STATUS_NOT_FOUND);
}

NTSTATUS
SrvShareEnum(
    IN PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    OUT PSRV_SHARE_INFO** pppShareInfo,
    IN OUT PULONG pNumEntries
    )
{
    NTSTATUS ntStatus = 0;
    ULONG count = 0;
    BOOLEAN inLock = FALSE;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSRV_SHARE_INFO* ppShares = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(inLock, &pShareList->Mutex);

    /* Count the number of share entries */
    pShareEntry = pShareList->pShareEntry;
    while (pShareEntry)
    {
        pShareEntry = pShareEntry->pNext;
        count++;
    }

    if (count)
    {
        ULONG i = 0;

        ntStatus = SrvAllocateMemory(
                        count * sizeof(PSRV_SHARE_INFO),
                        (PVOID*)&ppShares);
        BAIL_ON_NT_STATUS(ntStatus);

        pShareEntry = pShareList->pShareEntry;
        for (; i < count; i++)
        {
            ntStatus = SrvShareDuplicateInfo(pShareEntry->pInfo, &ppShares[i]);
            BAIL_ON_NT_STATUS(ntStatus);

            pShareEntry = pShareEntry->pNext;
        }
    }

    *pppShareInfo   = ppShares;
    *pNumEntries  = count;

cleanup:

    LWIO_UNLOCK_RWMUTEX(inLock, &pShareList->Mutex);

    return ntStatus;

error:

    if (ppShares)
    {
        SrvShareFreeInfoList(ppShares, count);
    }

    *pppShareInfo   = NULL;
    *pNumEntries = 0;

    goto cleanup;
}

NTSTATUS
SrvShareDuplicateInfo(
    IN PSRV_SHARE_INFO pShareInfo,
    OUT PSRV_SHARE_INFO* ppShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  inLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(inLock, &pShareInfo->Mutex);

    ntStatus = SrvShareDuplicateInfo_inlock(pShareInfo, ppShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(inLock, &pShareInfo->Mutex);

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
        pthread_rwlock_destroy(&pShareList->Mutex);
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
    IN PSRV_SHARE_INFO* ppInfoList,
    IN ULONG NumInfos
    )
{
    ULONG iInfo = 0;

    for (; iInfo < NumInfos; iInfo++)
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
    InterlockedIncrement(&pShareInfo->RefCount);

    return pShareInfo;
}

VOID
SrvShareReleaseInfo(
    IN PSRV_SHARE_INFO pShareInfo
    )
{
    if (InterlockedDecrement(&pShareInfo->RefCount) == 0)
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
        pthread_rwlock_destroy(&pShareInfo->Mutex);
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
        pShareInfoSrc->Service != pShareInfoDst->Service)
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
    pShareInfoDst->SecDescLen = pShareInfoSrcCopy->SecDescLen;
    pShareInfoSrcCopy->pSecDesc = NULL;
    pShareInfoSrcCopy->SecDescLen = 0;

    pShareInfoDst->pAbsSecDesc = pShareInfoSrcCopy->pAbsSecDesc;
    pShareInfoSrcCopy->pAbsSecDesc = NULL;


    pShareInfoDst->Service = pShareInfoSrcCopy->Service;
    pShareInfoDst->Flags = pShareInfoSrcCopy->Flags;

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

    ntStatus = SrvShareInfoCreate(
                    pShareInfo->pwszName,
                    pShareInfo->pwszPath,
                    pShareInfo->pwszComment,
                    pShareInfo->pSecDesc,
                    pShareInfo->SecDescLen,
                    pShareInfo->Service,
                    pShareInfo->Flags,
                    &pShareInfoCopy);
    BAIL_ON_NT_STATUS(ntStatus);

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

static
NTSTATUS
SrvShareInfoCreate(
    IN OPTIONAL PWSTR pwszShareName,
    IN OPTIONAL PWSTR pwszSharePath,
    IN OPTIONAL PWSTR pwszShareComment,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    IN ULONG SecDescLength,
    IN SHARE_SERVICE Service,
    IN ULONG Flags,
    OUT PSRV_SHARE_INFO* ppShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SHARE_INFO pShareInfo = NULL;

    ntStatus = SrvAllocateMemory(sizeof(*pShareInfo), OUT_PPVOID(&pShareInfo));
    BAIL_ON_NT_STATUS(ntStatus);

    pShareInfo->RefCount = 1;

    pthread_rwlock_init(&pShareInfo->Mutex, NULL);
    pShareInfo->pMutex = &pShareInfo->Mutex;

    if (pwszShareName)
    {
        ntStatus = SrvAllocateStringW(pwszShareName, &pShareInfo->pwszName);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pwszSharePath)
    {
        ntStatus = SrvAllocateStringW(pwszSharePath, &pShareInfo->pwszPath);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pwszShareComment)
    {
        ntStatus = SrvAllocateStringW(
                        pwszShareComment,
                        &pShareInfo->pwszComment);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (SecDescLength)
    {
        ntStatus = SrvShareSetSecurity(
                        pShareInfo,
                        pSecDesc,
                        SecDescLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pShareInfo->Service = Service;
    pShareInfo->Flags = Flags;

cleanup:

    *ppShareInfo = pShareInfo;

    return ntStatus;

error:

    if (pShareInfo)
    {
        SrvShareFreeInfo(pShareInfo);
        pShareInfo = NULL;
    }

    goto cleanup;
}
