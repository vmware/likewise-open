/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsmb.h
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        Public API

 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 */
#ifndef __LWIO_H__
#define __LWIO_H__

#include <lw/base.h>

#ifndef OVERLAPPED_DEFINED
#define OVERLAPPED_DEFINED 1

typedef struct _OVERLAPPED {
    unsigned long Internal;
    unsigned long InternalHigh;
    union {
        struct {
            DWORD Offset;
            DWORD OffsetHigh;
        };
        PVOID Pointer;
    };
    HANDLE hEvent;
} OVERLAPPED,  *POVERLAPPED;

#endif /* OVERLAPPED */

#ifndef SECURITY_ATTRIBUTES_DEFINED
#define SECURITY_ATTRIBUTES_DEFINED 1

typedef struct _SECURITY_ATTRIBUTES
{
    DWORD dwUid;
    DWORD dwGid;
    DWORD mode;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES;

#endif /* LPSECURITY_ATTRIBUTES_DEFINED */

#ifndef ACCESS_FLAGS_DEFINED
#define ACCESS_FLAGS_DEFINED 1

/* Work around problem with /usr/include/arpa/nameser_compat.h */
#ifdef DELETE
# undef DELETE
#endif

#define DELETE                          0x010000
#define READ_CONTROL                    0x020000
#define WRITE_DAC                       0x040000
#define WRITE_OWNER                     0x080000
#define SYNCHRONIZE                     0x100000

#define STANDARD_RIGHTS_REQUIRED        0x0F0000

#define STANDARD_RIGHTS_READ            READ_CONTROL
#define STANDARD_RIGHTS_WRITE           READ_CONTROL
#define STANDARD_RIGHTS_EXECUTE         READ_CONTROL

#define STANDARD_RIGHTS_ALL             0x1F0000

#define SPECIFIC_RIGHTS_ALL             0x000FFFF
#define ACCESS_SYSTEM_SECURITY          0x1000000

#define MAXIMUM_ALLOWED                 0x02000000

#define GENERIC_READ                    0x80000000
#define GENERIC_WRITE                   0x40000000
#define GENERIC_EXECUTE                 0x20000000
#define GENERIC_ALL                     0x10000000

#define SHARE_READ                      0x00000001
#define SHARE_WRITE                     0x00000002

#define PIPE_READMODE_BYTE              0x00000000
#define PIPE_READMODE_MESSAGE           0x00000002
#define PIPE_WAIT                       0x00000000
#define PIPE_NOWAIT                     0x00000001

#define FILE_READ_DATA                  0x0001
#define FILE_LIST_DIRECTORY             0x0001

#define FILE_WRITE_DATA                 0x0002
#define FILE_ADD_FILE                   0x0002

#define FILE_APPEND_DATA                0x0004
#define FILE_ADD_SUBDIRECTORY           0x0004
#define FILE_CREATE_PIPE_INSTANCE       0x0004

#define FILE_READ_EA                    0x0008
#define FILE_WRITE_EA                   0x0010

#define FILE_EXECUTE                    0x0020
#define FILE_TRAVERSE                   0x0020

#define FILE_DELETE_CHILD               0x0040

#define FILE_READ_ATTRIBUTES            0x0080
#define FILE_WRITE_ATTRIBUTES           0x0100

#define FILE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1FF)

#define FILE_GENERIC_READ         (STANDARD_RIGHTS_READ     |\
                                   FILE_READ_DATA           |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_READ_EA             |\
                                   SYNCHRONIZE)

#define FILE_GENERIC_WRITE        (STANDARD_RIGHTS_WRITE    |\
                                   FILE_WRITE_DATA          |\
                                   FILE_WRITE_ATTRIBUTES    |\
                                   FILE_WRITE_EA            |\
                                   FILE_APPEND_DATA         |\
                                   SYNCHRONIZE)

#define FILE_GENERIC_EXECUTE      (STANDARD_RIGHTS_EXECUTE  |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_EXECUTE             |\
                                   SYNCHRONIZE)
#endif

#ifndef OPEN_FLAGS_DEFINED
#define OPEN_FLAGS_DEFINED 1

#define FILE_FLAG_WRITE_THROUGH         0x80000000
#define FILE_FLAG_OVERLAPPED            0x40000000
#define FILE_FLAG_NO_BUFFERING          0x20000000
#define FILE_FLAG_RANDOM_ACCESS         0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN       0x08000000
#define FILE_FLAG_DELETE_ON_CLOSE       0x04000000
#define FILE_FLAG_BACKUP_SEMANTICS      0x02000000
#define FILE_FLAG_POSIX_SEMANTICS       0x01000000
#define FILE_FLAG_OPEN_REPARSE_POINT    0x00200000
#define FILE_FLAG_OPEN_NO_RECALL        0x00100000
#define FILE_FLAG_FIRST_PIPE_INSTANCE   0x00080000

#define CREATE_NEW                      1
#define CREATE_ALWAYS                   2
#define OPEN_EXISTING                   3
#define OPEN_ALWAYS                     4
#define TRUNCATE_EXISTING               5

#endif

typedef unsigned char uchar8_t;

#define NUL  ((uchar8_t) 0)
#define WNUL ((wchar16_t) 0)

typedef uint32_t SMB_ERROR;

#define SMB_ERROR_SUCCESS                      0x0000
#define SMB_ERROR_INVALID_CACHE_PATH           0xF001 // 61441
#define SMB_ERROR_INVALID_CONFIG_PATH          0xF002 // 61442
#define SMB_ERROR_INVALID_PREFIX_PATH          0xF003 // 61443
#define SMB_ERROR_INSUFFICIENT_BUFFER          0xF004 // 61444
#define SMB_ERROR_OUT_OF_MEMORY                0xF005 // 61445
#define SMB_ERROR_NOT_IMPLEMENTED              0xF006 // 61446
#define SMB_ERROR_REGEX_COMPILE_FAILED         0xF007 // 61447
#define SMB_ERROR_INTERNAL                     0xF008 // 61448
#define SMB_ERROR_INVALID_PARAMETER            0xF009 // 61449
#define SMB_ERROR_INVALID_CONFIG               0xF00A // 61450
#define SMB_ERROR_UNEXPECTED_TOKEN             0xF00B // 61451
#define SMB_ERROR_NULL_BUFFER                  0xF00C // 61452
#define SMB_ERROR_INVALID_LOG_LEVEL            0xF00D // 61453
#define SMB_ERROR_LWMSG_ERROR                  0xF00E // 61454
#define SMB_ERROR_MALFORMED_REQUEST            0xF00F // 61455
#define SMB_ERROR_LWMSG_EOF                    0xF010 // 61456
#define SMB_ERROR_NO_SUCH_ITEM                 0xF011 // 61457
#define SMB_ERROR_OVERFLOW                     0xF012 // 61458
#define SMB_ERROR_UNDERFLOW                    0xF013 // 61459
#define SMB_ERROR_SYSTEM                       0xF014 // 61460
#define SMB_ERROR_SERVER_UNREACHABLE           0xF015 // 61461
#define SMB_ERROR_STRING_CONV_FAILED           0xF016 // 61462
#define SMB_ERROR_PASSWORD_EXPIRED             0xF017 // 61463
#define SMB_ERROR_PASSWORD_MISMATCH            0xF018 // 61464
#define SMB_ERROR_CLOCK_SKEW                   0xF019 // 61465
#define SMB_ERROR_KRB5_NO_KEYS_FOUND           0xF01A // 61466
#define SMB_ERROR_KRB5_CALL_FAILED             0xF01B // 61467
#define SMB_ERROR_NO_BIT_AVAILABLE             0xF01C // 61468
#define SMB_ERROR_INVALID_HANDLE               0xF01D // 61469
#define SMB_ERROR_OUT_OF_HANDLES               0xF01E // 61470
#define SMB_ERROR_GSS                          0xF01F // 61471
#define SMB_ERROR_HOST_NOT_FOUND               0xF020 // 61472
#define SMB_ERROR_INVALID_VFS_PROVIDER         0xF021 // 61473
#define SMB_ERROR_SENTINEL                     0xF022 // 61474

#define SMB_ERROR_MASK(_e_)                    (_e_ & 0xF000)

/*
 * Logging
 */
typedef enum
{
    SMB_LOG_LEVEL_ALWAYS = 0,
    SMB_LOG_LEVEL_ERROR,
    SMB_LOG_LEVEL_WARNING,
    SMB_LOG_LEVEL_INFO,
    SMB_LOG_LEVEL_VERBOSE,
    SMB_LOG_LEVEL_DEBUG
} SMBLogLevel;

typedef enum
{
    SMB_LOG_TARGET_DISABLED = 0,
    SMB_LOG_TARGET_CONSOLE,
    SMB_LOG_TARGET_FILE,
    SMB_LOG_TARGET_SYSLOG
} SMBLogTarget;

typedef VOID (*PFN_SMB_LOG_MESSAGE)(
                            HANDLE      hLog,
                            SMBLogLevel logLevel,
                            PCSTR       pszFormat,
                            va_list     msgList
                            );

typedef struct __SMB_LOG_INFO {
    SMBLogLevel  maxAllowedLogLevel;
    SMBLogTarget logTarget;
    PSTR         pszPath;
} SMB_LOG_INFO, *PSMB_LOG_INFO;


DWORD
SMBInitialize(
    VOID
    );


DWORD
SMBOpenServer(
    PHANDLE phConnection
    );


DWORD
SMBRefreshConfiguration(
    HANDLE hConnection
    );


DWORD
SMBBuildLogInfo(
    SMBLogLevel    maxAllowedLogLevel,
    SMBLogTarget   logTarget,
    PCSTR          pszPath,
    PSMB_LOG_INFO* ppLogInfo
    );


DWORD
SMBSetLogLevel(
    HANDLE      hSMBConnection,
    SMBLogLevel logLevel
    );


DWORD
SMBGetLogInfo(
    HANDLE         hSMBConnection,
    PSMB_LOG_INFO* ppLogInfo
    );


DWORD
SMBSetLogInfo(
    HANDLE        hSMBConnection,
    PSMB_LOG_INFO pLogInfo
    );


VOID
SMBFreeLogInfo(
    PSMB_LOG_INFO pLogInfo
    );


DWORD
SMBCloseServer(
    HANDLE hConnection
    );


DWORD
SMBShutdown(
    VOID
    );


DWORD
SMBCallNamedPipeA(
    HANDLE  hConnection,
    HANDLE  hAccessToken,
    PCSTR  pszNamedPipeName,
    PVOID   pInBuffer,
    DWORD   dwInBufferSize,
    PVOID   pOutBuffer,
    DWORD   dwOutBufferSize,
    PDWORD  pdwBytesRead,
    DWORD   dwTimeout
    );


DWORD
SMBCallNamedPipeW(
    HANDLE  hConnection,
    HANDLE  hAccessToken,
    PCWSTR pwszNamedPipeName,
    PVOID     pInBuffer,
    DWORD     dwInBufferSize,
    PVOID     pOutBuffer,
    DWORD     dwOutBufferSize,
    PDWORD    pdwBytesRead,
    DWORD     dwTimeout
    );


DWORD
SMBCreateNamedPipeA(
    HANDLE    hConnection,
    HANDLE    hAccessToken,
    PCSTR    pwszName,
    DWORD     dwOpenMode,
    DWORD     dwPipeMode,
    DWORD     dwMaxInstances,
    DWORD     dwOutBufferSize,
    DWORD     dwInBufferSize,
    DWORD     dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHANDLE   phNamedPipe
    );


DWORD
SMBCreateNamedPipeW(
    HANDLE    hConnection,
    HANDLE    hAccessToken,
    PCWSTR pwszName,
    DWORD     dwOpenMode,
    DWORD     dwPipeMode,
    DWORD     dwMaxInstances,
    DWORD     dwOutBufferSize,
    DWORD     dwInBufferSize,
    DWORD     dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHANDLE   phNamedPipe
    );


DWORD
SMBGetNamedPipeInfo(
    HANDLE hConnection,
    HANDLE hNamedPipe,
    PDWORD pdwFlags,
    PDWORD pdwOutBufferSize,
    PDWORD pdwInBufferSize,
    PDWORD pdwMaxInstances
    );


DWORD
SMBConnectNamedPipe(
    HANDLE      hConnection,
    HANDLE      hNamedPipe,
    POVERLAPPED pOverlapped
    );


DWORD
SMBTransactNamedPipe(
    HANDLE      hConnection,
    HANDLE      hNamedPipe,
    PVOID       pInBuffer,
    DWORD       dwInBufferSize,
    PVOID       pOutBuffer,
    DWORD       dwOutBufferSize,
    PDWORD      pdwBytesRead,
    POVERLAPPED pOverlapped
    );


DWORD
SMBWaitNamedPipeA(
    HANDLE hConnection,
    HANDLE hAccessToken,
    PCSTR pszNamedPipeName,
    DWORD  dwTimeOut
    );


DWORD
SMBWaitNamedPipeW(
    HANDLE hConnection,
    HANDLE hAccessToken,
    PCWSTR pwszNamedPipeName,
    DWORD     dwTimeOut
    );


DWORD
SMBGetNamedPipeClientComputerNameA(
    HANDLE hConnection,
    HANDLE hNamedPipe,
    PSTR   pszClientComputerName,
    DWORD  dwClientComputerNameLength
    );


DWORD
SMBGetNamedPipeClientComputerNameW(
    HANDLE   hConnection,
    HANDLE   hNamedPipe,
    PWSTR pszClientComputerName,
    DWORD    dwClientComputerNameLength
    );


DWORD
SMBGetNamedPipeClientProcessId(
    HANDLE hConnection,
    HANDLE hNamedPipe,
    PDWORD pdwClientProcessId
    );


DWORD
SMBGetNamedPipeServerProcessId(
    HANDLE hConnection,
    HANDLE hNamedPipe,
    PDWORD pdwServerProcessId
    );


DWORD
SMBGetNamedPipeClientSessionId(
    HANDLE hConnection,
    HANDLE hNamedPipe,
    PDWORD pdwClientSessionId
    );


DWORD
SMBPeekNamedPipe(
    HANDLE hConnection,
    HANDLE hNamedPipe,
    PVOID  pInBuffer,
    DWORD  dwInBufferSize,
    PDWORD pdwBytesRead,
    PDWORD pdwTotalBytesAvail,
    PDWORD pdwBytesLeftThisMessage
    );


DWORD
SMBDisconnectNamedPipe(
    HANDLE hConnection,
    HANDLE hNamedPipe
    );


DWORD
SMBCreateFileA(
    HANDLE               hConnection,
    HANDLE               hAccessToken,
    PCSTR               pszFileName,
    DWORD                dwDesiredAccess,
    DWORD                dwSharedMode,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD                dwCreationDisposition,
    DWORD                dwFlagsAndAttributes,
    HANDLE               hTemplateFile,
    PHANDLE              phFile
    );


DWORD
SMBCreateFileW(
    HANDLE               hConnection,
    HANDLE               hAccessToken,
    PCWSTR            pwszFileName,
    DWORD                dwDesiredAccess,
    DWORD                dwSharedMode,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD                dwCreationDisposition,
    DWORD                dwFlagsAndAttributes,
    HANDLE               hTemplateFile,
    PHANDLE              phFile
    );


DWORD
SMBSetNamedPipeHandleState(
    HANDLE      hConnection,
    HANDLE      hPipe,
    PDWORD      pdwMode,
    PDWORD      pdwMaxCollectionCount,
    PDWORD      pdwMaxTimeout
    );


DWORD
SMBReadFile(
    HANDLE      hConnection,
    HANDLE      hFile,
    PVOID       pBuffer,
    DWORD       dwNumberOfBytesToRead,
    PDWORD      pdwBytesRead,
    POVERLAPPED pOverlapped
    );


DWORD
SMBWriteFile(
    HANDLE      hConnection,
    HANDLE      hFile,
    PVOID       pBuffer,
    DWORD       dwNumBytesToWrite,
    PDWORD      pdwNumBytesWritten,
    POVERLAPPED pOverlapped
    );


DWORD
SMBCloseHandle(
    HANDLE hConnection,
    HANDLE hFile
    );


DWORD
SMBGetSessionKey(
    HANDLE hConnection,
    HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    );


VOID
SMBFreeSessionKey(
    PBYTE pSessionKey
    );

DWORD
SMBCreatePlainAccessTokenW(
    PCWSTR pwszUsername,
    PCWSTR pwszPassword,
    PHANDLE phAccessToken
    );

DWORD
SMBCreatePlainAccessTokenA(
    PCSTR pszUsername,
    PCSTR pszPassword,
    PHANDLE phAccessToken
    );

DWORD
SMBCreateKrb5AccessTokenW(
    PCWSTR pwszPrincipal,
    PCWSTR pwszCachePath,
    PHANDLE phAccessToken
    );

DWORD
SMBCreateKrb5AccessTokenA(
    PCSTR pszPrincipal,
    PCSTR pszCachePath,
    PHANDLE phAccessToken
    );

DWORD
SMBCompareHandles(
    HANDLE hHandleOne,
    HANDLE hHandleTwo,
    BOOL* pbEqual
    );

DWORD
SMBCopyHandle(
    HANDLE hHandle,
    PHANDLE phHandleCopy
    );

/* Defines for transitional functions */
#ifdef UNICODE
#define SMBCallNamedPipe                    SMBCallNamedPipeW
#define SMBCreateNamedPipe                  SMBCreateNamedPipeW
#define SMBWaitNamedPipe                    SMBWaitNamedPipeW
#define SMBGetNamedPipeClientComputerName   SMBGetNamedPipeClientComputerNameW
#define SMBCreateFile                       SMBCreateFileW
#define SMBCreatePlainAccessToken           SMBCreatePlainAccessTokenW
#define SMBCreateKrb5AccessToken            SMBCreateKrb5AccessTokenW
#else
#define SMBCallNamedPipe                    SMBCallNamedPipeA
#define SMBCreateNamedPipe                  SMBCreateNamedPipeA
#define SMBWaitNamedPipe                    SMBWaitNamedPipeA
#define SMBGetNamedPipeClientComputerName   SMBGetNamedPipeClientComputerNameA
#define SMBCreateFile                       SMBCreateFileA
#define SMBCreatePlainAccessToken           SMBCreatePlainAccessTokenA
#define SMBCreateKrb5AccessToken            SMBCreateKrb5AccessTokenA
#endif

#ifndef SMB_NO_THREADS

DWORD
SMBSetThreadToken(
    HANDLE hAccessToken
    );

DWORD
SMBGetThreadToken(
    PHANDLE phAccessToken
    );

BOOL
SetThreadToken(
    PHANDLE phThread,
    HANDLE hAccessToken
    );

BOOL
OpenThreadToken(
    HANDLE hThread,
    DWORD dwDesiredAccess,
    BOOL bOpenAsSelf,
    PHANDLE phAccessToken
    );

DWORD
GetLastError(
    void
    );

void
SetLastError(
    DWORD dwError
    );


BOOL
CallNamedPipeA(
    PCSTR  pszNamedPipeName,
    PVOID   pInBuffer,
    DWORD   dwInBufferSize,
    PVOID   pOutBuffer,
    DWORD   dwOutBufferSize,
    PDWORD  pdwBytesRead,
    DWORD   dwTimeout
    );


BOOL
CallNamedPipeW(
    PCWSTR pwszNamedPipeName,
    PVOID     pInBuffer,
    DWORD     dwInBufferSize,
    PVOID     pOutBuffer,
    DWORD     dwOutBufferSize,
    PDWORD    pdwBytesRead,
    DWORD     dwTimeout
    );


HANDLE
CreateNamedPipeA(
    PCSTR    pwszName,
    DWORD     dwOpenMode,
    DWORD     dwPipeMode,
    DWORD     dwMaxInstances,
    DWORD     dwOutBufferSize,
    DWORD     dwInBufferSize,
    DWORD     dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes
    );


HANDLE
CreateNamedPipeW(
    PCWSTR pwszName,
    DWORD     dwOpenMode,
    DWORD     dwPipeMode,
    DWORD     dwMaxInstances,
    DWORD     dwOutBufferSize,
    DWORD     dwInBufferSize,
    DWORD     dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes
    );


BOOL
GetNamedPipeInfo(
    HANDLE hNamedPipe,
    PDWORD pdwFlags,
    PDWORD pdwOutBufferSize,
    PDWORD pdwInBufferSize,
    PDWORD pdwMaxInstances
    );


BOOL
ConnectNamedPipe(
    HANDLE      hNamedPipe,
    POVERLAPPED pOverlapped
    );


BOOL
TransactNamedPipe(
    HANDLE      hNamedPipe,
    PVOID       pInBuffer,
    DWORD       dwInBufferSize,
    PVOID       pOutBuffer,
    DWORD       dwOutBufferSize,
    PDWORD      pdwBytesRead,
    POVERLAPPED pOverlapped
    );


BOOL
WaitNamedPipeA(
    PCSTR pszNamedPipeName,
    DWORD  dwTimeOut
    );


BOOL
WaitNamedPipeW(
    PCWSTR pwszNamedPipeName,
    DWORD     dwTimeOut
    );


BOOL
GetNamedPipeClientComputerNameA(
    HANDLE hNamedPipe,
    PSTR   pszClientComputerName,
    DWORD  dwClientComputerNameLength
    );


BOOL
GetNamedPipeClientComputerNameW(
    HANDLE   hNamedPipe,
    PWSTR   pszClientComputerName,
    DWORD    dwClientComputerNameLength
    );


BOOL
GetNamedPipeClientProcessId(
    HANDLE hNamedPipe,
    PDWORD pdwClientProcessId
    );


BOOL
GetNamedPipeServerProcessId(
    HANDLE hNamedPipe,
    PDWORD pdwServerProcessId
    );


BOOL
GetNamedPipeClientSessionId(
    HANDLE hNamedPipe,
    PDWORD pdwClientSessionId
    );


BOOL
PeekNamedPipe(
    HANDLE hNamedPipe,
    PVOID  pInBuffer,
    DWORD  dwInBufferSize,
    PDWORD pdwBytesRead,
    PDWORD pdwTotalBytesAvail,
    PDWORD pdwBytesLeftThisMessage
    );


BOOL
DisconnectNamedPipe(
    HANDLE hNamedPipe
    );


HANDLE
CreateFileA(
    PCSTR               pszFileName,
    DWORD                dwDesiredAccess,
    DWORD                dwSharedMode,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD                dwCreationDisposition,
    DWORD                dwFlagsAndAttributes,
    HANDLE               hTemplateFile
    );


HANDLE
CreateFileW(
    PCWSTR              pwszFileName,
    DWORD                dwDesiredAccess,
    DWORD                dwSharedMode,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD                dwCreationDisposition,
    DWORD                dwFlagsAndAttributes,
    HANDLE               hTemplateFile
    );


BOOL
SetNamedPipeHandleState(
    HANDLE      hPipe,
    PDWORD      pdwMode,
    PDWORD      pdwMaxCollectionCount,
    PDWORD      pdwMaxTimeout
    );


BOOL
ReadFile(
    HANDLE      hFile,
    PVOID       pBuffer,
    DWORD       dwNumberOfBytesToRead,
    PDWORD      pdwBytesRead,
    POVERLAPPED pOverlapped
    );


BOOL
WriteFile(
    HANDLE      hFile,
    PVOID       pBuffer,
    DWORD       dwNumBytesToWrite,
    PDWORD      pdwNumBytesWritten,
    POVERLAPPED pOverlapped
    );

BOOL
CloseHandle(
    HANDLE hFile
    );

/* Defines for transitional functions */
#ifdef UNICODE
#define CallNamedPipe                    CallNamedPipeW
#define CreateNamedPipe                  CreateNamedPipeW
#define WaitNamedPipe                    WaitNamedPipeW
#define GetNamedPipeClientComputerName   GetNamedPipeClientComputerNameW
#define CreateFile                       CreateFileW
#else
#define CallNamedPipe                    CallNamedPipeA
#define CreateNamedPipe                  CreateNamedPipeA
#define WaitNamedPipe                    WaitNamedPipeA
#define GetNamedPipeClientComputerName   GetNamedPipeClientComputerNameA
#define CreateFile                       CreateFileA
#endif

#endif /* ! SMB_NO_THREADS */

#endif /* __LWIO_H__ */

