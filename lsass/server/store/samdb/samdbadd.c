
#include "includes.h"

static
DWORD
SamDbInsertObjectToDatabase(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszObjectDN,
    SAMDB_OBJECT_CLASS     objectClass,
    DIRECTORY_MOD          modifications[]
    );

static
DWORD
SamDbBuildAddObjectQuery(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSTR*                               ppszQuery
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
                    objectClass,
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
    SAMDB_OBJECT_CLASS     objectClass,
    DIRECTORY_MOD          modifications[]
    )
{
    DWORD dwError = 0;
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo = NULL;
    PSTR  pszQuery = NULL;
    DWORD iMap = 0;
    sqlite3_stmt* pSqlStatement = NULL;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bTxStarted = FALSE;

    dwError = SamDbFindObjectClassMapInfo(
                    objectClass,
                    pDirectoryContext->pObjectClassAttrMaps,
                    pDirectoryContext->dwNumObjectClassAttrMaps,
                    &pObjectClassMapInfo);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbBuildAddObjectQuery(
                    pDirectoryContext,
                    pObjectClassMapInfo,
                    modifications,
                    &pszQuery);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pDirectoryContext->rwLock);

    dwError = sqlite3_prepare_v2(
                    pDirectoryContext->pDbContext->pDbHandle,
                    pszQuery,
                    -1,
                    &pSqlStatement,
                    NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAM_DB_BEGIN_TRANSACTION(bTxStarted, pDirectoryContext);

    //
    // Iterate over the attributes defined for this kind of object
    //
    for (;pObjectClassMapInfo->dwNumMaps; iMap++)
    {
        PSAMDB_ATTRIBUTE_MAP_INFO pAttrMapInfo = NULL;
        PSAM_DB_ATTRIBUTE_MAP pAttrMap = NULL;

        pAttrMapInfo = &pObjectClassMapInfo->pAttributeMaps[iMap];

        dwError = SamDbAttributeLookupByName(
                        pDirectoryContext->pAttrLookup,
                        &pAttrMapInfo->wszAttributeName[0],
                        &pAttrMap);
        BAIL_ON_SAMDB_ERROR(dwError);

        // TODO:
        // Bind the incoming values to the sql statement
        // Generate any other values if specified in the attribute map

    }

    dwError = sqlite3_step(pSqlStatement);
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    SAM_DB_END_TRANSACTION(bTxStarted, dwError, pDirectoryContext);

    DIRECTORY_FREE_STRING(pszQuery);

    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &pDirectoryContext->rwLock);

    return dwError;

error:

    goto cleanup;
}

#define SAMDB_ADD_OBJECT_QUERY_ROWID     "rowid"
#define SAMDB_ADD_OBJECT_QUERY_SEPARATOR ","
#define SAMDB_ADD_OBJECT_QUERY_PREFIX    "INSERT INTO " SAM_DB_OBJECTS_TABLE "("
#define SAMDB_ADD_OBJECT_QUERY_MEDIAN    ") VALUES ("
#define SAMDB_ADD_OBJECT_QUERY_SUFFIX    ");"

static
DWORD
SamDbBuildAddObjectQuery(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSTR*                               ppszQuery
    )
{
    DWORD dwError = 0;
    PSTR  pszQuery = NULL;
    PSTR  pszQueryCursor = NULL;
    PSTR  pszQueryValuesCursor = NULL;
    DWORD dwQueryValuesOffset = 0;
    PSTR  pszCursor = 0;
    DWORD iMap = 0;
    DWORD iCol = 0;
    DWORD dwQueryLen = 0;
    DWORD dwColNamesLen = 0;
    DWORD dwColValuesLen = 0;
    CHAR  szBuf[32];

    //
    // We are building a query which will look like
    // INSERT INTO samdbobjects (col1,col2,col3) VALUES (?1,?2,?3);
    //
    for (;pObjectClassMapInfo->dwNumMaps; iMap++)
    {
        PSAMDB_ATTRIBUTE_MAP_INFO pAttrMapInfo = NULL;
        PSAM_DB_ATTRIBUTE_MAP pAttrMap = NULL;

        pAttrMapInfo = &pObjectClassMapInfo->pAttributeMaps[iMap];

        dwError = SamDbAttributeLookupByName(
                        pDirectoryContext->pAttrLookup,
                        &pAttrMapInfo->wszAttributeName[0],
                        &pAttrMap);
        BAIL_ON_SAMDB_ERROR(dwError);

        if (dwColNamesLen)
        {
            dwColNamesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_SEPARATOR) - 1;
        }

        dwColNamesLen += strlen(&pAttrMap->szDbColumnName[0]);

        if (dwColValuesLen)
        {
            dwColValuesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_SEPARATOR) - 1;
        }

        if (pAttrMap->bIsRowId)
        {
            dwColValuesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_ROWID) - 1;
        }
        else
        {
            sprintf(szBuf, "\?%d", ++iCol);

            dwColValuesLen += strlen(szBuf);
        }
    }

    dwQueryLen = sizeof(SAMDB_ADD_OBJECT_QUERY_PREFIX) - 1;
    dwQueryLen += dwColNamesLen;
    dwQueryLen += sizeof(SAMDB_ADD_OBJECT_QUERY_MEDIAN) - 1;
    dwQueryLen += dwColValuesLen;
    dwQueryLen += sizeof(SAMDB_ADD_OBJECT_QUERY_SUFFIX) - 1;
    dwQueryLen++;

    dwQueryValuesOffset = sizeof(SAMDB_ADD_OBJECT_QUERY_PREFIX) - 1;
    dwQueryValuesOffset += dwColNamesLen;
    dwQueryValuesOffset += sizeof(SAMDB_ADD_OBJECT_QUERY_MEDIAN) - 1;

    dwError = DirectoryAllocateMemory(
                    dwQueryLen,
                    (PVOID*)&pszQuery);
    BAIL_ON_SAMDB_ERROR(dwError);

    pszQueryCursor = pszQuery;
    pszQueryValuesCursor = pszQuery + dwQueryValuesOffset;
    iCol = 0;
    dwColNamesLen = 0;
    dwColValuesLen = 0;

    pszCursor = SAMDB_ADD_OBJECT_QUERY_PREFIX;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
    }

    for (iMap = 0;pObjectClassMapInfo->dwNumMaps; iMap++)
    {
        PSAMDB_ATTRIBUTE_MAP_INFO pAttrMapInfo = NULL;
        PSAM_DB_ATTRIBUTE_MAP pAttrMap = NULL;

        pAttrMapInfo = &pObjectClassMapInfo->pAttributeMaps[iMap];

        dwError = SamDbAttributeLookupByName(
                        pDirectoryContext->pAttrLookup,
                        &pAttrMapInfo->wszAttributeName[0],
                        &pAttrMap);
        BAIL_ON_SAMDB_ERROR(dwError);

        if (dwColNamesLen)
        {
            pszCursor = SAMDB_ADD_OBJECT_QUERY_SEPARATOR;
            while (pszCursor && *pszCursor)
            {
                *pszQueryCursor++ = *pszCursor++;
            }
            dwColNamesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_SEPARATOR)  - 1;
        }

        pszCursor = &pAttrMap->szDbColumnName[0];
        while (pszCursor && *pszCursor)
        {
            *pszQueryCursor++ = *pszCursor++;
            dwColNamesLen++;
        }

        if (dwColValuesLen)
        {
            pszCursor = SAMDB_ADD_OBJECT_QUERY_SEPARATOR;
            while (pszCursor && *pszCursor)
            {
                *pszQueryCursor++ = *pszCursor++;
            }
            dwColValuesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_SEPARATOR)  - 1;
        }

        if (pAttrMap->bIsRowId)
        {
            pszCursor = SAMDB_ADD_OBJECT_QUERY_ROWID;
            while (pszCursor && *pszCursor)
            {
                *pszQueryValuesCursor++ = *pszCursor;
            }
            dwColValuesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_ROWID) - 1;
        }
        else
        {
            sprintf(szBuf, "\?%d", ++iCol);

            pszCursor = &szBuf[0];
            while (pszCursor && *pszCursor)
            {
                *pszQueryValuesCursor++ = *pszCursor;
                dwColValuesLen++;
            }
        }
    }

    pszCursor = SAMDB_ADD_OBJECT_QUERY_SUFFIX;
    while (pszCursor && *pszCursor)
    {
        *pszQueryValuesCursor++ = *pszCursor++;
    }

    *ppszQuery = pszQuery;

cleanup:

    return dwError;

error:

    *ppszQuery = NULL;

    DIRECTORY_FREE_STRING(pszQuery);

    goto cleanup;
}
