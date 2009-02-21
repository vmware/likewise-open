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
#include "srvfinder_p.h"

typedef struct _SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER
{
    ULONG     NextEntryOffset;
    ULONG     FileIndex;
    LONG64    CreationTime;
    LONG64    LastAccessTime;
    LONG64    LastWriteTime;
    LONG64    ChangeTime;
    LONG64    EndOfFile;
    LONG64    AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    ULONG     FileNameLength;
    ULONG     EaSize;
    UCHAR     ShortNameLength;
    UCHAR     Reserved;
    wchar16_t ShortName[12];
} __attribute__((__packed__)) SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER, *PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER;

static
VOID
SrvFinderFreeRepository(
    PSRV_FINDER_REPOSITORY pFinderRepository
    );

static
int
SrvFinderCompareSearchSpaces(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SrvFinderFreeData(
    PVOID pData
    );

static
VOID
SrvFinderFreeSearchSpace(
    PSRV_SEARCH_SPACE pSearchSpace
    );

static
NTSTATUS
SrvFinderGetStdInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
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
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvFinderGetFullDirInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvFinderGetNamesInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvFinderGetBothDirInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
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
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    );

NTSTATUS
SrvFinderCreateRepository(
    PHANDLE phFinderRepository
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_FINDER_REPOSITORY pFinderRepository = NULL;

    ntStatus = LW_RTL_ALLOCATE(
                    &pFinderRepository,
                    SRV_FINDER_REPOSITORY,
                    sizeof(SRV_FINDER_REPOSITORY));
    BAIL_ON_NT_STATUS(ntStatus);

    pFinderRepository->refCount = 1;

    pthread_mutex_init(&pFinderRepository->mutex, NULL);
    pFinderRepository->pMutex = &pFinderRepository->mutex;

    ntStatus = LwRtlRBTreeCreate(
                    &SrvFinderCompareSearchSpaces,
                    NULL,
                    &SrvFinderFreeData,
                    &pFinderRepository->pSearchSpaceCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    *phFinderRepository = pFinderRepository;

cleanup:

    return ntStatus;

error:

    *phFinderRepository = NULL;

    if (pFinderRepository)
    {
        SrvFinderFreeRepository(pFinderRepository);
    }

    goto cleanup;
}

NTSTATUS
SrvFinderCreateSearchSpace(
    HANDLE         hFinderRepository,
    PWSTR          pwszFilesystemPath,
    PWSTR          pwszSearchPattern,
    USHORT         usSearchAttrs,
    ULONG          ulSearchStorageType,
    SMB_INFO_LEVEL infoLevel,
    BOOLEAN        bUseLongFilenames,
    PHANDLE        phFinder,
    PUSHORT        pusSearchId
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE      hFile = NULL;
    IO_STATUS_BLOCK     ioStatusBlock = {0};
    IO_FILE_NAME        fileName = {0};
    PVOID               pSecurityDescriptor = NULL;
    PVOID               pSecurityQOS = NULL;
    PIO_CREATE_SECURITY_CONTEXT pSecurityContext = NULL;
    PSRV_FINDER_REPOSITORY pFinderRepository = NULL;
    PSRV_SEARCH_SPACE pSearchSpace = NULL;
    USHORT   usCandidateSearchId = 0;
    BOOLEAN  bFound = FALSE;
    BOOLEAN  bInLock = FALSE;

    pFinderRepository = (PSRV_FINDER_REPOSITORY)hFinderRepository;

    fileName.FileName = pwszFilesystemPath;

    ntStatus = IoCreateFile(
                    &hFile,
                    NULL,
                    &ioStatusBlock,
                    pSecurityContext,
                    &fileName,
                    pSecurityDescriptor,
                    pSecurityQOS,
                    GENERIC_READ,
                    0,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ,
                    FILE_OPEN,
                    0,
                    NULL, /* EA Buffer */
                    0,    /* EA Length */
                    NULL  /* ECP List  */
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_LOCK_MUTEX(bInLock, &pFinderRepository->mutex);

    usCandidateSearchId = pFinderRepository->usNextSearchId;

    do
    {
        PSRV_SEARCH_SPACE pSearchSpace = NULL;

        if (!usCandidateSearchId || (usCandidateSearchId == UINT16_MAX))
        {
            usCandidateSearchId++;
        }

        ntStatus = LwRtlRBTreeFind(
                        pFinderRepository->pSearchSpaceCollection,
                        &usCandidateSearchId,
                        (PVOID*)&pSearchSpace);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = 0;
            bFound = TRUE;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((usCandidateSearchId != pFinderRepository->usNextSearchId) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_OPENED_FILES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LW_RTL_ALLOCATE(
                    &pSearchSpace,
                    SRV_SEARCH_SPACE,
                    sizeof(SRV_SEARCH_SPACE));
    BAIL_ON_NT_STATUS(ntStatus);

    pSearchSpace->refCount = 1;

    pthread_mutex_init(&pSearchSpace->mutex, NULL);
    pSearchSpace->pMutex = &pSearchSpace->mutex;

    pSearchSpace->usSearchId = usCandidateSearchId;

    ntStatus = LwRtlRBTreeAdd(
                    pFinderRepository->pSearchSpaceCollection,
                    &pSearchSpace->usSearchId,
                    pSearchSpace);
    BAIL_ON_NT_STATUS(ntStatus);

    pSearchSpace->hFile = hFile;
    hFile = NULL;
    pSearchSpace->infoLevel = infoLevel;
    pSearchSpace->usSearchAttrs = usSearchAttrs;
    pSearchSpace->ulSearchStorageType = ulSearchStorageType;
    pSearchSpace->bUseLongFilenames = bUseLongFilenames;

    ntStatus = SMBAllocateStringW(
                    pwszSearchPattern,
                    &pSearchSpace->pwszSearchPattern);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pSearchSpace->refCount);

    pFinderRepository->usNextSearchId = usCandidateSearchId + 1;

    *phFinder = pSearchSpace;
    *pusSearchId = usCandidateSearchId;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pFinderRepository->mutex);

    return ntStatus;

error:

    *phFinder = NULL;
    *pusSearchId = 0;

    if (pSearchSpace)
    {
        SrvFinderReleaseSearchSpace(pSearchSpace);
    }

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    goto cleanup;
}

NTSTATUS
SrvFinderGetSearchSpace(
    HANDLE  hFinderRepository,
    USHORT  usSearchId,
    PHANDLE phFinder
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_FINDER_REPOSITORY pFinderRepository = NULL;
    PSRV_SEARCH_SPACE pSearchSpace = NULL;
    BOOLEAN bInLock = FALSE;

    pFinderRepository = (PSRV_FINDER_REPOSITORY)hFinderRepository;

    SMB_LOCK_MUTEX(bInLock, &pFinderRepository->mutex);

    ntStatus = LwRtlRBTreeFind(
                    pFinderRepository->pSearchSpaceCollection,
                    &usSearchId,
                    (PVOID*)&pSearchSpace);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pSearchSpace->refCount);

    *phFinder = pSearchSpace;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pFinderRepository->mutex);

    return ntStatus;

error:

    *phFinder = NULL;

    goto cleanup;
}

NTSTATUS
SrvFinderGetSearchResults(
    HANDLE   hSearchSpace,
    BOOLEAN  bReturnSingleEntry,
    BOOLEAN  bRestartScan,
    USHORT   usDesiredSearchCount,
    USHORT   usMaxDataCount,
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

    SMB_LOCK_MUTEX(bInLock, &pSearchSpace->mutex);

    switch (pSearchSpace->infoLevel)
    {
        case SMB_INFO_STANDARD:

            ntStatus = SrvFinderGetStdInfoSearchResults(
                            pSearchSpace,
                            bReturnSingleEntry,
                            bRestartScan,
                            usDesiredSearchCount,
                            usMaxDataCount,
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

    SMB_UNLOCK_MUTEX(bInLock, &pSearchSpace->mutex);

    return ntStatus;

error:

    *ppData = NULL;
    *pusDataLen = 0;
    *pusSearchResultCount = 0;
    *pbEndOfSearch = FALSE;

    if (pData)
    {
        LwRtlMemoryFree(pData);
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
SrvFinderGetFullDirInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
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
SrvFinderGetNamesInfoSearchResults(
    PSRV_SEARCH_SPACE   pSearchSpace,
    BOOLEAN             bReturnSingleEntry,
    BOOLEAN             bRestartScan,
    USHORT              usDesiredSearchCount,
    USHORT              usMaxDataCount,
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
SrvFinderGetBothDirInfoSearchResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    BOOLEAN           bReturnSingleEntry,
    BOOLEAN           bRestartScan,
    USHORT            usDesiredSearchCount,
    USHORT            usMaxDataCount,
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

        ntStatus = LW_RTL_ALLOCATE(
                        &pSearchSpace->pFileInfo,
                        BYTE,
                        usBytesAllocated);
        BAIL_ON_NT_STATUS(ntStatus);

        pSearchSpace->usFileInfoLen = usBytesAllocated;
    }

    while (!bEndOfSearch && (usSearchResultCount < usDesiredSearchCount))
    {
        USHORT usIterSearchCount = 0;

        pFileInfoCursor = (PFILE_BOTH_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;
        if (!pFileInfoCursor || !pFileInfoCursor->NextEntryOffset)
        {
            IO_FILE_SPEC ioFileSpec;
            IO_STATUS_BLOCK ioStatusBlock = {0};

            ioFileSpec.Type = IO_FILE_SPEC_TYPE_UNKNOWN;
            // ioFileSpec.Options = IO_NAME_OPTION_CASE_SENSITIVE;
            RtlUnicodeStringInit(
                &ioFileSpec.FileName,
                pSearchSpace->pwszSearchPattern);

            pSearchSpace->pFileInfoCursor = NULL;

            do
            {
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

                    ntStatus = SMBReallocMemory(
                                    pSearchSpace->pFileInfo,
                                    (PVOID*)&pSearchSpace->pFileInfo,
                                    usNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pSearchSpace->usFileInfoLen = usNewSize;

                    continue;
                }
                BAIL_ON_NT_STATUS(ntStatus);

            } while (TRUE);
        }

        if (!bEndOfSearch)
        {
            ntStatus = SrvFinderMarshallBothDirInfoResults(
                            pSearchSpace,
                            usBytesAvailable - usDataLen,
                            usDesiredSearchCount - usSearchResultCount,
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
        ntStatus = STATUS_NO_SUCH_FILE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppData = pData;
    *pusDataLen = usDataLen;
    *pusSearchResultCount = usSearchResultCount;
    *pbEndOfSearch = bEndOfSearch;

cleanup:

    return ntStatus;

error:

    *ppData = NULL;
    *pusDataLen = 0;
    *pusSearchResultCount = 0;
    *pbEndOfSearch = FALSE;

    if (pData)
    {
        LwRtlMemoryFree(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvFinderMarshallBothDirInfoResults(
    PSRV_SEARCH_SPACE pSearchSpace,
    USHORT            usBytesAvailable,
    USHORT            usDesiredSearchCount,
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

    pFileInfoCursor = (PFILE_BOTH_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

    while (pFileInfoCursor &&
           (usSearchCount < usDesiredSearchCount) &&
           (usBytesAvailable > 0))
    {
        USHORT usInfoBytesRequired = 0;

        usInfoBytesRequired = sizeof(SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER);
        if (pSearchSpace->bUseLongFilenames)
        {
            usInfoBytesRequired += wc16slen(pFileInfoCursor->FileName) * sizeof(wchar16_t);
        }
        usInfoBytesRequired += sizeof(wchar16_t);

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
        ntStatus = LW_RTL_ALLOCATE(
                        &pData,
                        BYTE,
                        usBytesRequired);
        BAIL_ON_NT_STATUS(ntStatus);

        usDataLen = usBytesRequired;
    }
    else
    {
        USHORT usNewDataLen = usOrigDataLen + usBytesRequired;

        ntStatus = SMBReallocMemory(
                        pData,
                        (PVOID*)&pData,
                        usNewDataLen);
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
        pInfoHeader->CreationTime = pFileInfoCursor->CreationTime;
        pInfoHeader->LastAccessTime = pFileInfoCursor->LastAccessTime;
        pInfoHeader->LastWriteTime = pFileInfoCursor->LastWriteTime;
        pInfoHeader->ChangeTime = pFileInfoCursor->ChangeTime;
        pInfoHeader->EndOfFile = pFileInfoCursor->EndOfFile;
        pInfoHeader->AllocationSize = pFileInfoCursor->AllocationSize;
        pInfoHeader->FileAttributes = pFileInfoCursor->FileAttributes;
        pInfoHeader->FileNameLength = pFileInfoCursor->FileNameLength * sizeof(wchar16_t);

        if (!pSearchSpace->bUseLongFilenames)
        {
            memcpy((PBYTE)pInfoHeader->ShortName, (PBYTE)pFileInfoCursor->ShortName, sizeof(pInfoHeader->ShortName));

            pInfoHeader->ShortNameLength = pFileInfoCursor->ShortNameLength;
        }

        pDataCursor += sizeof(SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER);
        usOffset += sizeof(SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER);

        if (pSearchSpace->bUseLongFilenames)
        {
            USHORT usFileNameLen = wc16slen(pFileInfoCursor->FileName);

            if (usFileNameLen)
            {
                memcpy(pDataCursor, (PBYTE)pFileInfoCursor->FileName, usFileNameLen * sizeof(wchar16_t));
            }

            pDataCursor += usFileNameLen * sizeof(wchar16_t);
            usOffset += usFileNameLen * sizeof(wchar16_t);
        }
        pDataCursor += sizeof(wchar16_t);
        usOffset += sizeof(wchar16_t);

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
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *pusSearchCount = 0;
    *pusDataLen = 0;
    *ppData = NULL;

    if (pData)
    {
        LwRtlMemoryFree(pData);
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
    PBYTE*              ppData,
    PUSHORT             pusDataLen,
    PUSHORT             pusSearchResultCount,
    PBOOLEAN            pbEndOfSearch
    )
{
    return STATUS_NOT_SUPPORTED;
}

VOID
SrvFinderReleaseSearchSpace(
    HANDLE hFinder
    )
{
    PSRV_SEARCH_SPACE pSearchSpace = (PSRV_SEARCH_SPACE)hFinder;

    if (InterlockedDecrement(&pSearchSpace->refCount) == 0)
    {
        SrvFinderFreeSearchSpace(pSearchSpace);
    }
}

NTSTATUS
SrvFinderCloseSearchSpace(
    HANDLE hFinderRepository,
    USHORT usSearchId
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_FINDER_REPOSITORY pFinderRepository = NULL;
    BOOLEAN bInLock = FALSE;

    pFinderRepository = (PSRV_FINDER_REPOSITORY)hFinderRepository;

    SMB_LOCK_MUTEX(bInLock, &pFinderRepository->mutex);

    ntStatus = LwRtlRBTreeRemove(
                    pFinderRepository->pSearchSpaceCollection,
                    &usSearchId);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pFinderRepository->mutex);

    return ntStatus;

error:

    goto cleanup;
}

VOID
SrvFinderCloseRepository(
    HANDLE hFinderRepository
    )
{
    PSRV_FINDER_REPOSITORY pFinderRepository = NULL;

    pFinderRepository = (PSRV_FINDER_REPOSITORY)hFinderRepository;

    if (InterlockedDecrement(&pFinderRepository->refCount) == 0)
    {
        SrvFinderFreeRepository(pFinderRepository);
    }
}

static
VOID
SrvFinderFreeRepository(
    PSRV_FINDER_REPOSITORY pFinderRepository
    )
{
    if (pFinderRepository->pSearchSpaceCollection)
    {
        LwRtlRBTreeFree(pFinderRepository->pSearchSpaceCollection);
    }

    if (pFinderRepository->pMutex)
    {
        pthread_mutex_destroy(&pFinderRepository->mutex);
    }

    LwRtlMemoryFree(pFinderRepository);
}

static
int
SrvFinderCompareSearchSpaces(
    PVOID pKey1,
    PVOID pKey2
    )
{
    USHORT usKey1 = *((PUSHORT)pKey1);
    USHORT usKey2 = *((PUSHORT)pKey2);

    if (usKey1 > usKey2)
    {
        return 1;
    }
    else if (usKey1 < usKey2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
VOID
SrvFinderFreeData(
    PVOID pData
    )
{
    PSRV_SEARCH_SPACE pSearchSpace = (PSRV_SEARCH_SPACE)pData;

    if (InterlockedDecrement(&pSearchSpace->refCount) == 0)
    {
        SrvFinderFreeSearchSpace(pSearchSpace);
    }
}

static
VOID
SrvFinderFreeSearchSpace(
    PSRV_SEARCH_SPACE pSearchSpace
    )
{
    if (pSearchSpace->pMutex)
    {
        pthread_mutex_destroy(&pSearchSpace->mutex);
    }

    if (pSearchSpace->hFile)
    {
        IoCloseFile(pSearchSpace->hFile);
    }

    if (pSearchSpace->pFileInfo)
    {
        LwRtlMemoryFree(pSearchSpace->pFileInfo);
    }

    LwRtlMemoryFree(pSearchSpace);
}
