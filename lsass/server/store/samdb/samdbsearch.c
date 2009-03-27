#include "includes.h"

DWORD
SamDbSearchObject(
    HANDLE            hDirectory,
    PWSTR             pwszBase,
    ULONG             ulScope,
    PWSTR             pwszFilter,
    PWSTR             wszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    )
{
    DWORD dwError = 0;
    SAMDB_ENTRY_TYPE entryType = 0;

    dwError = SamDbBuildSqlQuery(
                    pwszFilter,
                    wszAttributes,
                    &pszQuery
                    );
    BAIL_ON_ERROR(dwError);

    dwError = SamDbMakeSqlQuery(
                    &ppszString
                    );
    BAIL_ON_ERROR(dwError);

    dwError = SamDbExecuteSqlQuery(
                    ppszString,

    dwError = SamDbMarshallData(
                    pszSqlQuery,
                    ppszResult,
                    nRows,
                    nCols,
                    &pDirectoryEntries,
                    &dwNumEntries
                    );

    *ppDirectoryEntries = pDirectoryEntries;
    *pdwNumEntries = dwNumEntries;

cleanup:

    return(dwError);

error:

    goto cleanup;

}

DWORD
SamDbBuildSqlQuery(
    PWSTR pwszFilter,
    PWSTR wszAttributes[],
    PSTR * ppszQuery
    )
{
    DWORD dwError = 0;

    dwError = SamDbExtractMemberShip(
                        wszAttributes,
                        &wszNewAttributes,
                        &bMembersAttributeExists
                        );
    BAIL_ON_ERROR(dwError);

    sprintf(pszQuery,"select ");

    for (i = 0; i < dwNumAttributes; i++) {

        strcat(pszQuery, wszNewAttributes[i]);
        strcat(pszQuery", ");
    }

    strcat(pszQuery, "from object_table");

    // Now concatenate the where clause

    strcat(pszQuery, pwszFilter);

    return (dwError);
}
