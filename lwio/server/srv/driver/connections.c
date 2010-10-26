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
 *        connections.c
 *
 * Abstract:
 *
 *        Likewise File System Driver (Srv)
 *
 *        DeviceIo Dispatch Routine
 *
 *        Connection Management
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

NTSTATUS
SrvDevCtlEnumerateConnections(
    IN     PBYTE  pInBuffer,
    IN     ULONG  ulInBufferSize,
    IN OUT PBYTE  pOutBuffer,
    IN     ULONG  ulOutBufferSize,
    IN OUT PULONG pulBytesTransferred
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pBuffer = pOutBuffer;
    ULONG ulBufferSize = ulOutBufferSize;
    ULONG ulPreambleSize = 0;
    ULONG ulBytesUsed = 0;
    ULONG ulTotalBytesUsed = 0;
    ULONG ulEntriesRead = 0;
    ULONG ulTotalEntries = 0;
    ULONG ulResumeHandle = 0;
    PCONNECTION_INFO_ENUM_IN_PARAMS pParamsIn = NULL;
    CONNECTION_INFO_ENUM_OUT_PARAMS ParamsOut = {0};
    wchar16_t wszClientPrefix[] = {'\\', '\\', 0};
    ULONG ulClientPrefixLen =
                (sizeof(wszClientPrefix)/sizeof(wszClientPrefix[0])) - 1;
    PWSTR pwszComputerName = NULL;
    PWSTR pwszShareName = NULL;
    BOOLEAN bMoreData = FALSE;

    ntStatus = LwConnectionInfoUnmarshalEnumInputParameters(
                        pInBuffer,
                        ulInBufferSize,
                        &pParamsIn);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pParamsIn->pwszQualifier)
    {
        if (!SMBWc16snCmp(pParamsIn->pwszQualifier,
			  &wszClientPrefix[0],
			  ulClientPrefixLen))
        {
            /*
             * If the qualifier starts with "\\" then it's obviously
             * UNC computer name
             */
            pwszComputerName = pParamsIn->pwszQualifier + ulClientPrefixLen;
        }
        else
        {
            pwszShareName = pParamsIn->pwszQualifier;
        }
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulResumeHandle = pParamsIn->dwResume;

    /*
     * Marshal the preamble so pBuffer is positioned where
     * connection info buffers start
     */
    ntStatus = LwConnectionInfoMarshalEnumOutputParameters(
                        &ParamsOut,
                        pBuffer,
                        ulBufferSize,
                        &ulPreambleSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pBuffer          += ulPreambleSize;
    ulBufferSize     -= ulPreambleSize;
    ulTotalBytesUsed += ulPreambleSize;

    ntStatus = SrvProtocolEnumerateConnections(
                        pwszComputerName,
                        pwszShareName,
                        pParamsIn->dwLevel,
                        pParamsIn->dwPreferredMaxLen,
                        pBuffer,
                        ulBufferSize,
                        &ulBytesUsed,
                        &ulEntriesRead,
                        &ulTotalEntries,
                        &ulResumeHandle);
    if (ntStatus == STATUS_MORE_ENTRIES)
    {
        bMoreData = TRUE;
        ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;

    ParamsOut.dwLevel           = pParamsIn->dwLevel;
    ParamsOut.dwNumEntries      = ulEntriesRead;
    ParamsOut.dwTotalNumEntries = ulTotalEntries;
    ParamsOut.dwResume          = ulResumeHandle;

    /*
     * Marshal the actual output buffer again after the number
     * of returned entries is known
     */
    ntStatus = LwConnectionInfoMarshalEnumOutputParameters(
                        &ParamsOut,
                        pOutBuffer,
                        ulBufferSize,
                        &ulPreambleSize);
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesTransferred = ulTotalBytesUsed;

cleanup:
    if (pParamsIn)
    {
        LwConnectionInfoFreeEnumInputParameters(pParamsIn);
    }

    return (NT_SUCCESS(ntStatus) && bMoreData ? STATUS_MORE_ENTRIES : ntStatus);

error:
    if (ulOutBufferSize)
    {
        memset(pOutBuffer, 0, ulOutBufferSize);
    }

    *pulBytesTransferred = 0;

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

