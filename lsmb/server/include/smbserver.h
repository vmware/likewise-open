#ifndef __SMBSERVER_H__
#define __SMBSERVER_H__

DWORD
ServerCreateNamedPipe(
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

DWORD
ServerGetNamedPipeInfo(
    HANDLE hNamedPipe,
    PDWORD pdwFlags,
    PDWORD pdwInBufferSize,
    PDWORD pdwOutBufferSize,
    PDWORD pdwMaxInstances
    );

DWORD
ServerConnectNamedPipe(
    HANDLE hNamedPipe
    );

DWORD
ServerTransactNamedPipe(
    HANDLE hNamedPipe,
    PVOID  pInBuffer,
    DWORD  dwInBufferSize,
    DWORD  dwOutBufferSize,
    PVOID* ppOutBuffer,
    PDWORD pdwOutBufferSize
    );

DWORD
ServerGetClientComputerName(
    HANDLE  hNamedPipe,
    DWORD   dwComputerNameMaxSize,
    LPWSTR* ppwszName,
    PDWORD  pdwLength
    );

DWORD
ServerGetClientProcessId(
    HANDLE hNamedPipe,
    PDWORD pdwId
    );

DWORD
ServerGetClientSessionId(
    HANDLE hNamedPipe,
    PDWORD pdwId
    );

DWORD
ServerDisconnectNamedPipe(
    HANDLE hNamedPipe
    );

DWORD
ServerReadFile(
    HANDLE hFile,
    DWORD  dwBytesToRead,
    PVOID* ppOutBuffer,
    PDWORD pdwBytesRead
    );

DWORD
ServerWriteFile(
    HANDLE hFile,
    DWORD  dwNumBytesToWrite,
    PVOID  pBuffer,
    PDWORD pdwNumBytesWritten
    );

DWORD
ServerCloseFile(
    HANDLE hFile
    );

#endif /* __SMBSERVER_H__ */
