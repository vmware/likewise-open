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
 * Abstract:
 *
 * Authors:
 *
 */

#include "includes.h"
#include "sqlite3.h"

#define DB_QUERY_GET_MACHINEPWD_BY_HOST_NAME                     \
    "SELECT DomainSID,                                           \
            upper(DomainName),                                   \
            upper(DomainDnsName),                                \
            upper(HostName),                                     \
            HostDnsDomain,                                       \
            upper(MachineAccountName),                           \
            MachineAccountPassword,                              \
            PwdCreationTimestamp,                                \
            PwdClientModifyTimestamp,                            \
            SchannelType                                         \
       FROM machinepwd                                           \
      WHERE upper(HostName) = upper(%Q)"

DWORD
ReadMachineAccountSqlite(
    PCSTR pszMachinePwdDb,
    PCSTR pszHostName
    )
{
    DWORD dwError = 0;
    PSTR pszError = NULL;
    sqlite3 *pDbHandle = NULL;
    PSTR pszQuery = NULL;
    int numRows = 0;
    int numCols = 0;
    PSTR *ppszResults = NULL;
    DWORD dwExpectedNumCols = 10;
    PCSTR pszValue = NULL;
    PSTR pszEndPtr = NULL;
    int i;

    LWPS_PASSWORD_INFOA InfoA;
    LWPS_PASSWORD_INFO InfoW;

    dwError = sqlite3_open(pszMachinePwdDb, &pDbHandle);

    pszQuery = sqlite3_mprintf(DB_QUERY_GET_MACHINEPWD_BY_HOST_NAME, pszHostName);

    dwError = sqlite3_get_table(pDbHandle, pszQuery, &ppszResults, &numRows, &numCols, &pszError);

    if (ppszResults == NULL || numRows == 0 || LW_IS_NULL_OR_EMPTY_STR(ppszResults[1]))
    {
        dwError = LW_ERROR_INVALID_ACCOUNT;
    }
    else if (numRows != 1)
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    else if (numCols != dwExpectedNumCols)
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_UP_ERROR(dwError);

    i = dwExpectedNumCols;

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        //fprintf(stdout, "pszDomainSid = %s\n", pszValue);
        dwError = LwAllocateString(pszValue, &InfoA.pszSid);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        //fprintf(stdout, "pszDomainName = %s\n", pszValue);
        dwError = LwAllocateString(pszValue, &InfoA.pszDomainName);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        //fprintf(stdout, "pszDomainDnsName = %s\n", pszValue);
        dwError = LwAllocateString(pszValue, &InfoA.pszDnsDomainName);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        //fprintf(stdout, "pszHostName = %s\n", pszValue);
        dwError = LwAllocateString(pszValue, &InfoA.pszHostname);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        //fprintf(stdout, "pszHostDnsName = %s\n", pszValue);
        dwError = LwAllocateString(pszValue, &InfoA.pszHostDnsDomain);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        //fprintf(stdout, "pszMachineAccountName = %s\n", pszValue);
        dwError = LwAllocateString(pszValue, &InfoA.pszMachineAccount);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        //fprintf(stdout, "pszMachineAccountPassword = %s\n", pszValue);
        dwError = LwAllocateString(pszValue, &InfoA.pszMachinePassword);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    //tPwdCreationTimestamp

    //tPwdClientModifyTimestamp
    pszValue = ppszResults[i++];
    InfoA.last_change_time = (time_t) strtoll(pszValue, &pszEndPtr, 10);
    if (!pszEndPtr || (pszEndPtr == pszValue) || *pszEndPtr)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    InfoA.dwSchannelType = (UINT32)atol(pszValue);


    UpAllocateMachineInformationContentsW(
            &InfoA,
            &InfoW);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwpsWritePasswordToAllStores(&InfoW);
    BAIL_ON_UP_ERROR(dwError);

cleanup:

    if (pDbHandle)
    {
        if (pszQuery)
        {
            sqlite3_free(pszQuery);
            pszQuery = NULL;
        }

        if (ppszResults)
        {
            sqlite3_free_table(ppszResults);
            ppszResults = NULL;
        }

        sqlite3_close(pDbHandle);
        pDbHandle = NULL;
    }

    UpFreeMachineInformationContentsA(&InfoA);
    UpFreeMachineInformationContentsW(&InfoW);

    return dwError;

error:
    goto cleanup;
}
