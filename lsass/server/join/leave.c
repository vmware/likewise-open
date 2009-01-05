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
 *        leave.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Leave from Active Directory
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
LsaNetLeaveDomain(
    PCSTR pszUsername,
    PCSTR pszPassword
    )
{
    DWORD dwError = 0;
    HANDLE hStore = (HANDLE)NULL;
    PSTR  pszHostname = NULL;
    PWSTR pwszHostname = NULL;
    PWSTR pwszDnsDomainName = NULL;
    DWORD dwOptions = (NETSETUP_ACCT_DELETE);
    PLWPS_PASSWORD_INFO pPassInfo = NULL;
    PLSA_MACHINE_ACCT_INFO pAcct = NULL;
    LSA_ACCESS_TOKEN_FREE_INFO accessInfo = {0};
    
    if (geteuid() != 0) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_SQLDB, &hStore);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwpsGetPasswordByHostName(
                hStore,
                pszHostname,
                &pPassInfo);
    if (dwError)
    {
        if (dwError == LWPS_ERROR_INVALID_ACCOUNT)
        {
            dwError = LSA_ERROR_NOT_JOINED_TO_AD;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaBuildMachineAccountInfo(
                    pPassInfo,
                    &pAcct);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!IsNullOrEmptyString(pAcct->pszDnsDomainName))
    {
        dwError = LsaMbsToWc16s(
                    pAcct->pszDnsDomainName,
                    &pwszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaMbsToWc16s(
                    pszHostname,
                    &pwszHostname);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!IsNullOrEmptyString(pszUsername) &&
        !IsNullOrEmptyString(pszPassword)) {

        dwError = LsaSetSMBAccessToken(
                    pAcct->pszDnsDomainName,
                    pszUsername,
                    pszPassword,
                    LSA_NET_JOIN_DOMAIN_NOTIMESYNC,
                    &accessInfo);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = Win32ErrorToErrno(
            NetUnjoinDomainLocal(
                pwszHostname,
                pwszDnsDomainName,
                NULL,
                NULL,
                dwOptions));
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LwpsDeleteEntriesInAllStores();
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pPassInfo)
    {
        LwpsFreePasswordInfo(hStore, pPassInfo);
    }

    if (hStore != (HANDLE)NULL) {
        LwpsClosePasswordStore(hStore);
    }
    
    if (pAcct)
    {
        LsaFreeMachineAccountInfo(pAcct);
    }

    LSA_SAFE_FREE_STRING(pszHostname);

    LSA_SAFE_FREE_MEMORY(pwszHostname);
    LSA_SAFE_FREE_MEMORY(pwszDnsDomainName);
    LsaFreeSMBAccessTokenContents(&accessInfo);

    return dwError;
    
error:

    goto cleanup;
}

