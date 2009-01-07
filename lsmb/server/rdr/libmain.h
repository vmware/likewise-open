
NTSTATUS
RdrTreeConnect(
    PCWSTR  pszShareName,
    TID * pTid
    );

NTSTATUS
RdrNTCreate(
    HANDLE hTreeObject,
    ULONG RootDirectoryFid,
    ACCESS_MASK DesiredAccess,
    ULONG ExtFileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    ULONG ImpersonationLevel,
    UCHAR SecurityFlags,
    LPWSTR pszFileName,
    FID * pFid
    );

NTSTATUS
RdrNTTransactCreate(
    HANDLE hTreeObject,
    ULONG RootDirectoryFid,
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
    FID * pFid
    );

NTSTATUS
RdrCreateTemporary(
    HANDLE hTreeObject,
    LPWSTR pszDirectoryName,
    LPWSTR * ppszFileName,
    USHORT * usFid
    );

NTSTATUS
RdrReadFile(
    HANDLE hTreeObject,
    USHORT usFid,
    ULONG ulOffset,
    UCHAR  *pBuffer,
    USHORT MaxCount
    );

NTSTATUS
RdrWriteFile(
    HANDLE hTreeObject,
    USHORT WriteMode,
    UCHAR *pBuffer,
    USHORT usByteCount,
    USHORT *pusBytesWritten
    );

NTSTATUS
RdrLockFile(
    HANDLE hTreeObject,
    USHORT usFid,
    UCHAR LockType,
    ULONG TimeOut,
    USHORT NumberofUnlocks,
    USHORT NumberOfLocks,
    LOCKING_ANDX_RANGE Unlocks[],
    LOCKING_ANDX_RANGE Locks[]
    );

NTSTATUS
RdrSeekFile(
    HANDLE hTreeObject,
    USHORT usFid,
    USHORT Mode,
    ULONG Offset,
    ULONG * plReturnedOffset
    );

NTSTATUS
RdrFlushFile(
    HANDLE hTreeObject
    );

NTSTATUS
RdrCloseFile(
    HANDLE hTreeObject
    );

NTSTATUS
RdrCloseFileAndDisconnect(
    HANDLE hTreeObject,
    USHORT usFid
    );

NTSTATUS
RdrDeleteFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    LPWSTR pszFileName
    );

NTSTATUS
RdrRenameFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    LPWSTR pszOldFileName,
    LPWSTR pszNewFileName
    );

NTSTATUS
RdrCopyFile(
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
RdrTrans2QueryPathInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    LPWSTR FileName
    );

NTSTATUS
RdrTrans2QueryFileInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    LPWSTR FileName
    );

NTSTATUS
RdrTrans2SetPathInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    LPWSTR FileName
    );

NTSTATUS
RdrTrans2CreateDirectory(
    HANDLE hTreeObject
    );

NTSTATUS
RdrTrans2DeleteDirectory(
    HANDLE hTreeObject,
    LPWSTR DirectoryName[]
    );

NTSTATUS
RdrTrans2CheckDirectory(
    HANDLE hTreeObject,
    LPWSTR DirectoryName[]
    );

NTSTATUS
RdrTrans2FindFirst2(
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
RdrTrans2FindNext2(
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
RdrNTTransactNotifyChange(
    HANDLE hTreeObject,
    USHORT Fid,
    BOOLEAN WatchTree,
    UCHAR Reserved
    );

NTSTATUS
RdrTrans2GetDFSReferral(
    HANDLE hTreeObject
    );

int
RefOrCreateSocket(
    uchar8_t    *pszHostname,
    SMB_SOCKET **ppSocket
    );

uint32_t
RefOrCreateAuthSession(
    SMB_SOCKET   *pSocket,
    uchar8_t     *pszPrincipal,
    SMB_SESSION **ppSession
    );

uint32_t
ReleaseAuthSession(
    SMB_SESSION *pSession
    );

uint32_t
RefOrCreateTree(
    SMB_SESSION *pSession,
    uchar8_t    *pwszPath,
    SMB_TREE   **pTree
    );

