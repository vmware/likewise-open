/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        peeknp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSASS)
 *
 *        PeekNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"


DWORD
ClientPeekNamedPipe(
    HANDLE hNamedPipe,
    PVOID  pInBuffer,
    DWORD  dwInBufferSize,
    PDWORD pdwBytesRead,
    PDWORD pdwTotalBytesAvail,
    PDWORD pdwBytesLeftThisMessage
    )
{
    return SMB_ERROR_NOT_IMPLEMENTED;
}

