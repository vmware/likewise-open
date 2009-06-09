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
 *        checkdir.c
 *
 * Abstract:
 *
 *        Likewise SMB Server
 *
 *        SMBCheckDirectory
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */


#include "includes.h"

NTSTATUS
SrvProcessCheckDirectory(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE    pTree = NULL;
    PSMB_CHECK_DIRECTORY_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PWSTR       pwszPathFragment = NULL; // Do not free
    ULONG       ulOffset = 0;
    PSMB_CHECK_DIRECTORY_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    USHORT      usPacketByteCount = 0;
    PWSTR       pwszFilesystemPath = NULL;
    PSMB_PACKET pSmbResponse = NULL;
    BOOLEAN     bInLock = FALSE;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PVOID           pSecurityDescriptor = NULL;
    PVOID           pSecurityQOS = NULL;
    IO_FILE_NAME    fileName = {0};
    PIO_ASYNC_CONTROL_BLOCK     pAsyncControlBlock = NULL;

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

    ntStatus = WireUnmarshallCheckDirectoryRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    ulOffset,
                    &pRequestHeader,
                    &pwszPathFragment);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pwszPathFragment || !*pwszPathFragment)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->pShareInfo->mutex);

    ntStatus = SrvBuildFilePath(
                    pTree->pShareInfo->pwszPath,
                    pwszPathFragment,
                    &pwszFilesystemPath);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    fileName.FileName = pwszFilesystemPath;

    ntStatus = IoCreateFile(
                    &hFile,
                    pAsyncControlBlock,
                    &ioStatusBlock,
                    pSession->pIoSecurityContext,
                    &fileName,
                    pSecurityDescriptor,
                    pSecurityQOS,
                    GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE,
                    0, /* allocation size */
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    FILE_OPEN,
                    FILE_DIRECTORY_FILE,
                    NULL, /* EA Buffer */
                    0,    /* EA Length */
                    NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketAllocate(
                    SrvTransportGetAllocator(pConnection),
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    SrvTransportGetAllocator(pConnection),
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                    pSmbResponse->pRawBuffer,
                    pSmbResponse->bufferLen,
                    COM_CHECK_DIRECTORY,
                    0,
                    TRUE,
                    pSmbRequest->pSMBHeader->tid,
                    pSmbRequest->pSMBHeader->pid,
                    pSmbRequest->pSMBHeader->uid,
                    pSmbRequest->pSMBHeader->mid,
                    pConnection->serverProperties.bRequireSecuritySignatures,
                    pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallCheckDirectoryResponse(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    &pResponseHeader,
                    &usPacketByteCount);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usPacketByteCount;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (pTree)
    {
        LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pwszFilesystemPath)
    {
        LwRtlMemoryFree(pwszFilesystemPath);
    }

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            SrvTransportGetAllocator(pConnection),
            pSmbResponse);
    }

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
