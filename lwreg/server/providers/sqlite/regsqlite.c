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
 *        regsqlite.c
 *
 * Abstract:
 *
 *        Wrapper functions for sqlite
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#include "includes.h"

DWORD
RegSqliteBindString(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN PCSTR pszValue
    )
{
    return sqlite3_bind_text(pstQuery, Index, pszValue,
                             -1, SQLITE_TRANSIENT);
}

DWORD
RegSqliteBindInt64(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN int64_t Value
    )
{
    return sqlite3_bind_int64(pstQuery, Index, (sqlite3_int64)Value);
}

DWORD
RegSqliteBindInt32(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN int Value
    )
{
    return sqlite3_bind_int(pstQuery, Index, Value);
}

DWORD
RegSqliteBindBoolean(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN BOOLEAN bValue
    )
{
    return sqlite3_bind_int(pstQuery, Index, bValue ? 1 : 0);
}

DWORD
RegSqliteReadInt64(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    int64_t *pqwResult)
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    //Do not free
    PSTR pszEndPtr = NULL;
    //Do not free
    PCSTR pszColumnValue = (PCSTR)sqlite3_column_text(pstQuery, *piColumnPos);

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        dwError = LWREG_ERROR_DATA_ERROR;
        BAIL_ON_REG_ERROR(dwError);
    }
#endif

    *pqwResult = strtoll(pszColumnValue, &pszEndPtr, 10);
    if (pszEndPtr == NULL || pszEndPtr == pszColumnValue || *pszEndPtr != '\0')
    {
        dwError = LWREG_ERROR_DATA_ERROR;
        BAIL_ON_REG_ERROR(dwError);
    }

    (*piColumnPos)++;

error:
    return dwError;
}

DWORD
RegSqliteReadString(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    PSTR *ppszResult)
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    //Do not free
    PCSTR pszColumnValue = (PCSTR)sqlite3_column_text(pstQuery, *piColumnPos);

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        dwError = LWREG_ERROR_DATA_ERROR;
        BAIL_ON_REG_ERROR(dwError);
    }
#endif

    dwError = RegStrDupOrNull(
            pszColumnValue,
            ppszResult);
    BAIL_ON_REG_ERROR(dwError);

    (*piColumnPos)++;

cleanup:
    return dwError;

error:
    *ppszResult = NULL;

    goto cleanup;
}

#if 0
DWORD
RegSqliteReadWcString(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    PWSTR *ppszResult
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    //Do not free
    PCWSTR pszColumnValue = (PCWSTR)sqlite3_column_text16(pstQuery, *piColumnPos);

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        dwError = LWREG_ERROR_DATA_ERROR;
        BAIL_ON_REG_ERROR(dwError);
    }
#endif

    dwError = LwWcStrDupOrNull(
            pszColumnValue,
            ppszResult);
    BAIL_ON_REG_ERROR(dwError);

    (*piColumnPos)++;

cleanup:
    return dwError;

error:
    *ppszResult = NULL;

    goto cleanup;
}
#endif

DWORD
RegSqliteReadTimeT(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    time_t *pResult)
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    uint64_t qwTemp;

    dwError = RegSqliteReadUInt64(
        pstQuery,
        piColumnPos,
        name,
        &qwTemp);
    BAIL_ON_REG_ERROR(dwError);

    *pResult = qwTemp;

error:
    return dwError;
}

DWORD
RegSqliteReadUInt64(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    uint64_t *pqwResult)
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    //Do not free
    PSTR pszEndPtr = NULL;
    //Do not free
    PCSTR pszColumnValue = (PCSTR)sqlite3_column_text(pstQuery, *piColumnPos);

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        dwError = LWREG_ERROR_DATA_ERROR;
        BAIL_ON_REG_ERROR(dwError);
    }
#endif

    BAIL_ON_INVALID_STRING(pszColumnValue);
    *pqwResult = strtoull(pszColumnValue, &pszEndPtr, 10);
    if (pszEndPtr == NULL || pszEndPtr == pszColumnValue || *pszEndPtr != '\0')
    {
        dwError = LWREG_ERROR_DATA_ERROR;
        BAIL_ON_REG_ERROR(dwError);
    }

    (*piColumnPos)++;

error:
    return dwError;
}

DWORD
RegSqliteReadUInt32(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    DWORD *pdwResult)
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    uint64_t qwTemp;
    int iColumnPos = *piColumnPos;

    dwError = RegSqliteReadUInt64(
        pstQuery,
        &iColumnPos,
        name,
        &qwTemp);
    BAIL_ON_REG_ERROR(dwError);

    if (qwTemp > UINT_MAX)
    {
        dwError = ERANGE;
        BAIL_ON_REG_ERROR(dwError);
    }

    *pdwResult = qwTemp;
    *piColumnPos = iColumnPos;

error:
    return dwError;
}

#if 0
DWORD
RegSqliteReadBoolean(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    BOOLEAN *pbResult)
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    DWORD dwTemp;

    dwError = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        name,
        &dwTemp);
    BAIL_ON_REG_ERROR(dwError);

    *pbResult = (dwTemp != 0);

error:
    return dwError;
}

DWORD
RegSqliteReadStringInPlace(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT PSTR pszResult,
    //Includes NULL
    IN size_t sMaxSize)
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    //Do not free
    PCSTR pszColumnValue = (PCSTR)sqlite3_column_text(pstQuery, *piColumnPos);
    size_t sRequiredSize = 0;

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        dwError = LWREG_ERROR_DATA_ERROR;
        BAIL_ON_REG_ERROR(dwError);
    }
#endif

    sRequiredSize = strlen(pszColumnValue) + 1;
    if (sRequiredSize > sMaxSize)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_REG_ERROR(dwError);
    }

    memcpy(pszResult, pszColumnValue, sRequiredSize);

    (*piColumnPos)++;

cleanup:
    return dwError;

error:
    pszResult[0] = '\0';

    goto cleanup;
}

DWORD
RegSqliteReadSid(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT PSID* ppSid)
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    PSTR pszSid = NULL;
    PSID pSid = NULL;
    int iColumnPos = *piColumnPos;

    dwError = RegSqliteReadString(
        pstQuery,
        &iColumnPos,
        name,
        &pszSid);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegAllocateSidFromCString(
            &pSid,
            pszSid);
    BAIL_ON_REG_ERROR(dwError);

    *ppSid = pSid;
    *piColumnPos = iColumnPos;

cleanup:
    LSA_SAFE_FREE_STRING(pszSid);

    return dwError;

error:

    *ppSid = NULL;
    LSA_SAFE_FREE_MEMORY(pSid);
    goto cleanup;
}

DWORD
RegSqliteReadGuid(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT uuid_t** ppGuid)
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    PSTR pszGuid = NULL;
    uuid_t *pGuid = NULL;
    int iColumnPos = *piColumnPos;

    dwError = RegSqliteReadString(
        pstQuery,
        &iColumnPos,
        name,
        &pszGuid);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(*pGuid),
                    (PVOID*)&pGuid);
    BAIL_ON_REG_ERROR(dwError);

    if (uuid_parse(
            pszGuid,
            *pGuid) < 0)
    {
        // uuid_parse returns -1 on error, but does not set errno
        dwError = LW_ERROR_INVALID_OBJECTGUID;
        BAIL_ON_REG_ERROR(dwError);
    }

    *ppGuid = pGuid;
    *piColumnPos = iColumnPos;

cleanup:
    LSA_SAFE_FREE_STRING(pszGuid);

    return dwError;

error:

    *ppGuid = NULL;
    LSA_SAFE_FREE_MEMORY(pGuid);
    goto cleanup;
}

DWORD
RegSqliteAllocPrintf(
    OUT PSTR* ppszSqlCommand,
    IN PCSTR pszSqlFormat,
    IN ...
    )
{
    DWORD dwError = 0;
    va_list args;

    va_start(args, pszSqlFormat);
    *ppszSqlCommand = sqlite3_vmprintf(pszSqlFormat, args);
    va_end(args);

    if (!*ppszSqlCommand)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
    }

    return dwError;
}
#endif


DWORD
RegSqliteExecCallbackWithRetry(
    IN sqlite3* pDb,
    IN pthread_rwlock_t* pLock,
    IN PFN_REG_SQLITE_EXEC_CALLBACK pfnCallback,
    IN PVOID pContext
    )
{
    PSTR pszError = NULL;
    DWORD dwError = LWREG_ERROR_SUCCESS;
    DWORD dwRetry;
    BOOLEAN bInLock = FALSE;

    ENTER_SQLITE_LOCK(pLock, bInLock);

    for (dwRetry = 0; dwRetry < 20; dwRetry++)
    {
        dwError = pfnCallback(pDb, pContext, &pszError);
        if (dwError == SQLITE_BUSY)
        {
            SQLITE3_SAFE_FREE_STRING(pszError);
            dwError = 0;
            // Rollback the half completed transaction
            RegSqliteExec(pDb, "ROLLBACK", NULL);

            REG_LOG_ERROR("There is a conflict trying to access the "
                          "cache database.  This would happen if another "
                          "process is trying to access it.  Retrying...");
        }
        else
        {
            BAIL_ON_SQLITE3_ERROR(dwError, pszError);
            break;
        }
    }
    BAIL_ON_SQLITE3_ERROR(dwError, pszError);

error:
    LEAVE_SQLITE_LOCK(pLock, bInLock);
    SQLITE3_SAFE_FREE_STRING(pszError);

    return dwError;
}

DWORD
RegSqliteBasicCallback(
    IN sqlite3 *pDb,
    IN PVOID pContext,
    OUT PSTR* ppszError
    )
{
    PCSTR pszTransaction = (PCSTR)pContext;

    return RegSqliteExec(pDb, pszTransaction, ppszError);
}

DWORD
RegSqliteExecWithRetry(
    IN sqlite3* pDb,
    IN pthread_rwlock_t* pLock,
    IN PCSTR pszTransaction
    )
{
    return RegSqliteExecCallbackWithRetry(
                pDb,
                pLock,
                RegSqliteBasicCallback,
                (PVOID)pszTransaction);
}

DWORD
RegSqliteExec(
    IN sqlite3* pSqlDatabase,
    IN PCSTR pszSqlCommand,
    OUT PSTR* ppszSqlError
    )
{
    return sqlite3_exec(pSqlDatabase, pszSqlCommand,
                        NULL, NULL, ppszSqlError);
}
