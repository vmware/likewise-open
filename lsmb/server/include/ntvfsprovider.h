#ifndef __VFSPROVIDER_H__
#define __VFSPROVIDER_H__

typedef NTSTATUS (*PVFS_LW_CREATEFILE)(
                    PSMB_SECURITY_TOKEN_REP pSecurityToken,
                    LPCWSTR pwszFileName,
                    DWORD   dwDesiredAccess,
                    DWORD   dwSharedMode,
                    DWORD   dwCreationDisposition,
                    DWORD   dwFlagsAndAttributes,
                    PSECURITY_ATTRIBUTES pSecurityAttributes,
                    PHANDLE phFile
                    );

typedef NTSTATUS (*PVFS_LW_READFILE)(
                    HANDLE hFile,
                    DWORD  dwBytesToRead,
                    PVOID* ppOutBuffer,
                    PDWORD pdwBytesRead
                    );

typedef NTSTATUS (*PVFS_LW_WRITEFILE)(
                    HANDLE hFile,
                    DWORD  dwNumBytesToWrite,
                    PVOID  pBuffer,
                    PDWORD pdwNumBytesWritten
                    );

typedef NTSTATUS (*PVFS_LW_GETSESSIONKEY)(
                    HANDLE hFile,
                    PDWORD pdwSessionKeyLength,
                    PBYTE* ppSessionKey
                    );

typedef NTSTATUS (*PVFS_LW_CLOSEFILE)(
                    HANDLE hFile
                    );

typedef NTSTATUS (*PVFS_TREE_CONNECT)(
                    PCWSTR  pszShareName,
                    TID * pTid
                    );

typedef NTSTATUS (*PVFS_NT_CREATE)(
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

typedef NTSTATUS (*PVFS_NT_TRANSACT_CREATE)(
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

typedef NTSTATUS (*PVFS_CREATE_TEMPORARY)(
                    HANDLE hTreeObject,
                    LPWSTR pszDirectoryName,
                    LPWSTR * ppszFileName,
                    USHORT * usFid
                    );

typedef NTSTATUS (*PVFS_READ_FILE)(
                    HANDLE hTreeObject,
                    USHORT usFid,
                    ULONG ulOffset,
                    UCHAR  *pBuffer,
                    USHORT MaxCount
                    );

typedef NTSTATUS (*PVFS_WRITE_FILE)(
                    HANDLE hTreeObject,
                    USHORT WriteMode,
                    UCHAR *pBuffer,
                    USHORT usByteCount,
                    USHORT *pusBytesWritten
                    );

typedef NTSTATUS (*PVFS_LOCK_FILE)(
                    HANDLE hTreeObject,
                    USHORT usFid,
                    UCHAR LockType,
                    ULONG TimeOut,
                    USHORT NumberofUnlocks,
                    USHORT NumberOfLocks,
                    LOCKING_ANDX_RANGE Unlocks[],
                    LOCKING_ANDX_RANGE Locks[]
                    );

typedef NTSTATUS (*PVFS_SEEK_FILE)(
                    HANDLE hTreeObject,
                    USHORT usFid,
                    USHORT Mode,
                    ULONG Offset,
                    ULONG * plReturnedOffset
                    );

typedef NTSTATUS (*PVFS_FLUSH_FILE)(
                    HANDLE hTreeObject
                    );

typedef NTSTATUS (*PVFS_CLOSE_FILE)(
                    HANDLE hTreeObject
                    );

typedef NTSTATUS (*PVFS_CLOSE_FILE_AND_DISCONNECT)(
                    HANDLE hTreeObject,
                    USHORT usFid
                    );

typedef NTSTATUS (*PVFS_DELETE_FILE)(
                    HANDLE hTreeObject,
                    USHORT usSearchAttributes,
                    LPWSTR pszFileName
                    );

typedef NTSTATUS (*PVFS_RENAME_FILE)(
                    HANDLE hTreeObject,
                    USHORT usSearchAttributes,
                    LPWSTR pszOldFileName,
                    LPWSTR pszNewFileName
                    );

typedef NTSTATUS (*PVFS_COPY_FILE)(
                    HANDLE hTreeObject,
                    HANDLE hTreeObject2,
                    USHORT OpenFunction,
                    USHORT Flags,
                    UCHAR SourceFileFormat,
                    LPWSTR SourceFileName,
                    UCHAR TargetFileFormat,
                    LPWSTR TargetFileName
                    );

typedef NTSTATUS (*PVFS_TRANS2_QUERY_PATH_INFORMATION)(
                    HANDLE hTreeObject,
                    USHORT InformationLevel,
                    ULONG Reserved,
                    LPWSTR FileName
                    );

typedef NTSTATUS (*PVFS_TRANS2_QUERY_FILE_INFORMATION)(
                    HANDLE hTreeObject,
                    USHORT InformationLevel,
                    ULONG Reserved,
                    LPWSTR FileName
                    );

typedef NTSTATUS (*PVFS_TRANS2_SET_PATH_INFORMATION)(
                    HANDLE hTreeObject,
                    USHORT InformationLevel,
                    ULONG Reserved,
                    LPWSTR FileName
                    );

typedef NTSTATUS (*PVFS_TRANS2_CREATE_DIRECTORY)(
                    HANDLE hTreeObject
                    );

typedef NTSTATUS (*PVFS_TRANS2_DELETE_DIRECTORY)(
                    HANDLE hTreeObject,
                    LPWSTR DirectoryName[]
                    );

typedef NTSTATUS (*PVFS_TRANS2_CHECK_DIRECTORY)(
                    HANDLE hTreeObject,
                    LPWSTR DirectoryName[]
                    );

typedef NTSTATUS (*PVFS_TRANS2_FIND_FIRST2)(
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

typedef NTSTATUS (*PVFS_TRANS2_FIND_NEXT2)(
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

typedef NTSTATUS (*PVFS_NT_TRANSACT_NOTIFY_CHANGE)(
                    HANDLE hTreeObject,
                    USHORT Fid,
                    BOOLEAN WatchTree,
                    UCHAR Reserved
                    );

typedef NTSTATUS (*PVFS_TRANS2_GET_DFS_REFERRAL)(
                    HANDLE hTreeObject
                    );

typedef struct _NTVFS_DRIVER
{
    PVFS_LW_CREATEFILE             pfnVFSLWCreateFile;

    PVFS_LW_READFILE               pfnVFSLWReadFile;

    PVFS_LW_WRITEFILE              pfnVFSLWWriteFile;

    PVFS_LW_GETSESSIONKEY          pfnVFSLWGetSessionKey;

    PVFS_LW_CLOSEFILE              pfnVFSLWCloseFile;

	PVFS_TREE_CONNECT              pfnVFSTreeConnect;

	PVFS_NT_CREATE                 pfnVFSNTCreate;

	PVFS_NT_TRANSACT_CREATE        pfnVFSNTTransactCreate;

	PVFS_CREATE_TEMPORARY          pfnVFSCreateTemporary;

	PVFS_READ_FILE                 pfnVFSReadFile;

	PVFS_WRITE_FILE                pfnVFSWriteFile;

	PVFS_LOCK_FILE                 pfnVFSLockFile;

	PVFS_SEEK_FILE                 pfnVFSSeekFile;

	PVFS_FLUSH_FILE                pfnVFSFlushFile;

	PVFS_CLOSE_FILE                pfnVFSCloseFile;

	PVFS_CLOSE_FILE_AND_DISCONNECT pfnVFSCloseFileAndDisconnect;

	PVFS_DELETE_FILE               pfnVFSDeleteFile;

	PVFS_RENAME_FILE               pfnVFSRenameFile;

	PVFS_COPY_FILE                 pfnVFSCopyFile;

	PVFS_TRANS2_QUERY_FILE_INFORMATION pfnVFSTrans2QueryFileInformation;

	PVFS_TRANS2_SET_PATH_INFORMATION   pfnVFSTrans2SetPathInformation;

	PVFS_TRANS2_QUERY_PATH_INFORMATION pfnVFSTrans2QueryPathInformation;

	PVFS_TRANS2_CREATE_DIRECTORY   pfnVFSTrans2CreateDirectory;

	PVFS_TRANS2_DELETE_DIRECTORY   pfnVFSDeleteDirectory;

	PVFS_TRANS2_CHECK_DIRECTORY    pfnVFSCheckDirectory;

	PVFS_TRANS2_FIND_FIRST2        pfnVFSTrans2FindFirst2;

	PVFS_TRANS2_FIND_NEXT2         pfnVFSTrans2FindNext2;

	PVFS_NT_TRANSACT_NOTIFY_CHANGE pfnVFSNTTransactNotifyChange;

	PVFS_TRANS2_GET_DFS_REFERRAL   pfnVFSTrans2GetDfsReferral;

} NTVFS_DRIVER, *PNTVFS_DRIVER;

#define IOMGR_SYMBOL_NAME_INITIALIZE_PROVIDER "VFSInitializeProvider"

typedef DWORD (*PFNVFSINITIALIZEPROVIDER)(
                    PCSTR pszConfigFilePath,
                    PSTR* ppszProviderName,
                    PNTVFS_DRIVER* ppFnTable
                    );

#define IOMGR_SYMBOL_NAME_SHUTDOWN_PROVIDER "VFSShutdownProvider"

typedef DWORD (*PFNVFSSHUTDOWNPROVIDER)(
                    PSTR pszProviderName,
                    PNTVFS_DRIVER pFnTable
                    );

#endif

