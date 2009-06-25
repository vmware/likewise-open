
#ifndef __LWIOCOPY_H__
#define __LWIOCOPY_H__

#include "config.h"
#include "lwiosys.h"
#include "lwio/lwio.h"
#include "lwiodef.h"
#include "lwioutils.h"
#include "lwio/ntfileapi.h"
#include <lwio/win32fileapi.h>
#include <krb5/krb5.h>
#include "lwnet.h"

#define BUFF_SIZE 1024
#define MAX_BUFFER 4096

#define BAIL_ON_NULL_POINTER(p)                    \
        if (NULL == p) {                          \
           status = LWIO_ERROR_INVALID_PARAMETER; \
           goto error;                            \
        }

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


HANDLE
LwioRemoteCreateFile(
    IN PCSTR pszFileName
    );


HANDLE
LwioRemoteOpenFile(
    IN PCSTR pszFileName
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
