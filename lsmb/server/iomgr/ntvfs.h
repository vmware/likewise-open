
DWORD
NTVfsTreeConnect(
    IN PCWSTR  pszShareName,
    OUT HANDLE * phTreeObject
    );

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
    );

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
    );

NTSTATUS
NTVfsCreateTemporary(
    HANDLE hTreeObject,
    LPWSTR pszDirectoryName,
    LPWSTR * ppszFileName,
    HANDLE * usFid
    );

NTSTATUS
NTVfsReadFile(
    HANDLE hTreeObject,
    HANDLE hFileObject,
    ULONG ulOffset,
    UCHAR  *pBuffer,
    USHORT MaxCount
    );

NTSTATUS
NTVfsWriteFile(
    HANDLE hTreeObject,
    USHORT WriteMode,
    UCHAR *pBuffer,
    USHORT usByteCount,
    USHORT *pusBytesWritten
    );

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
    );



NTSTATUS
NTVfsSeekFile(
    HANDLE hTreeObject,
    HANDLE hFileObject,
    USHORT Mode,
    ULONG Offset,
    ULONG * plReturnedOffset
    );

NTSTATUS
NTVfsFlushFile(
    HANDLE hTreeObject
    );

NTSTATUS
NTVfsCloseFile(
    HANDLE hTreeObject
    );


NTSTATUS
NTVfsCloseFileAndDisconnect(
    HANDLE hTreeObject,
    HANDLE hFileObject
    );

NTSTATUS
NTVfsDeleteFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    LPWSTR pszFileName
    );


NTSTATUS
NTVfsRenameFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    LPWSTR pszOldFileName,
    LPWSTR pszNewFileName
    );

NTSTATUS
NTVfsNTRenameFile(
    HANDLE hTreeObject
    );


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
    );

NTSTATUS
NTVfsTrans2QueryFileInformation(
    HANDLE hTreeObject
    );

NTSTATUS
NTVfsTrans2SetPathInformation(
    HANDLE hTreeObject
    );

NTSTATUS
NTVfsTrans2QueryPathInformation(
    HANDLE hTreeObject
    );


NTSTATUS
NTVfsTrans2CreateDirectory(
    HANDLE hTreeObject
    );

NTSTATUS
NTVfsDeleteDirectory(
    HANDLE hTreeObject,
    LPWSTR DirectoryName[]
    );

NTSTATUS
NTVfsCheckDirectory(
    HANDLE hTreeObject,
    LPWSTR DirectoryName[]
    );

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
    );

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
    );

NTSTATUS
NTVfsNTTransactNotifyChange(
    HANDLE hTreeObject,
    USHORT Fid,
    BOOLEAN WatchTree,
    UCHAR Reserved
    );

NTSTATUS
NTVfsTrans2GetDfsReferral(
    HANDLE hTreeObject
    );
