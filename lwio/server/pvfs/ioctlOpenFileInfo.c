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
 *        ioctlOpenFileInfo.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Device I/O Control handler
 *        Open File Information
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/***********************************************************************
 **********************************************************************/

static NTSTATUS
PvfsFillOpenFileInfo(
    OUT PVOID pBuffer,
    IN  ULONG BufLen,
    OUT PULONG BytesConsumed,
    IN  ULONG Level
    );

NTSTATUS
PvfsIoCtlOpenFileInfo(
    IN     PPVFS_IRP_CONTEXT pIrpContext,
    IN     PVOID  InputBuffer,
    IN     ULONG  InputBufferLength,
    OUT    PVOID  OutputBuffer,
    IN OUT PULONG pOutputBufferLength
    )
{
    NTSTATUS ntError =  STATUS_UNSUCCESSFUL;
    PIO_OPEN_FILE_INFO_INPUT_BUFFER pOpenFileInfoInput = NULL;
    PPVFS_CCB pCcb = NULL;
    PIRP pIrp = pIrpContext->pIrp;
    ULONG BytesConsumed = 0;


    ntError = PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* Sanity checks */

    BAIL_ON_INVALID_PTR(InputBuffer, ntError);
    BAIL_ON_INVALID_PTR(OutputBuffer, ntError);

    if (InputBufferLength < sizeof(IO_OPEN_FILE_INFO_INPUT_BUFFER))
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    pOpenFileInfoInput = (PIO_OPEN_FILE_INFO_INPUT_BUFFER)InputBuffer;

    switch(pOpenFileInfoInput->Level)
    {
    case 0:
    case 100:
        ntError = PvfsFillOpenFileInfo(
                      OutputBuffer,
                      *pOutputBufferLength,
                      &BytesConsumed,
                      pOpenFileInfoInput->Level);
        break;

    default:
        ntError = STATUS_INVALID_INFO_CLASS;
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

    *pOutputBufferLength = BytesConsumed;

cleanup:
    if (pCcb)
    {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsOpenFileInfo(
    PVOID pKey,
    PVOID pData,
    PVOID pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
PvfsFillOpenFileInfo(
    OUT PVOID pBuffer,
    IN  ULONG BufLen,
    OUT PULONG pBytesConsumed,
    IN  ULONG Level
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PVFS_OPEN_FILE_INFO OpenFileInfo = {0};
    BOOLEAN bLocked = FALSE;
    DWORD dwIndex = 0;

    LWIO_LOCK_RWMUTEX_SHARED(bLocked, &gFcbTable.rwLock);

    /* Setup the traversal structure.  Pass in the PVOID output buffer
       we were originally given */

    OpenFileInfo.Level = Level;
    OpenFileInfo.BytesAvailable = BufLen;
    OpenFileInfo.Offset = 0;
    OpenFileInfo.pData = pBuffer;
    OpenFileInfo.pPreviousEntry = NULL;

    for (dwIndex=0; dwIndex<gFcbTable.pFcbTable->sTableSize; dwIndex++)
    {
        if (gFcbTable.pFcbTable->ppEntries[dwIndex] == NULL)
        {
            continue;
        }

        ntError = LwRtlRBTreeTraverse(
                      gFcbTable.pFcbTable->ppEntries[dwIndex]->pTree,
                      LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                      PvfsOpenFileInfo,
                      &OpenFileInfo);
        BAIL_ON_NT_STATUS(ntError);
    }

    *pBytesConsumed = BufLen - OpenFileInfo.BytesAvailable;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bLocked, &gFcbTable.rwLock);

    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsFillOpenFileInfo0(
    PVOID pBuffer,
    ULONG BufferLength,
    PVOID pPreviousEntry,
    PPVFS_FCB pFcb,
    PULONG pBytesUsed
    );

static
NTSTATUS
PvfsFillOpenFileInfo100(
    PVOID pBuffer,
    ULONG BufferLength,
    PVOID pPreviousEntry,
    PPVFS_FCB pFcb,
    PULONG pBytesUsed
    );

static
NTSTATUS
PvfsOpenFileInfo(
    PVOID pKey,
    PVOID pData,
    PVOID pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = (PPVFS_FCB)pData;
    PPVFS_OPEN_FILE_INFO pOpenFileInfo = (PPVFS_OPEN_FILE_INFO)pUserData;
    ULONG BytesUsed = 0;

    switch(pOpenFileInfo->Level)
    {
    case 0:
        ntError = PvfsFillOpenFileInfo0(
                      pOpenFileInfo->pData + pOpenFileInfo->Offset,
                      pOpenFileInfo->BytesAvailable,
                      pOpenFileInfo->pPreviousEntry,
                      pFcb,
                      &BytesUsed);
        break;

    case 100:
        ntError = PvfsFillOpenFileInfo100(
                      pOpenFileInfo->pData + pOpenFileInfo->Offset,
                      pOpenFileInfo->BytesAvailable,
                      pOpenFileInfo->pPreviousEntry,
                      pFcb,
                      &BytesUsed);
        break;

    default:
        ntError = STATUS_INVALID_INFO_CLASS;
    }
    BAIL_ON_NT_STATUS(ntError);

    pOpenFileInfo->pPreviousEntry  = pOpenFileInfo->pData
                                     + pOpenFileInfo->Offset;

    pOpenFileInfo->BytesAvailable -= BytesUsed;
    pOpenFileInfo->Offset         += BytesUsed;

    *pbContinue = TRUE;

cleanup:
    return ntError;

error:
    *pbContinue = FALSE;

    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsFillOpenFileInfo0(
    PVOID pBuffer,
    ULONG BufferLength,
    PVOID pPreviousEntry,
    PPVFS_FCB pFcb,
    PULONG pBytesUsed
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIO_OPEN_FILE_INFO_0 pInfo0 = (PIO_OPEN_FILE_INFO_0)pBuffer;
    PIO_OPEN_FILE_INFO_0 pPrev = (PIO_OPEN_FILE_INFO_0)pPreviousEntry;
    PWSTR pwszFilename = NULL;
    ULONG FilenameByteCount = 0;
    BOOLEAN bControlLocked = FALSE;
    BOOLEAN bCcbListLocked = FALSE;

    LWIO_LOCK_MUTEX(bControlLocked, &pFcb->ControlBlock);
    ntError = LwRtlWC16StringAllocateFromCString(
                  &pwszFilename,
                  pFcb->pszFilename);
    LWIO_UNLOCK_MUTEX(bControlLocked, &pFcb->ControlBlock);
    BAIL_ON_NT_STATUS(ntError);

    FilenameByteCount = (LwRtlWC16StringNumChars(pwszFilename)+1) *
                        sizeof(WCHAR);

    if (BufferLength < (sizeof(IO_OPEN_FILE_INFO_0)+FilenameByteCount))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pInfo0->NextEntryOffset = 0;
    pInfo0->FileNameLength = FilenameByteCount;
    memcpy(pInfo0->pwszFileName, pwszFilename, FilenameByteCount);

    LWIO_LOCK_RWMUTEX_SHARED(bCcbListLocked, &pFcb->rwCcbLock);
    pInfo0->OpenHandleCount = PvfsListLength(pFcb->pCcbList);
    LWIO_UNLOCK_RWMUTEX(bCcbListLocked, &pFcb->rwCcbLock);

    if (pPrev)
    {
        pPrev->NextEntryOffset = PVFS_PTR_DIFF(pPreviousEntry, pBuffer);
    }

    *pBytesUsed = sizeof(IO_OPEN_FILE_INFO_0)
                  + FilenameByteCount
                  - sizeof(WCHAR);

cleanup:
    LwRtlWC16StringFree(&pwszFilename);

    return ntError;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsFillOpenFileInfo100(
    PVOID pBuffer,
    ULONG BufferLength,
    PVOID pPreviousEntry,
    PPVFS_FCB pFcb,
    PULONG pBytesUsed
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIO_OPEN_FILE_INFO_100 pInfo100 = (PIO_OPEN_FILE_INFO_100)pBuffer;
    PIO_OPEN_FILE_INFO_100 pPrev = (PIO_OPEN_FILE_INFO_100)pPreviousEntry;
    PWSTR pwszFilename = NULL;
    ULONG FilenameByteCount = 0;
    BOOLEAN bControlLocked = FALSE;
    BOOLEAN bCcbListLocked = FALSE;

    LWIO_LOCK_MUTEX(bControlLocked, &pFcb->ControlBlock);
    ntError = LwRtlWC16StringAllocateFromCString(
                  &pwszFilename,
                  pFcb->pszFilename);
    BAIL_ON_NT_STATUS(ntError);
    LWIO_UNLOCK_MUTEX(bControlLocked, &pFcb->ControlBlock);

    FilenameByteCount = (LwRtlWC16StringNumChars(pwszFilename)+1) *
                        sizeof(WCHAR);

    if (BufferLength < (sizeof(IO_OPEN_FILE_INFO_100)+FilenameByteCount))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pInfo100->bDeleteOnClose = PvfsFcbIsPendingDelete(pFcb);

    pInfo100->NextEntryOffset = 0;
    pInfo100->FileNameLength = FilenameByteCount;
    memcpy(pInfo100->pwszFileName, pwszFilename, FilenameByteCount);

    LWIO_LOCK_RWMUTEX_SHARED(bCcbListLocked, &pFcb->rwCcbLock);
    pInfo100->OpenHandleCount = PvfsListLength(pFcb->pCcbList);
    LWIO_UNLOCK_RWMUTEX(bCcbListLocked, &pFcb->rwCcbLock);

    if (pPrev)
    {
        pPrev->NextEntryOffset = PVFS_PTR_DIFF(pPreviousEntry, pBuffer);
    }

    *pBytesUsed = sizeof(IO_OPEN_FILE_INFO_100)
                  + FilenameByteCount
                  - sizeof(WCHAR);

cleanup:
    LWIO_UNLOCK_MUTEX(bControlLocked, &pFcb->ControlBlock);

    LwRtlWC16StringFree(&pwszFilename);

    return ntError;

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
