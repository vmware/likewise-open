/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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

#include "includes.h"

DWORD
LwMbsToWc16s(
    PCSTR pszInput,
    PWSTR* ppszOutput
    )
{
    DWORD dwError = 0;
    PWSTR pszOutput = NULL;

    if (!pszInput) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    pszOutput = ambstowc16s(pszInput);
    if (!pszOutput) {
        dwError = LW_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LW_ERROR(dwError);
    }

    *ppszOutput = pszOutput;

cleanup:

    return dwError;

error:

    *ppszOutput = NULL;

    goto cleanup;
}

DWORD
LwWc16snToMbs(
    PCWSTR pwszInput,
    PSTR*  ppszOutput,
    size_t sMaxChars
    )
{
    DWORD dwError = 0;
    PWSTR pwszTruncated = NULL;
    PSTR pszOutput = NULL;

    if (!pwszInput) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    pwszTruncated = _wc16sndup(pwszInput, sMaxChars);
    if (!pwszTruncated) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    pszOutput = awc16stombs(pwszTruncated);
    if (!pszOutput) {
        dwError = LW_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LW_ERROR(dwError);
    }

    *ppszOutput = pszOutput;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszTruncated);

    return dwError;

error:

    *ppszOutput = NULL;

    goto cleanup;
}

DWORD
LwWc16sToMbs(
    PCWSTR pwszInput,
    PSTR*  ppszOutput
    )
{
    DWORD dwError = 0;
    PSTR pszOutput = NULL;

    if (!pwszInput) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    pszOutput = awc16stombs(pwszInput);
    if (!pszOutput) {
        dwError = LW_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LW_ERROR(dwError);
    }

    *ppszOutput = pszOutput;

cleanup:

    return dwError;

error:

    *ppszOutput = NULL;

    goto cleanup;
}

DWORD
LwWc16sLen(
    PCWSTR  pwszInput,
    size_t* psLen
    )
{
    DWORD dwError = 0;
    size_t sLen = 0;

    if (!pwszInput) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    sLen = wc16slen(pwszInput);

    *psLen = sLen;

cleanup:

    return dwError;

error:

    *psLen = 0;

    goto cleanup;
}

DWORD
LwSW16printf(
    PWSTR*  ppwszStrOutput,
    PCSTR   pszFormat,
    ...)
{
    DWORD dwError = 0;
    INT ret = 0;
    PWSTR pwszStrOutput = NULL;
    va_list args;

    va_start(args, pszFormat);

    ret = sw16printf(
                  pwszStrOutput,
                  pszFormat,
                  args);

    if (ret == -1){
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    *ppwszStrOutput = pwszStrOutput;

cleanup:

    va_end(args);

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszStrOutput);

    goto cleanup;

}

DWORD
LwWc16ToUpper(
    IN OUT PWSTR pwszString
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    wc16supper(pwszString);

    return dwError;
}

DWORD
LwAllocateWc16String(
    PWSTR *ppwszOutputString,
    PCWSTR pwszInputString
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwLen = 0;
    PWSTR pwszOutputString = NULL;

    if (!pwszInputString)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwLen = wc16slen(pwszInputString);

    dwError = LwAllocateMemory(dwLen + 1,
                               OUT_PPVOID(&pwszOutputString));
    BAIL_ON_LW_ERROR(dwError);

    if (dwLen)
    {
        wc16sncpy(pwszOutputString, pwszInputString, dwLen);
    }

    *ppwszOutputString = pwszOutputString;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszOutputString);

    *ppwszOutputString = NULL;
    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
