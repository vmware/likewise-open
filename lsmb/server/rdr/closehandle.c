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
 *        CloseHandle API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"


DWORD
RdrCloseFileEx(
    HANDLE hFile
    )
{
    DWORD dwError = 0;
    PSMB_CLIENT_FILE_HANDLE pFile = (PSMB_CLIENT_FILE_HANDLE)hFile;

    if (pFile->pTree)
    {
        SMBTreeRelease(pFile->pTree);
    }

    SMB_SAFE_FREE_STRING(pFile->pszCachePath);
    SMB_SAFE_FREE_STRING(pFile->pszPrincipal);

    SMBFreeMemory(pFile);

    return dwError;
}
