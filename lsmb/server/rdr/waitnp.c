/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        waitnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSASS)
 *
 *        WaitNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"


DWORD
ClientWaitNamedPipe(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszName,
    DWORD dwTimeout
    )
{
    return SMB_ERROR_NOT_IMPLEMENTED;
}
