#include "includes.h"

NTSTATUS
SamDbSearchObject(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Filter,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PDIRECTORY_VALUES * ppDirectoryValues
    PDWORD pdwNumValues
    )
{
    NTSTATUS ntStatus = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;

    ntStatus = SamDbConvertFiltertoTable(
                    Filter,
                    &dwTable
                    );
    switch (dwTable) {

        case SAMDB_USER:
            SamDbSearchUsers(
                    hDirectory,
                    Attributes,
                    AttributesOnly,
                    ppDirectoryValues,
                    pdwNumValues
                    );
            break;

        case SAMDB_GROUP:
            SamDbSearchGroups(
                    hDirectory,
                    Attributes,
                    AttributesOnly,
                    ppDirectoryValues,
                    pdwNumValues
                    );
            break;

        case SAMDB_DOMAIN:
            SamDbSearchDomains(
                    hDirectory,
                    Attributes,
                    AttributesOnly,
                    ppDirectoryValues,
                    pdwNumValues
                    );
            break;
    }

    return ntStatus;

}

NTSTATUS
SamDbSearchUsers(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Filter,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PDIRECTORY_VALUES * ppDirectoryValues
    PDWORD pdwNumValues
    )
{
    NTSTATUS ntStatus = 0;
    DWORD nRows = 0;
    DWORD nCols = 0;
    DWORD iRow = 0;
    DWORD iCol = 0;


    dwError = sqlite3_get_table(
                        pDbHandle,
                        pszQuery,
                        &ppszResult,
                        &nRows,
                        &nCols,
                        &pszError
                        );
    BAIL_ON_LSA_ERROR(dwError);

    for (iRow = 0; iRow < nRows; iRow++) {

        dwError = SamDbAllocateMemory(
                        sizeof(


DWORD
MarshallDbEntryToDirectoryEntry(
    PSTR pszResult
    )
{

}





static
DWORD
LsaProviderLocal_DbWriteToGroupInfo_0_Unsafe(
    PSTR*  ppszResult,
    int    nRows,
    int    nCols,
    DWORD  nHeaderColsToSkip,
    PLSA_GROUP_INFO_0** pppGroupInfoList,
    PDWORD pdwNumGroupsFound
    )
{
    DWORD dwError = 0;
    DWORD iCol = 0, iRow = 0;
    DWORD iVal = nHeaderColsToSkip;
    DWORD dwGroupInfoLevel = 0;
    PLSA_GROUP_INFO_0* ppGroupInfoList = NULL;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    DWORD dwNumGroupsFound = nRows;

    dwError = LsaAllocateMemory(
                    sizeof(PLSA_GROUP_INFO_0) * nRows,
                    (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    for (iRow = 0; iRow < nRows; iRow++) {

        dwError = LsaAllocateMemory(
                        sizeof(LSA_GROUP_INFO_0),
                        (PVOID*)&pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);

        for (iCol = 0; iCol < nCols; iCol++) {
            switch(iCol) {
                case 0: /* Name */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pGroupInfo->pszName);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 1: /* Gid */
                {
                    pGroupInfo->gid = atoi(ppszResult[iVal]);
                    break;
                }
            }
            iVal++;
        }

        dwError = LsaAllocateStringPrintf(
					&pGroupInfo->pszSid,
					LOCAL_GROUP_SID_FORMAT,
					pGroupInfo->gid);
        BAIL_ON_LSA_ERROR(dwError);

        *(ppGroupInfoList + iRow) = pGroupInfo;
        pGroupInfo = NULL;
    }

    *pppGroupInfoList = ppGroupInfoList;
    *pdwNumGroupsFound = dwNumGroupsFound;

cleanup:

    return dwError;

error:

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwNumGroupsFound);
    }

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = 0;

    goto cleanup;
}
