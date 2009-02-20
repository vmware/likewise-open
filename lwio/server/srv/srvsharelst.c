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

#include "includes.h"

#if 0
static
int
SrvShareCompare(
    PVOID pShareName1,
    PVOID pShareName2
    );

static
VOID
SrvShareRelease(
    PVOID pShareInfo
    );

#endif

NTSTATUS
SrvShareInitContextContents(
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSHARE_DB_INFO* ppShareInfoList = NULL;
    ULONG          ulOffset = 0;
    ULONG          ulLimit  = 256;
    ULONG          ulNumSharesFound = 0;
    HANDLE         hDb = (HANDLE)NULL;

    ntStatus = SrvShareDbInit(pDbContext);
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pDbContext->mutex);

    pDbContext->pShareEntry = NULL;

    ntStatus = SrvShareDbOpen(
                    pDbContext,
                    &hDb);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        ULONG iShare = 0;

        if (ppShareInfoList)
        {
            SrvShareDbFreeInfoList(ppShareInfoList, ulNumSharesFound);
            ppShareInfoList = NULL;
            ulNumSharesFound = 0;
        }

        ntStatus = SrvShareDbEnum(
                        pDbContext,
                        hDb,
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

            ntStatus = SrvAddShareInfoToList(pDbContext,
                                             pShareInfo,
                                             &pShareEntry);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulOffset += ulNumSharesFound;

    } while (ulNumSharesFound == ulLimit);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pDbContext->mutex);

    if (ppShareInfoList)
    {
        SrvShareDbFreeInfoList(ppShareInfoList, ulNumSharesFound);
    }

    if (hDb != (HANDLE)NULL)
    {
        SrvShareDbClose(pDbContext, hDb);
    }

    return ntStatus;

error:
    SrvShareFreeList(pDbContext);

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
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext
    )
{
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pDbContext->mutex);

    if (pDbContext->pShareEntry)
    {
        SrvShareFreeList(pDbContext);
    }

    SMB_UNLOCK_RWMUTEX(bInLock, &pDbContext->mutex);

    SrvShareDbShutdown(pDbContext);
}


static
BOOLEAN
IsEqualShareEntry(
    PWSTR pwszName,
    PWSTR pwszShareName
    )
{
    if (pwszName == pwszShareName) {
        return TRUE;
    }

    if ((pwszName && !pwszShareName) ||
        (!pwszName && pwszShareName)) {
        return FALSE;
    }

    if (SMBWc16sCmp(pwszName, pwszShareName)) {
        return FALSE;
    }

    return TRUE;
}


NTSTATUS
SrvAddShareToList(
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext,
    PSRV_SHARE_ENTRY pShareEntry
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_SHARE_ENTRY pShareList = pDbContext->pShareEntry;

    if (!pShareEntry) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    pShareEntry->pNext = pShareList;
    pShareList         = pShareEntry;

    pDbContext->pShareEntry = pShareList;

cleanup:
    return ntStatus;
}


NTSTATUS
SrvAddShareInfoToList(
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext,
    PSHARE_DB_INFO    pShareInfo,
    PSRV_SHARE_ENTRY *ppShareEntry
    )
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    PSRV_SHARE_ENTRY pShareEntry = NULL;

    ntStatus = LwIoAllocateMemory(sizeof(SRV_SHARE_ENTRY),
                                  (void**)&pShareEntry);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pShareInfo->pwszName) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto error;
    }

    dwError = SMBAllocateStringW(pShareInfo->pwszName,
                                 &pShareEntry->info.pwszName);
    BAIL_ON_SMB_ERROR(dwError);

    if (pShareInfo->pwszPath) {
        dwError = SMBAllocateStringW(pShareInfo->pwszPath,
                                     &pShareEntry->info.pwszPath);
        BAIL_ON_SMB_ERROR(dwError);
    }

    if (pShareInfo->pwszComment) {
        dwError = SMBAllocateStringW(pShareInfo->pwszComment,
                                     &pShareEntry->info.pwszComment);
        BAIL_ON_SMB_ERROR(dwError);
    }

    if (pShareInfo->pwszSID) {
        dwError = SMBAllocateStringW(pShareInfo->pwszSID,
                                     &pShareEntry->info.pwszSID);
        BAIL_ON_SMB_ERROR(dwError);
    }

    pShareEntry->info.service = pShareInfo->service;

    ntStatus = SrvAddShareToList(pDbContext, pShareEntry);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareEntry->info.refcount = 1;

    *ppShareEntry = pShareEntry;

cleanup:
    return ntStatus;

error:
    if (pShareEntry) {
        SMB_SAFE_FREE_MEMORY(pShareEntry->info.pwszName);
        SMB_SAFE_FREE_MEMORY(pShareEntry->info.pwszPath);
        SMB_SAFE_FREE_MEMORY(pShareEntry->info.pwszComment);

        LwIoFreeMemory((void*)pShareEntry);
    }

    *ppShareEntry = NULL;
    goto cleanup;
}


NTSTATUS
SrvFindShareByName(
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext,
    PWSTR pwszShareName,
    PSHARE_DB_INFO *ppShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PSRV_SHARE_ENTRY pShareEntry = NULL;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pDbContext->mutex);

    pShareEntry = pDbContext->pShareEntry;

    while (pShareEntry) {
        if (IsEqualShareEntry(pwszShareName,
                              pShareEntry->info.pwszName)) {

            *ppShareInfo = &pShareEntry->info;

            InterlockedIncrement(&pShareEntry->info.refcount);
            goto cleanup;
        }

        pShareEntry = pShareEntry->pNext;
    }

    ntStatus = STATUS_NOT_FOUND;

cleanup:
    SMB_UNLOCK_RWMUTEX(bInLock, &pDbContext->mutex);

    return ntStatus;
}


NTSTATUS
SrvRemoveShareFromList(
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext,
    PWSTR pwszShareName,
    PSRV_SHARE_ENTRY *ppShareEntry
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSRV_SHARE_ENTRY pPrevShareEntry = NULL;

    pShareEntry = pDbContext->pShareEntry;

    while (pShareEntry) {
        if (IsEqualShareEntry(pwszShareName,
                              pShareEntry->info.pwszName)) {

            if (pPrevShareEntry) {
                pPrevShareEntry->pNext = pShareEntry->pNext;

            } else {
                pDbContext->pShareEntry = pShareEntry->pNext;
            }

            *ppShareEntry = pShareEntry;
            goto cleanup;
        }

        pPrevShareEntry = pShareEntry;
        pShareEntry     = pShareEntry->pNext;
    }

    ntStatus = STATUS_NOT_FOUND;

cleanup:
    return ntStatus;
}


NTSTATUS
SrvShareFreeList(
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_SHARE_ENTRY pShareEntry = pDbContext->pShareEntry;

    while (pShareEntry) {
        ntStatus = SrvRemoveShareFromList(pDbContext,
                                          pShareEntry->info.pwszName,
                                          &pShareEntry);
        BAIL_ON_NT_STATUS(ntStatus);

        if (pShareEntry) {
            SMB_SAFE_FREE_MEMORY(pShareEntry->info.pwszName);
            SMB_SAFE_FREE_MEMORY(pShareEntry->info.pwszPath);
            SMB_SAFE_FREE_MEMORY(pShareEntry->info.pwszComment);

            LwIoFreeMemory((void*)pShareEntry);
        }

        pShareEntry = pDbContext->pShareEntry;
    }

error:
    return ntStatus;
}


NTSTATUS
SrvShareAddShare(
    PWSTR  pwszShareName,
    PWSTR  pwszSharePath,
    PWSTR  pwszShareComment
    )
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext = NULL;
    HANDLE hDb = (HANDLE)NULL;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSHARE_DB_INFO pShareInfo = NULL;
    PSTR pszShareName = NULL;
    PSTR pszSharePath = NULL;
    PSTR pszShareComment = NULL;

    if (pwszShareName) {
        dwError = SMBWc16sToMbs(pwszShareName,
                                &pszShareName);

        ntStatus = LwUnixErrnoToNtStatus(dwError);
        BAIL_ON_SMB_ERROR(dwError);

        if (IsNullOrEmptyString(pszShareName)) {
            ntStatus = STATUS_INVALID_PARAMETER;
        }

    } else {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    BAIL_ON_NT_STATUS(ntStatus);

    pszSharePath = "";

    if (pwszSharePath) {
        dwError = SMBWc16sToMbs(pwszSharePath,
                                &pszSharePath);

        ntStatus = LwUnixErrnoToNtStatus(dwError);
        BAIL_ON_SMB_ERROR(dwError);
    }

    pszShareComment = "";

    if (pwszShareComment) {
        dwError = SMBWc16sToMbs(pwszShareComment,
                                &pszShareComment);

        ntStatus = LwUnixErrnoToNtStatus(dwError);
        BAIL_ON_SMB_ERROR(dwError);
    }

    pDbContext = &gSMBSrvGlobals.shareDBContext;
    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pDbContext->mutex);

    ntStatus = SrvFindShareByName(
                        pDbContext,
                        pwszShareName,
                        &pShareInfo
                        );
    if (!ntStatus) {
        ntStatus = STATUS_OBJECT_NAME_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LwIoAllocateMemory(sizeof(SRV_SHARE_ENTRY),
                                  (void**)&pShareEntry);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAddShareToList(pDbContext, pShareEntry);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareEntry->info.pwszName    = pwszShareName;
    pShareEntry->info.pwszPath    = pwszSharePath;
    pShareEntry->info.pwszComment = pwszShareComment;

    ntStatus = SrvShareDbOpen(
                    pDbContext,
                    &hDb);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareDbAdd(
                        pDbContext,
                        hDb,
                        pszShareName,
                        pszSharePath,
                        pszShareComment,
                        NULL,
                        LWIO_SRV_SHARE_STRING_ID_DISK
                        );
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    if (hDb != (HANDLE)NULL)
    {
       SrvShareDbClose(pDbContext, hDb);
    }

    SMB_UNLOCK_RWMUTEX(bInLock, &pDbContext->mutex);

    return ntStatus;

error:
    /* Remove share from the list after we failed to add
       it to the database */
    if (pShareEntry) {
        SrvRemoveShareFromList(
                   pDbContext,
                   pShareEntry->info.pwszName,
                   &pShareEntry
                   );

        if (!pShareEntry) goto cleanup;

        SMB_SAFE_FREE_MEMORY(pShareEntry->info.pwszName);
        SMB_SAFE_FREE_MEMORY(pShareEntry->info.pwszPath);
        SMB_SAFE_FREE_MEMORY(pShareEntry->info.pwszComment);

        LwIoFreeMemory((void*)pShareEntry);
    }

    goto cleanup;
}


NTSTATUS
SrvShareDeleteShare(
    PWSTR pwszShareName
    )
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext = NULL;
    HANDLE hDb = (HANDLE)NULL;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSTR pszShareName = NULL;

    if (pwszShareName) {
        dwError = SMBWc16sToMbs(pwszShareName,
                                &pszShareName);
        BAIL_ON_SMB_ERROR(dwError);

        if (IsNullOrEmptyString(pszShareName)) {
            ntStatus = STATUS_INVALID_PARAMETER;
        }

    } else {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    BAIL_ON_NT_STATUS(ntStatus);

    pDbContext = &gSMBSrvGlobals.shareDBContext;
    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pDbContext->mutex);

    ntStatus = SrvShareDbOpen(
                    pDbContext,
                    &hDb);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareDbDelete(
                        pDbContext,
                        hDb,
                        pszShareName
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvRemoveShareFromList(
                        pDbContext,
                        pwszShareName,
                        &pShareEntry
                        );
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    if (hDb != (HANDLE)NULL)
    {
       SrvShareDbClose(pDbContext, hDb);
    }

    if (pShareEntry) {
        SMB_SAFE_FREE_MEMORY(pShareEntry->info.pwszName);
        SMB_SAFE_FREE_MEMORY(pShareEntry->info.pwszPath);
        SMB_SAFE_FREE_MEMORY(pShareEntry->info.pwszComment);

        LwIoFreeMemory((void*)pShareEntry);
    }

    SMB_UNLOCK_RWMUTEX(bInLock, &pDbContext->mutex);

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SrvShareSetInfo(
    PWSTR pwszShareName,
    DWORD dwLevel,
    PVOID pBuffer
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
                    hDb,
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
    PWSTR pwszShareName,
    DWORD dwLevel,
    PBYTE pOutBuffer,
    DWORD dwOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;

#if 0

    ENTER_READER_LOCK();

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
            break:

        case 502:
            break;

        case 503:
            break;

    }

error:
    LEAVE_READER_LOCK();

#endif

    return(ntStatus);
}


NTSTATUS
SrvShareEnumShares(
    DWORD dwLevel,
    PSHARE_DB_INFO *ppShareInfo,
    PDWORD pdwNumEntries
    )
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    DWORD dwCount = 0;
    DWORD i = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext = NULL;
    PSRV_SHARE_ENTRY pShareEntry = NULL;
    PSHARE_DB_INFO pShareInfo = NULL;
    PSHARE_DB_INFO pShares = NULL;

#if 0

    ntStatus = ValidateServerSecurity(
                        hAccessToken,
                        GENERIC_READ,
                        pServerSD
                        );
    BAIL_ON_NT_STATUS(ntStatus);
#endif

    pDbContext = &gSMBSrvGlobals.shareDBContext;
    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pDbContext->mutex);

    /* Count the number of share entries */
    pShareEntry = pDbContext->pShareEntry;
    while (pShareEntry) {
        pShareEntry = pShareEntry->pNext;
        dwCount++;
    }

    dwError = LwIoAllocateMemory(dwCount * sizeof(SHARE_DB_INFO),
                                 (void**)&pShares);
    BAIL_ON_SMB_ERROR(dwError);

    pShareEntry = pDbContext->pShareEntry;
    for (i = 0; i < dwCount; i++) {
        pShares[i].pwszName     = pShareEntry->info.pwszName;
        pShares[i].pwszPath     = pShareEntry->info.pwszPath;
        pShares[i].pwszComment  = pShareEntry->info.pwszComment;
        pShares[i].pwszSID      = pShareEntry->info.pwszSID;
        pShares[i].service      = pShareEntry->info.service;
    }

    *ppShareInfo   = pShares;
    *pdwNumEntries = dwCount;

cleanup:
    SMB_UNLOCK_RWMUTEX(bInLock, &pDbContext->mutex);

    return ntStatus;

error:
    if (pShareInfo) {
        LwIoFreeMemory((void*)pShareInfo);
    }

    *ppShareInfo   = NULL;
    *pdwNumEntries = 0;
    goto cleanup;
}

#if 0

NTSTATUS
SrvCreateShareEntry(
    PWSTR pwszShareName,
    PWSTR pwszPathName,
    PWSTR pwszComment,
    PBYTE pSecurityDescriptor,
    DWORD dwSDSize,
    DWORD dwFlags,
    PSRV_SHARE_ENTRY * ppShareEntry
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_SHARE_OBJECT),
                    &pShareEntry
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateString(
                    pszShareName,
                    &pShareEntry->pszShareName
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateString(
                    pszPathName,
                    &pShareEntry->pszPathName
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateString(
                    pszComment,
                    &pShareEntry->pszComment
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateSecurityDescriptor(
                    pSecurityDescriptor,
                    dwSDSize,
                    &pShareEntry->pSecurityDescriptor
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pShareEntry->dwFlags = dwFlags;

    *ppShareEntry = pShareEntry

    return(ntStatus);

error:

    if (pShareEntry) {

        SrvFreeShareEntry(pShareEntry);

    }

    *ppShareEntry = NULL;

    return(ntStatus);
}
#endif


#if 0
static
int
SrvShareCompare(
    PVOID pShareName1,
    PVOID pShareName2
    )
{
    assert(pShareName1 != NULL);
    assert(pShareName2 != NULL);

    return wc16scmp((PWSTR)pShareName1, (PWSTR)pShareName2);
}

static
VOID
SrvShareRelease(
    PVOID pShareInfo
    )
{
    PSHARE_DB_INFO pInfo = (PSHARE_DB_INFO)pShareInfo;

    if (InterlockedDecrement(&pInfo->refcount) == 0)
    {
        SrvShareDbReleaseInfo(pInfo);
    }
}

#endif


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
