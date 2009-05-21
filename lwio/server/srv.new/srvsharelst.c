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
 *        sharedb.c
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
SrvAddShareInfoToList_inlock(
    PLWIO_SRV_SHARE_LIST pShareList,
    PSHARE_DB_INFO    pShareInfo,
    PSRV_SHARE_ENTRY *ppShareEntry
    );


static
void
SrvAddShareToList_inlock(
    PLWIO_SRV_SHARE_LIST pShareList,
    PSRV_SHARE_ENTRY pShareEntry
    );


static
NTSTATUS
SrvRemoveShareFromList_inlock(
    PLWIO_SRV_SHARE_LIST pShareList,
    PWSTR pszShareName
    );


static
void
SrvShareFreeEntry_inlock(
    PSRV_SHARE_ENTRY pShareEntry
    );


static
void
SrvShareFreeList_inlock(
    PLWIO_SRV_SHARE_LIST pShareList
    );


static
NTSTATUS
SrvFindShareByName_inlock(
    PLWIO_SRV_SHARE_LIST pShareList,
    PWSTR pwszShareName,
    PSHARE_DB_INFO *ppShareInfo
    );


NTSTATUS
SrvShareInitContextContents(
    PLWIO_SRV_SHARE_LIST pShareList
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSHARE_DB_INFO* ppShareInfoList = NULL;
    ULONG          ulOffset = 0;
    ULONG          ulLimit  = 256;
    ULONG          ulNumSharesFound = 0;

    pthread_rwlock_init(&pShareList->mutex, NULL);
    pShareList->pMutex = &pShareList->mutex;

    ntStatus = SrvShareDbInit();
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pShareList->mutex);

    pShareList->pShareEntry = NULL;

    do
    {
        ULONG iShare = 0;

        if (ppShareInfoList)
        {
            SrvShareDbFreeInfoList(ppShareInfoList, ulNumSharesFound);
            ppShareInfoList = NULL;
            ulNumSharesFound = 0;
        }

        ntStatus = SrvShareDbEnum_inlock(
                        ulOffset,
                        ulLimit,
                        &ppShareInfoList,
                        &ulNumSharesFound);
        if (ntStatus == STATUS_NO_MORE_ENTRIES)
        {
            ntStatus = 0;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        for (; iShare < ulNumSharesFound; iShare++)
        {
            PSHARE_DB_INFO pShareInfo = *(ppShareInfoList + iShare);
            PSRV_SHARE_ENTRY pShareEntry = NULL;

            ntStatus = SrvAddShareInfoToList_inlock(pShareList,
                                                    pShareInfo,
                                                    &pShareEntry);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulOffset += ulNumSharesFound;

    } while (ulNumSharesFound == ulLimit);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    if (ppShareInfoList)
    {
        SrvShareDbFreeInfoList(ppShareInfoList, ulNumSharesFound);
    }

    return ntStatus;

error:

    SrvShareFreeList_inlock(pShareList);

    goto cleanup;
}


NTSTATUS
SrvShareGetServiceStringId(
    SHARE_SERVICE  service,
    PSTR*          ppszService
    )
{
    NTSTATUS ntStatus = 0;
    PCSTR    pszId = NULL;
    PSTR     pszService = NULL;

    switch (service)
    {
        case SHARE_SERVICE_DISK_SHARE:

            pszId = LWIO_SRV_SHARE_STRING_ID_DISK;

            break;

        case SHARE_SERVICE_PRINTER:

            pszId = LWIO_SRV_SHARE_STRING_ID_PRINTER;

            break;

        case SHARE_SERVICE_COMM_DEVICE:

            pszId = LWIO_SRV_SHARE_STRING_ID_COMM;

            break;

        case SHARE_SERVICE_NAMED_PIPE:

            pszId = LWIO_SRV_SHARE_STRING_ID_IPC;

            break;

        case SHARE_SERVICE_ANY:

            pszId = LWIO_SRV_SHARE_STRING_ID_ANY;

            break;

        default:

            ntStatus = STATUS_NOT_FOUND;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBAllocateString(
                    pszId,
                    &pszService);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppszService = pszService;

cleanup:

    return ntStatus;

error:

    *ppszService = NULL;

    goto cleanup;
}


NTSTATUS
SrvShareGetServiceId(
    PCSTR          pszService,
    SHARE_SERVICE* pService
    )
{
    NTSTATUS ntStatus = 0;
    SHARE_SERVICE service = SHARE_SERVICE_UNKNOWN;

    if (IsNullOrEmptyString(pszService))
    {
        ntStatus = STATUS_NOT_FOUND;
    }
    else if (!strcmp(pszService, LWIO_SRV_SHARE_STRING_ID_IPC))
    {
        service = SHARE_SERVICE_NAMED_PIPE;
    }
    else if (!strcmp(pszService, LWIO_SRV_SHARE_STRING_ID_DISK))
    {
        service = SHARE_SERVICE_DISK_SHARE;
    }
    else if (!strcmp(pszService, LWIO_SRV_SHARE_STRING_ID_COMM))
    {
        service = SHARE_SERVICE_COMM_DEVICE;
    }
    else if (!strcmp(pszService, LWIO_SRV_SHARE_STRING_ID_PRINTER))
    {
        service = SHARE_SERVICE_PRINTER;
    }
    else if (!strcmp(pszService, LWIO_SRV_SHARE_STRING_ID_ANY))
    {
        service = SHARE_SERVICE_ANY;
    }
    else
    {
        ntStatus = STATUS_NOT_FOUND;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *pService = service;

cleanup:

    return ntStatus;

error:

    *pService = SHARE_SERVICE_UNKNOWN;

    goto cleanup;
}


VOID
SrvShareFreeContextContents(
    PLWIO_SRV_SHARE_LIST pShareList
    )
{
    if (pShareList->pShareEntry)
	{
		SrvShareFreeList_inlock(pShareList);
	}

    if (pShareList->pMutex)
    {
	pthread_rwlock_destroy(&pShareList->mutex);
	pShareList->pMutex = NULL;
    }

    SrvShareDbShutdown();
}


static
NTSTATUS
SrvAddShareInfoToList_inlock(
    PLWIO_SRV_SHARE_LIST pShareList,
    PSHARE_DB_INFO    pShareInfo,
    PSRV_SHARE_ENTRY *ppShareEntry
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_SHARE_ENTRY pShareEntry = NULL;

    ntStatus = LW_RTL_ALLOCATE(
                    &pShareEntry,
                    SRV_SHARE_ENTRY,
                    sizeof(SRV_SHARE_ENTRY));
    BAIL_ON_NT_STATUS(ntStatus);

    pShareEntry->pInfo = pShareInfo;

    SrvAddShareToList_inlock(pShareList, pShareEntry);

    InterlockedIncrement(&pShareInfo->refcount);
    *ppShareEntry = pShareEntry;

cleanup:

    return ntStatus;

error:

    if (pShareEntry) {
        SrvShareFreeEntry_inlock(pShareEntry);
    }

    *ppShareEntry = NULL;

    goto cleanup;
}


static
void
SrvAddShareToList_inlock(
    PLWIO_SRV_SHARE_LIST pShareList,
    PSRV_SHARE_ENTRY pShareEntry
    )
{
    pShareEntry->pNext = pShareList->pShareEntry;
    pShareList->pShareEntry = pShareEntry;
}


static
NTSTATUS
SrvRemoveShareFromList_inlock(
    PLWIO_SRV_SHARE_LIST pShareList,
    PWSTR pwszShareName
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSRV_SHARE_ENTRY pPrevShareEntry = NULL;

    pShareEntry = pShareList->pShareEntry;

    while (pShareEntry) {
        if (SMBWc16sCaseCmp(pwszShareName,
                        pShareEntry->pInfo->pwszName) == 0) {

            if (pPrevShareEntry) {
                pPrevShareEntry->pNext = pShareEntry->pNext;

            } else {
                pShareList->pShareEntry = pShareEntry->pNext;
            }
            goto cleanup;
        }

        pPrevShareEntry = pShareEntry;
        pShareEntry     = pShareEntry->pNext;
    }

    ntStatus = STATUS_NOT_FOUND;

cleanup:
    if (pShareEntry) {
        SrvShareFreeEntry_inlock(pShareEntry);
    }

    return ntStatus;
}


static
void
SrvShareFreeEntry_inlock(
    PSRV_SHARE_ENTRY pShareEntry
    )
{
    SrvShareDbReleaseInfo(pShareEntry->pInfo);
    LwIoFreeMemory((void*)pShareEntry);
}


static
void
SrvShareFreeList_inlock(
    PLWIO_SRV_SHARE_LIST pShareList
    )
{
    PSRV_SHARE_ENTRY pShareEntry = pShareList->pShareEntry;

    while (pShareEntry) {
        pShareList->pShareEntry = pShareEntry->pNext;
        if (pShareEntry) {
            SrvShareFreeEntry_inlock(pShareEntry);
        }

        pShareEntry = pShareList->pShareEntry;
    }
}


NTSTATUS
SrvFindShareByName(
    PLWIO_SRV_SHARE_LIST pShareList,
    PWSTR pwszShareName,
    PSHARE_DB_INFO *ppShareInfo
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareList->mutex);

    ntStatus = SrvFindShareByName_inlock(
                        pShareList,
                        pwszShareName,
                        ppShareInfo
                        );

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    return ntStatus;
}


static
NTSTATUS
SrvFindShareByName_inlock(
    PLWIO_SRV_SHARE_LIST pShareList,
    PWSTR pwszShareName,
    PSHARE_DB_INFO *ppShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSHARE_DB_INFO pShareInfo = NULL;

    pShareEntry = pShareList->pShareEntry;

    while (pShareEntry) {
        if (SMBWc16sCaseCmp(pwszShareName,
                        pShareEntry->pInfo->pwszName) == 0) {
            pShareInfo = pShareEntry->pInfo;
            break;
        }

        pShareEntry = pShareEntry->pNext;
    }

    if (!pShareInfo) goto cleanup;

    InterlockedIncrement(&pShareEntry->pInfo->refcount);
    *ppShareInfo = pShareInfo;

cleanup:
    ntStatus = (pShareInfo) ? STATUS_SUCCESS : STATUS_NOT_FOUND;
    return ntStatus;
}


NTSTATUS
SrvShareAddShare(
    PLWIO_SRV_SHARE_LIST pShareList,
    PWSTR  pwszShareName,
    PWSTR  pwszSharePath,
    PWSTR  pwszShareComment,
    PBYTE  pSecDesc,
    ULONG  ulSecDescLen,
    ULONG  ulShareType
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulError = 0;
    BOOLEAN bInLock = FALSE;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSHARE_DB_INFO pShareInfo = NULL;
    PSTR pszShareName = NULL;
    PSTR pszSharePath = NULL;
    PSTR pszShareComment = NULL;

    if (pwszShareName) {
        ulError = SMBWc16sToMbs(pwszShareName,
                                &pszShareName);

        ntStatus = LwUnixErrnoToNtStatus(ulError);
        BAIL_ON_NT_STATUS(ntStatus);

        if (IsNullOrEmptyString(pszShareName)) {
            ntStatus = STATUS_INVALID_PARAMETER;
        }

    } else {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    BAIL_ON_NT_STATUS(ntStatus);

    pszSharePath = "";

    if (pwszSharePath) {
        ulError = SMBWc16sToMbs(pwszSharePath,
                                &pszSharePath);

        ntStatus = LwUnixErrnoToNtStatus(ulError);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pszShareComment = "";

    if (pwszShareComment) {
        ulError = SMBWc16sToMbs(pwszShareComment,
                                &pszShareComment);

        ntStatus = LwUnixErrnoToNtStatus(ulError);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pShareList->mutex);

    ntStatus = SrvFindShareByName_inlock(
                        pShareList,
                        pwszShareName,
                        &pShareInfo
                        );
    if (!ntStatus) {
        ntStatus = STATUS_OBJECT_NAME_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LW_RTL_ALLOCATE(
                    &pShareEntry,
                    SRV_SHARE_ENTRY,
                    sizeof(SRV_SHARE_ENTRY));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LW_RTL_ALLOCATE(
                    &pShareInfo,
                    SHARE_DB_INFO,
                    sizeof(SHARE_DB_INFO));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBAllocateStringW(
                    pwszShareName,
                    &pShareInfo->pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBAllocateStringW(
                    pwszSharePath,
                    &pShareInfo->pwszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pwszShareComment)
    {
        ntStatus = SMBAllocateStringW(
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
	ntStatus = LW_RTL_ALLOCATE(
						&pShareInfo->pSecDesc,
						BYTE,
						ulSecDescLen);
	BAIL_ON_NT_STATUS(ntStatus);

	memcpy(pShareInfo->pSecDesc, pSecDesc, ulSecDescLen);

	pShareInfo->ulSecDescLen = ulSecDescLen;
    }
    else
    {
		pShareInfo->pSecDesc     = NULL;
		pShareInfo->ulSecDescLen = 0;
    }
    pShareInfo->service     = ulShareType;

    pShareInfo->refcount = 1;
    pShareInfo->bMarkedForDeletion = FALSE;

    pthread_rwlock_init(&pShareInfo->mutex, NULL);
    pShareInfo->pMutex = &pShareInfo->mutex;

    pShareEntry->pInfo = pShareInfo;
    InterlockedIncrement(&pShareInfo->refcount);

    SrvAddShareToList_inlock(pShareList, pShareEntry);

    ntStatus = SrvShareDbAdd_inlock(
                        pszShareName,
                        pszSharePath,
                        pszShareComment,
                        pSecDesc,
                        ulSecDescLen,
                        LWIO_SRV_SHARE_STRING_ID_DISK
                        );
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    if (pShareInfo) {
        SrvShareDbReleaseInfo(pShareInfo);
    }

    return ntStatus;

error:
    /* Remove share from the list after we failed to add
       it to the database */
    if (pShareEntry) {
        SrvRemoveShareFromList_inlock(
                   pShareList,
                   pShareEntry->pInfo->pwszName
                   );
    }

    goto cleanup;
}


NTSTATUS
SrvShareDeleteShare(
    PLWIO_SRV_SHARE_LIST pShareList,
    PWSTR pwszShareName
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszShareName = NULL;

    if (pwszShareName) {
        ulError = SMBWc16sToMbs(pwszShareName,
                                &pszShareName);
        ntStatus = LwUnixErrnoToNtStatus(ulError);
        BAIL_ON_NT_STATUS(ntStatus);

        if (IsNullOrEmptyString(pszShareName)) {
            ntStatus = STATUS_INVALID_PARAMETER;
        }

    } else {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pShareList->mutex);

    ntStatus = SrvShareDbDelete(pszShareName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvRemoveShareFromList_inlock(
                        pShareList,
                        pwszShareName
                        );
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SrvShareSetInfo(
    PLWIO_SRV_SHARE_LIST pShareList,
    PWSTR pwszShareName,
    PSHARE_DB_INFO pShareInfo
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
SrvShareGetInfo(
    PLWIO_SRV_SHARE_LIST pShareList,
    PWSTR pwszShareName,
    PSHARE_DB_INFO *ppShareInfo
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSHARE_DB_INFO pShareInfo = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareList->mutex);

    ntStatus = SrvFindShareByName_inlock(
                        pShareList,
                        pwszShareName,
                        &pShareInfo
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    *ppShareInfo = pShareInfo;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvShareEnumShares(
    PLWIO_SRV_SHARE_LIST pShareList,
    ULONG dwLevel,
    PSHARE_DB_INFO** pppShareInfo,
    PULONG pdwNumEntries
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulError = 0;
    ULONG dwCount = 0;
    BOOLEAN bInLock = FALSE;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSHARE_DB_INFO* ppShares = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareList->mutex);

    /* Count the number of share entries */
    pShareEntry = pShareList->pShareEntry;
    while (pShareEntry) {
        pShareEntry = pShareEntry->pNext;
        dwCount++;
    }

    if (dwCount)
    {
        ULONG i = 0;

        ntStatus = LW_RTL_ALLOCATE(
                        &ppShares,
                        PSHARE_DB_INFO,
                        dwCount * sizeof(PSHARE_DB_INFO));
        BAIL_ON_NT_STATUS(ntStatus);

        pShareEntry = pShareList->pShareEntry;
        for (; i < dwCount; i++)
        {
            InterlockedIncrement(&pShareEntry->pInfo->refcount);

            ppShares[i] = pShareEntry->pInfo;

            pShareEntry = pShareEntry->pNext;
        }
    }

    *pppShareInfo   = ppShares;
    *pdwNumEntries = dwCount;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    return ntStatus;

error:

    if (ppShares)
    {
        ULONG i = 0;
        for (; i < dwCount; i++)
        {
            PSHARE_DB_INFO pShareInfo = ppShares[i];

            if (pShareInfo)
            {
                SrvShareDbReleaseInfo(pShareInfo);
            }
        }

        LwIoFreeMemory(ppShares);
    }

    if (ntStatus == STATUS_SUCCESS &&
        ulError != 0) {
        ntStatus = LwUnixErrnoToNtStatus(ulError);
    }

    *pppShareInfo   = NULL;
    *pdwNumEntries = 0;

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
