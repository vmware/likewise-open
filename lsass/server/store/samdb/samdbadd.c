
#include "includes.h"

typedef DWORD (*PFN_SAMDB_ADD_VALUE_GENERATOR)(
                    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
                    PWSTR                  pwszDN,
                    PWSTR                  pwszParentDN,
                    PWSTR                  pwszObjectName,
                    PWSTR                  pwszDomainName,
                    PATTRIBUTE_VALUE*      ppAttrValues,
                    PDWORD                 pdwNumValues
                    );

typedef struct _SAMDB_ADD_VALUE_GENERATOR
{
    PSTR                          pszDbColName;
    PFN_SAMDB_ADD_VALUE_GENERATOR pfnValueGenerator;

} SAMDB_ADD_VALUE_GENERATOR, *PSAMDB_ADD_VALUE_GENERATOR;

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
    PSTR*                               ppszQuery,
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    );

static
DWORD
SamDbBuildAddColumnValueList(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    );

static
DWORD
SamDbAddGenerateValues(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PSAM_DB_DN             pDN,
    SAMDB_OBJECT_CLASS     objectClass,
    PSAM_DB_COLUMN_VALUE   pColumnValueList
    );

static
DWORD
SamDbAddGenerateUID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGenerateGID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGenerateObjectSID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGenerateParentDN(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGenerateDN(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGenerateDomain(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGeneratePrimaryGroup(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddConvertUnicodeAttrValues(
    PATTRIBUTE_VALUE  pSrcValues,
    DWORD             dwSrcNumValues,
    PATTRIBUTE_VALUE* ppAttrValues,
    PDWORD            pdwNumValues
    );

static
DWORD
SamDbAddBindValues(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    sqlite3_stmt*          pSqlStatement
    );

static
DWORD
SamDbFindDomainSID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDomainName,
    PSTR*                  ppszDomainSID
    );

static SAMDB_ADD_VALUE_GENERATOR gSamDbValueGenerators[] =
{
    {
        SAM_DB_COL_UID,
        &SamDbAddGenerateUID
    },
    {
        SAM_DB_COL_GID,
        &SamDbAddGenerateGID
    },
    {
        SAM_DB_COL_OBJECT_SID,
        &SamDbAddGenerateObjectSID
    },
    {
        SAM_DB_COL_PARENT_DN,
        &SamDbAddGenerateParentDN
    },
    {
        SAM_DB_COL_DISTINGUISHED_NAME,
        &SamDbAddGenerateDN
    },
    {
        SAM_DB_COL_DOMAIN,
        &SamDbAddGenerateDomain
    },
    {
        SAM_DB_COL_PRIMARY_GROUP,
        &SamDbAddGeneratePrimaryGroup
    }
};

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
    sqlite3_stmt* pSqlStatement = NULL;
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bTxStarted = FALSE;
    PSAM_DB_DN pDN = NULL;

    dwError = SamDbParseDN(
                pwszObjectDN,
                &pDN);
    BAIL_ON_SAMDB_ERROR(dwError);

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
                    &pszQuery,
                    &pColumnValueList);
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

    dwError = SamDbAddGenerateValues(
                    pDirectoryContext,
                    pDN,
                    objectClass,
                    pColumnValueList);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddBindValues(
                    pDirectoryContext,
                    pColumnValueList,
                    pSqlStatement);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_step(pSqlStatement);
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    SAM_DB_END_TRANSACTION(bTxStarted, dwError, pDirectoryContext);

    if (pColumnValueList)
    {
        SamDbFreeColumnValueList(pColumnValueList);
    }

    DIRECTORY_FREE_STRING(pszQuery);

    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &pDirectoryContext->rwLock);

    if (pDN)
    {
        SamDbFreeDN(pDN);
    }

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
    PSTR*                               ppszQuery,
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    )
{
    DWORD dwError = 0;
    PSTR  pszQuery = NULL;
    PSTR  pszQueryCursor = NULL;
    PSTR  pszQueryValuesCursor = NULL;
    DWORD dwQueryValuesOffset = 0;
    PSTR  pszCursor = 0;
    DWORD iCol = 0;
    DWORD dwQueryLen = 0;
    DWORD dwColNamesLen = 0;
    DWORD dwColValuesLen = 0;
    CHAR  szBuf[32];
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    PSAM_DB_COLUMN_VALUE pIter = NULL;

    dwError = SamDbBuildAddColumnValueList(
                    pDirectoryContext,
                    pObjectClassMapInfo,
                    modifications,
                    &pColumnValueList);
    BAIL_ON_SAMDB_ERROR(dwError);

    //
    // We are building a query which will look like
    // INSERT INTO samdbobjects (col1,col2,col3) VALUES (?1,?2,?3);
    //
    for (pIter = pColumnValueList; pIter; pIter = pIter->pNext)
    {
        if (pIter->pAttrMap->bIsRowId) continue;

        if (dwColNamesLen)
        {
            dwColNamesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_SEPARATOR) - 1;
        }

        dwColNamesLen += strlen(&pIter->pAttrMap->szDbColumnName[0]);

        if (dwColValuesLen)
        {
            dwColValuesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_SEPARATOR) - 1;
        }

        if (pIter->pAttrMap->bIsRowId)
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

    for (pIter = pColumnValueList; pIter; pIter = pIter->pNext)
    {
        if (pIter->pAttrMap->bIsRowId) continue;

        if (dwColNamesLen)
        {
            pszCursor = SAMDB_ADD_OBJECT_QUERY_SEPARATOR;
            while (pszCursor && *pszCursor)
            {
                *pszQueryCursor++ = *pszCursor++;
            }
            dwColNamesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_SEPARATOR)  - 1;
        }

        pszCursor = &pIter->pAttrMap->szDbColumnName[0];
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
                *pszQueryValuesCursor++ = *pszCursor++;
            }
            dwColValuesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_SEPARATOR)  - 1;
        }

        if (pIter->pAttrMap->bIsRowId)
        {
            pszCursor = SAMDB_ADD_OBJECT_QUERY_ROWID;
            while (pszCursor && *pszCursor)
            {
                *pszQueryValuesCursor++ = *pszCursor++;
            }
            dwColValuesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_ROWID) - 1;
        }
        else
        {
            sprintf(szBuf, "\?%d", ++iCol);

            pszCursor = &szBuf[0];
            while (pszCursor && *pszCursor)
            {
                *pszQueryValuesCursor++ = *pszCursor++;
                dwColValuesLen++;
            }
        }
    }

    pszCursor = SAMDB_ADD_OBJECT_QUERY_MEDIAN;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
    }

    pszCursor = SAMDB_ADD_OBJECT_QUERY_SUFFIX;
    while (pszCursor && *pszCursor)
    {
        *pszQueryValuesCursor++ = *pszCursor++;
    }

    *ppszQuery = pszQuery;
    *ppColumnValueList = pColumnValueList;

cleanup:

    return dwError;

error:

    *ppszQuery = NULL;

    DIRECTORY_FREE_STRING(pszQuery);

    if (pColumnValueList)
    {
        SamDbFreeColumnValueList(pColumnValueList);
    }

    goto cleanup;
}

static
DWORD
SamDbBuildAddColumnValueList(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    )
{
    DWORD dwError = 0;
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    PSAM_DB_COLUMN_VALUE pColumnValue = NULL;
    DWORD dwNumMods = 0;
    DWORD iMap = 0;

    //
    // Build column values for the attributes specified by the user
    //
    while (modifications[dwNumMods].pwszAttrName &&
           modifications[dwNumMods].pAttrValues)
    {
        dwError = DirectoryAllocateMemory(
                        sizeof(SAM_DB_COLUMN_VALUE),
                        (PVOID*)&pColumnValue);
        BAIL_ON_SAMDB_ERROR(dwError);

        pColumnValue->pDirMod = &modifications[dwNumMods];

        dwError = SamDbAttributeLookupByName(
                        pDirectoryContext->pAttrLookup,
                        pColumnValue->pDirMod->pwszAttrName,
                        &pColumnValue->pAttrMap);
        BAIL_ON_SAMDB_ERROR(dwError);

        for (iMap = 0; iMap < pObjectClassMapInfo->dwNumMaps; iMap++)
        {
            PSAMDB_ATTRIBUTE_MAP_INFO pMapInfo = NULL;

            pMapInfo = &pObjectClassMapInfo->pAttributeMaps[iMap];

            if (!wc16scasecmp(&pMapInfo->wszAttributeName[0],
                              modifications[dwNumMods].pwszAttrName))
            {
                pColumnValue->pAttrMapInfo = pMapInfo;

                break;
            }
        }

        if (!pColumnValue->pAttrMapInfo)
        {
            dwError = LSA_ERROR_NO_SUCH_ATTRIBUTE;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        pColumnValue->pNext = pColumnValueList;
        pColumnValueList = pColumnValue;
        pColumnValue = NULL;

        dwNumMods++;
    }

    //
    // Find attributes that are mandatory, are not specified by the user
    // and are set to be generated
    //
    for(iMap = 0; iMap < pObjectClassMapInfo->dwNumMaps; iMap++)
    {
        PSAMDB_ATTRIBUTE_MAP_INFO pMapInfo = NULL;

        pMapInfo = &pObjectClassMapInfo->pAttributeMaps[iMap];

        if (pMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_MANDATORY)
        {
            PSAM_DB_COLUMN_VALUE pIter = NULL;
            BOOLEAN bFound = FALSE;

            for (pIter = pColumnValueList;
                 !bFound && pIter;
                 pIter = pIter->pNext)
            {
                if (!wc16scasecmp(&pIter->pAttrMapInfo->wszAttributeName[0],
                                  &pMapInfo->wszAttributeName[0]))
                {
                    bFound = TRUE;
                }
            }

            if (!bFound)
            {
                PSAM_DB_ATTRIBUTE_MAP pAttrMap = NULL;

                dwError = SamDbAttributeLookupByName(
                                pDirectoryContext->pAttrLookup,
                                &pMapInfo->wszAttributeName[0],
                                &pAttrMap);
                BAIL_ON_SAMDB_ERROR(dwError);

                dwError = DirectoryAllocateMemory(
                                sizeof(SAM_DB_COLUMN_VALUE),
                                (PVOID*)&pColumnValue);
                BAIL_ON_SAMDB_ERROR(dwError);

                pColumnValue->pAttrMap = pAttrMap;
                pColumnValue->pAttrMapInfo = pMapInfo;

                pColumnValue->pNext = pColumnValueList;
                pColumnValueList = pColumnValue;
                pColumnValue = NULL;
            }
        }
    }

    *ppColumnValueList = SamDbReverseColumnValueList(pColumnValueList);

cleanup:

    return dwError;

error:

    *ppColumnValueList = NULL;

    if (pColumnValueList)
    {
        SamDbFreeColumnValueList(pColumnValueList);
    }
    if (pColumnValue)
    {
        SamDbFreeColumnValueList(pColumnValue);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGenerateValues(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PSAM_DB_DN             pDN,
    SAMDB_OBJECT_CLASS     objectClass,
    PSAM_DB_COLUMN_VALUE   pColumnValueList
    )
{
    DWORD dwError = 0;
    PSAM_DB_COLUMN_VALUE pIter = pColumnValueList;
    PWSTR pwszObjectName = NULL;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszParentDN = NULL;

    dwError = SamDbGetDNComponents(
                    pDN,
                    &pwszObjectName,
                    &pwszDomainName,
                    &pwszParentDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    for (; pIter; pIter = pIter->pNext)
    {
        if (!pIter->pDirMod &&
            pIter->pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_GENERATED_BY_DB)
        {
            continue;
        }

        if (!pIter->pDirMod &&
            ((pIter->pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_GENERATE_IF_NOT_SPECIFIED) ||
             (pIter->pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_GENERATE_ALWAYS) ||
             (pIter->pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_DERIVATIVE)))
        {
            PFN_SAMDB_ADD_VALUE_GENERATOR pfnValueGenerator = NULL;
            DWORD dwNumGenerators = sizeof(gSamDbValueGenerators)/sizeof(gSamDbValueGenerators[0]);
            DWORD iGen = 0;

            for (; !pfnValueGenerator && (iGen < dwNumGenerators); iGen++)
            {
                if (!strcasecmp(gSamDbValueGenerators[iGen].pszDbColName,
                                &pIter->pAttrMap->szDbColumnName[0]))
                {
                    pfnValueGenerator = gSamDbValueGenerators[iGen].pfnValueGenerator;
                }
            }

            if (!pfnValueGenerator)
            {
                dwError = LSA_ERROR_INTERNAL;
                BAIL_ON_SAMDB_ERROR(dwError);
            }

            dwError = pfnValueGenerator(
                            pDirectoryContext,
                            pDN->pwszDN,
                            pwszParentDN,
                            pwszObjectName,
                            pwszDomainName,
                            &pIter->pAttrValues,
                            &pIter->ulNumValues);
            BAIL_ON_SAMDB_ERROR(dwError);
        }
        else
        if (pIter->pDirMod && pIter->pDirMod->pAttrValues)
        {
            dwError = SamDbAddConvertUnicodeAttrValues(
                            pIter->pDirMod->pAttrValues,
                            pIter->pDirMod->ulNumValues,
                            &pIter->pAttrValues,
                            &pIter->ulNumValues);
            BAIL_ON_SAMDB_ERROR(dwError);
        }
        else
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_SAMDB_ERROR(dwError);
        }



    }

cleanup:

    DIRECTORY_FREE_MEMORY(pwszObjectName);
    DIRECTORY_FREE_MEMORY(pwszDomainName);
    DIRECTORY_FREE_MEMORY(pwszParentDN);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbAddGenerateUID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_INTEGER;

    dwError = SamDbGetNextAvailableUID(
                    pDirectoryContext,
                    &pAttrValue->data.ulValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGenerateGID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_INTEGER;

    dwError = SamDbGetNextAvailableGID(
                    pDirectoryContext,
                    &pAttrValue->data.ulValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGenerateObjectSID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    DWORD dwRID = 0;
    PSTR  pszDomainSID = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    if (!pwszDomainName || !*pwszDomainName)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbFindDomainSID(
                    pDirectoryContext,
                    pwszDomainName,
                    &pszDomainSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;

    dwError = SamDbGetNextAvailableRID(
                    pDirectoryContext,
                    &dwRID);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                    &pAttrValue->data.pszStringValue,
                    "%s-%u",
                    pszDomainSID,
                    dwRID);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    DIRECTORY_FREE_STRING(pszDomainSID);

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGenerateParentDN(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;

    if (pwszParentDN)
    {
        dwError = LsaWc16sToMbs(
                        pwszParentDN,
                        &pAttrValue->data.pszStringValue);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGenerateDN(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;

    if (pwszDN)
    {
        dwError = LsaWc16sToMbs(
                        pwszDN,
                        &pAttrValue->data.pszStringValue);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGenerateDomain(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;

    if (pwszDomainName)
    {
        dwError = LsaWc16sToMbs(
                        pwszDomainName,
                        &pAttrValue->data.pszStringValue);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *ppAttrValues  = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGeneratePrimaryGroup(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_INTEGER;
    pAttrValue->data.ulValue = DOMAIN_ALIAS_RID_USERS;

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddConvertUnicodeAttrValues(
    PATTRIBUTE_VALUE  pSrcValues,
    DWORD             dwSrcNumValues,
    PATTRIBUTE_VALUE* ppAttrValues,
    PDWORD            pdwNumValues
    )
{
    DWORD dwError = 0;
    DWORD iValue = 0;
    PATTRIBUTE_VALUE pTgtValues = NULL;
    DWORD dwNumValues = 0;

    dwError = DirectoryAllocateMemory(
                dwSrcNumValues * sizeof(ATTRIBUTE_VALUE),
                (PVOID*)&pTgtValues);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwNumValues = dwSrcNumValues;

    for (; iValue < dwNumValues; iValue++)
    {
        PATTRIBUTE_VALUE pSrcValue = NULL;
        PATTRIBUTE_VALUE pTgtValue = NULL;

        pSrcValue = &pSrcValues[iValue];
        pTgtValue = &pTgtValues[iValue];

        switch (pSrcValue->Type)
        {
            case DIRECTORY_ATTR_TYPE_ANSI_STRING:

                dwError = DirectoryAllocateString(
                                pSrcValue->data.pszStringValue,
                                &pTgtValue->data.pszStringValue);
                BAIL_ON_SAMDB_ERROR(dwError);

                pTgtValue->Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;

                break;

            case DIRECTORY_ATTR_TYPE_UNICODE_STRING:

                dwError = LsaWc16sToMbs(
                                pSrcValue->data.pwszStringValue,
                                &pTgtValue->data.pszStringValue);
                BAIL_ON_SAMDB_ERROR(dwError);

                pTgtValue->Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;

                break;


            case DIRECTORY_ATTR_TYPE_INTEGER:

                pTgtValue->Type = DIRECTORY_ATTR_TYPE_INTEGER;

                pTgtValue->data.ulValue = pSrcValue->data.ulValue;

                break;

            case DIRECTORY_ATTR_TYPE_BOOLEAN:

                pTgtValue->Type = DIRECTORY_ATTR_TYPE_BOOLEAN;

                pTgtValue->data.ulValue = pSrcValue->data.ulValue;

                break;

            case DIRECTORY_ATTR_TYPE_LARGE_INTEGER:

                pTgtValue->Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER;

                pTgtValue->data.llValue = pSrcValue->data.llValue;

                break;

            case DIRECTORY_ATTR_TYPE_OCTET_STREAM:

                dwError = DirectoryAllocateMemory(
                            sizeof(OCTET_STRING),
                            (PVOID*)&pTgtValue->data.pOctetString);
                BAIL_ON_SAMDB_ERROR(dwError);

                if (pSrcValue->data.pOctetString->ulNumBytes)
                {
                    dwError = DirectoryAllocateMemory(
                                    pSrcValue->data.pOctetString->ulNumBytes,
                                    (PVOID*)&pTgtValue->data.pOctetString->pBytes);
                    BAIL_ON_SAMDB_ERROR(dwError);

                    pTgtValue->data.pOctetString->ulNumBytes = pSrcValue->data.pOctetString->ulNumBytes;

                    memcpy( pTgtValue->data.pOctetString->pBytes,
                            pSrcValue->data.pOctetString->pBytes,
                            pSrcValue->data.pOctetString->ulNumBytes);
                }

                break;

            default:

                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);

        }
    }

    *ppAttrValues = pTgtValues;
    *pdwNumValues = dwNumValues;

cleanup:

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pTgtValues)
    {
        DirectoryFreeAttributeValues(pTgtValues, dwNumValues);
    }

    goto cleanup;
}

static
DWORD
SamDbAddBindValues(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    sqlite3_stmt*          pSqlStatement
    )
{
    DWORD dwError = 0;
    PSAM_DB_COLUMN_VALUE pIter = pColumnValueList;
    DWORD iParam = 0;

    for (; pIter; pIter = pIter->pNext)
    {
        if (pIter->pAttrValues)
        {
            if (pIter->ulNumValues > 1)
            {
                dwError = LSA_ERROR_INVALID_PARAMETER;
            }
        }
        else if (pIter->pDirMod)
        {
            if (pIter->ulNumValues > 1)
            {
                dwError = LSA_ERROR_INVALID_PARAMETER;
            }
        }
        BAIL_ON_SAMDB_ERROR(dwError);

        switch (pIter->pAttrMap->attributeType)
        {
            case SAMDB_ATTR_TYPE_TEXT:
            {
                PSTR pszValue = NULL;

                if (pIter->pAttrValues)
                {
                    pszValue = pIter->pAttrValues[0].data.pszStringValue;
                }
                else
                if (pIter->pDirMod)
                {
                    pszValue = pIter->pDirMod->pAttrValues[0].data.pszStringValue;
                }

                if (pszValue)
                {
                    dwError = sqlite3_bind_text(
                                    pSqlStatement,
                                    ++iParam,
                                    pszValue,
                                    -1,
                                    SQLITE_TRANSIENT);
                }
                else
                {
                    dwError = sqlite3_bind_null(pSqlStatement, ++iParam);
                }
                BAIL_ON_SAMDB_ERROR(dwError);

                break;
            }

            case SAMDB_ATTR_TYPE_INT32:
            case SAMDB_ATTR_TYPE_BOOLEAN:
            case SAMDB_ATTR_TYPE_DATETIME:

                if (pIter->pAttrValues)
                {
                    dwError = sqlite3_bind_int(
                                    pSqlStatement,
                                    ++iParam,
                                    pIter->pAttrValues[0].data.ulValue);
                }
                else
                if (pIter->pDirMod)
                {
                    dwError = sqlite3_bind_int(
                                    pSqlStatement,
                                    ++iParam,
                                    pIter->pDirMod->pAttrValues[0].data.ulValue);
                }
                BAIL_ON_SAMDB_ERROR(dwError);

                break;

            case SAMDB_ATTR_TYPE_INT64:

                if (pIter->pAttrValues)
                {
                    dwError = sqlite3_bind_int(
                                    pSqlStatement,
                                    ++iParam,
                                    pIter->pAttrValues[0].data.llValue);
                }
                else
                if (pIter->pDirMod)
                {
                    dwError = sqlite3_bind_int(
                                    pSqlStatement,
                                    ++iParam,
                                    pIter->pDirMod->pAttrValues[0].data.llValue);
                }
                BAIL_ON_SAMDB_ERROR(dwError);

                break;

            case SAMDB_ATTR_TYPE_BLOB:
            {
                POCTET_STRING pOctetString = NULL;

                if (pIter->pAttrValues)
                {
                    pOctetString = pIter->pAttrValues[0].data.pOctetString;
                }
                else
                if (pIter->pDirMod)
                {
                    pOctetString = pIter->pDirMod->pAttrValues[0].data.pOctetString;
                }

                if (pOctetString)
                {
                    dwError = sqlite3_bind_blob(
                                    pSqlStatement,
                                    ++iParam,
                                    pOctetString->pBytes,
                                    pOctetString->ulNumBytes,
                                    SQLITE_TRANSIENT);
                }
                else
                {
                    dwError = sqlite3_bind_null(pSqlStatement, ++iParam);
                }
                BAIL_ON_SAMDB_ERROR(dwError);

                break;
            }

            default:

                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);
        }
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbFindDomainSID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDomainName,
    PSTR*                  ppszDomainSID
    )
{
    DWORD dwError = 0;
    wchar16_t wszSID[] = SAM_DB_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttributes[] =
                {
                    &wszSID[0],
                    NULL
                };
    PDIRECTORY_ENTRY pDirEntries = NULL;
    DWORD dwNumEntries = 0;
    PSTR  pszFilter    = NULL;
    PWSTR pwszFilter   = NULL;
    PSTR  pszDomainSID = NULL;
    PSTR  pszDomainName = NULL;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_DOMAIN;
    PCSTR pszQueryClause = " WHERE " SAM_DB_COL_DOMAIN       " = \"%s\"" \
                           "   AND " SAM_DB_COL_OBJECT_CLASS " = %d;";

    dwError = LsaWc16sToMbs(
                    pwszDomainName,
                    &pszDomainName);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszQueryClause,
                    pszDomainName,
                    objectClass);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszFilter,
                    &pwszFilter);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbSearchObject_inlock(
                    pDirectoryContext,
                    NULL,
                    0,
                    pwszFilter,
                    wszAttributes,
                    FALSE,
                    &pDirEntries,
                    &dwNumEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (!dwNumEntries)
    {
        dwError = LSA_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_SAMDB_ERROR(dwError);
    }
    else if (dwNumEntries != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    if (!pDirEntries[0].ulNumAttributes ||
        !pDirEntries[0].pAttributes[0].ulNumValues ||
        pDirEntries[0].pAttributes[0].pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = LsaWc16sToMbs(
                    pDirEntries[0].pAttributes[0].pValues[0].data.pwszStringValue,
                    &pszDomainSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppszDomainSID = pszDomainSID;

cleanup:

    DIRECTORY_FREE_STRING(pszDomainName);
    DIRECTORY_FREE_STRING(pszFilter);
    DIRECTORY_FREE_MEMORY(pwszFilter);

    if (pDirEntries)
    {
        DirectoryFreeEntries(pDirEntries, dwNumEntries);
    }

    return dwError;

error:

    *ppszDomainSID = NULL;

    goto cleanup;
}
