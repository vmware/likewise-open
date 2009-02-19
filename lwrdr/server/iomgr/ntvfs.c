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
 *       ntvfs.c
 *
 * Abstract:
 *
 *        Likewise IO Manager 
 *
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
NTVfsTreeConnect(
    IN PCWSTR  pszShareName,
    OUT HANDLE * phTreeObject
    )
{
    DWORD ntStatus = 0;
#if 0

    PNTVFS_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    HANDLE hProvider = (HANDLE)NULL;


    ENTER_NTVFS_PROVIDER_LIST_READER_LOCK(bInLock);

    for (pProvider = gpNTVfsProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {

        ntStatus = pProvider->pFnTable->pfnNTVfsTreeConnect(
                                        pszShareName,
                                        &hProviderTreeObject
                                        );
        if (!ntStatus) {

            ntStatus =  NTVfsCreateTreeObject(
                                Tid,
                                hProviderTreeObject,
                                &hTreeObject
                                );
            break;

        } else if ((ntStatus == NTVFS_STATUS_NOT_HANDLED) ||
                   (ntStatus == NTVFS_STATUS_NO_SUCH_USER)) {

            NTVfsCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;

            continue;

        } else {

            BAIL_ON_NTVFS_STATUS(ntStatus);

        }
    }

    if (pProvider == NULL)
    {
       ntStatus = NTVFS_STATUS_NO_SUCH_USER;
    }
    BAIL_ON_NTVFS_STATUS(ntStatus);

cleanup:

    LEAVE_NTVFS_PROVIDER_LIST_READER_LOCK(bInLock);

    if (!ntStatus)
    {
        NTVfsIncrementMetricValue(LsaMetricSuccessfulUserLookupsByName);
    }
    else
    {
        NTVfsIncrementMetricValue(LsaMetricFailedUserLookupsByName);
    }

    return(ntStatus);

error:

    if (ntStatus == NTVFS_STATUS_NOT_HANDLED ||
                   ntStatus == NTVFS_STATUS_NO_SUCH_USER) {
        NTVFS_LOG_VERBOSE("Unknown share: [%s] is unknown", IsNullOrEmptyString(pszShareName) ? "" : pszShareName);
    }
    else {
        NTVFS_LOG_STATUS("Failed to find user by name [%s] [code %d]", IsNullOrEmptyString(pszLoginId) ? "" : pszLoginId, ntStatus);
    }

    *phTreeObject = NULL;

    goto cleanup;
#else
    return(ntStatus);
#endif
}


NTSTATUS
NTVfsNTCreate(
    HANDLE hTreeObject,
    HANDLE hRootDirectoryObject,
    ACCESS_MASK DesiredAccess,
    ULONG ExtFileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    ULONG ImpersonationLevel,
    UCHAR SecurityFlags,
    LPWSTR pszFileName,
    HANDLE * phFileObject
    )
{

    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
NTVfsNTTransactCreate(
    HANDLE hTreeObject,
    HANDLE hRootDirectoryObject,
    ULONG ulFlags,
    ACCESS_MASK DesiredAccess,
    ULONG ExtFileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    ULONG SecurityDescriptorLength,
    ULONG EaLengyh,
    ULONG ImpersonationLevel,
    LPWSTR pszFileName,
    HANDLE * phFileObject
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsNTTransactCreate(
                    hTreeObject,
                    hRootDirectoryObject,
                    ulFlags,
                    DesiredAccess,
                    ExtFileAttributes,
                    ShareAccess,
                    CreateDisposition,
                    CreateOptions,
                    SecurityDescriptorLength,
                    EaLength,
                    ImpersonationLevel,
                    pszFileName,
                    phFileObject
                    );
#endif
    return(ntStatus);
}


NTSTATUS
NTVfsCreateTemporary(
    HANDLE hTreeObject,
    LPWSTR pszDirectoryName,
    LPWSTR * ppszFileName,
    HANDLE * hFileObject
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsCreateTemporary(
                    hTreeObject,
                    pszDirectoryName,
                    ppszFileName,
                    hFileObject
                    );
#endif

    return (ntStatus);
}


NTSTATUS
NTVfsReadFile(
    HANDLE hTreeObject,
    HANDLE hFileObject,
    ULONG ulOffset,
    UCHAR  *pBuffer,
    USHORT MaxCount
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsReadFile(
                    hTreeObject,
                    hFileObject,
                    ulOffset,
                    pBuffer,
                    MaxCount
                    );
#endif

    return (ntStatus);

}


NTSTATUS
NTVfsWriteFile(
    HANDLE hTreeObject,
    USHORT WriteMode,
    UCHAR *pBuffer,
    USHORT usByteCount,
    USHORT *pusBytesWritten
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsNTTransactCreate(
                    hTreeObject,
                    WriteMode,
                    pBuffer,
                    usByteCount,
                    pusBytesWritten
                    );
#endif

    return(ntStatus);

}

NTSTATUS
NTVfsLockFile(
    HANDLE hTreeObject,
    HANDLE hFileObject,
    UCHAR LockType,
    ULONG TimeOut,
    USHORT NumberofUnlocks,
    USHORT NumberOfLocks,
    LOCKING_ANDX_RANGE Unlocks[],
    LOCKING_ANDX_RANGE Locks[]
    )
{

    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsLockFile(
                    hTreeObject,
                    hFileObject,
                    LockType,
                    TimeOut,
                    NumberOfUnlocks,
                    NumberOfLocks,
                    Unlocks[],
                    Locks[]
                    );
#endif

    return(ntStatus);

}



NTSTATUS
NTVfsSeekFile(
    HANDLE hTreeObject,
    HANDLE hFileObject,
    USHORT Mode,
    ULONG Offset,
    ULONG * plReturnedOffset
    )
{

    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsSeekFile(
                    hTreeObject,
                    hFileObject,
                    Mode,
                    Offset,
                    plReturnedOffset
                    );
#endif

    return(ntStatus);

}

NTSTATUS
NTVfsFlushFile(
    HANDLE hTreeObject
    )
{

    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsFlushFile(
                    hTreeObject
                    );
#endif

    return(ntStatus);

}

NTSTATUS
NTVfsCloseFile(
    HANDLE hTreeObject
    )
{

    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsCloseFile(
                    hTreeObject
                    );
#endif

    return(ntStatus);

}


NTSTATUS
NTVfsCloseFileAndDisconnect(
    HANDLE hTreeObject,
    HANDLE hFileObject
    )
{

    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsCloseFileAndDisconnect(
                        hTreeObject,
                        hFileObject
                        );
#endif

    return(ntStatus);

}

NTSTATUS
NTVfsDeleteFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    LPWSTR pszFileName
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsDeleteFile(
                    hTreeObject,
                    usSearchAttributes,
                    pszFileName
                    );
#endif

    return(ntStatus);

}


NTSTATUS
NTVfsRenameFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    LPWSTR pszOldFileName,
    LPWSTR pszNewFileName
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsRenameFile(
                    hTreeObject,
                    usSearchAttributes,
                    pszOldFileName,
                    pszNewFileName
                    );
#endif

    return (ntStatus);
}

NTSTATUS
NTVfsNTRenameFile(
    HANDLE hTreeObject
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsNTRenameFile(
                        hTreeObject
                        );
#endif

    return(ntStatus);
}


NTSTATUS
NTVfsCopyFile(
    HANDLE hTreeObject,
    HANDLE hTreeObject2,
    USHORT OpenFunction,
    USHORT Flags,
    UCHAR SourceFileFormat,
    LPWSTR SourceFileName,
    UCHAR TargetFileFormat,
    LPWSTR TargetFileName
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsCopyFile(
                        hTreeObject,
                        hTreeObject2,
                        OpenFunction,
                        Flags,
                        SourceFileFormat,
                        TargetFileFormat,
                        TargetFileName
                        );
#endif

    return(ntStatus);
}

NTSTATUS
NTVfsTrans2QueryFileInformation(
    HANDLE hTreeObject
    )
{
    NTSTATUS ntStatus = 0;

#if 0

    ntStatus = pProvider->pfnProvider->NTVfsNTTransactCreate(
                                    hTreeObject,
                                    hRootDirectoryObject,
                                    ulFlags,
                                    DesiredAccess,
                                    ExtFileAttributes,
                                    ShareAccess,
                                    CreateDisposition,
                                    CreateOptions,
                                    SecurityDescriptorLength,
                                    EaLength,
                                    ImpersonationLevel,
                                    pszFileName,
                                    phFileObject
                                    );
#endif

    return(ntStatus);

}


NTSTATUS
NTVfsTrans2SetPathInformation(
    HANDLE hTreeObject
    )
{
    NTSTATUS ntStatus = 0;

#if 0

    ntStatus = pProvider->pfnProvider->NTVfsNTTransactCreate(
                                    hTreeObject,
                                    hRootDirectoryObject,
                                    ulFlags,
                                    DesiredAccess,
                                    ExtFileAttributes,
                                    ShareAccess,
                                    CreateDisposition,
                                    CreateOptions,
                                    SecurityDescriptorLength,
                                    EaLength,
                                    ImpersonationLevel,
                                    pszFileName,
                                    phFileObject
                                    );
#endif

    return(ntStatus);
}


NTSTATUS
NTVfsTrans2QueryPathInformation(
    HANDLE hTreeObject
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsNTTransactCreate(
                                    hTreeObject,
                                    hRootDirectoryObject,
                                    ulFlags,
                                    DesiredAccess,
                                    ExtFileAttributes,
                                    ShareAccess,
                                    CreateDisposition,
                                    CreateOptions,
                                    SecurityDescriptorLength,
                                    EaLength,
                                    ImpersonationLevel,
                                    pszFileName,
                                    phFileObject
                                    );
#endif

    return(ntStatus);
}


NTSTATUS
NTVfsTrans2CreateDirectory(
    HANDLE hTreeObject
    )
{

    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsNTTransactCreate(
                                    hTreeObject,
                                    hRootDirectoryObject,
                                    ulFlags,
                                    DesiredAccess,
                                    ExtFileAttributes,
                                    ShareAccess,
                                    CreateDisposition,
                                    CreateOptions,
                                    SecurityDescriptorLength,
                                    EaLength,
                                    ImpersonationLevel,
                                    pszFileName,
                                    phFileObject
                                    );
#endif

    return(ntStatus);
}

NTSTATUS
NTVfsDeleteDirectory(
    HANDLE hTreeObject,
    LPWSTR DirectoryName[]
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsNTTransactCreate(
                                    hTreeObject,
                                    hRootDirectoryObject,
                                    ulFlags,
                                    DesiredAccess,
                                    ExtFileAttributes,
                                    ShareAccess,
                                    CreateDisposition,
                                    CreateOptions,
                                    SecurityDescriptorLength,
                                    EaLength,
                                    ImpersonationLevel,
                                    pszFileName,
                                    phFileObject
                                    );
#endif

    return(ntStatus);
}

NTSTATUS
NTVfsCheckDirectory(
    HANDLE hTreeObject,
    LPWSTR DirectoryName[]
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsNTTransactCreate(
                                    hTreeObject,
                                    hRootDirectoryObject,
                                    ulFlags,
                                    DesiredAccess,
                                    ExtFileAttributes,
                                    ShareAccess,
                                    CreateDisposition,
                                    CreateOptions,
                                    SecurityDescriptorLength,
                                    EaLength,
                                    ImpersonationLevel,
                                    pszFileName,
                                    phFileObject
                                    );
#endif

    return(ntStatus);
}

NTSTATUS
NTVfsTrans2FindFirst2(
    HANDLE hTreeObject,
    USHORT SearchAttributes,
    USHORT Flags,
    USHORT InformationLevel,
    ULONG SearchStorageType,
    LPWSTR FileName,
    USHORT * pusSid,
    USHORT * puSearchCount,
    USHORT * pusEndofSearch,
    USHORT * pusLastNameOffset,
    PVOID * lppBuffer
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsNTTransactCreate(
                                    hTreeObject,
                                    hRootDirectoryObject,
                                    ulFlags,
                                    DesiredAccess,
                                    ExtFileAttributes,
                                    ShareAccess,
                                    CreateDisposition,
                                    CreateOptions,
                                    SecurityDescriptorLength,
                                    EaLength,
                                    ImpersonationLevel,
                                    pszFileName,
                                    phFileObject
                                    );
#endif

    return(ntStatus);
}

NTSTATUS
NTVfsTrans2FindNext2(
    HANDLE hTreeObject,
    USHORT usSid,
    USHORT SearchCount,
    USHORT InformationLevel,
    ULONG ResumeKey,
    USHORT Flags,
    LPWSTR FileName,
    USHORT * pusSearchCount,
    USHORT * pusEndOfSearch,
    USHORT *pusEaErrorOffset,
    USHORT * pusLastNameOffset
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsNTTransactCreate(
                                    hTreeObject,
                                    hRootDirectoryObject,
                                    ulFlags,
                                    DesiredAccess,
                                    ExtFileAttributes,
                                    ShareAccess,
                                    CreateDisposition,
                                    CreateOptions,
                                    SecurityDescriptorLength,
                                    EaLength,
                                    ImpersonationLevel,
                                    pszFileName,
                                    phFileObject
                                    );

#endif

    return ntStatus;
}

NTSTATUS
NTVfsNTTransactNotifyChange(
    HANDLE hTreeObject,
    USHORT Fid,
    BOOLEAN WatchTree,
    UCHAR Reserved
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsNTTransactCreate(
                                    hTreeObject,
                                    hRootDirectoryObject,
                                    ulFlags,
                                    DesiredAccess,
                                    ExtFileAttributes,
                                    ShareAccess,
                                    CreateDisposition,
                                    CreateOptions,
                                    SecurityDescriptorLength,
                                    EaLength,
                                    ImpersonationLevel,
                                    pszFileName,
                                    phFileObject
                                    );
#endif

    return(ntStatus);
}

NTSTATUS
NTVfsTrans2GetDfsReferral(
    HANDLE hTreeObject
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = pProvider->pfnProvider->NTVfsTrans2GetDfsReferral(
                                    hTreeObject,
                                    hRootDirectoryObject,
                                    ulFlags,
                                    DesiredAccess,
                                    ExtFileAttributes,
                                    ShareAccess,
                                    CreateDisposition,
                                    CreateOptions,
                                    SecurityDescriptorLength,
                                    EaLength,
                                    ImpersonationLevel,
                                    pszFileName,
                                    phFileObject
                                    );
#endif

    return(ntStatus);

}


