#ifndef __IOMGR_H__
#define __IOMGR_H__

DWORD
IOMgrInitialize(
    PCSTR pszConfigFilePath
    );

DWORD
IOMgrCallNamedPipe(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszNamedPipeName,
    PVOID     pInBuffer,
    DWORD     dwInBufferSize,
    DWORD     dwOutBufferSize,
    DWORD     dwTimeout,
    PVOID*    ppOutBuffer,
    PDWORD    pdwOutBufferSize
    );

DWORD
IOMgrCreateNamedPipe(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszName,
    DWORD     dwOpenMode,
    DWORD     dwPipeMode,
    DWORD     dwMaxInstances,
    DWORD     dwOutBufferSize,
    DWORD     dwInBufferSize,
    DWORD     dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PSMB_FILE_HANDLE* phNamedPipe
    );

DWORD
IOMgrGetNamedPipeInfo(
    PSMB_FILE_HANDLE hNamedPipe,
    PDWORD pdwFlags,
    PDWORD pdwInBufferSize,
    PDWORD pdwOutBufferSize,
    PDWORD pdwMaxInstances
    );

DWORD
IOMgrConnectNamedPipe(
    PSMB_FILE_HANDLE hNamedPipe
    );

DWORD
IOMgrTransactNamedPipe(
    PSMB_FILE_HANDLE hNamedPipe,
    PVOID     pInBuffer,
    DWORD     dwInBufferSize,
    DWORD     dwOutBufferSize,
    PVOID*    ppOutBuffer,
    PDWORD    pdwOutBufferSize
    );

DWORD
IOMgrWaitNamedPipe(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszName,
    DWORD dwTimeout
    );

DWORD
IOMgrGetClientComputerName(
    PSMB_FILE_HANDLE hNamedPipe,
    DWORD     dwComputerNameMaxSize,
    LPWSTR* ppwszName,
    PDWORD    pdwLength
    );

DWORD
IOMgrGetClientProcessId(
    PSMB_FILE_HANDLE hNamedPipe,
    PDWORD    pdwId
    );

DWORD
IOMgrGetServerProcessId(
    PSMB_FILE_HANDLE hNamedPipe,
    PDWORD    pdwId
    );

DWORD
IOMgrGetClientSessionId(
    PSMB_FILE_HANDLE hNamedPipe,
    PDWORD    pdwId
    );

DWORD
IOMgrPeekNamedPipe(
    PSMB_FILE_HANDLE hNamedPipe,
    PVOID pInBuffer,
    DWORD dwInBufferSize,
    PDWORD pdwBytesRead,
    PDWORD pdwTotalBytesAvail,
    PDWORD pdwBytesLeftThisMessage
    );

DWORD
IOMgrDisconnectNamedPipe(
    PSMB_FILE_HANDLE hNamedPipe
    );

DWORD
IOMgrCreateFile(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszFileName,
    DWORD dwDesiredAccess,
    DWORD dwSharedMode,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PSMB_FILE_HANDLE* phFile
    );

DWORD
IOMgrSetNamedPipeHandleState(
    PSMB_FILE_HANDLE hFile,
    PDWORD pdwMode,
    PDWORD pdwCollectionCount,
    PDWORD pdwTimeout
    );

DWORD
IOMgrReadFile(
    PSMB_FILE_HANDLE hFile,
    DWORD  dwBytesToRead,
    PVOID* ppOutBuffer,
    PDWORD pdwBytesRead
    );

DWORD
IOMgrWriteFile(
    PSMB_FILE_HANDLE hFile,
    DWORD  dwNumBytesToWrite,
    PVOID  pBuffer,
    PDWORD pdwNumBytesWritten
    );

DWORD
IOMgrCloseFile(
    PSMB_FILE_HANDLE hFile
    );

DWORD
IOMgrGetSessionKey(
    PSMB_FILE_HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    );

DWORD
IOMgrShutdown(
    VOID
    );

#endif /* __IOMGR_H__ */

