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
 *        fileBothDirInfo,c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FileBothDirectoryInformation
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"


/* Forward declarations */

static NTSTATUS
PvfsQueryFileBothDirInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );


/* File Globals */



/* Code */


/********************************************************
 *******************************************************/

NTSTATUS
PvfsFileBothDirInfo(
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
        ntError = PvfsQueryFileBothDirInfo(pIrpContext);
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

/********************************************************
 *******************************************************/

static NTSTATUS
FillFileBothDirInfoStatic(
    PFILE_BOTH_DIR_INFORMATION pFileInfo,
    PWSTR pwszShortFilename,
    PPVFS_STAT pStat
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    size_t FilenameLen = 0;
    size_t FilenameLenBytes = 0;

    /* Check if this is a valid 8.3 filename */
    FilenameLen = RtlWC16StringNumChars(pwszShortFilename);
    FilenameLenBytes = FilenameLen * sizeof(WCHAR);

    if (FilenameLen > 12 ) {
        ntError = STATUS_FILE_INVALID;
        BAIL_ON_NT_STATUS(ntError);
    }

    pFileInfo->ShortNameLength = FilenameLenBytes;
    if (pFileInfo->ShortNameLength > 0) {
        memcpy(pFileInfo->ShortName, pwszShortFilename, FilenameLenBytes);
    }

    /* Fill in Timestamps */

    ntError = PvfsUnixToWinTime(&pFileInfo->LastAccessTime, pStat->s_atime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->LastWriteTime, pStat->s_mtime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->ChangeTime, pStat->s_ctime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->CreationTime, pStat->s_crtime);
    BAIL_ON_NT_STATUS(ntError);

    /* Make this up for now */

    pFileInfo->FileAttributes = FILE_ATTRIBUTE_ARCHIVE;
    if (S_ISDIR(pStat->s_mode)) {
        pFileInfo->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }

    /* File details */

    pFileInfo->EaSize         = 0;
    pFileInfo->EndOfFile      = pStat->s_size;
    pFileInfo->AllocationSize = pStat->s_alloc;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/********************************************************
 Returns: STATUS_BUFFER_TOO_SMALL (not enough space)
          STATUS_OBJECT_PATH_NOT_FOUND (stat() failed)
 *******************************************************/

static NTSTATUS
FillFileBothDirInfoBuffer(
    PVOID pBuffer,
    DWORD dwBufLen,
    PSTR pszParent,
    PPVFS_DIRECTORY_ENTRY pEntry,
    PDWORD pdwConsumed
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PFILE_BOTH_DIR_INFORMATION pFileInfo = (PFILE_BOTH_DIR_INFORMATION)pBuffer;
    PWSTR pwszFilename = NULL;
    PSTR pszFullPath = NULL;
    DWORD dwNeeded = 0;
    size_t W16FilenameLen = 0;
    size_t W16FilenameLenBytes = 0;

    /* Check for enough space for static members */

    if (dwBufLen < sizeof(*pFileInfo)) {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Build the absolute path and stat() it */

    ntError = RtlCStringAllocatePrintf(&pszFullPath,
                                       "%s/%s",
                                       pszParent,
                                       pEntry->pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysStat(pszFullPath, &pEntry->Stat);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlWC16StringAllocateFromCString(&pwszFilename,
                                               pEntry->pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = FillFileBothDirInfoStatic(pFileInfo,
                                        pwszFilename,     /* Assuming only 8.3 */
                                        &pEntry->Stat);
    BAIL_ON_NT_STATUS(ntError);

    /* Save what we have used so far */

    *pdwConsumed = sizeof(*pFileInfo);

    /* Calculate space */

    W16FilenameLen = RtlWC16StringNumChars(pwszFilename);
    W16FilenameLenBytes = W16FilenameLen * sizeof(WCHAR);
    dwNeeded = sizeof(*pFileInfo) + W16FilenameLenBytes;

    if (dwNeeded > dwBufLen) {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pFileInfo->FileNameLength = W16FilenameLenBytes;
    memcpy(pFileInfo->FileName, pwszFilename, W16FilenameLenBytes);

    *pdwConsumed = dwNeeded;
    ntError = STATUS_SUCCESS;

cleanup:
    PVFS_SAFE_FREE_MEMORY(pszFullPath);
    PVFS_SAFE_FREE_MEMORY(pwszFilename);

    return ntError;

error:
    goto cleanup;
}


/********************************************************
 *******************************************************/

static NTSTATUS
PvfsQueryFileBothDirInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = (PPVFS_CCB)IoFileGetContext(pIrp->FileHandle);
    PFILE_BOTH_DIR_INFORMATION pFileInfo = NULL;
    IRP_ARGS_QUERY_DIRECTORY Args = pIrpContext->pIrp->Args.QueryDirectory;
    PVOID pBuffer = NULL;
    DWORD dwBufLen = 0;
    DWORD dwOffset = 0;
    DWORD dwConsumed = 0;

    /* Sanity checks */

    PVFS_BAIL_ON_INVALID_CCB(pCcb, ntError);

    if (!PVFS_IS_DIR(pCcb)) {
        ntError = STATUS_NOT_A_DIRECTORY;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAccessCheckFileHandle(pCcb,  FILE_LIST_DIRECTORY);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(Args.FileInformation, ntError);

    if (Args.Length < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pFileInfo = (PFILE_BOTH_DIR_INFORMATION)Args.FileInformation;

    /* Scen the first time through */

    if (!pCcb->pDirContext->bScanned)
    {
        ntError = PvfsEnumerateDirectory(pCcb, &pIrp->Args.QueryDirectory);
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Check for ending condition */

    if (pCcb->pDirContext->dwIndex == pCcb->pDirContext->dwNumEntries)
    {
        ntError = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntError);
    }


    /* Fill in the buffer */

    pBuffer = Args.FileInformation;
    dwBufLen = Args.Length;
    dwOffset = 0;
    pFileInfo = NULL;

    do
    {
        PPVFS_DIRECTORY_ENTRY pEntry = NULL;
        DWORD dwIndex;

        /* Set the off set in the previous record */

        if (pFileInfo) {
            pFileInfo->NextEntryOffset = (pBuffer+dwOffset) - (PVOID)pFileInfo;
        }

        dwIndex = pCcb->pDirContext->dwIndex;
        pEntry  = &pCcb->pDirContext->pDirEntries[dwIndex];
        ntError = FillFileBothDirInfoBuffer(pBuffer + dwOffset,
                                            dwBufLen - dwOffset,
                                            pCcb->pszFilename,
                                            pEntry,
                                            &dwConsumed);

        /* Break from loop if we ran out of buffer space.
           Fail on all other errors */

        if (ntError == STATUS_BUFFER_TOO_SMALL) {
            break;
        }

        /* This deals with a possible race where the directory
           contents was read but the file was removed before we could
           stat() it.  Just skip the file and move on. */

        if (ntError == STATUS_OBJECT_PATH_NOT_FOUND) {
            pCcb->pDirContext->dwIndex++;
            continue;
        }

        /* Catch any other errors and bail */

        BAIL_ON_NT_STATUS(ntError);

        /* Save current record in order to update the offset
           next time around */

        pFileInfo = (PFILE_BOTH_DIR_INFORMATION)(pBuffer + dwOffset);

        dwOffset += dwConsumed;
        pCcb->pDirContext->dwIndex++;

        if (Args.ReturnSingleEntry) {
            break;
        }
    }
    /* Exit loop when we are out of buffer or out of entries.  The
       filling function can also break us out of the loop. */
    while (((dwBufLen - dwOffset) > sizeof(FILE_BOTH_DIR_INFORMATION)) &&
             (pCcb->pDirContext->dwIndex < pCcb->pDirContext->dwNumEntries));

    /* Update final offset */

    if (pFileInfo) {
        pFileInfo->NextEntryOffset = 0;
    }

    pIrp->IoStatusBlock.BytesTransferred = dwOffset;
    ntError = STATUS_SUCCESS;

cleanup:
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

