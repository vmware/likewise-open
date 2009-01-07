/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        createnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CreateNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "npvfs.h"

DWORD
NPServerReadFile(
    PSMB_FILE_HANDLE pSMBFile,
    DWORD  dwBytesToRead,
    PVOID* ppOutBuffer,
    PDWORD pdwBytesRead
    );

DWORD
NPServerWriteFile(
    PSMB_FILE_HANDLE pSMBFile,
    DWORD  dwNumBytesToWrite,
    PVOID  pBuffer,
    PDWORD pdwNumBytesWritten
    );

DWORD
NPServerCloseFile(
    PSMB_FILE_HANDLE pSMBFile
    );

DWORD
NPServerConnectNamedPipe(
    HANDLE hNamedPipe
    );


DWORD
NPServerCreateNamedPipe(
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
NPServerDisconnectNamedPipe(
    HANDLE hNamedPipe
    );


DWORD
NPServerGetClientComputerName(
    HANDLE hNamedPipe,
    DWORD     dwComputerNameMaxSize,
    LPWSTR* ppwszName,
    PDWORD    pdwLength
    );


DWORD
NPServerGetClientProcessId(
    HANDLE hNamedPipe,
    PDWORD    pdwId
    );


DWORD
NPServerGetClientSessionId(
    HANDLE hNamedPipe,
    PDWORD    pdwId
    );


DWORD
NPServerGetNamedPipeInfo(
    HANDLE hNamedPipe,
    PDWORD pdwFlags,
    PDWORD pdwInBufferSize,
    PDWORD pdwOutBufferSize,
    PDWORD pdwMaxInstances
    );
