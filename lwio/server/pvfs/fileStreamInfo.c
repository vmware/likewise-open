/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        fileStreamInfo.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FileStreamInformation Handler
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

//
// File static function prototypes
//

static
NTSTATUS
PvfsQueryFileStreamInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static
NTSTATUS
PvfsMarshallFileStreamInfoBuffer(
    IN OUT PFILE_STREAM_INFORMATION pFileStreamInfo,
    IN OUT ULONG BytesAvailable,
    OUT PULONG BytesConsumed,
    IN PPVFS_FILE_NAME StreamName,
    IN PPVFS_STAT pStat
    );

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsFileStreamInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    switch(Type)
    {
    case PVFS_SET:
        ntError = STATUS_NOT_SUPPORTED;
        break;

    case PVFS_QUERY:
        ntError = PvfsQueryFileStreamInfo(pIrpContext);
        break;

    default:
        ntError = STATUS_INVALID_PARAMETER;
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

////////////////////////////////////////////////////////////////////////

static
NTSTATUS
PvfsQueryFileStreamInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PFILE_STREAM_INFORMATION pFileInfo = NULL;
    IRP_ARGS_QUERY_SET_INFORMATION Args = pIrpContext->pIrp->Args.QuerySetInformation;
    PVOID buffer = Args.FileInformation;
    ULONG bufferLength = Args.Length;
    ULONG bufferConsumed = 0;
    ULONG bufferOffset = 0;
    PPVFS_FILE_NAME streamNames = NULL;
    LONG streamCount = 0;
    LONG i = 0;

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(Args.FileInformation, ntError);

    ntError = PvfsAccessCheckFileHandle(pCcb, FILE_READ_ATTRIBUTES);
    BAIL_ON_NT_STATUS(ntError);

    /* Make sure buffer is large enough for the static structure
       fields and the magic WCHAR("::$DATA") string */

    if (Args.Length < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    // The input buffer must be large enough to hold the complete list of
    // streams

    ntError = PvfsEnumerateStreams(pCcb, &streamNames, &streamCount);
    BAIL_ON_NT_STATUS(ntError);

    for (i=0; i<streamCount; i++)
    {
        PVFS_STAT streamStat = { 0 };

        // Reset as we move the cursor forward
        bufferConsumed = 0;

        pFileInfo = (PFILE_STREAM_INFORMATION)(buffer + bufferOffset);

        ntError = PvfsSysStatByFileName(&streamNames[i], &streamStat);
        if (ntError != STATUS_SUCCESS)
        {
            // Skip failures
            continue;
        }

        ntError = PvfsMarshallFileStreamInfoBuffer(
                      pFileInfo,
                      bufferLength - bufferOffset,
                      &bufferConsumed,
                      &streamNames[i],
                      &streamStat);
        BAIL_ON_NT_STATUS(ntError);

        bufferOffset += bufferConsumed;
        pFileInfo->NextEntryOffset = bufferConsumed;
    }

    if (pFileInfo)
    {
        pFileInfo->NextEntryOffset = 0;
    }

    pIrp->IoStatusBlock.BytesTransferred = bufferOffset;
    ntError = STATUS_SUCCESS;

error:
    if (streamNames)
    {
        PvfsFreeFileNameList(streamNames, streamCount);
    }

    if (pCcb)
    {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;
}

////////////////////////////////////////////////////////////////////////

static
NTSTATUS
PvfsMarshallFileStreamInfoBuffer(
    IN OUT PFILE_STREAM_INFORMATION pFileStreamInfo,
    IN OUT ULONG BytesAvailable,
    OUT PULONG BytesConsumed,
    IN PPVFS_FILE_NAME StreamName,
    IN PPVFS_STAT pStat
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR pszStreamName = NULL;
    PWSTR pwszStreamName = NULL;
    ULONG streamNameSize = 0;
    ULONG bytesNeeded = 0;

    if (BytesAvailable < sizeof(*pFileStreamInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAllocateCStringStreamFromFileName(
                  &pszStreamName,
                  StreamName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = LwRtlWC16StringAllocateFromCString(&pwszStreamName, pszStreamName);
    BAIL_ON_NT_STATUS(ntError);

    streamNameSize = LwRtlWC16StringNumChars(pwszStreamName) * sizeof(*pwszStreamName);

    bytesNeeded = sizeof(*pFileStreamInfo) + streamNameSize - sizeof(WCHAR);

    PVFS_ALIGN_MEMORY(bytesNeeded, 8);

    if (BytesAvailable < bytesNeeded)
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pFileStreamInfo->NextEntryOffset = 0;
    pFileStreamInfo->StreamSize = pStat->s_size;
    pFileStreamInfo->StreamAllocationSize = pStat->s_alloc;
    pFileStreamInfo->StreamNameLength = streamNameSize;
    LwRtlCopyMemory(pFileStreamInfo->StreamName, pwszStreamName, streamNameSize);

    *BytesConsumed = bytesNeeded;

error:
    if (pszStreamName)
    {
        LwRtlCStringFree(&pszStreamName);
    }
    if (pwszStreamName)
    {
        LwRtlWC16StringFree(&pwszStreamName);
    }

    return ntError;
}
