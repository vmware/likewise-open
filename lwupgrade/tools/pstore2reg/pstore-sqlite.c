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
    int i;

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
        fprintf(stdout, "pszDomainSid = %s\n", pszValue);
        //dwError = LwAllocateString(pszValue, pszDomainSid);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        fprintf(stdout, "pszDomainName = %s\n", pszValue);
        //dwError = LwAllocateString(pszValue, pszDomainName);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        fprintf(stdout, "pszDomainDnsName = %s\n", pszValue);
        //dwError = LwAllocateString(pszValue, pszDomainDnsName);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        fprintf(stdout, "pszHostName = %s\n", pszValue);
        //dwError = LwAllocateString(pszValue, pszHostName);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        fprintf(stdout, "pszHostDnsName = %s\n", pszValue);
        //dwError = LwAllocateString(pszValue, pszHostDnsName);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        fprintf(stdout, "pszMachineAccountName = %s\n", pszValue);
        //dwError = LwAllocateString(pszValue, pszMachineAccountName);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        fprintf(stdout, "pszMachineAccountPassword = %s\n", pszValue);
        //dwError = LwAllocateString(pszValue, &pszMachineAccountPassword);
        BAIL_ON_UP_ERROR(dwError);
    }

    //tPwdCreationTimestamp
    //tPwdClientModifyTimestamp
    //dwSchannelType

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

    return dwError;

error:
    goto cleanup;
}
