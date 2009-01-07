/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        callnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CallNamedPipe API (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"


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
    )
{
    return SMB_ERROR_NOT_IMPLEMENTED;
}

