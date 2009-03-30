
#include "includes.h"

static
DWORD
SamDbInsertObjectToDatabase(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszObjectDN,
    DIRECTORY_MOD          modifications[]
    );

DWORD
SamDbAddObject(
    HANDLE        hBindHandle,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = hBindHandle;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;

    dwError = SamDbGetObjectClass(
                    modifications,
                    &objectClass);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbSchemaAddValidateDirMods(
                    pDirectoryContext,
                    objectClass,
                    modifications);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbInsertObjectToDatabase(
                    pDirectoryContext,
                    pwszObjectDN,
                    modifications);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

   return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbInsertObjectToDatabase(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszObjectDN,
    DIRECTORY_MOD          modifications[]
    )
{
    DWORD dwError = 0;

    // TODO:

    return dwError;
}

