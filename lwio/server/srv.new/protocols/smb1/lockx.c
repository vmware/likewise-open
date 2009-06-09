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
NTSTATUS
SrvExecuteLargeFileLocks(
    PLWIO_SRV_FILE                    pFile,
    PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader,
    PLOCKING_ANDX_RANGE_LARGE_FILE   pUnlockRangeLarge,
    PLOCKING_ANDX_RANGE_LARGE_FILE   pLockRangeLarge
    );

static
NTSTATUS
SrvExecuteLocks(
    PLWIO_SRV_FILE                    pFile,
    PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader,
    PLOCKING_ANDX_RANGE              pUnlockRange,
    PLOCKING_ANDX_RANGE              pLockRange
    );

static
NTSTATUS
SrvUnlockFile(
    PLWIO_SRV_FILE       pFile,
    PLOCKING_ANDX_RANGE pLockInfo,
    ULONG               ulKey
    );

static
NTSTATUS
SrvUnlockLargeFile(
    PLWIO_SRV_FILE                  pFile,
    PLOCKING_ANDX_RANGE_LARGE_FILE pLockInfo,
    ULONG                          ulKey
    );

static
NTSTATUS
SrvBuildLockingAndXResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessLockAndX(
	IN  PLWIO_SRV_CONNECTION pConnection,
	IN  PSMB_PACKET          pSmbRequest,
	OUT PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE pTree = NULL;
    PLWIO_SRV_FILE pFile = NULL;
    PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader = NULL;  // Do not free
    PLOCKING_ANDX_RANGE              pUnlockRange = NULL;    // Do not free
    PLOCKING_ANDX_RANGE_LARGE_FILE   pUnlockRangeLarge = NULL; // Do not free
    PLOCKING_ANDX_RANGE              pLockRange = NULL;      // Do not free
    PLOCKING_ANDX_RANGE_LARGE_FILE   pLockRangeLarge = NULL; // Do not free
    PSMB_PACKET pSmbResponse = NULL;
    ULONG ulOffset = 0;

    ntStatus = SrvConnectionFindSession(
                    pConnection,
                    pSmbRequest->pSMBHeader->uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionFindTree(
                    pSession,
                    pSmbRequest->pSMBHeader->tid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ulOffset = (PBYTE)pSmbRequest->pParams - (PBYTE)pSmbRequest->pSMBHeader;

    ntStatus = WireUnmarshallLockingAndXRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    ulOffset,
                    &pRequestHeader,
                    &pUnlockRange,
                    &pUnlockRangeLarge,
                    &pLockRange,
                    &pLockRangeLarge);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeFindFile(
                    pTree,
                    pRequestHeader->usFid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pRequestHeader->ucLockType & LWIO_LOCK_TYPE_LARGE_FILES)
    {
        ntStatus = SrvExecuteLargeFileLocks(
                        pFile,
                        pRequestHeader,
                        pUnlockRangeLarge,
                        pLockRangeLarge);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = SrvExecuteLocks(
                        pFile,
                        pRequestHeader,
                        pUnlockRange,
                        pLockRange);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvBuildLockingAndXResponse(
                    pConnection,
                    pSmbRequest,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (pFile)
    {
        SrvFileRelease(pFile);
    }
    if (pTree)
    {
        SrvTreeRelease(pTree);
    }
    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteLargeFileLocks(
    PLWIO_SRV_FILE                    pFile,
    PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader,
    PLOCKING_ANDX_RANGE_LARGE_FILE   pUnlockRangeLarge,
    PLOCKING_ANDX_RANGE_LARGE_FILE   pLockRangeLarge
    )
{
    NTSTATUS ntStatus = 0;
    USHORT   iLock = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    LONG64   llOffset = 0;
    LONG64   llLength = 0;
    ULONG    ulKey = 0;
    BOOLEAN  bFailImmediately = TRUE;
    BOOLEAN  bExclusiveLock = FALSE;
    PLOCKING_ANDX_RANGE_LARGE_FILE* ppLockStateArray = NULL;
    USHORT   usLocked = 0;

    if ((pRequestHeader->usNumUnlocks && !pUnlockRangeLarge) ||
        (pRequestHeader->usNumLocks && !pLockRangeLarge))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    for (iLock = 0; iLock < pRequestHeader->usNumUnlocks; iLock++)
    {
        PLOCKING_ANDX_RANGE_LARGE_FILE pLockInfo = &pUnlockRangeLarge[iLock];

        ntStatus = SrvUnlockLargeFile(
                        pFile,
                        pLockInfo,
                        ulKey);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pRequestHeader->usNumLocks)
    {
        ntStatus = LW_RTL_ALLOCATE(
                        &ppLockStateArray,
                        PLOCKING_ANDX_RANGE_LARGE_FILE,
                        sizeof(PLOCKING_ANDX_RANGE_LARGE_FILE) * pRequestHeader->usNumLocks);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    for (iLock = 0; iLock < pRequestHeader->usNumLocks; iLock++)
    {
        PLOCKING_ANDX_RANGE_LARGE_FILE pLockInfo = &pLockRangeLarge[iLock];

        llOffset = (((LONG64)pLockInfo->ulOffsetHigh) << 32) | ((LONG64)pLockInfo->ulOffsetLow);

        llLength = (((LONG64)pLockInfo->ulLengthHigh) << 32) | ((LONG64)pLockInfo->ulLengthLow);

        bExclusiveLock = !(pRequestHeader->ucLockType & LWIO_LOCK_TYPE_SHARED_LOCK);

        ntStatus = IoLockFile(
                        pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        llOffset,
                        llLength,
                        ulKey,
                        bFailImmediately,
                        bExclusiveLock);
        BAIL_ON_NT_STATUS(ntStatus);

        ppLockStateArray[iLock] = pLockInfo;
    }

cleanup:

    if (ppLockStateArray)
    {
        LwRtlMemoryFree(ppLockStateArray);
    }

    return ntStatus;

error:

    for (iLock = 0; iLock < usLocked; iLock++)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvUnlockLargeFile(
                        pFile,
                        ppLockStateArray[iLock],
                        ulKey);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to unlock large file [fid: %u] [code: %d]", pFile->fid, ntStatus2);
        }
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteLocks(
    PLWIO_SRV_FILE                    pFile,
    PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader,
    PLOCKING_ANDX_RANGE              pUnlockRange,
    PLOCKING_ANDX_RANGE              pLockRange
    )
{
    NTSTATUS ntStatus = 0;
    USHORT   iLock = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    ULONG ulKey = 0;
    BOOLEAN bFailImmediately = TRUE;
    BOOLEAN bExclusiveLock = FALSE;
    PLOCKING_ANDX_RANGE* ppLockStateArray = NULL;
    USHORT  usLocked = 0;

    if ((pRequestHeader->usNumUnlocks && !pUnlockRange) ||
        (pRequestHeader->usNumLocks && !pLockRange))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    for (iLock = 0; iLock < pRequestHeader->usNumUnlocks; iLock++)
    {
        PLOCKING_ANDX_RANGE pLockInfo = &pUnlockRange[iLock];

        ntStatus = SrvUnlockFile(
                        pFile,
                        pLockInfo,
                        ulKey);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pRequestHeader->usNumLocks)
    {
        ntStatus = LW_RTL_ALLOCATE(
                        &ppLockStateArray,
                        PLOCKING_ANDX_RANGE,
                        sizeof(PLOCKING_ANDX_RANGE) * pRequestHeader->usNumLocks);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    for (iLock = 0; iLock < pRequestHeader->usNumLocks; iLock++)
    {
        PLOCKING_ANDX_RANGE pLockInfo = &pLockRange[iLock];

        bExclusiveLock = !(pRequestHeader->ucLockType & LWIO_LOCK_TYPE_SHARED_LOCK);

        ntStatus = IoLockFile(
                        pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        pLockInfo->ulOffset,
                        pLockInfo->ulLength,
                        ulKey,
                        bFailImmediately,
                        bExclusiveLock);
        BAIL_ON_NT_STATUS(ntStatus);

        ppLockStateArray[usLocked++] = pLockInfo;
    }

cleanup:

    if (ppLockStateArray)
    {
        LwRtlMemoryFree(ppLockStateArray);
    }

    return ntStatus;

error:

    for (iLock = 0; iLock < usLocked; iLock++)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvUnlockFile(
                        pFile,
                        ppLockStateArray[iLock],
                        ulKey);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to unlock file [fid: %u] [code: %d]", pFile->fid, ntStatus2);
        }
    }

    goto cleanup;
}

static
NTSTATUS
SrvUnlockFile(
    PLWIO_SRV_FILE       pFile,
    PLOCKING_ANDX_RANGE pLockInfo,
    ULONG               ulKey
    )
{
    NTSTATUS ntStatus = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};

    ntStatus = IoUnlockFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    pLockInfo->ulOffset,
                    pLockInfo->ulLength,
                    ulKey);

    return ntStatus;
}

static
NTSTATUS
SrvUnlockLargeFile(
    PLWIO_SRV_FILE                  pFile,
    PLOCKING_ANDX_RANGE_LARGE_FILE pLockInfo,
    ULONG                          ulKey
    )
{
    NTSTATUS ntStatus = 0;
    LONG64   llOffset = 0;
    LONG64   llLength = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};

    llOffset = (((LONG64)pLockInfo->ulOffsetHigh) << 32) | ((LONG64)pLockInfo->ulOffsetLow);

    llLength = (((LONG64)pLockInfo->ulLengthHigh) << 32) | ((LONG64)pLockInfo->ulLengthLow);

    ntStatus = IoUnlockFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    llOffset,
                    llLength,
                    ulKey);

    return ntStatus;
}

static
NTSTATUS
SrvBuildLockingAndXResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_LOCKING_ANDX_RESPONSE_HEADER pResponseHeader = NULL;
    USHORT usNumPackageBytesUsed = 0;
    PSMB_PACKET pSmbResponse = NULL;

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_LOCKING_ANDX,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 2;

    ntStatus = WireMarshallLockingAndXResponse(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    &pResponseHeader,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponseHeader->usByteCount = 0;

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

