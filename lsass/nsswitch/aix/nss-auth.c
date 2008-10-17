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

#include "includes.h"
#include "nss-auth.h"
#include "nss-user.h"

int
LsaNssNormalizeUsername(
    PSTR pszInput,
    PSTR pszOutput
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PLSA_USER_INFO_0 pInfo = NULL;
    const DWORD dwInfoLevel = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    uint64_t qwConvert = 0;
    int iDigit = 0;
    PSTR pszPos = NULL;

    if (strlen(pszInput) < S_NAMELEN)
    {
        strcpy(pszOutput, pszInput);
        goto cleanup;
    }

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindUserByName(
                hLsaConnection,
                pszInput,
                dwInfoLevel,
                (PVOID*)&pInfo);
    BAIL_ON_LSA_ERROR(dwError);

    qwConvert = pInfo->uid;

    pszPos = pszOutput + S_NAMELEN - 1;
    *pszPos-- = 0;

    if (qwConvert < 10000000)
    {
        // Mangle the username with the old rules
        while(pszPos > pszOutput)
        {
            iDigit = qwConvert % 10;
            *pszPos = iDigit + '0';

            qwConvert /= 10;
            pszPos--;
        }
    }
    else
    {
        // Mangle the username with the new rules. The mangled user name will
        // start with _ and the second character will be a letter. The uid
        // number (with padding) will be in base 32.
        qwConvert += 10737418240ull;

        while(pszPos > pszOutput)
        {
            iDigit = qwConvert % 32;
            if (iDigit < 10)
            {
                *pszPos = iDigit + '0';
            }
            else
            {
                *pszPos = iDigit + 'a' - 10;
            }

            qwConvert /= 32;
            pszPos--;
        }
    }
    *pszPos = '_';

cleanup:

    if (pInfo != NULL)
    {
        LsaFreeUserInfo(
                dwInfoLevel,
                (PVOID)pInfo);
    }
    if (dwError != LSA_ERROR_SUCCESS)
    {
        LsaNssMapErrorCode(dwError, &errno);
        return 0;
    }
    return strlen(pszOutput);

error:

    *pszOutput = 0;
    goto cleanup;
}

int
LsaNssAuthenticate(
    PSTR pszUser,
    PSTR pszResponse,
    int* pReenter,
    PSTR* ppszOutputMessage
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PLSA_USER_INFO_0 pInfo = NULL;
    const DWORD dwInfoLevel = 0;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNssFindUserByAixName(
                hLsaConnection,
                pszUser,
                dwInfoLevel,
                (PVOID*)&pInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAuthenticateUser(
                hLsaConnection,
                pInfo->pszName,
                pszResponse);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckUserInList(
                        hLsaConnection,
                        pInfo->pszName,
                        NULL);
    BAIL_ON_LSA_ERROR(dwError);

    // Need to ensure that home directories are created.
    dwError = LsaOpenSession(
                  hLsaConnection,
                  pInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pInfo != NULL)
    {
        LsaFreeUserInfo(
                dwInfoLevel,
                pInfo);
    }
    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    switch(dwError)
    {
        case LSA_ERROR_SUCCESS:
            return AUTH_SUCCESS;
        case LSA_ERROR_NOT_HANDLED:
        case LSA_ERROR_NO_SUCH_USER:
            return AUTH_NOTFOUND;
        case LSA_ERROR_ACCOUNT_EXPIRED:
        case LSA_ERROR_ACCOUNT_DISABLED:
        case LSA_ERROR_ACCOUNT_LOCKED:
            return AUTH_FAILURE;
        default:
            return AUTH_UNAVAIL;
    }

error:
    
    goto cleanup;
}

int
LsaNssIsPasswordExpired(
        PSTR pszUser,
        PSTR* ppszMessage
        )
{
    PLSA_USER_INFO_2 pInfo = NULL;
    const DWORD dwInfoLevel = 2;
    HANDLE hLsaConnection = (HANDLE)NULL;
    DWORD dwError = LSA_ERROR_SUCCESS;

    *ppszMessage = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNssFindUserByAixName(
                hLsaConnection,
                pszUser,
                dwInfoLevel,
                (PVOID*)&pInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pInfo->bPasswordExpired)
    {
        dwError = LsaAllocateStringPrintf(
                ppszMessage,
                "%s's password is expired",
                pszUser);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LSA_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pInfo->bPromptPasswordChange &&
        pInfo->dwDaysToPasswordExpiry)
    {
        dwError = LsaAllocateStringPrintf(
                ppszMessage,
                "Your password will expire in %d days\n",
                pInfo->dwDaysToPasswordExpiry);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pInfo != NULL)
    {
        LsaFreeUserInfo(
                dwInfoLevel,
                (PVOID)pInfo);
    }
    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }
    switch(dwError)
    {
        case LSA_ERROR_SUCCESS:
            return 0;
        case LSA_ERROR_PASSWORD_EXPIRED:
            return 1;
        default:
            // password is expired and cannot login
            LsaNssMapErrorCode(dwError, &errno);
            return 2;
    }

error:

    goto cleanup;
}

int
LsaNssChangePassword(
        PSTR pszUser,
        PSTR pszOldPass,
        PSTR pszNewPass,
        PSTR* ppszError)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PLSA_USER_INFO_0 pInfo = NULL;
    const DWORD dwInfoLevel = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;

    *ppszError = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNssFindUserByAixName(
                hLsaConnection,
                pszUser,
                dwInfoLevel,
                (PVOID*)&pInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaChangePassword(
                hLsaConnection,
                pInfo->pszName,
                pszNewPass,
                pszOldPass);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pInfo != NULL)
    {
        LsaFreeUserInfo(
                dwInfoLevel,
                (PVOID)pInfo);
    }
    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }
    if(dwError != LSA_ERROR_SUCCESS)
    {
        LsaNssMapErrorCode(dwError, &errno);
        return -1;
    }
    return 0;

error:

    goto cleanup;
}
