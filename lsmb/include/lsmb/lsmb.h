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
 *        Likewise SMB Subsystem (LSMB)
 *
 *        Public API

 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 */
#ifndef __LSMB_H__
#define __LSMB_H__

#ifndef SMB_CLIENT_API
#define SMB_CLIENT_API
#endif

#ifndef SMB_SERVER_API
#define SMB_SERVER_API
#endif

#ifndef SMB_API
#define SMB_API
#endif

#ifndef WCHAR16_T_DEFINED
#define WCHAR16_T_DEFINED 1

typedef uint16_t wchar16_t;

#endif

#ifndef VOID_DEFINED
#define VOID_DEFINED 1

typedef void            VOID, *PVOID;

#endif

#ifndef PCVOID_DEFINED
#define PCVOID_DEFINED 1

typedef const void      *PCVOID;

#endif

#ifndef BOOLEAN_DEFINED
#define BOOLEAN_DEFINED 1

typedef uint8_t             BOOLEAN, *PBOOLEAN;

#endif

#ifndef BOOL_DEFINED
#define BOOL_DEFINED 1

typedef uint8_t             BOOL, *PBOOL;

#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef PSTR_DEFINED
#define PSTR_DEFINED 1

typedef char *          PSTR;

#endif

#ifndef LPSTR_DEFINED
#define LPSTR_DEFINED 1

typedef char *          LPSTR;

#endif

#ifndef PCSTR_DEFINED
#define PCSTR_DEFINED 1

typedef const char *    PCSTR;

#endif

#ifndef LPCSTR_DEFINED
#define LPCSTR_DEFINED 1

typedef const char *    LPCSTR;

#endif


#ifndef PWSTR_DEFINED
#define PWSTR_DEFINED 1

typedef wchar16_t *          PWSTR;

#endif

#ifndef LPWSTR_DEFINED
#define LPWSTR_DEFINED 1

typedef wchar16_t *          LPWSTR;

#endif

#ifndef PCWSTR_DEFINED
#define PCWSTR_DEFINED 1

typedef const wchar16_t *    PCWSTR;

#endif

#ifndef LPCWSTR_DEFINED
#define LPCWSTR_DEFINED 1

typedef const wchar16_t *    LPCWSTR;

#endif

#ifndef BYTE_DEFINED
#define BYTE_DEFINED

typedef uint8_t BYTE, *PBYTE;

#endif

#ifndef UCHAR_DEFINED
#define UCHAR_DEFINED 1

typedef uint8_t UCHAR, *PUCHAR;

#endif

#ifndef ULONG_DEFINED
#define ULONG_DEFINED 1

typedef uint32_t ULONG, *PULONG;

#endif

#ifndef USHORT_DEFINED
#define USHORT_DEFINED 1

typedef uint16_t USHORT, *PUSHORT;

#endif

#ifndef HANDLE_DEFINED
#define HANDLE_DEFINED 1

typedef void *HANDLE, **PHANDLE;
#define INVALID_HANDLE_VALUE (NULL)

#endif

#ifndef CHAR_DEFINED
#define CHAR_DEFINED 1

typedef char            CHAR, *PCHAR;

#endif

#ifndef INT_DEFINED
#define INT_DEFINED 1

typedef int             INT, *PINT;

#endif

#ifndef LONG_DEFINED
#define LONG_DEFINED 1

typedef int32_t        LONG, *PLONG;

#endif

#ifndef DWORD_DEFINED
#define DWORD_DEFINED 1

typedef uint32_t        DWORD, *PDWORD;

#endif

#ifndef WORD_DEFINED
#define WORD_DEFINED 1

typedef uint16_t WORD, *PWORD;

#endif

#ifndef TCHAR_DEFINED
#define TCHAR_DEFINED 1

#if defined(UNICODE)

#define TCHAR wchar16_t

#else

#define TCHAR CHAR

#endif /* defined(UNICODE) */

#endif /* TCHAR_DEFINED */

#ifndef LPTSTR_DEFINED
#define LPTSTR_DEFINED 1

#if defined(UNICODE)

#define LPTSTR LPWSTR

#else

#define LPTSTR LPSTR

#endif /* defined(UNICODE) */

#endif /* LPTSTR_DEFINED */

#ifndef LPCTSTR_DEFINED
#define LPCTSTR_DEFINED 1

#if defined(UNICODE)

#define LPCTSTR LPCWSTR

#else

#define LPCTSTR LPCSTR

#endif /* defined(UNICODE) */

#endif /* LPCTSTR_DEFINED */


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

SMB_API
DWORD
SMBInitialize(
    VOID
    );

SMB_API
DWORD
SMBOpenServer(
    PHANDLE phConnection
    );

SMB_API
DWORD
SMBRefreshConfiguration(
    HANDLE hConnection
    );

SMB_API
DWORD
SMBBuildLogInfo(
    SMBLogLevel    maxAllowedLogLevel,
    SMBLogTarget   logTarget,
    PCSTR          pszPath,
    PSMB_LOG_INFO* ppLogInfo
    );

SMB_API
DWORD
SMBSetLogLevel(
    HANDLE      hSMBConnection,
    SMBLogLevel logLevel
    );

SMB_API
DWORD
SMBGetLogInfo(
    HANDLE         hSMBConnection,
    PSMB_LOG_INFO* ppLogInfo
    );

SMB_API
DWORD
SMBSetLogInfo(
    HANDLE        hSMBConnection,
    PSMB_LOG_INFO pLogInfo
    );

SMB_API
VOID
SMBFreeLogInfo(
    PSMB_LOG_INFO pLogInfo
    );

SMB_API
DWORD
SMBCloseServer(
    HANDLE hConnection
    );

SMB_API
DWORD
SMBShutdown(
    VOID
    );

SMB_CLIENT_API
DWORD
SMBCallNamedPipeA(
    HANDLE  hConnection,
    HANDLE  hAccessToken,
    LPCSTR  pszNamedPipeName,
    PVOID   pInBuffer,
    DWORD   dwInBufferSize,
    PVOID   pOutBuffer,
    DWORD   dwOutBufferSize,
    PDWORD  pdwBytesRead,
    DWORD   dwTimeout
    );

SMB_CLIENT_API
DWORD
SMBCallNamedPipeW(
    HANDLE  hConnection,
    HANDLE  hAccessToken,
    LPCWSTR pwszNamedPipeName,
    PVOID     pInBuffer,
    DWORD     dwInBufferSize,
    PVOID     pOutBuffer,
    DWORD     dwOutBufferSize,
    PDWORD    pdwBytesRead,
    DWORD     dwTimeout
    );

SMB_SERVER_API
DWORD
SMBCreateNamedPipeA(
    HANDLE    hConnection,
    HANDLE    hAccessToken,
    LPCSTR    pwszName,
    DWORD     dwOpenMode,
    DWORD     dwPipeMode,
    DWORD     dwMaxInstances,
    DWORD     dwOutBufferSize,
    DWORD     dwInBufferSize,
    DWORD     dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHANDLE   phNamedPipe
    );

SMB_SERVER_API
DWORD
SMBCreateNamedPipeW(
    HANDLE    hConnection,
    HANDLE    hAccessToken,
    LPCWSTR pwszName,
    DWORD     dwOpenMode,
    DWORD     dwPipeMode,
    DWORD     dwMaxInstances,
    DWORD     dwOutBufferSize,
    DWORD     dwInBufferSize,
    DWORD     dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHANDLE   phNamedPipe
    );

SMB_API
DWORD
SMBGetNamedPipeInfo(
    HANDLE hConnection,
    HANDLE hNamedPipe,
    PDWORD pdwFlags,
    PDWORD pdwOutBufferSize,
    PDWORD pdwInBufferSize,
    PDWORD pdwMaxInstances
    );

SMB_SERVER_API
DWORD
SMBConnectNamedPipe(
    HANDLE      hConnection,
    HANDLE      hNamedPipe,
    POVERLAPPED pOverlapped
    );

SMB_CLIENT_API
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

SMB_CLIENT_API
DWORD
SMBWaitNamedPipeA(
    HANDLE hConnection,
    HANDLE hAccessToken,
    LPCSTR pszNamedPipeName,
    DWORD  dwTimeOut
    );

SMB_CLIENT_API
DWORD
SMBWaitNamedPipeW(
    HANDLE hConnection,
    HANDLE hAccessToken,
    LPCWSTR pwszNamedPipeName,
    DWORD     dwTimeOut
    );

SMB_API
DWORD
SMBGetNamedPipeClientComputerNameA(
    HANDLE hConnection,
    HANDLE hNamedPipe,
    PSTR   pszClientComputerName,
    DWORD  dwClientComputerNameLength
    );

SMB_API
DWORD
SMBGetNamedPipeClientComputerNameW(
    HANDLE   hConnection,
    HANDLE   hNamedPipe,
    LPWSTR pszClientComputerName,
    DWORD    dwClientComputerNameLength
    );

SMB_API
DWORD
SMBGetNamedPipeClientProcessId(
    HANDLE hConnection,
    HANDLE hNamedPipe,
    PDWORD pdwClientProcessId
    );

SMB_SERVER_API
DWORD
SMBGetNamedPipeServerProcessId(
    HANDLE hConnection,
    HANDLE hNamedPipe,
    PDWORD pdwServerProcessId
    );

SMB_API
DWORD
SMBGetNamedPipeClientSessionId(
    HANDLE hConnection,
    HANDLE hNamedPipe,
    PDWORD pdwClientSessionId
    );

SMB_API
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

SMB_API
DWORD
SMBDisconnectNamedPipe(
    HANDLE hConnection,
    HANDLE hNamedPipe
    );

SMB_CLIENT_API
DWORD
SMBCreateFileA(
    HANDLE               hConnection,
    HANDLE               hAccessToken,
    LPCSTR               pszFileName,
    DWORD                dwDesiredAccess,
    DWORD                dwSharedMode,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD                dwCreationDisposition,
    DWORD                dwFlagsAndAttributes,
    HANDLE               hTemplateFile,
    PHANDLE              phFile
    );

SMB_CLIENT_API
DWORD
SMBCreateFileW(
    HANDLE               hConnection,
    HANDLE               hAccessToken,
    LPCWSTR            pwszFileName,
    DWORD                dwDesiredAccess,
    DWORD                dwSharedMode,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD                dwCreationDisposition,
    DWORD                dwFlagsAndAttributes,
    HANDLE               hTemplateFile,
    PHANDLE              phFile
    );

SMB_CLIENT_API
DWORD
SMBSetNamedPipeHandleState(
    HANDLE      hConnection,
    HANDLE      hPipe,
    PDWORD      pdwMode,
    PDWORD      pdwMaxCollectionCount,
    PDWORD      pdwMaxTimeout
    );

SMB_CLIENT_API
DWORD
SMBReadFile(
    HANDLE      hConnection,
    HANDLE      hFile,
    PVOID       pBuffer,
    DWORD       dwNumberOfBytesToRead,
    PDWORD      pdwBytesRead,
    POVERLAPPED pOverlapped
    );

SMB_CLIENT_API
DWORD
SMBWriteFile(
    HANDLE      hConnection,
    HANDLE      hFile,
    PVOID       pBuffer,
    DWORD       dwNumBytesToWrite,
    PDWORD      pdwNumBytesWritten,
    POVERLAPPED pOverlapped
    );

SMB_CLIENT_API
DWORD
SMBCloseHandle(
    HANDLE hConnection,
    HANDLE hFile
    );

SMB_API
DWORD
SMBGetSessionKey(
    HANDLE hConnection,
    HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    );

SMB_API
VOID
SMBFreeSessionKey(
    PBYTE pSessionKey
    );

DWORD
SMBCreatePlainAccessTokenW(
    LPCWSTR pwszUsername,
    LPCWSTR pwszPassword,
    PHANDLE phAccessToken
    );

DWORD
SMBCreatePlainAccessTokenA(
    LPCSTR pszUsername,
    LPCSTR pszPassword,
    PHANDLE phAccessToken
    );

DWORD
SMBCreateKrb5AccessTokenW(
    LPCWSTR pwszPrincipal,
    LPCWSTR pwszCachePath,
    PHANDLE phAccessToken
    );

DWORD
SMBCreateKrb5AccessTokenA(
    LPCSTR pszPrincipal,
    LPCSTR pszCachePath,
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

SMB_CLIENT_API
BOOL
CallNamedPipeA(
    LPCSTR  pszNamedPipeName,
    PVOID   pInBuffer,
    DWORD   dwInBufferSize,
    PVOID   pOutBuffer,
    DWORD   dwOutBufferSize,
    PDWORD  pdwBytesRead,
    DWORD   dwTimeout
    );

SMB_CLIENT_API
BOOL
CallNamedPipeW(
    LPCWSTR pwszNamedPipeName,
    PVOID     pInBuffer,
    DWORD     dwInBufferSize,
    PVOID     pOutBuffer,
    DWORD     dwOutBufferSize,
    PDWORD    pdwBytesRead,
    DWORD     dwTimeout
    );

SMB_SERVER_API
HANDLE
CreateNamedPipeA(
    LPCSTR    pwszName,
    DWORD     dwOpenMode,
    DWORD     dwPipeMode,
    DWORD     dwMaxInstances,
    DWORD     dwOutBufferSize,
    DWORD     dwInBufferSize,
    DWORD     dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes
    );

SMB_SERVER_API
HANDLE
CreateNamedPipeW(
    LPCWSTR pwszName,
    DWORD     dwOpenMode,
    DWORD     dwPipeMode,
    DWORD     dwMaxInstances,
    DWORD     dwOutBufferSize,
    DWORD     dwInBufferSize,
    DWORD     dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes
    );

SMB_API
BOOL
GetNamedPipeInfo(
    HANDLE hNamedPipe,
    PDWORD pdwFlags,
    PDWORD pdwOutBufferSize,
    PDWORD pdwInBufferSize,
    PDWORD pdwMaxInstances
    );

SMB_SERVER_API
BOOL
ConnectNamedPipe(
    HANDLE      hNamedPipe,
    POVERLAPPED pOverlapped
    );

SMB_CLIENT_API
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

SMB_CLIENT_API
BOOL
WaitNamedPipeA(
    LPCSTR pszNamedPipeName,
    DWORD  dwTimeOut
    );

SMB_CLIENT_API
BOOL
WaitNamedPipeW(
    LPCWSTR pwszNamedPipeName,
    DWORD     dwTimeOut
    );

SMB_API
BOOL
GetNamedPipeClientComputerNameA(
    HANDLE hNamedPipe,
    PSTR   pszClientComputerName,
    DWORD  dwClientComputerNameLength
    );

SMB_API
BOOL
GetNamedPipeClientComputerNameW(
    HANDLE   hNamedPipe,
    LPWSTR   pszClientComputerName,
    DWORD    dwClientComputerNameLength
    );

SMB_API
BOOL
GetNamedPipeClientProcessId(
    HANDLE hNamedPipe,
    PDWORD pdwClientProcessId
    );

SMB_SERVER_API
BOOL
GetNamedPipeServerProcessId(
    HANDLE hNamedPipe,
    PDWORD pdwServerProcessId
    );

SMB_API
BOOL
GetNamedPipeClientSessionId(
    HANDLE hNamedPipe,
    PDWORD pdwClientSessionId
    );

SMB_API
BOOL
PeekNamedPipe(
    HANDLE hNamedPipe,
    PVOID  pInBuffer,
    DWORD  dwInBufferSize,
    PDWORD pdwBytesRead,
    PDWORD pdwTotalBytesAvail,
    PDWORD pdwBytesLeftThisMessage
    );

SMB_API
BOOL
DisconnectNamedPipe(
    HANDLE hNamedPipe
    );

SMB_CLIENT_API
HANDLE
CreateFileA(
    LPCSTR               pszFileName,
    DWORD                dwDesiredAccess,
    DWORD                dwSharedMode,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD                dwCreationDisposition,
    DWORD                dwFlagsAndAttributes,
    HANDLE               hTemplateFile
    );

SMB_CLIENT_API
HANDLE
CreateFileW(
    LPCWSTR              pwszFileName,
    DWORD                dwDesiredAccess,
    DWORD                dwSharedMode,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD                dwCreationDisposition,
    DWORD                dwFlagsAndAttributes,
    HANDLE               hTemplateFile
    );

SMB_CLIENT_API
BOOL
SetNamedPipeHandleState(
    HANDLE      hPipe,
    PDWORD      pdwMode,
    PDWORD      pdwMaxCollectionCount,
    PDWORD      pdwMaxTimeout
    );

SMB_CLIENT_API
BOOL
ReadFile(
    HANDLE      hFile,
    PVOID       pBuffer,
    DWORD       dwNumberOfBytesToRead,
    PDWORD      pdwBytesRead,
    POVERLAPPED pOverlapped
    );

SMB_CLIENT_API
BOOL
WriteFile(
    HANDLE      hFile,
    PVOID       pBuffer,
    DWORD       dwNumBytesToWrite,
    PDWORD      pdwNumBytesWritten,
    POVERLAPPED pOverlapped
    );

SMB_CLIENT_API
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

#endif /* __LSMB_H__ */

