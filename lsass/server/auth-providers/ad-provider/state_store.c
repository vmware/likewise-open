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
 *        state_store.c
 *
 * Abstract:
 *
 *        Caching for AD Provider Database Interface
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */

#include "adprovider.h"
#include "state_store_p.h"

static
DWORD
ADState_Setup(
    IN sqlite3* pSqlHandle
    )
{
    DWORD dwError = 0;
    PSTR pszError = NULL;

    dwError = LsaSqliteExec(pSqlHandle,
                                AD_STATE_CREATE_TABLES,
                                &pszError);
    if (dwError)
    {
        LSA_LOG_DEBUG("SQL failed: code = %d, message = '%s'\nSQL =\n%s",
                      dwError, pszError, AD_STATE_CREATE_TABLES);
    }
    BAIL_ON_SQLITE3_ERROR(dwError, pszError);

cleanup:
    SQLITE3_SAFE_FREE_STRING(pszError);
    return dwError;

error:
    goto cleanup;
}

DWORD
ADState_OpenDb(
    ADSTATE_CONNECTION_HANDLE* phDb
    )
{
    DWORD dwError = 0;
    BOOLEAN bLockCreated = FALSE;
    PADSTATE_CONNECTION pConn = NULL;
    BOOLEAN bExists = FALSE;

    dwError = LsaAllocateMemory(
                    sizeof(ADSTATE_CONNECTION),
                    (PVOID*)&pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pthread_rwlock_init(&pConn->lock, NULL);
    BAIL_ON_LSA_ERROR(dwError);
    bLockCreated = TRUE;

    dwError = LsaCheckDirectoryExists(LSASS_DB_DIR, &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        dwError = LsaCreateDirectory(LSASS_DB_DIR, cacheDirMode);
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* restrict access to u+rwx to the db folder */
    dwError = LsaChangeOwnerAndPermissions(LSASS_DB_DIR, 0, 0, S_IRWXU);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_open(ADSTATE_DB, &pConn->pDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaChangeOwnerAndPermissions(ADSTATE_DB, 0, 0, S_IRWXU);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_Setup(pConn->pDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            "lwiproviderdata.DirectoryMode, "
            "lwiproviderdata.ADConfigurationMode, "
            "lwiproviderdata.ADMaxPwdAge, "
            "lwiproviderdata.Domain, "
            "lwiproviderdata.ShortDomain, "
            "lwiproviderdata.ComputerDN, "
            "lwiproviderdata.CellDN "
            "from lwiproviderdata ",
            -1, //search for null termination in szQuery to get length
            &pConn->pstGetProviderData,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            "lwidomaintrusts.RowIndex, "
            "lwidomaintrusts.DnsDomainName, "
            "lwidomaintrusts.NetbiosDomainName, "
            "lwidomaintrusts.Sid, "
            "lwidomaintrusts.Guid, "
            "lwidomaintrusts.TrusteeDnsDomainName, "
            "lwidomaintrusts.TrustFlags, "
            "lwidomaintrusts.TrustType, "
            "lwidomaintrusts.TrustAttributes, "
            "lwidomaintrusts.TrustDirection, "
            "lwidomaintrusts.TrustMode, "
            "lwidomaintrusts.ForestName, "
            "lwidomaintrusts.Flags "
            "from lwidomaintrusts ORDER BY RowIndex ASC",
            -1, //search for null termination in szQuery to get length
            &pConn->pstGetDomainTrustList,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            "lwilinkedcells.RowIndex, "
            "lwilinkedcells.CellDN, "
            "lwilinkedcells.Domain, "
            "lwilinkedcells.IsForestCell "
            "from lwilinkedcells ORDER BY RowIndex ASC",
            -1, //search for null termination in szQuery to get length
            &pConn->pstGetCellList,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    *phDb = pConn;

cleanup:

    return dwError;

error:
    if (pConn != NULL)
    {
        if (bLockCreated)
        {
            pthread_rwlock_destroy(&pConn->lock);
        }
        ADState_FreePreparedStatements(pConn);

        if (pConn->pDb != NULL)
        {
            sqlite3_close(pConn->pDb);
        }
        LSA_SAFE_FREE_MEMORY(pConn);
    }
    *phDb = NULL;

    goto cleanup;
}

static
DWORD
ADState_FreePreparedStatements(
    IN OUT PADSTATE_CONNECTION pConn
    )
{
    int i;
    DWORD dwError = LSA_ERROR_SUCCESS;
    sqlite3_stmt * * const pppstFreeList[] = {
        &pConn->pstGetProviderData,
        &pConn->pstGetDomainTrustList,
        &pConn->pstGetCellList,
    };

    for (i = 0; i < sizeof(pppstFreeList)/sizeof(pppstFreeList[0]); i++)
    {
        if (*pppstFreeList[i] != NULL)
        {
            dwError = sqlite3_finalize(*pppstFreeList[i]);
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
            *pppstFreeList[i] = NULL;
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

void
ADState_SafeCloseDb(
    ADSTATE_CONNECTION_HANDLE* phDb
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    ADSTATE_CONNECTION_HANDLE hDb = NULL;

    if (phDb == NULL || *phDb == NULL)
    {
        goto cleanup;
    }

    hDb = *phDb;

    dwError = ADState_FreePreparedStatements(hDb);
    if (dwError != LSA_ERROR_SUCCESS)
    {
        LSA_LOG_ERROR("Error freeing prepared statements [%d]", dwError);
        dwError = LSA_ERROR_SUCCESS;
    }

    if (hDb->pDb != NULL)
    {
        sqlite3_close(hDb->pDb);
        hDb->pDb = NULL;
    }

    dwError = pthread_rwlock_destroy(&hDb->lock);
    if (dwError != LSA_ERROR_SUCCESS)
    {
        LSA_LOG_ERROR("Error destroying lock [%d]", dwError);
        dwError = LSA_ERROR_SUCCESS;
    }
    LSA_SAFE_FREE_MEMORY(hDb);

cleanup:
    return;
}

DWORD
ADState_GetProviderData(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    OUT PAD_PROVIDER_DATA* ppResult
    )
{
    DWORD dwError = 0;
    PADSTATE_CONNECTION pConn = (PADSTATE_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    const int nExpectedCols = 7;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PAD_PROVIDER_DATA pResult = NULL;
    DWORD dwTemp = 0;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    pstQuery = pConn->pstGetProviderData;

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        // No results found
        dwError = ENOENT;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (dwError == SQLITE_ROW)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    else
    {
        BAIL_ON_SQLITE3_ERROR(dwError,
                sqlite3_errmsg(sqlite3_db_handle(pstQuery)));
    }

    nGotColumns = sqlite3_column_count(pstQuery);
    if (nGotColumns != nExpectedCols)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(AD_PROVIDER_DATA),
                    (PVOID*)&pResult);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadUInt32(
        pstQuery,
        &iColumnPos,
        "DirectoryMode",
        &pResult->dwDirectoryMode);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadUInt32(
        pstQuery,
        &iColumnPos,
        "ADConfigurationMode",
        &dwTemp);
    BAIL_ON_LSA_ERROR(dwError);
    pResult->adConfigurationMode = dwTemp;

    dwError = LsaSqliteReadUInt64(
        pstQuery,
        &iColumnPos,
        "ADMaxPwdAge",
        &pResult->adMaxPwdAge);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadStringInPlace(
        pstQuery,
        &iColumnPos,
        "Domain",
        pResult->szDomain,
        sizeof(pResult->szDomain));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadStringInPlace(
        pstQuery,
        &iColumnPos,
        "ShortDomain",
        pResult->szShortDomain,
        sizeof(pResult->szShortDomain));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadStringInPlace(
        pstQuery,
        &iColumnPos,
        "ComputerDN",
        pResult->szComputerDN,
        sizeof(pResult->szComputerDN));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadStringInPlace(
        pstQuery,
        &iColumnPos,
        "CellDN",
        pResult->cell.szCellDN,
        sizeof(pResult->cell.szCellDN));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_ROW)
    {
        dwError = LSA_ERROR_DUPLICATE_DOMAINNAME;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    else
    {
        BAIL_ON_SQLITE3_ERROR(dwError,
                sqlite3_errmsg(sqlite3_db_handle(pstQuery)));
    }
    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR(dwError,
            sqlite3_errmsg(sqlite3_db_handle(pstQuery)));

    dwError = ADState_GetCellListNoLock(
        hDb,
        &pResult->pCellList
        );
    BAIL_ON_LSA_ERROR(dwError);

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    *ppResult = pResult;
    
cleanup:
    
    return dwError;

error:
    *ppResult = NULL;
    sqlite3_reset(pstQuery);
    if (pResult)
    {
        ADProviderFreeProviderData(pResult);
    }
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    goto cleanup;
}

DWORD
ADState_StoreProviderData(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    IN PAD_PROVIDER_DATA pProvider
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszSqlCommand = NULL;
    PSTR pszCellCommand = NULL;
    PADSTATE_CONNECTION pConn = (PADSTATE_CONNECTION)hDb;

    dwError = ADState_GetCacheCellListCommand(
                hDb,
                pProvider->pCellList,
                &pszCellCommand);
    BAIL_ON_LSA_ERROR(dwError);

    pszSqlCommand = sqlite3_mprintf(
        "begin;"
            "replace into lwiproviderdata ("
                "DirectoryMode, "
                "ADConfigurationMode, "
                "ADMaxPwdAge, "
                "Domain, "
                "ShortDomain, "
                "ComputerDN, "
                "CellDN "
            ") values ("
                "%d,"
                "%d,"
                "%lld,"
                "%Q,"
                "%Q,"
                "%Q,"
                "%Q"
            ");\n"
            "%s"
        "end;",
        pProvider->dwDirectoryMode,
        pProvider->adConfigurationMode,
        pProvider->adMaxPwdAge,
        pProvider->szDomain,
        pProvider->szShortDomain,
        pProvider->szComputerDN,
        pProvider->cell.szCellDN,
        pszCellCommand
    );

    if (pszSqlCommand == NULL)
    {
        dwError = (DWORD)sqlite3_errcode(pConn->pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
    }

    dwError = LsaSqliteExecWithRetry(
        pConn->pDb,
        &pConn->lock,
        pszSqlCommand);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    SQLITE3_SAFE_FREE_STRING(pszCellCommand);
    SQLITE3_SAFE_FREE_STRING(pszSqlCommand);
    return dwError;

error:

    goto cleanup;
}

DWORD
ADState_GetDomainTrustList(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    OUT PDLINKEDLIST* ppList
    )
{
    DWORD dwError = 0;
    PADSTATE_CONNECTION pConn = (PADSTATE_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    const int nExpectedCols = 13;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PDLINKEDLIST pList = NULL;
    PLSA_DM_ENUM_DOMAIN_INFO pEntry = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    pstQuery = pConn->pstGetDomainTrustList;

    while (TRUE)
    {
        dwError = (DWORD)sqlite3_step(pstQuery);
        if (dwError == SQLITE_DONE)
        {
            // No more results
            dwError = LSA_ERROR_SUCCESS;
            break;
        }
        if (dwError == SQLITE_ROW)
        {
            dwError = LSA_ERROR_SUCCESS;
        }
        else
        {
            BAIL_ON_SQLITE3_ERROR(dwError,
                    sqlite3_errmsg(sqlite3_db_handle(pstQuery)));
        }

        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateMemory(
                        sizeof(*pEntry),
                        (PVOID*)&pEntry);
        BAIL_ON_LSA_ERROR(dwError);

        iColumnPos = 0;
        dwError = ADState_UnpackDomainTrust(
                    pstQuery,
                    &iColumnPos,
                    pEntry);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDLinkedListAppend(
                    &pList,
                    pEntry);
        BAIL_ON_LSA_ERROR(dwError);
        pEntry = NULL;
    }

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR(dwError,
            sqlite3_errmsg(sqlite3_db_handle(pstQuery)));
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    *ppList = pList;
    
cleanup:
    
    return dwError;

error:
    *ppList = NULL;
    sqlite3_reset(pstQuery);
    if (pList)
    {
        ADState_FreeEnumDomainInfoList(pList);
    }
    if (pEntry)
    {
        ADState_FreeEnumDomainInfo(pEntry);
    }
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    goto cleanup;
}

DWORD
ADState_StoreDomainTrustList(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    IN PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo,
    IN DWORD dwDomainInfoCount
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszOldExpression = NULL;
    PSTR pszSqlCommand = NULL;
    PADSTATE_CONNECTION pConn = (PADSTATE_CONNECTION)hDb;
    DWORD dwIndex = 0;
    const LSA_DM_ENUM_DOMAIN_INFO* pDomain = NULL;
    char szGuid[UUID_STR_SIZE];
    PSTR pszSid = NULL;
    PWSTR pwszSid = NULL;

    pszSqlCommand = sqlite3_mprintf(
        "begin;\n"
            "delete from lwidomaintrusts;\n");
    if (pszSqlCommand == NULL)
    {
        dwError = (DWORD)sqlite3_errcode(pConn->pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
    }

    for (dwIndex = 0; dwIndex < dwDomainInfoCount; dwIndex++)
    {
        pDomain = ppDomainInfo[dwIndex];
        
        LSA_SAFE_FREE_MEMORY(pwszSid);
        LSA_SAFE_FREE_STRING(pszSid);

        if (pDomain->pSid != NULL)
        {
            dwError = SidToString(
                    pDomain->pSid,
                    &pwszSid);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaWc16sToMbs(
                    pwszSid,
                    &pszSid);
            BAIL_ON_LSA_ERROR(dwError);
        }

        // Writes into a 37-byte caller allocated string
        uuid_unparse(*pDomain->pGuid, szGuid);

        SQLITE3_SAFE_FREE_STRING(pszOldExpression);
        pszOldExpression = pszSqlCommand;
        pszSqlCommand = sqlite3_mprintf(
            "%s"
            "replace into lwidomaintrusts ("
                "RowIndex, "
                "DnsDomainName, "
                "NetbiosDomainName, "
                "Sid, "
                "Guid, "
                "TrusteeDnsDomainName, "
                "TrustFlags, "
                "TrustType, "
                "TrustAttributes, "
                "TrustDirection, "
                "TrustMode, "
                "ForestName, "
                "Flags "
            ") values ("
                "%lu, "
                "%Q, "
                "%Q, "
                "%Q, "
                "%Q, "
                "%Q, "
                "%d, "
                "%d, "
                "%d, "
                "%d, "
                "%d, "
                "%Q, "
                "%d "
            ");\n",
            pszOldExpression,
            dwIndex,
            pDomain->pszDnsDomainName,
            pDomain->pszNetbiosDomainName,
            pszSid,
            szGuid,
            pDomain->pszTrusteeDnsDomainName,
            pDomain->dwTrustFlags,
            pDomain->dwTrustType,
            pDomain->dwTrustAttributes,
            pDomain->dwTrustDirection,
            pDomain->dwTrustMode,
            pDomain->pszForestName,
            pDomain->pszClientSiteName,
            pDomain->Flags);

        if (pszSqlCommand == NULL)
        {
            dwError = (DWORD)sqlite3_errcode(pConn->pDb);
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
        }
    }

    SQLITE3_SAFE_FREE_STRING(pszOldExpression);
    pszOldExpression = pszSqlCommand;
    pszSqlCommand = sqlite3_mprintf(
            "%s"
        "end;",
        pszOldExpression
        );
    if (pszSqlCommand == NULL)
    {
        dwError = (DWORD)sqlite3_errcode(pConn->pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
    }

    dwError = LsaSqliteExecWithRetry(
        pConn->pDb,
        &pConn->lock,
        pszSqlCommand);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_MEMORY(pwszSid);
    LSA_SAFE_FREE_STRING(pszSid);
    SQLITE3_SAFE_FREE_STRING(pszOldExpression);
    SQLITE3_SAFE_FREE_STRING(pszSqlCommand);
    return dwError;

error:

    goto cleanup;
}

static
DWORD
ADState_GetCellListNoLock(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    // Contains type PAD_LINKED_CELL_INFO
    IN OUT PDLINKEDLIST* ppList
    )
{
    DWORD dwError = 0;
    PADSTATE_CONNECTION pConn = (PADSTATE_CONNECTION)hDb;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    const int nExpectedCols = 4;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PDLINKEDLIST pList = NULL;
    PAD_LINKED_CELL_INFO pEntry = NULL;

    pstQuery = pConn->pstGetCellList;

    while (TRUE)
    {
        dwError = (DWORD)sqlite3_step(pstQuery);
        if (dwError == SQLITE_DONE)
        {
            // No more results
            dwError = LSA_ERROR_SUCCESS;
            break;
        }
        if (dwError == SQLITE_ROW)
        {
            dwError = LSA_ERROR_SUCCESS;
        }
        else
        {
            BAIL_ON_SQLITE3_ERROR(dwError,
                    sqlite3_errmsg(sqlite3_db_handle(pstQuery)));
        }

        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateMemory(
                        sizeof(*pEntry),
                        (PVOID*)&pEntry);
        BAIL_ON_LSA_ERROR(dwError);

        iColumnPos = 0;
        dwError = ADState_UnpackLinkedCellInfo(
                    pstQuery,
                    &iColumnPos,
                    pEntry);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDLinkedListAppend(
                    &pList,
                    pEntry);
        BAIL_ON_LSA_ERROR(dwError);
        pEntry = NULL;
    }

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR(dwError,
            sqlite3_errmsg(sqlite3_db_handle(pstQuery)));

    *ppList = pList;
    
cleanup:
    
    return dwError;

error:
    *ppList = NULL;
    sqlite3_reset(pstQuery);
    if (pList)
    {
        ADProviderFreeCellList(pList);
    }
    if (pEntry)
    {
        ADProviderFreeCellInfo(pEntry);
    }
    goto cleanup;
}

static
DWORD
ADState_GetCacheCellListCommand(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    // Contains type PAD_LINKED_CELL_INFO
    IN const DLINKEDLIST* pCellList,
    OUT PSTR* ppszCommand
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszOldExpression = NULL;
    PSTR pszSqlCommand = NULL;
    PADSTATE_CONNECTION pConn = (PADSTATE_CONNECTION)hDb;
    const DLINKEDLIST* pPos = pCellList;
    size_t sIndex = 0;
    const AD_LINKED_CELL_INFO* pCell = NULL;

    pszSqlCommand = sqlite3_mprintf(
        "delete from lwilinkedcells;\n");
    if (pszSqlCommand == NULL)
    {
        dwError = (DWORD)sqlite3_errcode(pConn->pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
    }

    while (pPos != NULL)
    {
        pCell = (const PAD_LINKED_CELL_INFO)pPos->pItem;

        SQLITE3_SAFE_FREE_STRING(pszOldExpression);
        pszOldExpression = pszSqlCommand;
        pszSqlCommand = sqlite3_mprintf(
            "%s"
            "replace into lwilinkedcells ("
                "RowIndex, "
                "CellDN, "
                "Domain, "
                "IsForestCell "
            ") values ("
                "%lu, "
                "%Q, "
                "%Q, "
                "%d "
            ");\n",
            pszOldExpression,
            sIndex++,
            pCell->pszCellDN,
            pCell->pszDomain,
            pCell->bIsForestCell
            );

        if (pszSqlCommand == NULL)
        {
            dwError = (DWORD)sqlite3_errcode(pConn->pDb);
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
        }

        pPos = pPos->pNext;
    }

    *ppszCommand = pszSqlCommand;

cleanup:

    SQLITE3_SAFE_FREE_STRING(pszOldExpression);
    return dwError;

error:

    *ppszCommand = NULL;
    SQLITE3_SAFE_FREE_STRING(pszSqlCommand);
    goto cleanup;
}

VOID
ADState_FreeEnumDomainInfoList(
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    IN OUT PDLINKEDLIST pList
    )
{
    LsaDLinkedListForEach(
        pList,
        ADState_FreeEnumDomainInfoCallback,
        NULL);

    LsaDLinkedListFree(pList);
}

static
VOID
ADState_FreeEnumDomainInfoCallback(
    IN OUT PVOID pData,
    IN PVOID pUserData
    )
{
    PLSA_DM_ENUM_DOMAIN_INFO pInfo =
        (PLSA_DM_ENUM_DOMAIN_INFO)pData;

    ADState_FreeEnumDomainInfo(pInfo);
}

static
VOID
ADState_FreeEnumDomainInfo(
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo
    )
{
    if (pDomainInfo)
    {
        LSA_SAFE_FREE_STRING(pDomainInfo->pszDnsDomainName);
        LSA_SAFE_FREE_STRING(pDomainInfo->pszNetbiosDomainName);
        LSA_SAFE_FREE_MEMORY(pDomainInfo->pSid);
        LSA_SAFE_FREE_MEMORY(pDomainInfo->pGuid);
        LSA_SAFE_FREE_STRING(pDomainInfo->pszTrusteeDnsDomainName);
        LSA_SAFE_FREE_STRING(pDomainInfo->pszForestName);
        LSA_SAFE_FREE_STRING(pDomainInfo->pszClientSiteName);
        if (pDomainInfo->DcInfo)
        {
            // ISSUE-2008/09/10-dalmeida -- need ASSERT macro
            LSA_LOG_ALWAYS("ASSERT!!! - DcInfo should never be set by DB code!");
        }
        if (pDomainInfo->GcInfo)
        {
            // ISSUE-2008/09/10-dalmeida -- need ASSERT macro
            LSA_LOG_ALWAYS("ASSERT!!! - GcInfo should never be set by DB code!");
        }
        LsaFreeMemory(pDomainInfo);
    }
}

static
DWORD
ADState_UnpackDomainTrust(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO pResult
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    //Skip the index
    (*piColumnPos)++;

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "DnsDomainName",
        (PSTR*)&pResult->pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "NetbiosDomainName",
        (PSTR*)&pResult->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadSid(
        pstQuery,
        piColumnPos,
        "Sid",
        &pResult->pSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadGuid(
        pstQuery,
        piColumnPos,
        "Guid",
        &pResult->pGuid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "TrusteeDnsDomainName",
        (PSTR*)&pResult->pszTrusteeDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "TrustFlags",
        &pResult->dwTrustFlags);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "TrustType",
        &pResult->dwTrustType);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "TrustAttributes",
        &pResult->dwTrustAttributes);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "TrustDirection",
        &pResult->dwTrustDirection);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "TrustMode",
        &pResult->dwTrustMode);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "ForestName",
        (PSTR*)&pResult->pszForestName);
    BAIL_ON_LSA_ERROR(dwError);

    pResult->pszClientSiteName = NULL;

    dwError = LsaSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "Flags",
        &pResult->Flags);
    BAIL_ON_LSA_ERROR(dwError);

    pResult->DcInfo = NULL;
    pResult->GcInfo = NULL;

error:
    return dwError;
}

static
DWORD
ADState_UnpackLinkedCellInfo(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN OUT PAD_LINKED_CELL_INFO pResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    //Skip the index
    (*piColumnPos)++;

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "CellDN",
        (PSTR*)&pResult->pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "Domain",
        (PSTR*)&pResult->pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadBoolean(
        pstQuery,
        piColumnPos,
        "IsForestCell",
        &pResult->bIsForestCell);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}
