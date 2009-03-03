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
WireUnmarshallNtTransactionSetupData(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    ULONG       ulParameterOffset,
    ULONG       ulDataOffset,
    PUSHORT*    ppSetup,
    UCHAR       ucSetupLen,
    PBYTE*      ppParameters,
    USHORT      parameterLen,
    PBYTE*      ppData,
    USHORT      dataLen
    );

static
NTSTATUS
WireUnmarshallNtTransactionParameterData(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    ULONG       ulParameterOffset,
    ULONG       ulDataOffset,
    PBYTE*      ppParameters,
    ULONG       parameterLen,
    PBYTE*      ppData,
    ULONG       dataLen
    );

NTSTATUS
WireUnmarshallNtTransactionRequest(
    const PBYTE                     pBuffer,
    ULONG                           ulNumBytesAvailable,
    ULONG                           ulOffset,
    PNT_TRANSACTION_REQUEST_HEADER* ppHeader,
    PUSHORT*                        ppSetup,
    PUSHORT*                        ppByteCount,
    PBYTE*                          ppParameters,
    PBYTE*                          ppData
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pDataCursor = pBuffer;
    PNT_TRANSACTION_REQUEST_HEADER pHeader = NULL;
    PUSHORT pSetup = NULL;
    PUSHORT pByteCount = NULL;
    PBYTE   pParameters = NULL;
    PBYTE   pData = NULL;

    if (ulNumBytesAvailable < sizeof(NT_TRANSACTION_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PNT_TRANSACTION_REQUEST_HEADER) pDataCursor;

    pDataCursor += sizeof(NT_TRANSACTION_REQUEST_HEADER);
    ulNumBytesAvailable -= sizeof(NT_TRANSACTION_REQUEST_HEADER);
    ulOffset += sizeof(NT_TRANSACTION_REQUEST_HEADER);

    pByteCount = &pHeader->usByteCount;

    ntStatus = WireUnmarshallNtTransactionSetupData(
                    pDataCursor,
                    ulNumBytesAvailable,
                    ulOffset,
                    pHeader->ulParameterOffset,
                    pHeader->ulDataOffset,
                    &pSetup,
                    pHeader->ucSetupCount,
                    &pParameters,
                    pHeader->ulParameterCount,
                    &pData,
                    pHeader->ulDataCount);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppHeader = pHeader;
    *ppSetup = pSetup;
    *ppByteCount = pByteCount;
    *ppParameters = pParameters;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    *ppSetup = NULL;
    *ppByteCount = NULL;
    *ppParameters = NULL;
    *ppData = NULL;

    goto cleanup;
}

static
NTSTATUS
WireUnmarshallNtTransactionSetupData(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    ULONG       ulParameterOffset,
    ULONG       ulDataOffset,
    PUSHORT*    ppSetup,
    UCHAR       ucSetupLen,
    PBYTE*      ppParameters,
    USHORT      parameterLen,
    PBYTE*      ppData,
    USHORT      dataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pDataCursor = pBuffer;
    USHORT  usSetupLen = 0;
    PUSHORT pSetup = NULL;
    PBYTE   pParameters = NULL;
    PBYTE   pData = NULL;

    usSetupLen = (ucSetupLen * sizeof(USHORT));

    if (usSetupLen)
    {
        if (ulNumBytesAvailable < usSetupLen)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pSetup = (PUSHORT) pDataCursor;
        pDataCursor += usSetupLen;
        ulNumBytesAvailable -= usSetupLen;
        ulOffset += usSetupLen;
    }

    ntStatus = WireUnmarshallNtTransactionParameterData(
                    pDataCursor,
                    ulNumBytesAvailable,
                    ulOffset,
                    ulParameterOffset,
                    ulDataOffset,
                    &pParameters,
                    parameterLen,
                    &pData,
                    dataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSetup = pSetup;
    *ppParameters = pParameters;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *ppSetup = NULL;
    *ppParameters = NULL;
    *ppData = NULL;

    goto cleanup;
}

static
NTSTATUS
WireUnmarshallNtTransactionParameterData(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    ULONG       ulParameterOffset,
    ULONG       ulDataOffset,
    PBYTE*      ppParameters,
    ULONG       parameterLen,
    PBYTE*      ppData,
    ULONG       dataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pDataCursor = pBuffer;
    PBYTE    pParameters = NULL;
    PBYTE    pData = NULL;

    if (ulOffset % 4)
    {
        USHORT usAlignment = (4 - (ulOffset % 4));

        if (ulNumBytesAvailable < usAlignment)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pDataCursor += usAlignment;
        ulNumBytesAvailable -= usAlignment;
        ulOffset += usAlignment;
    }

    if (ulOffset > ulParameterOffset)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else if (ulOffset < ulParameterOffset)
    {
        USHORT usOffsetDelta = ulParameterOffset - ulOffset;

        if (ulNumBytesAvailable < usOffsetDelta)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulOffset += usOffsetDelta;
        pDataCursor += usOffsetDelta;
        ulNumBytesAvailable -= usOffsetDelta;
    }

    if (ulNumBytesAvailable < parameterLen)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (parameterLen)
    {
        pParameters = pDataCursor;

        pDataCursor += parameterLen;
        ulNumBytesAvailable -= parameterLen;
        ulOffset += parameterLen;
    }

    if (dataLen)
    {
        if (ulOffset % 4)
        {
            USHORT usAlignment = (4 - (ulOffset % 4));

            if (ulNumBytesAvailable < usAlignment)
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pDataCursor += usAlignment;
            ulNumBytesAvailable -= usAlignment;
            ulOffset += usAlignment;
        }

        if (ulOffset > ulDataOffset)
        {
            ntStatus = STATUS_DATA_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else if (ulOffset < ulDataOffset)
        {
            USHORT usOffsetDelta = ulDataOffset - ulOffset;

            if (ulNumBytesAvailable < usOffsetDelta)
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ulOffset += usOffsetDelta;
            pDataCursor += usOffsetDelta;
            ulNumBytesAvailable -= usOffsetDelta;
        }

        pData = pDataCursor;
    }

    *ppParameters = pParameters;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *ppParameters = NULL;
    *ppData = NULL;

    goto cleanup;
}
