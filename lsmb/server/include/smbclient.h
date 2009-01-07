#ifndef __SMBCLIENT_H__
#define __SMBCLIENT_H__

DWORD
SMBSrvClientInit(
    PCSTR pszConfigFilePath
    );

DWORD
ClientCallNamedPipe(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR   pwszNamedPipeName,
    PVOID     pInBuffer,
    DWORD     dwInBufferSize,
    DWORD     dwOutBufferSize,
    DWORD     dwTimeout,
    PVOID*    ppOutBuffer,
    PDWORD    pdwOutBufferSize
    );

DWORD
ClientGetNamedPipeInfo(
    HANDLE hNamedPipe,
    PDWORD pdwFlags,
    PDWORD pdwInBufferSize,
    PDWORD pdwOutBufferSize,
    PDWORD pdwMaxInstances
    );

DWORD
ClientTransactNamedPipe(
    HANDLE hNamedPipe,
    PVOID  pInBuffer,
    DWORD  dwInBufferSize,
    DWORD  dwOutBufferSize,
    PVOID* ppOutBuffer,
    PDWORD pdwOutBufferSize
    );

DWORD
ClientWaitNamedPipe(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszName,
    DWORD dwTimeout
    );

DWORD
ClientGetServerProcessId(
    HANDLE hNamedPipe,
    PDWORD    pdwId
    );

DWORD
ClientPeekNamedPipe(
    HANDLE hNamedPipe,
    PVOID pInBuffer,
    DWORD dwInBufferSize,
    PDWORD pdwBytesRead,
    PDWORD pdwTotalBytesAvail,
    PDWORD pdwBytesLeftThisMessage
    );

DWORD
ClientCreateFile(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszFileName,
    DWORD dwDesiredAccess,
    DWORD dwSharedMode,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    HANDLE* phFile
    );

DWORD
ClientSetNamedPipeHandleState(
    HANDLE hPipe,
    PDWORD pdwMode,
    PDWORD pdwCollectionCount,
    PDWORD pdwTimeout
    );

DWORD
ClientReadFile(
    HANDLE hFile,
    DWORD  dwBytesToRead,
    PVOID* ppOutBuffer,
    PDWORD pdwBytesRead
    );

DWORD
ClientWriteFile(
    HANDLE hFile,
    DWORD  dwNumBytesToWrite,
    PVOID  pBuffer,
    PDWORD pdwNumBytesWritten
    );

DWORD
ClientCloseFile(
    HANDLE hFile
    );

DWORD
ClientGetSessionKey(
    HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    );

DWORD
SMBSrvClientShutdown(
    VOID
    );

#endif /* __SMBCLIENT_H__ */
