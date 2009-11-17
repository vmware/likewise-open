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
 *        regerror.c
 *
 * Abstract:
 *
 *        Likewise Registry
 *
 *        Error Message API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */

#include "includes.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

DWORD
RegGetErrorMessageForLoggingEvent(
    DWORD dwErrCode,
    PSTR* ppszErrorMsg
    )
{
    DWORD dwErrorBufferSize = 0;
    DWORD dwError = 0;
    DWORD dwLen = 0;
    PSTR  pszErrorMsg = NULL;
    PSTR  pszErrorBuffer = NULL;

    dwErrorBufferSize = LwGetErrorString(dwErrCode, NULL, 0);

    if (!dwErrorBufferSize)
        goto cleanup;

    dwError = LwAllocateMemory(
                dwErrorBufferSize,
                (PVOID*)&pszErrorBuffer);
    BAIL_ON_REG_ERROR(dwError);

    dwLen = LwGetErrorString(dwErrCode, pszErrorBuffer, dwErrorBufferSize);

    if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
    {
        dwError = LwAllocateStringPrintf(
                     &pszErrorMsg,
                     "Error: %s [error code: %d]",
                     pszErrorBuffer,
                     dwErrCode);
        BAIL_ON_REG_ERROR(dwError);
    }

    *ppszErrorMsg = pszErrorMsg;

cleanup:

    LW_SAFE_FREE_STRING(pszErrorBuffer);

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszErrorMsg);

    *ppszErrorMsg = NULL;

    goto cleanup;
}

VOID
RegPrintError(
    IN OPTIONAL PCSTR pszErrorPrefix,
    IN DWORD dwError
    )
{
    PCSTR pszUseErrorPrefix = NULL;
    size_t size = 0;
    PSTR pszErrorString = NULL;

    if (dwError)
    {
        pszUseErrorPrefix = pszErrorPrefix;
        if (!pszUseErrorPrefix)
        {
            pszUseErrorPrefix = "REGISTRY ERROR: ";
        }

        size = LwGetErrorString(dwError, NULL, 0);
        if (size)
        {
            pszErrorString = malloc(size);
            if (pszErrorString)
            {
                LwGetErrorString(dwError, pszErrorString, size);
            }
        }
        if (LW_IS_NULL_OR_EMPTY_STR(pszErrorString))
        {
            fprintf(stderr,
                    "%s (error = %u - %s)\n",
                     pszUseErrorPrefix,
                     dwError,
                     LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
        }
        else
        {
            fprintf(stderr,
                    "%s (error = %u - %s)\n%s\n",
                    pszUseErrorPrefix,
                    dwError,
                    LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)),
                    pszErrorString);
        }
    }
}
