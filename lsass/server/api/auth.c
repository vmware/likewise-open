/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        auth.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Authentication API (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

DWORD
LsaSrvAuthenticateUser(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_AUTHENTICATION};
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnAuthenticateUser(
                                            hProvider,
                                            pszLoginId,
                                            pszPassword);
        if (!dwError) {
           break;
        }
        else if ((dwError == LSA_ERROR_NOT_HANDLED) |
                 (dwError == LSA_ERROR_NO_SUCH_USER)) {
           LsaSrvCloseProvider(pProvider, hProvider);
           hProvider = (HANDLE)NULL;
           continue;
        } else {
           BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (pProvider == NULL)
    {
        dwError = LSA_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    if (!dwError)
    {
        LsaSrvIncrementMetricValue(LsaMetricSuccessfulAuthentications);
    }
    else
    {
        LsaSrvIncrementMetricValue(LsaMetricFailedAuthentications);
    }

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;

error:

    if (LsaSrvEventlogEnabled()){
        LsaSrvWriteLoginFailedEvent(
            hServer,
            pszLoginId,
            dwError);
    }
    if (dwError == LSA_ERROR_NOT_HANDLED ||
                   dwError == LSA_ERROR_NO_SUCH_USER) {
        LSA_LOG_VERBOSE("Failed authenticate unknown user [%s]", IsNullOrEmptyString(pszLoginId) ? "" : pszLoginId);
    }
    else {
        LSA_LOG_ERROR("Failed authenticate user [%s] [code %d]", IsNullOrEmptyString(pszLoginId) ? "" : pszLoginId, dwError);
    }

    goto cleanup;
}

DWORD
LsaSrvAuthenticateUserEx(
    HANDLE hServer,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO *ppUserInfo
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_AUTHENTICATION};
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnAuthenticateUserEx(
                                            hProvider,
                                            pUserParams,
                                            ppUserInfo);
        if (!dwError) 
	{
            break;
        }
        else if ((dwError == LSA_ERROR_NOT_HANDLED) |
                 (dwError == LSA_ERROR_NO_SUCH_USER)) 
	{
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;
            continue;
        } else {
	    BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (pProvider == NULL) {
        dwError = LSA_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hProvider != (HANDLE)NULL) 
    {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    if (!dwError) 
    {
        LsaSrvIncrementMetricValue(LsaMetricSuccessfulAuthentications);
    }
    else
    {
        LsaSrvIncrementMetricValue(LsaMetricFailedAuthentications);
    }

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;

error:

    if (LsaSrvEventlogEnabled()) 
    {
        LsaSrvWriteLoginFailedEvent(
            hServer,
            pUserParams && pUserParams->pszAccountName ? 
	        pUserParams->pszAccountName : "(no name)",
            dwError);
    }

    if (dwError == LSA_ERROR_NOT_HANDLED ||
                   dwError == LSA_ERROR_NO_SUCH_USER) 
    {
        LSA_LOG_VERBOSE("Failed authenticate unknown user [%s]",
			pUserParams && pUserParams->pszAccountName ? 
			    pUserParams->pszAccountName : "");	
    }
    else 
    {
        LSA_LOG_ERROR("Failed authenticate user [%s] [code %d]",
		      pUserParams && pUserParams->pszAccountName ? 
		          pUserParams->pszAccountName : "");	
    }

    goto cleanup;	
}

DWORD
LsaSrvValidateUser(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_AUTHENTICATION};
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LSA_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnValidateUser(
                                            hProvider,
                                            pszLoginId,
                                            pszPassword);
        if (!dwError) {
           break;
        }
        else if ((dwError == LSA_ERROR_NOT_HANDLED) ||
                 (dwError == LSA_ERROR_NO_SUCH_USER)) {
           LsaSrvCloseProvider(pProvider, hProvider);
           hProvider = (HANDLE)NULL;
           continue;
        } else {
           BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;

error:

    if (LsaSrvEventlogEnabled()){
        LsaSrvWriteLoginFailedEvent(
            hServer,
            pszLoginId,
            dwError);
    }

    goto cleanup;
}

DWORD
LsaSrvCheckUserInList(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszListName
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_AUTHENTICATION};
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LSA_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnCheckUserInList(
                                            hProvider,
                                            pszLoginId,
                                            pszListName);
        if (!dwError) {
           break;
        }
        else if ((dwError == LSA_ERROR_NOT_HANDLED) ||
                 (dwError == LSA_ERROR_NO_SUCH_USER)) {
           LsaSrvCloseProvider(pProvider, hProvider);
           hProvider = (HANDLE)NULL;
           continue;
        } else {
           BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return(dwError);

error:

    if (LsaSrvEventlogEnabled()){
        LsaSrvWriteLoginFailedEvent(
            hServer,
            pszLoginId,
            dwError);
    }

    goto cleanup;
}

DWORD
LsaSrvChangePassword(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword
    )
{
    DWORD  dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_AUTHENTICATION};
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    BOOLEAN bInLock = FALSE;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LSA_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnChangePassword(
                                        hProvider,
                                        pszLoginId,
                                        pszPassword,
                                        pszOldPassword);
        if (!dwError) {
           break;
        }
        else if ((dwError == LSA_ERROR_NOT_HANDLED) ||
                 (dwError == LSA_ERROR_NO_SUCH_USER)) {
           LsaSrvCloseProvider(pProvider, hProvider);
           hProvider = (HANDLE)NULL;
           continue;
        } else {
           BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    if (hProvider != (HANDLE)NULL) {
       LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    if (!dwError)
    {
        LsaSrvIncrementMetricValue(LsaMetricSuccessfulChangePassword);
    }
    else
    {
        LsaSrvIncrementMetricValue(LsaMetricFailedChangePassword);
    }

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return(dwError);

error:

    LSA_LOG_VERBOSE("Failed to change password of user [%s] [code %d]", IsNullOrEmptyString(pszLoginId) ? "" : pszLoginId, dwError);
    goto cleanup;
}
