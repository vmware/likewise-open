/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * */

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
 *        files.c
 *
 * Abstract:
 *
 *        Likewise File System Driver (Srv)
 *
 *        DeviceIo Dispatch Routine
 *
 *        File Management
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

NTSTATUS
SrvDevCtlEnumerateFiles(
    IN     PBYTE  pInBuffer,
    IN     ULONG  ulInBufferSize,
    IN OUT PBYTE  pOutBuffer,
    IN     ULONG  ulOutBufferSize,
    IN OUT PULONG pulBytesTransferred
    )
{
    NTSTATUS ntStatus         = STATUS_SUCCESS;
    PBYTE    pBuffer          = pOutBuffer;
    ULONG    ulBufferSize     = ulOutBufferSize;
    ULONG    ulPreambleSize   = 0;
    ULONG    ulBytesUsed      = 0;
    ULONG    ulTotalBytesUsed = 0;
    ULONG    ulEntriesRead    = 0;
    ULONG    ulTotalEntries   = 0;
    ULONG    ulResumeHandle   = 0;
    PULONG   pulResumeHandle  = NULL;
    PFILE_INFO_ENUM_IN_PARAMS   pParamsIn         = NULL;
    FILE_INFO_ENUM_OUT_PREAMBLE paramsOutPreamble = {0};

    ntStatus = LwFileInfoUnmarshalEnumInputParameters(
                        pInBuffer,
                        ulInBufferSize,
                        &pParamsIn);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pParamsIn->pdwResumeHandle)
    {
        ulResumeHandle = *pParamsIn->pdwResumeHandle;
        pulResumeHandle = &ulResumeHandle;
    }

    paramsOutPreamble.dwInfoLevel    = pParamsIn->dwInfoLevel;
    paramsOutPreamble.dwEntriesRead  = ulEntriesRead;
    paramsOutPreamble.dwTotalEntries = ulTotalEntries;
    if (pulResumeHandle)
    {
        paramsOutPreamble.pdwResumeHandle = &ulResumeHandle;
    }

    ntStatus = LwFileInfoMarshalEnumOutputPreamble(
                    pBuffer,
                    ulBufferSize,
                    &paramsOutPreamble,
                    &ulPreambleSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pBuffer          += ulPreambleSize;
    ulBufferSize     -= ulPreambleSize;
    ulTotalBytesUsed += ulPreambleSize;

    ntStatus = SrvProtocolEnumerateFiles(
                    pParamsIn->pwszBasepath,
                    pParamsIn->pwszUsername,
                    pParamsIn->dwInfoLevel,
                    pBuffer,
                    ulBufferSize,
                    &ulBytesUsed,
                    &ulEntriesRead,
                    &ulTotalEntries,
                    pulResumeHandle);
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;

    paramsOutPreamble.dwEntriesRead  = ulEntriesRead;
    paramsOutPreamble.dwTotalEntries = ulTotalEntries;

    ntStatus = LwFileInfoMarshalEnumOutputPreamble(
                    pOutBuffer,
                    ulPreambleSize,
                    &paramsOutPreamble,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesTransferred = ulTotalBytesUsed;

cleanup:

    if (pParamsIn)
    {
        LwFileInfoFreeEnumInputParameters(pParamsIn);
    }

    return ntStatus;

error:

    if (ulOutBufferSize)
    {
        memset(pOutBuffer, 0, ulOutBufferSize);
    }

    *pulBytesTransferred = 0;

    goto cleanup;
}

NTSTATUS
SrvDevCtlGetFileInfo(
    IN     PBYTE  pInBuffer,
    IN     ULONG  ulInBufferSize,
    IN OUT PBYTE  pOutBuffer,
    IN     ULONG  ulOutBufferSize,
    IN OUT PULONG pulBytesTransferred
    )
{
    NTSTATUS ntStatus        = STATUS_SUCCESS;
    ULONG    ulBytesUsed     = 0;
    PFILE_INFO_GET_INFO_IN_PARAMS pParamsIn = NULL;

    ntStatus = LwFileInfoUnmarshalGetInfoInParameters(
                        pInBuffer,
                        ulInBufferSize,
                        &pParamsIn);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProtocolGetFileInfo(
                        pParamsIn->dwInfoLevel,
                        pParamsIn->dwFileId,
                        pOutBuffer,
                        ulOutBufferSize,
                        &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesTransferred = ulBytesUsed;

cleanup:

    if (pParamsIn)
    {
        LwFileInfoFreeGetInfoInParameters(pParamsIn);
    }

    return ntStatus;

error:

    if (ulOutBufferSize)
    {
        memset(pOutBuffer, 0, ulOutBufferSize);
    }
    *pulBytesTransferred = 0;

    goto cleanup;
}

NTSTATUS
SrvDevCtlCloseFile(
    IN     PBYTE  pInBuffer,
    IN     ULONG  ulInBufferSize,
    IN OUT PBYTE  pOutBuffer,
    IN     ULONG  ulOutBufferSize,
    IN OUT PULONG pulBytesTransferred
    )
{
    NTSTATUS ntStatus        = STATUS_SUCCESS;
    PBYTE    pBuffer         = NULL;
    ULONG    ulBufferSize    = 0;
    PFILE_INFO_CLOSE_PARAMS pParamsIn = NULL;
    FILE_INFO_CLOSE_PARAMS  paramsOut = {0};

    ntStatus = LwFileInfoUnmarshalCloseParameters(
                        pInBuffer,
                        ulInBufferSize,
                        &pParamsIn);
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO: close file by id
    ntStatus = STATUS_NOT_SUPPORTED;
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwFileInfoMarshalCloseParameters(
                        &paramsOut,
                        &pBuffer,
                        &ulBufferSize);
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesTransferred = ulBufferSize;

cleanup:

    if (pBuffer)
    {
        SrvFreeMemory(pBuffer);
    }
    if (pParamsIn)
    {
        SrvFreeMemory(pParamsIn);
    }

    return ntStatus;

error:

    if (ulOutBufferSize)
    {
        memset(pOutBuffer, 0, ulOutBufferSize);
    }
    *pulBytesTransferred = 0;

    goto cleanup;
}
