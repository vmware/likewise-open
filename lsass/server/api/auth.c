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
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    
    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
        
    dwError = LSA_ERROR_NOT_HANDLED;
    
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
    
    return(dwError);
    
error:

    LsaSrvWriteLoginFailedEvent(hServer, pszLoginId);

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
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    
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
    
    return(dwError);
    
error:

    LsaSrvWriteLoginFailedEvent(hServer, pszLoginId);

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
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    BOOLEAN bInLock = FALSE;
    
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
    
    return(dwError);
    
error:

    goto cleanup;
}
