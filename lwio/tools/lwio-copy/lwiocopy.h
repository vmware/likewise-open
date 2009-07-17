
#ifndef __LWIOCOPY_H__
#define __LWIOCOPY_H__

NTSTATUS
CopyFile(
    IN PCSTR pSrc,
    IN PCSTR pDest,
    BOOLEAN  bCopyRecursive
    );

NTSTATUS
LwioLocalOpenFile(
    IN PCSTR pszFileName,
    IN INT dwMode,
    IN INT dwPerms,
    OUT INT *dwHandle
    );


NTSTATUS
LwioLocalCreateDir(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    );


NTSTATUS
LwioLocalCreateDirInternal(
    IN PSTR pszPath,
    IN PSTR pszLastSlash,
    IN mode_t dwFileMode
    );


NTSTATUS
LwioLocalChangePermissions(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    );


NTSTATUS
LwioLocalCheckDirExists(
    IN PCSTR pszPath,
    IN PBOOLEAN pbDirExists
    );


NTSTATUS
LwioLocalRemoveDir(
    IN PCSTR pszPath
    );


NTSTATUS
LwioLocalRemoveFile(
    IN PCSTR pszPath
    );

NTSTATUS
LwioRemoteOpenFile(
    IN  PCSTR           pszFileName,
    IN  ULONG           ulDesiredAccess,
    IN  ULONG           ulShareAccess,
    IN  ULONG           ulCreateDisposition,
    IN  ULONG           ulCreateOptions,
    OUT PIO_FILE_HANDLE phFile
    );

NTSTATUS
LwioRemoteWriteFile(
    IN HANDLE hFile,
    IN PVOID pBuffer,
    IN DWORD dwNumBytesToWrite,
    OUT PDWORD pdwNumBytesWritten
    );


NTSTATUS
LwioRemoteReadFile(
    IN HANDLE hFile,
    OUT PVOID pBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT PDWORD pdwBytesRead
    );

NTSTATUS
LwioCheckRemotePathIsDirectory(
    IN     PCSTR    pszPath,
    IN OUT PBOOLEAN pbIsDirectory
    );

NTSTATUS
LwioCopyFileFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

NTSTATUS
LwioCopyDirFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

NTSTATUS
LwioCopyFileToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

NTSTATUS
LwioCopyDirToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

NTSTATUS
LwioCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

NTSTATUS
LwioCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    );

#endif
