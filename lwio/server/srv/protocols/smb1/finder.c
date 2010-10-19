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
 *        srvfinder.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        File and Directory object finder
 *
 * Author: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
SrvFinderGetStdInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvFinderGetEASizeSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvFinderGetEASFromListSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvFinderGetDirInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvFinderMarshallDirInfoResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    USHORT            usBytesAvailable,
    USHORT            usDesiredSearchCount,
    PUSHORT           pusDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchCount
    );

static
NTSTATUS
SrvFinderGetFullDirInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvFinderMarshallFullDirInfoResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    USHORT            usBytesAvailable,
    USHORT            usDesiredSearchCount,
    PUSHORT           pusDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchCount
    );

static
NTSTATUS
SrvFinderGetNamesInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvFinderMarshallNamesInfoResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    USHORT            usBytesAvailable,
    USHORT            usDesiredSearchCount,
    PUSHORT           pusDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchCount
    );

static
NTSTATUS
SrvFinderGetBothDirInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvFinderMarshallBothDirInfoResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    USHORT            usBytesAvailable,
    USHORT            usDesiredSearchCount,
    PUSHORT           pusDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchCount
    );

static
NTSTATUS
SrvFinderGetIdBothDirInfoSearchResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    BOOLEAN           bReturnSingleEntry,
    BOOLEAN           bRestartScan,
    USHORT            usDesiredSearchCount,
    USHORT            usMaxDataCount,
    USHORT            usDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchResultCount,
    PBOOLEAN          pbEndOfSearch
    );

static
NTSTATUS
SrvFinderMarshallIdBothDirInfoResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    USHORT            usBytesAvailable,
    USHORT            usDesiredSearchCount,
    PUSHORT           pusDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchCount
    );

static
NTSTATUS
SrvFinderGetIdFullDirInfoSearchResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    BOOLEAN           bReturnSingleEntry,
    BOOLEAN           bRestartScan,
    USHORT            usDesiredSearchCount,
    USHORT            usMaxDataCount,
    USHORT            usDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchResultCount,
    PBOOLEAN          pbEndOfSearch
    );

static
NTSTATUS
SrvFinderMarshallIdFullDirInfoResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    USHORT            usBytesAvailable,
    USHORT            usDesiredSearchCount,
    PUSHORT           pusDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchCount
    );


static
NTSTATUS
SrvFinderGetUnixSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    );

NTSTATUS
SrvFinderGetSearchResults(
    HANDLE   hSearchSpace,
    BOOLEAN  bReturnSingleEntry,
    BOOLEAN  bRestartScan,
    USHORT   usDesiredSearchCount,
    USHORT   usMaxDataCount,
    USHORT   usDataOffset,
    PBYTE*   ppData,
    PUSHORT  pusDataLen,
    PUSHORT  pusSearchResultCount,
    PBOOLEAN pbEndOfSearch
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    USHORT   usDataLen = 0;
    USHORT   usSearchResultCount = 0;
    BOOLEAN  bEndOfSearch = FALSE;
    PSRV_SEARCH_SPACE pSearchSpace = (PSRV_SEARCH_SPACE)hSearchSpace;
    BOOLEAN bInLock = FALSE;

    /*
     * If the DesiredSeachCount is 0 Windows returns 1 entry! Reset the
     * DesiredSearchCount to 1 to match Windows behavior.
     */
    if (usDesiredSearchCount == 0) {
        usDesiredSearchCount = 1;
    }

    LWIO_LOCK_MUTEX(bInLock, &pSearchSpace->mutex);

    switch (pSearchSpace->infoLevel)
    {
        case SMB_INFO_STANDARD:

            ntStatus = SrvFinderGetStdInfoSearchResults(
                            pSearchSpace,
                            bReturnSingleEntry,
                            bRestartScan,
                            usDesiredSearchCount,
                            usMaxDataCount,
                            usDataOffset,
                            &pData,
                            &usDataLen,
                            &usSearchResultCount,
                            &bEndOfSearch);

            break;

        case SMB_INFO_QUERY_EA_SIZE:

            ntStatus = SrvFinderGetEASizeSearchResults(
                            pSearchSpace,
                            bReturnSingleEntry,
                            bRestartScan,
                            usDesiredSearchCount,
                            usMaxDataCount,
                            usDataOffset,
                            &pData,
                            &usDataLen,
                            &usSearchResultCount,
                            &bEndOfSearch);

            break;

        case SMB_INFO_QUERY_EAS_FROM_LIST:

            ntStatus = SrvFinderGetEASFromListSearchResults(
                            pSearchSpace,
                            bReturnSingleEntry,
                            bRestartScan,
                            usDesiredSearchCount,
                            usMaxDataCount,
                            usDataOffset,
                            &pData,
                            &usDataLen,
                            &usSearchResultCount,
                            &bEndOfSearch);

            break;

        case SMB_FIND_FILE_DIRECTORY_INFO:

            ntStatus = SrvFinderGetDirInfoSearchResults(
                            pSearchSpace,
                            bReturnSingleEntry,
                            bRestartScan,
                            usDesiredSearchCount,
                            usMaxDataCount,
                            usDataOffset,
                            &pData,
                            &usDataLen,
                            &usSearchResultCount,
                            &bEndOfSearch);

            break;

        case SMB_FIND_FILE_FULL_DIRECTORY_INFO:

            ntStatus = SrvFinderGetFullDirInfoSearchResults(
                            pSearchSpace,
                            bReturnSingleEntry,
                            bRestartScan,
                            usDesiredSearchCount,
                            usMaxDataCount,
                            usDataOffset,
                            &pData,
                            &usDataLen,
                            &usSearchResultCount,
                            &bEndOfSearch);

            break;

        case SMB_FIND_FILE_NAMES_INFO:

            ntStatus = SrvFinderGetNamesInfoSearchResults(
                            pSearchSpace,
                            bReturnSingleEntry,
                            bRestartScan,
                            usDesiredSearchCount,
                            usMaxDataCount,
                            usDataOffset,
                            &pData,
                            &usDataLen,
                            &usSearchResultCount,
                            &bEndOfSearch);

            break;

        case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:

            ntStatus = SrvFinderGetBothDirInfoSearchResults(
                            pSearchSpace,
                            bReturnSingleEntry,
                            bRestartScan,
                            usDesiredSearchCount,
                            usMaxDataCount,
                            usDataOffset,
                            &pData,
                            &usDataLen,
                            &usSearchResultCount,
                            &bEndOfSearch);

            break;

        case SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO:

            ntStatus = SrvFinderGetIdFullDirInfoSearchResults(
                            pSearchSpace,
                            bReturnSingleEntry,
                            bRestartScan,
                            usDesiredSearchCount,
                            usMaxDataCount,
                            usDataOffset,
                            &pData,
                            &usDataLen,
                            &usSearchResultCount,
                            &bEndOfSearch);

            break;

        case SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO:

            ntStatus = SrvFinderGetIdBothDirInfoSearchResults(
                            pSearchSpace,
                            bReturnSingleEntry,
                            bRestartScan,
                            usDesiredSearchCount,
                            usMaxDataCount,
                            usDataOffset,
                            &pData,
                            &usDataLen,
                            &usSearchResultCount,
                            &bEndOfSearch);

            break;

        case SMB_FIND_FILE_UNIX:

            ntStatus = SrvFinderGetUnixSearchResults(
                            pSearchSpace,
                            bReturnSingleEntry,
                            bRestartScan,
                            usDesiredSearchCount,
                            usMaxDataCount,
                            usDataOffset,
                            &pData,
                            &usDataLen,
                            &usSearchResultCount,
                            &bEndOfSearch);

            break;

        default:

            ntStatus = STATUS_NOT_SUPPORTED;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppData = pData;
    *pusDataLen = usDataLen;
    *pusSearchResultCount = usSearchResultCount;
    *pbEndOfSearch = bEndOfSearch;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSearchSpace->mutex);

    return ntStatus;

error:

    *ppData = NULL;
    *pusDataLen = 0;
    *pusSearchResultCount = 0;
    *pbEndOfSearch = TRUE;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvFinderGetStdInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvFinderGetEASizeSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvFinderGetEASFromListSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvFinderGetDirInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    USHORT   usDataLen = 0;
    USHORT   usSearchResultCount = 0;
    USHORT   usBytesAvailable = usMaxDataCount;
    BOOLEAN  bEndOfSearch = FALSE;
    PFILE_DIRECTORY_INFORMATION pFileDirInfoCursor = NULL;

    if (!pSearchSpace->pFileInfo)
    {
        // Keep space for 10 items
        USHORT usBytesAllocated = (sizeof(FILE_DIRECTORY_INFORMATION) + 256 * sizeof(wchar16_t)) * 10;

        ntStatus = SrvAllocateMemory(
                        usBytesAllocated,
                        (PVOID*)&pSearchSpace->pFileInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pSearchSpace->usFileInfoLen = usBytesAllocated;
    }

    while (!bEndOfSearch && (usSearchResultCount < usDesiredSearchCount))
    {
        USHORT usIterSearchCount = 0;

        pFileDirInfoCursor = (PFILE_DIRECTORY_INFORMATION)pSearchSpace->pFileInfoCursor;
        if (!pFileDirInfoCursor)
        {
            IO_MATCH_FILE_SPEC ioFileSpec = {0};
            IO_STATUS_BLOCK ioStatusBlock = {0};

            ioFileSpec.Type = IO_MATCH_FILE_SPEC_TYPE_UNKNOWN;
            // ioFileSpec.Options = IO_NAME_OPTION_CASE_SENSITIVE;
            RtlUnicodeStringInit(
                &ioFileSpec.Pattern,
                pSearchSpace->pwszSearchPattern);

            pSearchSpace->pFileInfoCursor = NULL;

            do
            {
                memset(pSearchSpace->pFileInfo, 0x0, pSearchSpace->usFileInfoLen);

                ntStatus = IoQueryDirectoryFile(
                                pSearchSpace->hFile,
                                NULL,
                                &ioStatusBlock,
                                pSearchSpace->pFileInfo,
                                pSearchSpace->usFileInfoLen,
                                FileDirectoryInformation,
                                bReturnSingleEntry,
                                &ioFileSpec,
                                bRestartScan);
                if (ntStatus == STATUS_SUCCESS)
                {
                    pSearchSpace->pFileInfoCursor = pSearchSpace->pFileInfo;

                    break;
                }
                else if (ntStatus == STATUS_NO_MORE_MATCHES)
                {
                    bEndOfSearch = TRUE;
                    ntStatus = 0;

                    break;
                }
                else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
                {
                    USHORT usNewSize = pSearchSpace->usFileInfoLen + 256 * sizeof(wchar16_t);

                    ntStatus = SrvReallocMemory(
                                    pSearchSpace->pFileInfo,
                                    usNewSize,
                                    (PVOID*)&pSearchSpace->pFileInfo);
                    BAIL_ON_NT_STATUS(ntStatus);

                    memset(pSearchSpace->pFileInfo + pSearchSpace->usFileInfoLen,
                           0,
                           usNewSize - pSearchSpace->usFileInfoLen);

                    pSearchSpace->usFileInfoLen = usNewSize;

                    continue;
                }
                BAIL_ON_NT_STATUS(ntStatus);

            } while (TRUE);
        }

        if (pSearchSpace->pFileInfoCursor)
        {
            ntStatus = SrvFinderMarshallDirInfoResults(
                            pSearchSpace,
                            usBytesAvailable - usDataLen,
                            usDesiredSearchCount - usSearchResultCount,
                            &usDataOffset,
                            &pData,
                            &usDataLen,
                            &usIterSearchCount);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!usIterSearchCount)
            {
                break;
            }

            usSearchResultCount += usIterSearchCount;
        }
    }

    if (!usSearchResultCount)
    {
        ntStatus = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppData = pData;
    *pusDataLen = usDataLen;
    *pusSearchResultCount = usSearchResultCount;
    *pbEndOfSearch = pSearchSpace->pFileInfoCursor ? FALSE : bEndOfSearch;

cleanup:

    return ntStatus;

error:

    *ppData = NULL;
    *pusDataLen = 0;
    *pusSearchResultCount = 0;
    *pbEndOfSearch = FALSE;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvFinderMarshallDirInfoResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    USHORT            usBytesAvailable,
    USHORT            usDesiredSearchCount,
    PUSHORT           pusDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchCount
    )
{
    NTSTATUS ntStatus = 0;
    USHORT   usBytesRequired = 0;
    PSMB_FIND_FILE_DIRECTORY_INFO_HEADER pInfoHeader = NULL;
    PFILE_DIRECTORY_INFORMATION pFileInfoCursor = NULL;
    PBYTE    pData = *ppData;
    USHORT   usDataLen = 0;
    USHORT   usOrigDataLen = *pusDataLen;
    PBYTE    pDataCursor = NULL;
    USHORT   usSearchCount = 0;
    USHORT   iSearchCount = 0;
    USHORT   usOffset = 0;
    USHORT   usDataOffset = *pusDataOffset;

    pFileInfoCursor = (PFILE_DIRECTORY_INFORMATION)pSearchSpace->pFileInfoCursor;

    while (pFileInfoCursor &&
           (usSearchCount < usDesiredSearchCount) &&
           (usBytesAvailable > 0))
    {
        USHORT usInfoBytesRequired = 0;

        usInfoBytesRequired = sizeof(SMB_FIND_FILE_DIRECTORY_INFO_HEADER);
        usDataOffset += sizeof(SMB_FIND_FILE_DIRECTORY_INFO_HEADER);

        usInfoBytesRequired += pFileInfoCursor->FileNameLength;
        usDataOffset += pFileInfoCursor->FileNameLength;

        if (!pFileInfoCursor->FileNameLength)
        {
            usInfoBytesRequired += sizeof(wchar16_t);
            usDataOffset += sizeof(wchar16_t);
        }

        if (usDataOffset % 8)
        {
            USHORT usAlignment = (8 - (usDataOffset % 8));

            usInfoBytesRequired += usAlignment;
            usDataOffset += usAlignment;
        }

        if (usBytesAvailable < usInfoBytesRequired)
        {
            break;
        }

        usSearchCount++;
        usBytesAvailable -= usInfoBytesRequired;
        usBytesRequired += usInfoBytesRequired;

        if (pFileInfoCursor->NextEntryOffset)
        {
            pFileInfoCursor = (PFILE_DIRECTORY_INFORMATION)(((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileInfoCursor = NULL;
        }
    }

    if (!pData)
    {
        ntStatus = SrvAllocateMemory(usBytesRequired, (PVOID*)&pData);
        BAIL_ON_NT_STATUS(ntStatus);

        usDataLen = usBytesRequired;
    }
    else
    {
        USHORT usNewDataLen = usOrigDataLen + usBytesRequired;

        ntStatus = SrvReallocMemory(pData, usNewDataLen, (PVOID*)&pData);
        BAIL_ON_NT_STATUS(ntStatus);

        usDataLen = usNewDataLen;

        // Got to fill the last offset from previous data
        usOffset = usOrigDataLen;
        pDataCursor = pData;
        pInfoHeader = (PSMB_FIND_FILE_DIRECTORY_INFO_HEADER)pDataCursor;
        while (pInfoHeader->NextEntryOffset)
        {
            usOffset -= pInfoHeader->NextEntryOffset;
            pInfoHeader = (PSMB_FIND_FILE_DIRECTORY_INFO_HEADER)((PBYTE)pInfoHeader + pInfoHeader->NextEntryOffset);
        }
    }

    usDataOffset = *pusDataOffset;

    pDataCursor = pData + usOrigDataLen;
    for (; iSearchCount < usSearchCount; iSearchCount++)
    {
        pFileInfoCursor = (PFILE_DIRECTORY_INFORMATION)pSearchSpace->pFileInfoCursor;

        if (pInfoHeader)
        {
            pInfoHeader->NextEntryOffset = usOffset;
        }

        pInfoHeader = (PSMB_FIND_FILE_DIRECTORY_INFO_HEADER)pDataCursor;

        usOffset = 0;

        pInfoHeader->FileIndex = pFileInfoCursor->FileIndex;
        pInfoHeader->CreationTime = pFileInfoCursor->CreationTime;
        pInfoHeader->LastAccessTime = pFileInfoCursor->LastAccessTime;
        pInfoHeader->LastWriteTime = pFileInfoCursor->LastWriteTime;
        pInfoHeader->ChangeTime = pFileInfoCursor->ChangeTime;
        pInfoHeader->EndOfFile = pFileInfoCursor->EndOfFile;
        pInfoHeader->AllocationSize = pFileInfoCursor->AllocationSize;
        pInfoHeader->FileAttributes = pFileInfoCursor->FileAttributes;
        pInfoHeader->FileNameLength = pFileInfoCursor->FileNameLength;

        pDataCursor += sizeof(SMB_FIND_FILE_DIRECTORY_INFO_HEADER);
        usOffset += sizeof(SMB_FIND_FILE_DIRECTORY_INFO_HEADER);
        usDataOffset += sizeof(SMB_FIND_FILE_DIRECTORY_INFO_HEADER);

        if (pFileInfoCursor->FileNameLength)
        {
            memcpy(pDataCursor, (PBYTE)pFileInfoCursor->FileName, pFileInfoCursor->FileNameLength);

            pDataCursor += pFileInfoCursor->FileNameLength;
            usOffset += pFileInfoCursor->FileNameLength;
            usDataOffset += pFileInfoCursor->FileNameLength;
        }
        else
        {
            pDataCursor += sizeof(wchar16_t);
            usOffset += sizeof(wchar16_t);
            usDataOffset += sizeof(wchar16_t);
        }

        if (usDataOffset % 8)
        {
            USHORT usAlignment = (8 - (usDataOffset % 8));

            pDataCursor += usAlignment;
            usOffset += usAlignment;
            usDataOffset += usAlignment;
        }

        if (pFileInfoCursor->NextEntryOffset)
        {
            pSearchSpace->pFileInfoCursor = (((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pSearchSpace->pFileInfoCursor = NULL;
        }
    }

    if (pInfoHeader)
    {
        pInfoHeader->NextEntryOffset = 0;
    }

    *pusSearchCount = usSearchCount;
    *pusDataLen = usDataLen;
    *pusDataOffset = usDataOffset;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *pusSearchCount = 0;
    *pusDataLen = 0;
    *ppData = NULL;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvFinderGetFullDirInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    USHORT   usDataLen = 0;
    USHORT   usSearchResultCount = 0;
    USHORT   usBytesAvailable = usMaxDataCount;
    BOOLEAN  bEndOfSearch = FALSE;
    PFILE_FULL_DIR_INFORMATION pFileInfoCursor = NULL;

    if (!pSearchSpace->pFileInfo)
    {
        // Keep space for 10 items
        USHORT usBytesAllocated = (sizeof(FILE_FULL_DIR_INFORMATION) + 256 * sizeof(wchar16_t)) * 10;

        ntStatus = SrvAllocateMemory(
                        usBytesAllocated,
                        (PVOID*)&pSearchSpace->pFileInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pSearchSpace->usFileInfoLen = usBytesAllocated;
    }

    while (!bEndOfSearch && (usSearchResultCount < usDesiredSearchCount))
    {
        USHORT usIterSearchCount = 0;

        pFileInfoCursor = (PFILE_FULL_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;
        if (!pFileInfoCursor)
        {
            IO_MATCH_FILE_SPEC ioFileSpec = {0};
            IO_STATUS_BLOCK ioStatusBlock = {0};

            ioFileSpec.Type = IO_MATCH_FILE_SPEC_TYPE_UNKNOWN;
            // ioFileSpec.Options = IO_NAME_OPTION_CASE_SENSITIVE;
            RtlUnicodeStringInit(
                &ioFileSpec.Pattern,
                pSearchSpace->pwszSearchPattern);

            pSearchSpace->pFileInfoCursor = NULL;

            do
            {
                memset(pSearchSpace->pFileInfo, 0x0, pSearchSpace->usFileInfoLen);

                ntStatus = IoQueryDirectoryFile(
                                pSearchSpace->hFile,
                                NULL,
                                &ioStatusBlock,
                                pSearchSpace->pFileInfo,
                                pSearchSpace->usFileInfoLen,
                                FileFullDirectoryInformation,
                                bReturnSingleEntry,
                                &ioFileSpec,
                                bRestartScan);
                if (ntStatus == STATUS_SUCCESS)
                {
                    pSearchSpace->pFileInfoCursor = pSearchSpace->pFileInfo;

                    break;
                }
                else if (ntStatus == STATUS_NO_MORE_MATCHES)
                {
                    bEndOfSearch = TRUE;
                    ntStatus = 0;

                    break;
                }
                else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
                {
                    USHORT usNewSize = pSearchSpace->usFileInfoLen + 256 * sizeof(wchar16_t);

                    ntStatus = SrvReallocMemory(
                                    pSearchSpace->pFileInfo,
                                    usNewSize,
                                    (PVOID*)&pSearchSpace->pFileInfo);
                    BAIL_ON_NT_STATUS(ntStatus);

                    memset(pSearchSpace->pFileInfo + pSearchSpace->usFileInfoLen,
                           0,
                           usNewSize - pSearchSpace->usFileInfoLen);

                    pSearchSpace->usFileInfoLen = usNewSize;

                    continue;
                }
                BAIL_ON_NT_STATUS(ntStatus);

            } while (TRUE);
        }

        if (pSearchSpace->pFileInfoCursor)
        {
            ntStatus = SrvFinderMarshallFullDirInfoResults(
                            pSearchSpace,
                            usBytesAvailable - usDataLen,
                            usDesiredSearchCount - usSearchResultCount,
                            &usDataOffset,
                            &pData,
                            &usDataLen,
                            &usIterSearchCount);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!usIterSearchCount)
            {
                break;
            }

            usSearchResultCount += usIterSearchCount;
        }
    }

    if (!usSearchResultCount)
    {
        ntStatus = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppData = pData;
    *pusDataLen = usDataLen;
    *pusSearchResultCount = usSearchResultCount;
    *pbEndOfSearch = pSearchSpace->pFileInfoCursor ? FALSE : bEndOfSearch;

cleanup:

    return ntStatus;

error:

    *ppData = NULL;
    *pusDataLen = 0;
    *pusSearchResultCount = 0;
    *pbEndOfSearch = FALSE;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvFinderMarshallFullDirInfoResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    USHORT            usBytesAvailable,
    USHORT            usDesiredSearchCount,
    PUSHORT           pusDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchCount
    )
{
    NTSTATUS ntStatus = 0;
    USHORT   usBytesRequired = 0;
    PSMB_FIND_FILE_FULL_DIRECTORY_INFO_HEADER pInfoHeader = NULL;
    PFILE_FULL_DIR_INFORMATION pFileInfoCursor = NULL;
    PBYTE    pData = *ppData;
    USHORT   usDataLen = 0;
    USHORT   usOrigDataLen = *pusDataLen;
    PBYTE    pDataCursor = NULL;
    USHORT   usSearchCount = 0;
    USHORT   iSearchCount = 0;
    USHORT   usOffset = 0;
    USHORT   usDataOffset = *pusDataOffset;

    pFileInfoCursor = (PFILE_FULL_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

    while (pFileInfoCursor &&
           (usSearchCount < usDesiredSearchCount) &&
           (usBytesAvailable > 0))
    {
        USHORT usInfoBytesRequired = 0;

        usInfoBytesRequired = sizeof(SMB_FIND_FILE_FULL_DIRECTORY_INFO_HEADER);
        usDataOffset += sizeof(SMB_FIND_FILE_FULL_DIRECTORY_INFO_HEADER);

        if (pSearchSpace->bUseLongFilenames)
        {
            usInfoBytesRequired += pFileInfoCursor->FileNameLength;
            usDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (!pFileInfoCursor->FileNameLength)
        {
            usInfoBytesRequired += sizeof(wchar16_t);
            usDataOffset += sizeof(wchar16_t);
        }

        if (usDataOffset % 8)
        {
            USHORT usAlignment = (8 - (usDataOffset % 8));

            usInfoBytesRequired += usAlignment;
            usDataOffset += usAlignment;
        }

        if (usBytesAvailable < usInfoBytesRequired)
        {
            break;
        }

        usSearchCount++;
        usBytesAvailable -= usInfoBytesRequired;
        usBytesRequired += usInfoBytesRequired;

        if (pFileInfoCursor->NextEntryOffset)
        {
            pFileInfoCursor = (PFILE_FULL_DIR_INFORMATION)(((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileInfoCursor = NULL;
        }
    }

    if (!pData)
    {
        ntStatus = SrvAllocateMemory(
                        usBytesRequired,
                        (PVOID*)&pData);
        BAIL_ON_NT_STATUS(ntStatus);

        usDataLen = usBytesRequired;
    }
    else
    {
        USHORT usNewDataLen = usOrigDataLen + usBytesRequired;

        ntStatus = SrvReallocMemory(pData, usNewDataLen, (PVOID*)&pData);
        BAIL_ON_NT_STATUS(ntStatus);

        usDataLen = usNewDataLen;

        // Got to fill the last offset from previous data
        usOffset = usOrigDataLen;
        pDataCursor = pData;
        pInfoHeader = (PSMB_FIND_FILE_FULL_DIRECTORY_INFO_HEADER)pDataCursor;
        while (pInfoHeader->NextEntryOffset)
        {
            usOffset -= pInfoHeader->NextEntryOffset;
            pInfoHeader = (PSMB_FIND_FILE_FULL_DIRECTORY_INFO_HEADER)((PBYTE)pInfoHeader + pInfoHeader->NextEntryOffset);
        }
    }

    usDataOffset = *pusDataOffset;

    pDataCursor = pData + usOrigDataLen;
    for (; iSearchCount < usSearchCount; iSearchCount++)
    {
        pFileInfoCursor = (PFILE_FULL_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

        if (pInfoHeader)
        {
            pInfoHeader->NextEntryOffset = usOffset;
        }

        pInfoHeader = (PSMB_FIND_FILE_FULL_DIRECTORY_INFO_HEADER)pDataCursor;

        usOffset = 0;

        pInfoHeader->FileIndex = pFileInfoCursor->FileIndex;
        pInfoHeader->EaSize = pFileInfoCursor->EaSize;
        pInfoHeader->CreationTime = pFileInfoCursor->CreationTime;
        pInfoHeader->LastAccessTime = pFileInfoCursor->LastAccessTime;
        pInfoHeader->LastWriteTime = pFileInfoCursor->LastWriteTime;
        pInfoHeader->ChangeTime = pFileInfoCursor->ChangeTime;
        pInfoHeader->EndOfFile = pFileInfoCursor->EndOfFile;
        pInfoHeader->AllocationSize = pFileInfoCursor->AllocationSize;
        pInfoHeader->FileAttributes = pFileInfoCursor->FileAttributes;
        pInfoHeader->FileNameLength = pFileInfoCursor->FileNameLength;

        pDataCursor += sizeof(SMB_FIND_FILE_FULL_DIRECTORY_INFO_HEADER);
        usOffset += sizeof(SMB_FIND_FILE_FULL_DIRECTORY_INFO_HEADER);
        usDataOffset += sizeof(SMB_FIND_FILE_FULL_DIRECTORY_INFO_HEADER);

        if (pSearchSpace->bUseLongFilenames && pFileInfoCursor->FileNameLength)
        {
            memcpy(pDataCursor, (PBYTE)pFileInfoCursor->FileName, pFileInfoCursor->FileNameLength);

            pDataCursor += pFileInfoCursor->FileNameLength;
            usOffset += pFileInfoCursor->FileNameLength;
            usDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (!pFileInfoCursor->FileNameLength)
        {
            pDataCursor += sizeof(wchar16_t);
            usOffset += sizeof(wchar16_t);
            usDataOffset += sizeof(wchar16_t);
        }

        if (usDataOffset % 8)
        {
            USHORT usAlignment = (8 - (usDataOffset % 8));

            pDataCursor += usAlignment;
            usOffset += usAlignment;
            usDataOffset += usAlignment;
        }

        if (pFileInfoCursor->NextEntryOffset)
        {
            pSearchSpace->pFileInfoCursor = (((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pSearchSpace->pFileInfoCursor = NULL;
        }
    }

    if (pInfoHeader)
    {
        pInfoHeader->NextEntryOffset = 0;
    }

    *pusSearchCount = usSearchCount;
    *pusDataLen = usDataLen;
    *pusDataOffset = usDataOffset;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *pusSearchCount = 0;
    *pusDataLen = 0;
    *ppData = NULL;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvFinderGetNamesInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    USHORT   usDataLen = 0;
    USHORT   usSearchResultCount = 0;
    USHORT   usBytesAvailable = usMaxDataCount;
    BOOLEAN  bEndOfSearch = FALSE;
    PFILE_NAMES_INFORMATION pFileInfoCursor = NULL;

    if (!pSearchSpace->pFileInfo)
    {
        // Keep space for 10 items
        USHORT usBytesAllocated = (sizeof(FILE_NAMES_INFORMATION) + 256 * sizeof(wchar16_t)) * 10;

        ntStatus = SrvAllocateMemory(
                        usBytesAllocated,
                        (PVOID*)&pSearchSpace->pFileInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pSearchSpace->usFileInfoLen = usBytesAllocated;
    }

    while (!bEndOfSearch && (usSearchResultCount < usDesiredSearchCount))
    {
        USHORT usIterSearchCount = 0;

        pFileInfoCursor = (PFILE_NAMES_INFORMATION)pSearchSpace->pFileInfoCursor;
        if (!pFileInfoCursor)
        {
            IO_MATCH_FILE_SPEC ioFileSpec = {0};
            IO_STATUS_BLOCK ioStatusBlock = {0};

            ioFileSpec.Type = IO_MATCH_FILE_SPEC_TYPE_UNKNOWN;
            // ioFileSpec.Options = IO_NAME_OPTION_CASE_SENSITIVE;
            RtlUnicodeStringInit(
                &ioFileSpec.Pattern,
                pSearchSpace->pwszSearchPattern);

            pSearchSpace->pFileInfoCursor = NULL;

            do
            {
                memset(pSearchSpace->pFileInfo, 0x0, pSearchSpace->usFileInfoLen);

                ntStatus = IoQueryDirectoryFile(
                                pSearchSpace->hFile,
                                NULL,
                                &ioStatusBlock,
                                pSearchSpace->pFileInfo,
                                pSearchSpace->usFileInfoLen,
                                FileNamesInformation,
                                bReturnSingleEntry,
                                &ioFileSpec,
                                bRestartScan);
                if (ntStatus == STATUS_SUCCESS)
                {
                    pSearchSpace->pFileInfoCursor = pSearchSpace->pFileInfo;

                    break;
                }
                else if (ntStatus == STATUS_NO_MORE_MATCHES)
                {
                    bEndOfSearch = TRUE;
                    ntStatus = 0;

                    break;
                }
                else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
                {
                    USHORT usNewSize = pSearchSpace->usFileInfoLen + 256 * sizeof(wchar16_t);

                    ntStatus = SrvReallocMemory(
                                    pSearchSpace->pFileInfo,
                                    usNewSize,
                                    (PVOID*)&pSearchSpace->pFileInfo);
                    BAIL_ON_NT_STATUS(ntStatus);

                    memset(pSearchSpace->pFileInfo + pSearchSpace->usFileInfoLen,
                           0,
                           usNewSize - pSearchSpace->usFileInfoLen);

                    pSearchSpace->usFileInfoLen = usNewSize;

                    continue;
                }
                BAIL_ON_NT_STATUS(ntStatus);

            } while (TRUE);
        }

        if (pSearchSpace->pFileInfoCursor)
        {
            ntStatus = SrvFinderMarshallNamesInfoResults(
                            pSearchSpace,
                            usBytesAvailable - usDataLen,
                            usDesiredSearchCount - usSearchResultCount,
                            &usDataOffset,
                            &pData,
                            &usDataLen,
                            &usIterSearchCount);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!usIterSearchCount)
            {
                break;
            }

            usSearchResultCount += usIterSearchCount;
        }
    }

    if (!usSearchResultCount)
    {
        ntStatus = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppData = pData;
    *pusDataLen = usDataLen;
    *pusSearchResultCount = usSearchResultCount;
    *pbEndOfSearch = pSearchSpace->pFileInfoCursor ? FALSE : bEndOfSearch;

cleanup:

    return ntStatus;

error:

    *ppData = NULL;
    *pusDataLen = 0;
    *pusSearchResultCount = 0;
    *pbEndOfSearch = FALSE;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvFinderMarshallNamesInfoResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    USHORT            usBytesAvailable,
    USHORT            usDesiredSearchCount,
    PUSHORT           pusDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchCount
    )
{
    NTSTATUS ntStatus = 0;
    USHORT   usBytesRequired = 0;
    PSMB_FIND_FILE_NAMES_INFO_HEADER pInfoHeader = NULL;
    PFILE_NAMES_INFORMATION pFileInfoCursor = NULL;
    PBYTE    pData = *ppData;
    USHORT   usDataLen = 0;
    USHORT   usOrigDataLen = *pusDataLen;
    PBYTE    pDataCursor = NULL;
    USHORT   usSearchCount = 0;
    USHORT   iSearchCount = 0;
    USHORT   usOffset = 0;
    USHORT   usDataOffset = *pusDataOffset;

    pFileInfoCursor = (PFILE_NAMES_INFORMATION)pSearchSpace->pFileInfoCursor;

    while (pFileInfoCursor &&
           (usSearchCount < usDesiredSearchCount) &&
           (usBytesAvailable > 0))
    {
        USHORT usInfoBytesRequired = 0;

        usInfoBytesRequired = sizeof(SMB_FIND_FILE_NAMES_INFO_HEADER);
        usDataOffset += sizeof(SMB_FIND_FILE_NAMES_INFO_HEADER);

        if (pSearchSpace->bUseLongFilenames)
        {
            usInfoBytesRequired += pFileInfoCursor->FileNameLength;
            usDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (!pFileInfoCursor->FileNameLength)
        {
            usInfoBytesRequired += sizeof(wchar16_t);
            usDataOffset += sizeof(wchar16_t);
        }

        if (usDataOffset % 8)
        {
            USHORT usAlignment = (8 - (usDataOffset % 8));

            usInfoBytesRequired += usAlignment;
            usDataOffset += usAlignment;
        }

        if (usBytesAvailable < usInfoBytesRequired)
        {
            break;
        }

        usSearchCount++;
        usBytesAvailable -= usInfoBytesRequired;
        usBytesRequired += usInfoBytesRequired;

        if (pFileInfoCursor->NextEntryOffset)
        {
            pFileInfoCursor = (PFILE_NAMES_INFORMATION)(((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileInfoCursor = NULL;
        }
    }

    if (!pData)
    {
        ntStatus = SrvAllocateMemory(usBytesRequired, (PVOID*)&pData);
        BAIL_ON_NT_STATUS(ntStatus);

        usDataLen = usBytesRequired;
    }
    else
    {
        USHORT usNewDataLen = usOrigDataLen + usBytesRequired;

        ntStatus = SrvReallocMemory(
                        pData,
                        usNewDataLen,
                        (PVOID*)&pData);
        BAIL_ON_NT_STATUS(ntStatus);

        usDataLen = usNewDataLen;

        // Got to fill the last offset from previous data
        usOffset = usOrigDataLen;
        pDataCursor = pData;
        pInfoHeader = (PSMB_FIND_FILE_NAMES_INFO_HEADER)pDataCursor;
        while (pInfoHeader->NextEntryOffset)
        {
            usOffset -= pInfoHeader->NextEntryOffset;
            pInfoHeader = (PSMB_FIND_FILE_NAMES_INFO_HEADER)((PBYTE)pInfoHeader + pInfoHeader->NextEntryOffset);
        }
    }

    usDataOffset = *pusDataOffset;

    pDataCursor = pData + usOrigDataLen;
    for (; iSearchCount < usSearchCount; iSearchCount++)
    {
        pFileInfoCursor = (PFILE_NAMES_INFORMATION)pSearchSpace->pFileInfoCursor;

        if (pInfoHeader)
        {
            pInfoHeader->NextEntryOffset = usOffset;
        }

        pInfoHeader = (PSMB_FIND_FILE_NAMES_INFO_HEADER)pDataCursor;

        usOffset = 0;

        pInfoHeader->FileIndex = pFileInfoCursor->FileIndex;
        pInfoHeader->FileNameLength = pFileInfoCursor->FileNameLength;

        pDataCursor += sizeof(SMB_FIND_FILE_NAMES_INFO_HEADER);
        usOffset += sizeof(SMB_FIND_FILE_NAMES_INFO_HEADER);
        usDataOffset += sizeof(SMB_FIND_FILE_NAMES_INFO_HEADER);

        if (pFileInfoCursor->FileNameLength)
        {
            memcpy(pDataCursor,
                   (PBYTE)pFileInfoCursor->FileName,
                   pFileInfoCursor->FileNameLength);

            pDataCursor += pFileInfoCursor->FileNameLength;
            usOffset += pFileInfoCursor->FileNameLength;
            usDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (!pFileInfoCursor->FileNameLength)
        {
            pDataCursor += sizeof(wchar16_t);
            usOffset += sizeof(wchar16_t);
            usDataOffset += sizeof(wchar16_t);
        }

        if (usDataOffset % 8)
        {
            USHORT usAlignment = (8 - (usDataOffset % 8));

            pDataCursor += usAlignment;
            usOffset += usAlignment;
            usDataOffset += usAlignment;
        }

        if (pFileInfoCursor->NextEntryOffset)
        {
            pSearchSpace->pFileInfoCursor = (((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pSearchSpace->pFileInfoCursor = NULL;
        }
    }

    if (pInfoHeader)
    {
        pInfoHeader->NextEntryOffset = 0;
    }

    *pusSearchCount = usSearchCount;
    *pusDataLen = usDataLen;
    *pusDataOffset = usDataOffset;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *pusSearchCount = 0;
    *pusDataLen = 0;
    *ppData = NULL;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvFinderGetBothDirInfoSearchResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    BOOLEAN           bReturnSingleEntry,
    BOOLEAN           bRestartScan,
    USHORT            usDesiredSearchCount,
    USHORT            usMaxDataCount,
    USHORT            usDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchResultCount,
    PBOOLEAN          pbEndOfSearch
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    USHORT   usDataLen = 0;
    USHORT   usSearchResultCount = 0;
    USHORT   usBytesAvailable = usMaxDataCount;
    BOOLEAN  bEndOfSearch = FALSE;
    PFILE_BOTH_DIR_INFORMATION pFileInfoCursor = NULL;

    if (!pSearchSpace->pFileInfo)
    {
        // Keep space for 10 items
        USHORT usBytesAllocated = (sizeof(FILE_BOTH_DIR_INFORMATION) + 256 * sizeof(wchar16_t)) * 10;

        ntStatus = SrvAllocateMemory(
                        usBytesAllocated,
                        (PVOID*)&pSearchSpace->pFileInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pSearchSpace->usFileInfoLen = usBytesAllocated;
    }

    while (!bEndOfSearch && (usSearchResultCount < usDesiredSearchCount))
    {
        USHORT usIterSearchCount = 0;

        pFileInfoCursor = (PFILE_BOTH_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;
        if (!pFileInfoCursor)
        {
            IO_MATCH_FILE_SPEC ioFileSpec = {0};
            IO_STATUS_BLOCK ioStatusBlock = {0};

            ioFileSpec.Type = IO_MATCH_FILE_SPEC_TYPE_UNKNOWN;
            // ioFileSpec.Options = IO_NAME_OPTION_CASE_SENSITIVE;
            RtlUnicodeStringInit(
                &ioFileSpec.Pattern,
                pSearchSpace->pwszSearchPattern);

            pSearchSpace->pFileInfoCursor = NULL;

            do
            {
                memset(pSearchSpace->pFileInfo, 0x0, pSearchSpace->usFileInfoLen);

                ntStatus = IoQueryDirectoryFile(
                                pSearchSpace->hFile,
                                NULL,
                                &ioStatusBlock,
                                pSearchSpace->pFileInfo,
                                pSearchSpace->usFileInfoLen,
                                FileBothDirectoryInformation,
                                bReturnSingleEntry,
                                &ioFileSpec,
                                bRestartScan);
                if (ntStatus == STATUS_SUCCESS)
                {
                    pSearchSpace->pFileInfoCursor = pSearchSpace->pFileInfo;

                    break;
                }
                else if (ntStatus == STATUS_NO_MORE_MATCHES)
                {
                    bEndOfSearch = TRUE;
                    ntStatus = 0;

                    break;
                }
                else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
                {
                    USHORT usNewSize = pSearchSpace->usFileInfoLen + 256 * sizeof(wchar16_t);

                    ntStatus = SrvReallocMemory(
                                    pSearchSpace->pFileInfo,
                                    usNewSize,
                                    (PVOID*)&pSearchSpace->pFileInfo);
                    BAIL_ON_NT_STATUS(ntStatus);

                    memset(pSearchSpace->pFileInfo + pSearchSpace->usFileInfoLen,
                           0,
                           usNewSize - pSearchSpace->usFileInfoLen);

                    pSearchSpace->usFileInfoLen = usNewSize;

                    continue;
                }
                BAIL_ON_NT_STATUS(ntStatus);

            } while (TRUE);
        }

        if (pSearchSpace->pFileInfoCursor)
        {
            ntStatus = SrvFinderMarshallBothDirInfoResults(
                            pSearchSpace,
                            usBytesAvailable - usDataLen,
                            usDesiredSearchCount - usSearchResultCount,
                            &usDataOffset,
                            &pData,
                            &usDataLen,
                            &usIterSearchCount);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!usIterSearchCount)
            {
                break;
            }

            usSearchResultCount += usIterSearchCount;
        }
    }

    if (!usSearchResultCount)
    {
        ntStatus = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppData = pData;
    *pusDataLen = usDataLen;
    *pusSearchResultCount = usSearchResultCount;
    *pbEndOfSearch = pSearchSpace->pFileInfoCursor ? FALSE : bEndOfSearch;

cleanup:

    return ntStatus;

error:

    *ppData = NULL;
    *pusDataLen = 0;
    *pusSearchResultCount = 0;
    *pbEndOfSearch = FALSE;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvFinderMarshallBothDirInfoResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    USHORT            usBytesAvailable,
    USHORT            usDesiredSearchCount,
    PUSHORT           pusDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchCount
    )
{
    NTSTATUS ntStatus = 0;
    USHORT   usBytesRequired = 0;
    PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER pInfoHeader = NULL;
    PFILE_BOTH_DIR_INFORMATION pFileInfoCursor = NULL;
    PBYTE    pData = *ppData;
    USHORT   usDataLen = 0;
    USHORT   usOrigDataLen = *pusDataLen;
    PBYTE    pDataCursor = NULL;
    USHORT   usSearchCount = 0;
    USHORT   iSearchCount = 0;
    USHORT   usOffset = 0;
    USHORT   usDataOffset = *pusDataOffset;

    pFileInfoCursor = (PFILE_BOTH_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

    while (pFileInfoCursor &&
           (usSearchCount < usDesiredSearchCount) &&
           (usBytesAvailable > 0))
    {
        USHORT usInfoBytesRequired = 0;

        usInfoBytesRequired = sizeof(SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER);
        usDataOffset += sizeof(SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER);

        if (pSearchSpace->bUseLongFilenames)
        {
            usInfoBytesRequired += pFileInfoCursor->FileNameLength;
            usDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (!pFileInfoCursor->FileNameLength)
        {
            usInfoBytesRequired += sizeof(wchar16_t);
            usDataOffset += sizeof(wchar16_t);
        }

        if (usDataOffset % 8)
        {
            USHORT usAlignment = (8 - (usDataOffset % 8));

            usInfoBytesRequired += usAlignment;
            usDataOffset += usAlignment;
        }

        if (usBytesAvailable < usInfoBytesRequired)
        {
            break;
        }

        usSearchCount++;
        usBytesAvailable -= usInfoBytesRequired;
        usBytesRequired += usInfoBytesRequired;

        if (pFileInfoCursor->NextEntryOffset)
        {
            pFileInfoCursor = (PFILE_BOTH_DIR_INFORMATION)(((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileInfoCursor = NULL;
        }
    }

    if (!pData)
    {
        ntStatus = SrvAllocateMemory(
                        usBytesRequired,
                        (PVOID*)&pData);
        BAIL_ON_NT_STATUS(ntStatus);

        usDataLen = usBytesRequired;
    }
    else
    {
        USHORT usNewDataLen = usOrigDataLen + usBytesRequired;

        ntStatus = SrvReallocMemory(pData, usNewDataLen, (PVOID*)&pData);
        BAIL_ON_NT_STATUS(ntStatus);

        usDataLen = usNewDataLen;

        // Got to fill the last offset from previous data
        usOffset = usOrigDataLen;
        pDataCursor = pData;
        pInfoHeader = (PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER)pDataCursor;
        while (pInfoHeader->NextEntryOffset)
        {
            usOffset -= pInfoHeader->NextEntryOffset;
            pInfoHeader = (PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER)((PBYTE)pInfoHeader + pInfoHeader->NextEntryOffset);
        }
    }

    usDataOffset = *pusDataOffset;

    pDataCursor = pData + usOrigDataLen;
    for (; iSearchCount < usSearchCount; iSearchCount++)
    {
        pFileInfoCursor = (PFILE_BOTH_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

        if (pInfoHeader)
        {
            pInfoHeader->NextEntryOffset = usOffset;
        }

        pInfoHeader = (PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER)pDataCursor;

        usOffset = 0;

        pInfoHeader->FileIndex = pFileInfoCursor->FileIndex;
        pInfoHeader->EaSize = pFileInfoCursor->EaSize;
        pInfoHeader->CreationTime = pFileInfoCursor->CreationTime;
        pInfoHeader->LastAccessTime = pFileInfoCursor->LastAccessTime;
        pInfoHeader->LastWriteTime = pFileInfoCursor->LastWriteTime;
        pInfoHeader->ChangeTime = pFileInfoCursor->ChangeTime;
        pInfoHeader->EndOfFile = pFileInfoCursor->EndOfFile;
        pInfoHeader->AllocationSize = pFileInfoCursor->AllocationSize;
        pInfoHeader->FileAttributes = pFileInfoCursor->FileAttributes;
        pInfoHeader->FileNameLength = pFileInfoCursor->FileNameLength;
        pInfoHeader->Reserved = 0;

        if (!pSearchSpace->bUseLongFilenames)
        {
            memcpy((PBYTE)pInfoHeader->ShortName, (PBYTE)pFileInfoCursor->ShortName, sizeof(pInfoHeader->ShortName));

            pInfoHeader->ShortNameLength = pFileInfoCursor->ShortNameLength;
        } else {
            memset((PBYTE)pInfoHeader->ShortName, 0x0, sizeof(pInfoHeader->ShortName));

            pInfoHeader->ShortNameLength = 0;
    }


        pDataCursor += sizeof(SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER);
        usOffset += sizeof(SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER);
        usDataOffset += sizeof(SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER);

        if (pSearchSpace->bUseLongFilenames && pFileInfoCursor->FileNameLength)
        {
            memcpy(pDataCursor, (PBYTE)pFileInfoCursor->FileName, pFileInfoCursor->FileNameLength);

            pDataCursor += pFileInfoCursor->FileNameLength;
            usOffset += pFileInfoCursor->FileNameLength;
            usDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (!pFileInfoCursor->FileNameLength)
        {
            pDataCursor += sizeof(wchar16_t);
            usOffset += sizeof(wchar16_t);
            usDataOffset += sizeof(wchar16_t);
        }

        if (usDataOffset % 8)
        {
            USHORT usAlignment = (8 - (usDataOffset % 8));

            pDataCursor += usAlignment;
            usOffset += usAlignment;
            usDataOffset += usAlignment;
        }

        if (pFileInfoCursor->NextEntryOffset)
        {
            pSearchSpace->pFileInfoCursor = (((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pSearchSpace->pFileInfoCursor = NULL;
        }
    }

    if (pInfoHeader)
    {
        pInfoHeader->NextEntryOffset = 0;
    }

    *pusSearchCount = usSearchCount;
    *pusDataLen = usDataLen;
    *pusDataOffset = usDataOffset;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *pusSearchCount = 0;
    *pusDataLen = 0;
    *ppData = NULL;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvFinderGetIdBothDirInfoSearchResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    BOOLEAN           bReturnSingleEntry,
    BOOLEAN           bRestartScan,
    USHORT            usDesiredSearchCount,
    USHORT            usMaxDataCount,
    USHORT            usDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchResultCount,
    PBOOLEAN          pbEndOfSearch
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    USHORT   usDataLen = 0;
    USHORT   usSearchResultCount = 0;
    USHORT   usBytesAvailable = usMaxDataCount;
    BOOLEAN  bEndOfSearch = FALSE;
    PFILE_ID_BOTH_DIR_INFORMATION pFileInfoCursor = NULL;

    if (!pSearchSpace->pFileInfo)
    {
        // Keep space for 10 items
        USHORT usBytesAllocated = (sizeof(FILE_ID_BOTH_DIR_INFORMATION) + 256 * sizeof(wchar16_t)) * 10;

        ntStatus = SrvAllocateMemory(
                        usBytesAllocated,
                        (PVOID*)&pSearchSpace->pFileInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pSearchSpace->usFileInfoLen = usBytesAllocated;
    }

    while (!bEndOfSearch && (usSearchResultCount < usDesiredSearchCount))
    {
        USHORT usIterSearchCount = 0;

        pFileInfoCursor = (PFILE_ID_BOTH_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;
        if (!pFileInfoCursor)
        {
            IO_MATCH_FILE_SPEC ioFileSpec = {0};
            IO_STATUS_BLOCK ioStatusBlock = {0};

            ioFileSpec.Type = IO_MATCH_FILE_SPEC_TYPE_UNKNOWN;
            // ioFileSpec.Options = IO_NAME_OPTION_CASE_SENSITIVE;
            RtlUnicodeStringInit(
                &ioFileSpec.Pattern,
                pSearchSpace->pwszSearchPattern);

            pSearchSpace->pFileInfoCursor = NULL;

            do
            {
                memset(pSearchSpace->pFileInfo, 0x0, pSearchSpace->usFileInfoLen);

                ntStatus = IoQueryDirectoryFile(
                                pSearchSpace->hFile,
                                NULL,
                                &ioStatusBlock,
                                pSearchSpace->pFileInfo,
                                pSearchSpace->usFileInfoLen,
                                FileIdBothDirectoryInformation,
                                bReturnSingleEntry,
                                &ioFileSpec,
                                bRestartScan);
                if (ntStatus == STATUS_SUCCESS)
                {
                    pSearchSpace->pFileInfoCursor = pSearchSpace->pFileInfo;

                    break;
                }
                else if (ntStatus == STATUS_NO_MORE_MATCHES)
                {
                    bEndOfSearch = TRUE;
                    ntStatus = 0;

                    break;
                }
                else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
                {
                    USHORT usNewSize = pSearchSpace->usFileInfoLen + 256 * sizeof(wchar16_t);

                    ntStatus = SrvReallocMemory(
                                    pSearchSpace->pFileInfo,
                                    usNewSize,
                                    (PVOID*)&pSearchSpace->pFileInfo);
                    BAIL_ON_NT_STATUS(ntStatus);

                    memset(pSearchSpace->pFileInfo + pSearchSpace->usFileInfoLen,
                           0,
                           usNewSize - pSearchSpace->usFileInfoLen);

                    pSearchSpace->usFileInfoLen = usNewSize;

                    continue;
                }
                BAIL_ON_NT_STATUS(ntStatus);

            } while (TRUE);
        }

        if (pSearchSpace->pFileInfoCursor)
        {
            ntStatus = SrvFinderMarshallIdBothDirInfoResults(
                            pSearchSpace,
                            usBytesAvailable - usDataLen,
                            usDesiredSearchCount - usSearchResultCount,
                            &usDataOffset,
                            &pData,
                            &usDataLen,
                            &usIterSearchCount);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!usIterSearchCount)
            {
                break;
            }

            usSearchResultCount += usIterSearchCount;
        }
    }

    if (!usSearchResultCount)
    {
        ntStatus = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppData = pData;
    *pusDataLen = usDataLen;
    *pusSearchResultCount = usSearchResultCount;
    *pbEndOfSearch = pSearchSpace->pFileInfoCursor ? FALSE : bEndOfSearch;

cleanup:

    return ntStatus;

error:

    *ppData = NULL;
    *pusDataLen = 0;
    *pusSearchResultCount = 0;
    *pbEndOfSearch = FALSE;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvFinderMarshallIdBothDirInfoResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    USHORT            usBytesAvailable,
    USHORT            usDesiredSearchCount,
    PUSHORT           pusDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchCount
    )
{
    NTSTATUS ntStatus = 0;
    USHORT   usBytesRequired = 0;
    PSMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO_HEADER pInfoHeader = NULL;
    PFILE_ID_BOTH_DIR_INFORMATION pFileInfoCursor = NULL;
    PBYTE    pData = *ppData;
    USHORT   usDataLen = 0;
    USHORT   usOrigDataLen = *pusDataLen;
    PBYTE    pDataCursor = NULL;
    USHORT   usSearchCount = 0;
    USHORT   iSearchCount = 0;
    USHORT   usOffset = 0;
    USHORT   usDataOffset = *pusDataOffset;

    pFileInfoCursor = (PFILE_ID_BOTH_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

    while (pFileInfoCursor &&
           (usSearchCount < usDesiredSearchCount) &&
           (usBytesAvailable > 0))
    {
        USHORT usInfoBytesRequired = 0;

        usInfoBytesRequired = sizeof(SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO_HEADER);
        usDataOffset += sizeof(SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO_HEADER);

        if (pSearchSpace->bUseLongFilenames)
        {
            usInfoBytesRequired += pFileInfoCursor->FileNameLength;
            usDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (!pFileInfoCursor->FileNameLength)
        {
            usInfoBytesRequired += sizeof(wchar16_t);
            usDataOffset += sizeof(wchar16_t);
        }

        if (usDataOffset % 8)
        {
            USHORT usAlignment = (8 - (usDataOffset % 8));

            usInfoBytesRequired += usAlignment;
            usDataOffset += usAlignment;
        }

        if (usBytesAvailable < usInfoBytesRequired)
        {
            break;
        }

        usSearchCount++;
        usBytesAvailable -= usInfoBytesRequired;
        usBytesRequired += usInfoBytesRequired;

        if (pFileInfoCursor->NextEntryOffset)
        {
            pFileInfoCursor = (PFILE_ID_BOTH_DIR_INFORMATION)(((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileInfoCursor = NULL;
        }
    }

    if (!pData)
    {
        ntStatus = SrvAllocateMemory(
                        usBytesRequired,
                        (PVOID*)&pData);
        BAIL_ON_NT_STATUS(ntStatus);

        usDataLen = usBytesRequired;
    }
    else
    {
        USHORT usNewDataLen = usOrigDataLen + usBytesRequired;

        ntStatus = SrvReallocMemory(pData, usNewDataLen, (PVOID*)&pData);
        BAIL_ON_NT_STATUS(ntStatus);

        usDataLen = usNewDataLen;

        // Got to fill the last offset from previous data
        usOffset = usOrigDataLen;
        pDataCursor = pData;
        pInfoHeader = (PSMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO_HEADER)pDataCursor;
        while (pInfoHeader->NextEntryOffset)
        {
            usOffset -= pInfoHeader->NextEntryOffset;
            pInfoHeader = (PSMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO_HEADER)((PBYTE)pInfoHeader + pInfoHeader->NextEntryOffset);
        }
    }

    usDataOffset = *pusDataOffset;

    pDataCursor = pData + usOrigDataLen;
    for (; iSearchCount < usSearchCount; iSearchCount++)
    {
        pFileInfoCursor = (PFILE_ID_BOTH_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

        if (pInfoHeader)
        {
            pInfoHeader->NextEntryOffset = usOffset;
        }

        pInfoHeader = (PSMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO_HEADER)pDataCursor;

        usOffset = 0;

        pInfoHeader->FileIndex = pFileInfoCursor->FileIndex;
        pInfoHeader->EaSize = pFileInfoCursor->EaSize;
        pInfoHeader->CreationTime = pFileInfoCursor->CreationTime;
        pInfoHeader->LastAccessTime = pFileInfoCursor->LastAccessTime;
        pInfoHeader->LastWriteTime = pFileInfoCursor->LastWriteTime;
        pInfoHeader->ChangeTime = pFileInfoCursor->ChangeTime;
        pInfoHeader->EndOfFile = pFileInfoCursor->EndOfFile;
        pInfoHeader->AllocationSize = pFileInfoCursor->AllocationSize;
        pInfoHeader->FileAttributes = pFileInfoCursor->FileAttributes;
        pInfoHeader->FileNameLength = pFileInfoCursor->FileNameLength;
        pInfoHeader->FileId = pFileInfoCursor->FileId;
        pInfoHeader->Reserved = 0;

        if (!pSearchSpace->bUseLongFilenames)
        {
            memcpy((PBYTE)pInfoHeader->ShortName, (PBYTE)pFileInfoCursor->ShortName, sizeof(pInfoHeader->ShortName));

            pInfoHeader->ShortNameLength = pFileInfoCursor->ShortNameLength;
        }
        else
        {
            memset((PBYTE)pInfoHeader->ShortName, 0x0, sizeof(pInfoHeader->ShortName));

            pInfoHeader->ShortNameLength = 0;
        }

        pDataCursor += sizeof(SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO_HEADER);
        usOffset += sizeof(SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO_HEADER);
        usDataOffset += sizeof(SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO_HEADER);

        if (pSearchSpace->bUseLongFilenames && pFileInfoCursor->FileNameLength)
        {
            memcpy(pDataCursor, (PBYTE)pFileInfoCursor->FileName, pFileInfoCursor->FileNameLength);

            pDataCursor += pFileInfoCursor->FileNameLength;
            usOffset += pFileInfoCursor->FileNameLength;
            usDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (!pFileInfoCursor->FileNameLength)
        {
            pDataCursor += sizeof(wchar16_t);
            usOffset += sizeof(wchar16_t);
            usDataOffset += sizeof(wchar16_t);
        }

        if (usDataOffset % 8)
        {
            USHORT usAlignment = (8 - (usDataOffset % 8));

            pDataCursor += usAlignment;
            usOffset += usAlignment;
            usDataOffset += usAlignment;
        }

        if (pFileInfoCursor->NextEntryOffset)
        {
            pSearchSpace->pFileInfoCursor = (((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pSearchSpace->pFileInfoCursor = NULL;
        }
    }

    if (pInfoHeader)
    {
        pInfoHeader->NextEntryOffset = 0;
    }

    *pusSearchCount = usSearchCount;
    *pusDataLen = usDataLen;
    *pusDataOffset = usDataOffset;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *pusSearchCount = 0;
    *pusDataLen = 0;
    *ppData = NULL;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvFinderGetIdFullDirInfoSearchResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    BOOLEAN           bReturnSingleEntry,
    BOOLEAN           bRestartScan,
    USHORT            usDesiredSearchCount,
    USHORT            usMaxDataCount,
    USHORT            usDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchResultCount,
    PBOOLEAN          pbEndOfSearch
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    USHORT   usDataLen = 0;
    USHORT   usSearchResultCount = 0;
    USHORT   usBytesAvailable = usMaxDataCount;
    BOOLEAN  bEndOfSearch = FALSE;
    PFILE_ID_FULL_DIR_INFORMATION pFileInfoCursor = NULL;

    if (!pSearchSpace->pFileInfo)
    {
        // Keep space for 10 items
        USHORT usBytesAllocated = (sizeof(FILE_ID_FULL_DIR_INFORMATION) + 256 * sizeof(wchar16_t)) * 10;

        ntStatus = SrvAllocateMemory(
                        usBytesAllocated,
                        (PVOID*)&pSearchSpace->pFileInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pSearchSpace->usFileInfoLen = usBytesAllocated;
    }

    while (!bEndOfSearch && (usSearchResultCount < usDesiredSearchCount))
    {
        USHORT usIterSearchCount = 0;

        pFileInfoCursor = (PFILE_ID_FULL_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;
        if (!pFileInfoCursor)
        {
            IO_MATCH_FILE_SPEC ioFileSpec = {0};
            IO_STATUS_BLOCK ioStatusBlock = {0};

            ioFileSpec.Type = IO_MATCH_FILE_SPEC_TYPE_UNKNOWN;
            // ioFileSpec.Options = IO_NAME_OPTION_CASE_SENSITIVE;
            RtlUnicodeStringInit(
                &ioFileSpec.Pattern,
                pSearchSpace->pwszSearchPattern);

            pSearchSpace->pFileInfoCursor = NULL;

            do
            {
                memset(pSearchSpace->pFileInfo, 0x0, pSearchSpace->usFileInfoLen);

                ntStatus = IoQueryDirectoryFile(
                                pSearchSpace->hFile,
                                NULL,
                                &ioStatusBlock,
                                pSearchSpace->pFileInfo,
                                pSearchSpace->usFileInfoLen,
                                FileIdFullDirectoryInformation,
                                bReturnSingleEntry,
                                &ioFileSpec,
                                bRestartScan);
                if (ntStatus == STATUS_SUCCESS)
                {
                    pSearchSpace->pFileInfoCursor = pSearchSpace->pFileInfo;

                    break;
                }
                else if (ntStatus == STATUS_NO_MORE_MATCHES)
                {
                    bEndOfSearch = TRUE;
                    ntStatus = 0;

                    break;
                }
                else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
                {
                    USHORT usNewSize = pSearchSpace->usFileInfoLen + 256 * sizeof(wchar16_t);

                    ntStatus = SrvReallocMemory(
                                    pSearchSpace->pFileInfo,
                                    usNewSize,
                                    (PVOID*)&pSearchSpace->pFileInfo);
                    BAIL_ON_NT_STATUS(ntStatus);

                    memset(pSearchSpace->pFileInfo + pSearchSpace->usFileInfoLen,
                           0,
                           usNewSize - pSearchSpace->usFileInfoLen);

                    pSearchSpace->usFileInfoLen = usNewSize;

                    continue;
                }
                BAIL_ON_NT_STATUS(ntStatus);

            } while (TRUE);
        }

        if (pSearchSpace->pFileInfoCursor)
        {
            ntStatus = SrvFinderMarshallIdFullDirInfoResults(
                            pSearchSpace,
                            usBytesAvailable - usDataLen,
                            usDesiredSearchCount - usSearchResultCount,
                            &usDataOffset,
                            &pData,
                            &usDataLen,
                            &usIterSearchCount);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!usIterSearchCount)
            {
                break;
            }

            usSearchResultCount += usIterSearchCount;
        }
    }

    if (!usSearchResultCount)
    {
        ntStatus = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppData = pData;
    *pusDataLen = usDataLen;
    *pusSearchResultCount = usSearchResultCount;
    *pbEndOfSearch = pSearchSpace->pFileInfoCursor ? FALSE : bEndOfSearch;

cleanup:

    return ntStatus;

error:

    *ppData = NULL;
    *pusDataLen = 0;
    *pusSearchResultCount = 0;
    *pbEndOfSearch = FALSE;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvFinderMarshallIdFullDirInfoResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    USHORT            usBytesAvailable,
    USHORT            usDesiredSearchCount,
    PUSHORT           pusDataOffset,
    PBYTE*            ppData,
    PUSHORT           pusDataLen,
    PUSHORT           pusSearchCount
    )
{
    NTSTATUS ntStatus = 0;
    USHORT   usBytesRequired = 0;
    PSMB_FIND_FILE_ID_FULL_DIRECTORY_INFO_HEADER pInfoHeader = NULL;
    PFILE_ID_FULL_DIR_INFORMATION pFileInfoCursor = NULL;
    PBYTE    pData = *ppData;
    USHORT   usDataLen = 0;
    USHORT   usOrigDataLen = *pusDataLen;
    PBYTE    pDataCursor = NULL;
    USHORT   usSearchCount = 0;
    USHORT   iSearchCount = 0;
    USHORT   usOffset = 0;
    USHORT   usDataOffset = *pusDataOffset;

    pFileInfoCursor = (PFILE_ID_FULL_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

    while (pFileInfoCursor &&
           (usSearchCount < usDesiredSearchCount) &&
           (usBytesAvailable > 0))
    {
        USHORT usInfoBytesRequired = 0;

        usInfoBytesRequired = sizeof(SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO_HEADER);
        usDataOffset += sizeof(SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO_HEADER);

        if (pSearchSpace->bUseLongFilenames)
        {
            usInfoBytesRequired += pFileInfoCursor->FileNameLength;
            usDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (!pFileInfoCursor->FileNameLength)
        {
            usInfoBytesRequired += sizeof(wchar16_t);
            usDataOffset += sizeof(wchar16_t);
        }

        if (usDataOffset % 8)
        {
            USHORT usAlignment = (8 - (usDataOffset % 8));

            usInfoBytesRequired += usAlignment;
            usDataOffset += usAlignment;
        }

        if (usBytesAvailable < usInfoBytesRequired)
        {
            break;
        }

        usSearchCount++;
        usBytesAvailable -= usInfoBytesRequired;
        usBytesRequired += usInfoBytesRequired;

        if (pFileInfoCursor->NextEntryOffset)
        {
            pFileInfoCursor = (PFILE_ID_FULL_DIR_INFORMATION)(((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileInfoCursor = NULL;
        }
    }

    if (!pData)
    {
        ntStatus = SrvAllocateMemory(
                        usBytesRequired,
                        (PVOID*)&pData);
        BAIL_ON_NT_STATUS(ntStatus);

        usDataLen = usBytesRequired;
    }
    else
    {
        USHORT usNewDataLen = usOrigDataLen + usBytesRequired;

        ntStatus = SrvReallocMemory(pData, usNewDataLen, (PVOID*)&pData);
        BAIL_ON_NT_STATUS(ntStatus);

        usDataLen = usNewDataLen;

        // Got to fill the last offset from previous data
        usOffset = usOrigDataLen;
        pDataCursor = pData;
        pInfoHeader = (PSMB_FIND_FILE_ID_FULL_DIRECTORY_INFO_HEADER)pDataCursor;
        while (pInfoHeader->NextEntryOffset)
        {
            usOffset -= pInfoHeader->NextEntryOffset;
            pInfoHeader = (PSMB_FIND_FILE_ID_FULL_DIRECTORY_INFO_HEADER)((PBYTE)pInfoHeader + pInfoHeader->NextEntryOffset);
        }
    }

    usDataOffset = *pusDataOffset;

    pDataCursor = pData + usOrigDataLen;
    for (; iSearchCount < usSearchCount; iSearchCount++)
    {
        pFileInfoCursor = (PFILE_ID_FULL_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

        if (pInfoHeader)
        {
            pInfoHeader->NextEntryOffset = usOffset;
        }

        pInfoHeader = (PSMB_FIND_FILE_ID_FULL_DIRECTORY_INFO_HEADER)pDataCursor;

        usOffset = 0;

        pInfoHeader->FileIndex = pFileInfoCursor->FileIndex;
        pInfoHeader->EaSize = pFileInfoCursor->EaSize;
        pInfoHeader->CreationTime = pFileInfoCursor->CreationTime;
        pInfoHeader->LastAccessTime = pFileInfoCursor->LastAccessTime;
        pInfoHeader->LastWriteTime = pFileInfoCursor->LastWriteTime;
        pInfoHeader->ChangeTime = pFileInfoCursor->ChangeTime;
        pInfoHeader->EndOfFile = pFileInfoCursor->EndOfFile;
        pInfoHeader->AllocationSize = pFileInfoCursor->AllocationSize;
        pInfoHeader->FileAttributes = pFileInfoCursor->FileAttributes;
        pInfoHeader->FileNameLength = pFileInfoCursor->FileNameLength;
        pInfoHeader->FileId = pFileInfoCursor->FileId;

        pDataCursor += sizeof(SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO_HEADER);
        usOffset += sizeof(SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO_HEADER);
        usDataOffset += sizeof(SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO_HEADER);

        if (pSearchSpace->bUseLongFilenames && pFileInfoCursor->FileNameLength)
        {
            memcpy(pDataCursor, (PBYTE)pFileInfoCursor->FileName, pFileInfoCursor->FileNameLength);

            pDataCursor += pFileInfoCursor->FileNameLength;
            usOffset += pFileInfoCursor->FileNameLength;
            usDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (!pFileInfoCursor->FileNameLength)
        {
            pDataCursor += sizeof(wchar16_t);
            usOffset += sizeof(wchar16_t);
            usDataOffset += sizeof(wchar16_t);
        }

        if (usDataOffset % 8)
        {
            USHORT usAlignment = (8 - (usDataOffset % 8));

            pDataCursor += usAlignment;
            usOffset += usAlignment;
            usDataOffset += usAlignment;
        }

        if (pFileInfoCursor->NextEntryOffset)
        {
            pSearchSpace->pFileInfoCursor = (((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pSearchSpace->pFileInfoCursor = NULL;
        }
    }

    if (pInfoHeader)
    {
        pInfoHeader->NextEntryOffset = 0;
    }

    *pusSearchCount = usSearchCount;
    *pusDataLen = usDataLen;
    *pusDataOffset = usDataOffset;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *pusSearchCount = 0;
    *pusDataLen = 0;
    *ppData = NULL;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}


static
NTSTATUS
SrvFinderGetUnixSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    USHORT              usDataOffset,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    )
{
    return STATUS_NOT_SUPPORTED;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
