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
 *        sessions.c
 *
 * Abstract:
 *
 *        Likewise File System Driver (Srv)
 *
 *        DeviceIo Dispatch Routine
 *
 *        Session Management
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

NTSTATUS
SrvDevCtlEnumerateSessions(
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
    PSESSION_INFO_ENUM_IN_PARAMS   pParamsIn         = NULL;
    SESSION_INFO_ENUM_OUT_PREAMBLE paramsOutPreamble = {0};
    wchar16_t wszClientPrefix[] = {'\\', '\\', 0};
    ULONG     ulClientPrefixLen =
                (sizeof(wszClientPrefix)/sizeof(wszClientPrefix[0])) - 1;
    PWSTR     pwszUncClientname = NULL;

    ntStatus = LwSessionInfoUnmarshalEnumInputParameters(
                        pInBuffer,
                        ulInBufferSize,
                        &pParamsIn);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pParamsIn->pdwResumeHandle)
    {
        ulResumeHandle = *pParamsIn->pdwResumeHandle;
        pulResumeHandle = &ulResumeHandle;
    }

    if (pParamsIn->pwszUncClientname)
    {
        if (!SMBWc16snCmp(  pParamsIn->pwszUncClientname,
                            &wszClientPrefix[0],
                            ulClientPrefixLen))
        {
            pwszUncClientname =
                        pParamsIn->pwszUncClientname + ulClientPrefixLen;
        }
        else
        {
            ntStatus = STATUS_INVALID_COMPUTER_NAME;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    paramsOutPreamble.dwInfoLevel    = pParamsIn->dwInfoLevel;
    paramsOutPreamble.dwEntriesRead  = ulEntriesRead;
    paramsOutPreamble.dwTotalEntries = ulTotalEntries;
    if (pulResumeHandle)
    {
        paramsOutPreamble.pdwResumeHandle = &ulResumeHandle;
    }

    ntStatus = LwSessionInfoMarshalEnumOutputPreamble(
                    pBuffer,
                    ulBufferSize,
                    &paramsOutPreamble,
                    &ulPreambleSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pBuffer          += ulPreambleSize;
    ulBufferSize     -= ulPreambleSize;
    ulTotalBytesUsed += ulPreambleSize;

    ntStatus = SrvProtocolEnumerateSessions(
                    pwszUncClientname,
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

    ntStatus = LwSessionInfoMarshalEnumOutputPreamble(
                    pOutBuffer,
                    ulPreambleSize,
                    &paramsOutPreamble,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesTransferred = ulTotalBytesUsed;

cleanup:

    if (pParamsIn)
    {
        LwSessionInfoFreeEnumInputParameters(pParamsIn);
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
SrvDevCtlDeleteSession(
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
    PSESSION_INFO_DELETE_PARAMS pParamsIn = NULL;
    SESSION_INFO_DELETE_PARAMS  paramsOut = {0};

    ntStatus = LwSessionInfoUnmarshalDeleteParameters(
                        pInBuffer,
                        ulInBufferSize,
                        &pParamsIn);
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO: execute deletion
    ntStatus = STATUS_NOT_SUPPORTED;
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwSessionInfoMarshalDeleteParameters(
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
