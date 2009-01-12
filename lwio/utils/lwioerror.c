/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        lsaerror.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (SMBSS)
 *
 *        Error Message API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */

#include "includes.h"

static
const char* gSMBErrorMessages[] =
{
    // SMB_ERROR_INVALID_CACHE_PATH                              : 61441
    "An invalid cache path was specified",
    // SMB_ERROR_INVALID_CONFIG_PATH                             : 61442
    "The path to the configuration file is invalid",
    // SMB_ERROR_INVALID_PREFIX_PATH                             : 61443
    "The product installation folder could not be determined",
    // SMB_ERROR_INSUFFICIENT_BUFFER                             : 61444
    "The provided buffer is insufficient",
    // SMB_ERROR_OUT_OF_MEMORY                                   : 61445
    "Out of memory",
    // SMB_ERROR_NOT_IMPLEMENTED                                 : 61446
    "The requested feature has not been implemented yet",
    // SMB_ERROR_REGEX_COMPILE_FAILED                            : 61447
    "Failed to compile regular expression",
    // SMB_ERROR_INTERNAL                                        : 61448
    "Internal error",
    // SMB_ERROR_INVALID_PARAMETER                               : 61449
    "Invalid parameter",
    // SMB_ERROR_INVALID_CONFIG                                  : 61450
    "The specified configuration (file) is invalid",
    // SMB_ERROR_UNEXPECTED_TOKEN                                : 61451
    "An unexpected token was encountered in the configuration",
    // SMB_ERROR_NULL_BUFFER                                     : 61452
    "An invalid buffer was provided",
    // SMB_ERROR_INVALID_LOG_LEVEL                               : 61453
    "An invalid log level was specified",
    // SMB_ERROR_LWMSG_ERROR                                     : 61454
    "An error was detected in the lwmsg layer",
    // SMB_ERROR_MALFORMED_REQUEST                               : 61455
    "A malformed request was submitted",
    // SMB_ERROR_LWMSG_EOF                                       : 61456
    "End of file reported by lwmsg",
    // SMB_ERROR_NO_SUCH_ITEM                                    : 61457
    "The requested item was not found",
    // SMB_ERROR_OVERFLOW                                        : 61458
    "Arithmetic overflow",
    // SMB_ERROR_UNDERFLOW                                       : 61459
    "Arithmetic underflow",
    // SMB_ERROR_SYSTEM                                          : 61460
    "An unexpected system error was encountered",
    // SMB_ERROR_SERVER_UNREACHABLE                              : 61461
    "The SMB Server could not be reached",
    // SMB_ERROR_STRING_CONV_FAILED                              : 61462
    "String conversion (Unicode/Ansi) failed",
    // SMB_ERROR_PASSWORD_EXPIRED                                : 61463
    "Password expired",
    // SMB_ERROR_PASSWORD_MISMATCH                               : 61464
    "Incorrect password",
    // SMB_ERROR_CLOCK_SKEW                                      : 61465
    "Clock skew detected",
    // SMB_ERROR_KRB5_NO_KEYS_FOUND                              : 61466
    "No kerberos keys found",
    // SMB_ERROR_KRB5_CALL_FAILED                                : 61467
    "Kerberos call failed",
    // SMB_ERROR_NO_BIT_AVAILABLE                                : 61468
    "No bits are available in vector",
    // SMB_ERROR_INVALID_HANDLE                                  : 61469
    "An invalid SMB Handle was specified",
    // SMB_ERROR_OUT_OF_HANDLES                                  : 61470
    "Out of handles",
    // SMB_ERROR_GSS                                             : 61471
    "GSS-API Error",
    // SMB_ERROR_HOST_NOT_FOUND                                  : 61472
    "Failed to look up host in DNS",
    // SMB_ERROR_INVALID_VFS_PROVIDER                            : 61473
    "The virtual file system provider is invalid",
    // SMB_ERROR_NO_SUCH_SHARE                                   : 61474
    "The requested share could not be located",
    // SMB_ERROR_NO_MORE_SHARES                                  : 61475
    "No more shares found",
    // SMB_ERROR_DATA_ERROR                                      : 61476
    "A database format error has been encountered"
};

size_t
SMBGetErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;

    if (pszBuffer && stBufSize) {
       memset(pszBuffer, 0, stBufSize);
    }

    if (!dwError)
    {
        // No error string for success
        goto cleanup;
    }

    if (SMB_ERROR_MASK(dwError) != 0)
    {
        stResult = SMBMapSMBErrorToString(
                        dwError,
                        pszBuffer,
                        stBufSize);
    }
    else
    {
        stResult = SMBGetSystemErrorString(
                        dwError,
                        pszBuffer,
                        stBufSize);
    }

cleanup:

    return stResult;
}

size_t
SMBMapSMBErrorToString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;
    DWORD dwNMessages = sizeof(gSMBErrorMessages)/sizeof(PCSTR);

    if ((dwError >= SMB_ERROR_INVALID_CACHE_PATH) &&
        (dwError < SMB_ERROR_SENTINEL))
    {
        DWORD dwErrorOffset = dwError - 0xF000;

        if (dwErrorOffset < dwNMessages)
        {
            PCSTR pszMessage = gSMBErrorMessages[dwErrorOffset];
            DWORD dwRequiredLen = strlen(pszMessage) + 1;

            if (stBufSize >= dwRequiredLen) {
                memcpy(pszBuffer, pszMessage, dwRequiredLen);
            }

            stResult = dwRequiredLen;

            goto cleanup;
        }
    }

    stResult = SMBGetUnmappedErrorString(
                        dwError,
                        pszBuffer,
                        stBufSize);

cleanup:

    return stResult;
}

size_t
SMBGetSystemErrorString(
    DWORD  dwConvertError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    DWORD  dwError = SMB_ERROR_SUCCESS;
    size_t stResult = 0;
    PSTR   pszTempBuffer = NULL;

    int result = SMBStrError(dwConvertError, pszBuffer, stBufSize);
    if (result == EINVAL)
    {
        stResult = SMBGetUnmappedErrorString(
                        dwConvertError,
                        pszBuffer,
                        stBufSize);
        goto cleanup;
    }

    while (result != 0)
    {
        if (result == ERANGE)
        {
            // Guess
            stBufSize = stBufSize * 2 + 10;
        }
        else
        {
            stResult = SMBGetUnmappedErrorString(
                            dwConvertError,
                            pszBuffer,
                            stBufSize);
            goto cleanup;
        }
        SMB_SAFE_FREE_MEMORY(pszTempBuffer);

        dwError = SMBAllocateMemory(
                        stBufSize,
                        (PVOID*)&pszTempBuffer);
        BAIL_ON_SMB_ERROR(dwError);

        result = SMBStrError(dwConvertError, pszTempBuffer, stBufSize);
    }

    if (pszTempBuffer != NULL)
    {
        stResult = strlen(pszTempBuffer) + 1;
    }
    else
    {
        stResult = strlen(pszBuffer) + 1;
    }

cleanup:

    SMB_SAFE_FREE_MEMORY(pszTempBuffer);

    return stResult;

error:

    stResult = 0;

    goto cleanup;
}

size_t
SMBGetUnmappedErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;
    CHAR  szBuf[128] = "";
    DWORD dwRequiredLen = 0;

    dwRequiredLen = sprintf(szBuf, "Error [code=%d] occurred.", dwError) + 1;

    if (stBufSize >= dwRequiredLen) {
        memcpy(pszBuffer, szBuf, dwRequiredLen);
    }

    stResult = dwRequiredLen;

    return stResult;
}

DWORD
SMBGetErrorMessageForLoggingEvent(
    DWORD dwErrCode,
    PSTR* ppszErrorMsg)
{
    DWORD dwErrorBufferSize = 0;
    DWORD dwError = 0;
    DWORD dwLen = 0;
    PSTR  pszErrorMsg = NULL;
    PSTR  pszErrorBuffer = NULL;

    dwErrorBufferSize = SMBGetErrorString(dwErrCode, NULL, 0);

    if (!dwErrorBufferSize)
        goto cleanup;

    dwError = SMBAllocateMemory(
                dwErrorBufferSize,
                (PVOID*)&pszErrorBuffer);
    BAIL_ON_SMB_ERROR(dwError);

    dwLen = SMBGetErrorString(dwErrCode, pszErrorBuffer, dwErrorBufferSize);

    if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
    {
        dwError = SMBAllocateStringPrintf(
                     &pszErrorMsg,
                     "Error: %s [error code: %d]",
                     pszErrorBuffer,
                     dwErrCode);
        BAIL_ON_SMB_ERROR(dwError);
    }

    *ppszErrorMsg = pszErrorMsg;

cleanup:

    SMB_SAFE_FREE_STRING(pszErrorBuffer);

    return dwError;

error:

    SMB_SAFE_FREE_STRING(pszErrorMsg);

    *ppszErrorMsg = NULL;

    goto cleanup;
}
