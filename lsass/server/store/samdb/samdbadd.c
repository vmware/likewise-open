
#include "includes.h"

DWORD
SamDbAddObject(
    HANDLE hBindHandle,
    PWSTR  pwszObjectDN,
    DIRECTORY_MOD Modifications[]
    )
{
    DWORD dwError = 0;
#if 0
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = hBindHandle;
    PWSTR pwszObjectName = NULL;
    PWSTR pwszDomain = NULL;
    PSAM_DB_DN pDN = NULL;
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;

   dwError = SamDbGetObjectClass(Modifications, &dwObjectType);
   BAIL_ON_ERROR(dwError);

   dwError = SamDbAddValidateSchema(
                       dwObjectType,
                       Modifications
                       );
   BAIL_ON_ERROR(dwError);

   dwError = SamDbInsertObjecttoDatabase(
                       pwszObjectDN,
                       Modifications
                       );
   BAIL_ON_ERROR(dwError)
#endif

   return dwError;
}

