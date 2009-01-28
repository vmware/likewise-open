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

#include "includes.h"

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

NTSTATUS
SrvShareInitContextContents(
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_RB_TREE pShareCollection = NULL;
    PSHARE_DB_INFO* ppShareInfoList = NULL;
    ULONG          ulOffset = 0;
    ULONG          ulLimit  = 256;
    ULONG          ulNumSharesFound = 0;
    HANDLE         hDb = (HANDLE)NULL;

    ntStatus = SrvShareDbInit(pDbContext);
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pDbContext->mutex);

    ntStatus = SMBRBTreeCreate(
                    &SrvShareCompare,
                    NULL,
                    &SrvShareRelease,
                    &pShareCollection);
    BAIL_ON_NT_STATUS(ntStatus);

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

            ntStatus = SMBRBTreeAdd(
                            pShareCollection,
                            pShareInfo->pwszName,
                            pShareInfo);
            BAIL_ON_NT_STATUS(ntStatus);

            InterlockedIncrement(&pShareInfo->refcount);
        }

        ulOffset += ulNumSharesFound;

    } while (ulNumSharesFound == ulLimit);

    pDbContext->pShareCollection = pShareCollection;

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

    pDbContext->pShareCollection = NULL;

    if (pShareCollection)
    {
        SMBRBTreeFree(pShareCollection);
    }

    goto cleanup;
}

NTSTATUS
SrvShareFindShareByName(
    PSMB_SRV_SHARE_DB_CONTEXT pDbContext,
    PWSTR           pwszShareName,
    PSHARE_DB_INFO* ppShareInfo
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSHARE_DB_INFO pShareInfo = NULL;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pDbContext->mutex);

    ntStatus = SMBRBTreeFind(
                    pDbContext->pShareCollection,
                    pwszShareName,
                    (PVOID*)&pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pShareInfo->refcount);

    *ppShareInfo = pShareInfo;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pDbContext->mutex);

    return ntStatus;

error:

    *ppShareInfo = NULL;

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

    if (pDbContext->pShareCollection)
    {
        SMBRBTreeFree(pDbContext->pShareCollection);
    }

    SMB_UNLOCK_RWMUTEX(bInLock, &pDbContext->mutex);

    SrvShareDbShutdown(pDbContext);
}

NTSTATUS
SrvDevCtlAddShare(
    PBYTE lpInBuffer,
    DWORD dwInBuffer,
    PBYTE lpOutBuffer,
    DWORD dwOutBuffer
    )
{
    NTSTATUS ntStatus = 0;

#if 0

    ntStatus = SrvUnmarshallInBuffer_AddShare(
                    lpInBuffer,
                    dwInBuffer,
                    &pShareInfo,
                    &dwLevel
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareAddShare(
                    pShareInfo,
                    dwLevel
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshallOutBuffer_AddShare(
                        SRV_DEV_CTL_ADD_SHARE,
                        ntStatus
                        lpOutBuffer,
                        dwOutBuffer,
                        pdwBytesWritten
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

#endif

    return(ntStatus);
}

NTSTATUS
SrvDevCtlDeleteShare(
    PBYTE lpInBuffer,
    DWORD dwInBuffer,
    PBYTE lpOutBuffer,
    DWORD dwOutBuffer
    )
{
    NTSTATUS ntStatus = 0;

#if 0

    ntStatus = SrvUnmarshallInBuffer_DeleteShare(
                    lpInBuffer,
                    dwInBuffer,
                    &pShareInfo,
                    &dwLevel
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareDeleteShare(
                    lpszShareName
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshallOutBuffer_DeleteShare(
                        SRV_DEV_CTL_DELETE_SHARE,
                        ntStatus,
                        lpOutBuffer,
                        dwOutBuffer,
                        pdwBytesWritten
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

#endif

    return(ntStatus);
}

NTSTATUS
SrvDevCtlEnumShares(
    PBYTE lpInBuffer,
    DWORD dwInBuffer,
    PBYTE lpOutBuffer,
    DWORD dwOutBuffer
    )
{
    NTSTATUS ntStatus = 0;

#if 0

    ntStatus = SrvUnmarshallInBuffer_EnumShares(
                    lpInBuffer,
                    dwInBuffer,
                    &pShareInfo,
                    &dwLevel
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareEnumShares(
                    dwLevel
                    dwResumeHandle,
                    pOutBuffer,
                    dwOutBuffer,
                    &dwBytesWritten
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshallOutBuffer_EnumShares(
                        ntStatus,
                        lpOutBuffer,
                        dwOutBuffer,
                        pdwBytesWritten
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

#endif

    return(ntStatus);
}

NTSTATUS
SrvDevCtlSetShareInfo(
    PBYTE lpInBuffer,
    DWORD dwInBuffer,
    PBYTE lpOutBuffer,
    DWORD dwOutBuffer
    )
{
    NTSTATUS ntStatus = 0;

#if 0

    ntStatus = SrvMarshallInBuffer_SetShareInfo(
                    lpInBuffer,
                    dwInBuffer,
                    &pShareInfo,
                    &dwLevel
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareSetShareInfo(
                    pShareInfo,
                    dwLevel
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshallOutBuffer_SetShareInfo(
                        SRV_DEV_CTL_SET_SHARE_INFO,
                        ntStatus,
                        lpOutBuffer,
                        dwOutBuffer,
                        pdwBytesWritten
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

#endif

    return(ntStatus);
}

NTSTATUS
SrvDevCtlGetShareInfo(
    PBYTE lpInBuffer,
    DWORD dwInBuffer,
    PBYTE lpOutBuffer,
    DWORD dwOutBuffer
    )
{
    NTSTATUS ntStatus = 0;

#if 0

    ntStatus = SrvMarshallInBuffer_GetShareInfo(
                    lpInBuffer,
                    dwInBuffer,
                    &pShareInfo,
                    &dwLevel
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareGetShareInfo(
                    pShareInfo,
                    dwLevel
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshallOutBuffer_GetShareInfo(
                        SRV_DEV_CTL_GET_SHARE_INFO,
                        ntStatus,
                        lpOutBuffer,
                        dwOutBuffer,
                        pdwBytesWritten
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

#endif

    return(ntStatus);
}

#if 0
NTSTATUS
SrvFindShareinList(
    PWSTR pszShareName
    )
{
    PSRV_SHARE_ENTRY pShareEntry = NULL;

    while (pShareEntry) {

        if (IsEqualShareEntry(pShareName pShareEntry->pName)){

            *ppShareEntry = pShareEntry;
            return(ntStatus);
        }
        pShareEntry = pShareEntry->pNext
    }
    *ppShareEntry = NULL;
    return (ntStatus);
}
#endif

NTSTATUS
SrvShareAddShare(
    PWSTR  pwszShareName,
    DWORD  dwInfoLevel,
    PVOID  pBuffer
    )
{
    NTSTATUS ntStatus = 0;

#if 0

    ENTER_WRITER_LOCK();

    ntStatus = SrvFindShareinList(
                        pShareName,
                        &pShareEntry
                        );
    if (!ntStatus) {
        ntStatus = STATUS_OBJECT_EXISTS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pShareEntry->pNext = gpShareEntry;
    gpShareEntry = pShareEntry;

    ntStatus = SrvShareDbAdd(
                        hDb,
                        pShareEntry->pszShareName,
                        pShareEntry->pszPath,
                        pszComment
                        );

error:

    LEAVE_WRITER_LOCK();

#endif

    return(ntStatus);
}


NTSTATUS
SrvShareDeleteShare(
    PWSTR pszShareName
    )
{
    NTSTATUS ntStatus = 0;

#if 0

    ENTER_WRITER_LOCK();

    ntStatus = SrvFindShareinList(
                        pszShareName,
                        &ppShareEntry
                        );
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SrvShareDbDelete(
                        hDb,
                        pShareEntry->pszShareName
                        );
    BAIL_ON_NT_STATUS(ntStatus);


error:

    LEAvE_WRITER_LOCK();

#endif

    return(ntStatus);
}

NTSTATUS
SrvShareSetInfo(
    PWSTR pszShareName,
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

    ntStatus = SrvFindShareinList(
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
    PWSTR pszShareName,
    DWORD dwLevel,
    PBYTE pOutBuffer,
    DWORD dwOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;

#if 0

    ENTER_READER_LOCK();

    ntStatus = SrvFindShareinList(
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
    PBYTE pOutBuffer,
    DWORD dwOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;

#if 0

    ntStatus = ValidateServerSecurity(
                        hAccessToken,
                        GENERIC_READ,
                        pServerSD
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ENTER_READER_LOCK();

    ntStatus = SrvFindShareinList();

    ntStatus = SrvShareIndextoEntry(
                    dwResumeHandle,
                    &pShareEntry
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    while (pShareEntry) {


        switch(dwLevel) {

            case 0:
                ntStatus = GetBufferSize_L0(
                                pShareEntry,
                                &dwBufferSize
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                if (dwBufferRemaining <  dwBufferSize) {
                    //
                    // Insufficient Buffer
                    //
                }

                ntStatus = MarshallBuffer_L0(
                                pBuffer,
                                pCurrentOffset,
                                pShareEntry,
                                &pNewOffset,
                                &dwBufferRemaining
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                break;

            case 1:

               ntStatus = GetBufferSize_L1(
                                pShareEntry,
                                &dwBufferSize,
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                if (dwBufferRemaining < dwBufferSize) {
                    //
                    // Insufficient Buffer
                    //


                }
                ntStatus = MarshallBuffer_L1(
                                pBuffer,
                                pCurrentOffset,
                                pShareEntry,
                                &pNewOffset,
                                &dwBufferRemaining
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                break;

            case 2:

               ntStatus = GetBufferSize_L1(
                                pShareEntry,
                                &dwBufferSize,
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                if (dwBufferRemaining < dwBufferSize) {
                    //
                    // Insufficient Buffer
                    //


                }
                ntStatus = MarshallBuffer_L1(
                                pBuffer,
                                pCurrentOffset,
                                pShareEntry,
                                &pNewOffset,
                                &dwBufferRemaining
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                break;
                ntStatus = GetBufferSize_L2(
                                pShareEntry,

                 break;

            case 502:

               ntStatus = GetBufferSize_L1(
                                pShareEntry,
                                &dwBufferSize,
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                if (dwBufferRemaining < dwBufferSize) {
                    //
                    // Insufficient Buffer
                    //


                }
                ntStatus = MarshallBuffer_L1(
                                pBuffer,
                                pCurrentOffset,
                                pShareEntry,
                                &pNewOffset,
                                &dwBufferRemaining
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                break;
                break;

            case 503:
               ntStatus = GetBufferSize_L1(
                                pShareEntry,
                                &dwBufferSize,
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                if (dwBufferRemaining < dwBufferSize) {
                    //
                    // Insufficient Buffer
                    //


                }
                ntStatus = MarshallBuffer_L1(
                                pBuffer,
                                pCurrentOffset,
                                pShareEntry,
                                &pNewOffset,
                                &dwBufferRemaining
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                break;
        }
        pShareEntry   pShareEntry->pNext;
        dwResumeHandle++;
    }


error:

    LEAVE_READER_LOCK();

#endif

    return(ntStatus);
}

#if 0

NTSTATUS
SrvCreateShareEntry(
    PWSTR pszShareName,
    PWSTR pszPathName,
    PWSTR pszComment,
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

