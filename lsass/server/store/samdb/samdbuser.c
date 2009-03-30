/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        samdbuser.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      SamDbInitUserTable
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#include "includes.h"

DWORD
SamDbSetPassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    )
{
    DWORD dwError = 0;

    // TODO

    return dwError;
}

DWORD
SamDbChangePassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszOldPassword,
    PWSTR  pwszNewPassword
    )
{
    DWORD dwError = 0;

    // TODO

    return dwError;
}

DWORD
SamDbVerifyPassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    )
{
    DWORD dwError = 0;

    // TODO

    return dwError;
}

#if 0
DWORD
SamDbAddUser(
    HANDLE        hDirectory,
    PWSTR         pwszUserDN,
    DIRECTORY_MOD modifications[]
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    sqlite3_stmt* pSqlStatement = NULL;
    DWORD dwNumMods = 0;
    DWORD iMod = 0;
    PWSTR pwszUserName = NULL;
    PWSTR pwszDomain = NULL;
    PSTR  pszDomainName = NULL;
    PSTR  pszDomainSID = NULL;
    PSTR  pszUserDN   = NULL;
    PSTR  pszUserName = NULL;
    PSTR  pszUserSID = NULL;
    PSTR  pszPassword = NULL;
    PSTR  pszHomedir = NULL;
    PSTR  pszShell = NULL;
    PSTR  pszGecos = NULL;
    PSTR  pszFullName = NULL;
    DWORD dwPasswdChangeTime = 0;
    DWORD dwUID = 0;
    BOOLEAN bUIDSpecified = FALSE;
    DWORD dwPrimaryGroupRecordId = 0;
    LONG64 qwAccountExpiry = 0;
    DWORD dwUserInfoFlags = 0;
    BYTE  lmHash[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
    BYTE  ntHash[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    PSAM_DB_DOMAIN_INFO* ppDomainInfoList = NULL;
    DWORD dwNumDomains = 0;
    DWORD iParam = 0;
    BOOLEAN bInLock = FALSE;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_USER;
    BOOLEAN bTxStarted = FALSE;
    PCSTR pszQueryTemplate = "INSERT INTO " SAM_DB_OBJECTS_TABLE \
                                "(ObjectRecordId,"        \
                                 "ObjectSID,"             \
                                 "DistinguishedName,"     \
                                 "ObjectClass,"           \
                                 "UID,"                   \
                                 "GID,"                   \
                                 "Password,"              \
                                 "SamAccountName,"        \
                                 "Domain,"                \
                                 "Gecos,"                 \
                                 "UserInfoFlags,"         \
                                 "Homedir,"               \
                                 "Shell,"                 \
                                 "PasswdChangeTime,"      \
                                 "FullName,"              \
                                 "AccountExpiry,"         \
                                 "LMHash,"                \
                                 "NTHash"                 \
                                ")\n"                     \
                                "VALUES ("                \
   /* ObjectRecordId    */       "rowid,"                 \
   /* ObjectSID         */       "?1,"                    \
   /* DistinguishedName */       "?2,"                    \
   /* ObjectClass       */       "?3,"                    \
   /* UID               */       "?4,"                    \
   /* GID               */       "?5,"                    \
   /* Password          */       "?6,"                    \
   /* SamAccountName    */       "?7,"                    \
   /* Domain            */       "?8,"                    \
   /* Gecos             */       "?9,"                    \
   /* UserInfoFlags     */       "?10,"                   \
   /* Homedir           */       "?11,"                   \
   /* Shell             */       "?12,"                   \
   /* PasswdChangeTime  */       "?13,"                   \
   /* FullName          */       "?14,"                   \
   /* AccountExpiry     */       "?15,"                   \
   /* LMHash            */       "?16,"                   \
   /* NTHash            */       "?17"                    \
                                ")";

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    while (modifications[dwNumMods].pwszAttrName &&
           modifications[dwNumMods].pAttrValues)
    {
        dwNumMods++;
    }

    dwError = SamDbParseDN(
                    pwszUserDN,
                    &pwszUserName,
                    &pwszDomain,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (entryType != SAMDB_ENTRY_TYPE_USER)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbFindDomains(
                    hDirectory,
                    pwszDomain,
                    &ppDomainInfoList,
                    &dwNumDomains);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumDomains != 1)
    {
        dwError = LSA_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = LsaWc16sToMbs(
                    ppDomainInfoList[0]->pwszDomainName,
                    &pszDomainName);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pDirContext->rwLock);

    SAM_DB_BEGIN_TRANSACTION(bTxStarted, hDirectory);

    dwError = LsaWc16sToMbs(
                    pwszUserName,
                    &pszUserName);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaWc16sToMbs(
                    pwszUserDN,
                    &pszUserDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    for (; iMod < dwNumMods; iMod++)
    {
        NTSTATUS ntStatus = 0;
        PSAMDB_ATTRIBUTE_LOOKUP_ENTRY pLookupEntry = NULL;
        PATTRIBUTE_VALUE pAttrValue = NULL;

        ntStatus = LwRtlRBTreeFind(
                        pDirContext->pAttrLookup->pAttrTree,
                        modifications[iMod].pwszAttrName,
                        (PVOID*)&pLookupEntry);
        if (ntStatus)
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        switch (pLookupEntry->dwId)
        {
            case SAMDB_USER_TABLE_COLUMN_UID :

                if ((modifications[iMod].ulNumValues != 1) ||
                    (modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_INTEGER)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwUID = pAttrValue->ulValue;
                bUIDSpecified = TRUE;

                break;

            case SAMDB_USER_TABLE_COLUMN_SID:

                if ((modifications[iMod].ulNumValues != 1) ||
                    (modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttrValue->pwszStringValue,
                                &pszUserSID);
                BAIL_ON_SAMDB_ERROR(dwError);

                break;

            case SAMDB_USER_TABLE_COLUMN_PASSWORD:

                if ((modifications[iMod].ulNumValues != 1) ||
                    (modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttrValue->pwszStringValue,
                                &pszPassword);
                BAIL_ON_SAMDB_ERROR(dwError);

                break;

            case SAMDB_USER_TABLE_COLUMN_USER_INFO_FLAGS:

                if ((modifications[iMod].ulNumValues != 1) ||
                    (modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_INTEGER)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwUserInfoFlags = pAttrValue->ulValue;

                break;

            case SAMDB_USER_TABLE_COLUMN_FULL_NAME:

                if ((modifications[iMod].ulNumValues != 1) ||
                    (modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttrValue->pwszStringValue,
                                &pszFullName);
                BAIL_ON_SAMDB_ERROR(dwError);

                break;

            case SAMDB_USER_TABLE_COLUMN_GECOS:

                if ((modifications[iMod].ulNumValues != 1) ||
                    (modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttrValue->pwszStringValue,
                                &pszGecos);
                BAIL_ON_SAMDB_ERROR(dwError);

                break;

            case SAMDB_USER_TABLE_COLUMN_HOMEDIR:

                if ((modifications[iMod].ulNumValues != 1) ||
                    (modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttrValue->pwszStringValue,
                                &pszHomedir);
                BAIL_ON_SAMDB_ERROR(dwError);

                break;

            case SAMDB_USER_TABLE_COLUMN_SHELL:

                if ((modifications[iMod].ulNumValues != 1) ||
                    (modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttrValue->pwszStringValue,
                                &pszShell);
                BAIL_ON_SAMDB_ERROR(dwError);

                break;

            case SAMDB_USER_TABLE_COLUMN_PASSWORD_CHANGE_TIME:

                if ((modifications[iMod].ulNumValues != 1) ||
                    (modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_INTEGER)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwPasswdChangeTime = pAttrValue->ulValue;

                break;

            case SAMDB_USER_TABLE_COLUMN_ACCOUNT_EXPIRY:

                if ((modifications[iMod].ulNumValues != 1) ||
                    (modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_LARGE_INTEGER)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                qwAccountExpiry = pAttrValue->llValue;

                break;

            case SAMDB_USER_TABLE_COLUMN_PRIMARY_GROUP:

                if ((modifications[iMod].ulNumValues != 1) ||
                    (modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwError = SamDbFindGID_inlock(
                                hDirectory,
                                pAttrValue->pwszStringValue,
                                &dwPrimaryGroupRecordId);
                BAIL_ON_SAMDB_ERROR(dwError);

            default:

                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);

                break;
        }
    }

    if (!dwPrimaryGroupRecordId)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    if (!bUIDSpecified)
    {
        dwError = SamDbGetNextAvailableUID(
                        hDirectory,
                        &dwUID);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    if (!pszUserSID)
    {
        DWORD dwRID = 0;

        dwError = SamDbGetNextAvailableRID(
                        hDirectory,
                        &dwRID);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaWc16sToMbs(
                        ppDomainInfoList[0]->pwszDomainSID,
                        &pszDomainSID);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaAllocateStringPrintf(
                        &pszUserSID,
                        "%s-%u",
                        pszDomainSID,
                        dwRID);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbComputeLMHash(
                    pszPassword,
                    &lmHash[0],
                    sizeof(lmHash));
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbComputeNTHash(
                    pszPassword,
                    &ntHash[0],
                    sizeof(ntHash));
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
                    pDirContext->pDbContext->pDbHandle,
                    pszQueryTemplate,
                    -1,
                    &pSqlStatement,
                    NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_bind_text(
                    pSqlStatement,
                    ++iParam,
                    pszUserSID,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_bind_text(
                    pSqlStatement,
                    ++iParam,
                    pszUserDN,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_bind_int(pSqlStatement, ++iParam, objectClass);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_bind_int(pSqlStatement, ++iParam, dwUID);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_bind_int(pSqlStatement, ++iParam, dwPrimaryGroupRecordId);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (!pszPassword)
    {
        dwError = sqlite3_bind_null(pSqlStatement, ++iParam);
    }
    else
    {
        dwError = sqlite3_bind_text(
                        pSqlStatement,
                        ++iParam,
                        pszPassword,
                        -1,
                        SQLITE_TRANSIENT);
    }
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_bind_text(
                    pSqlStatement,
                    ++iParam,
                    pszUserName,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_bind_text(
                    pSqlStatement,
                    ++iParam,
                    pszDomainName,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (!pszGecos)
    {
        dwError = sqlite3_bind_null(pSqlStatement, ++iParam);
    }
    else
    {
        dwError = sqlite3_bind_text(
                        pSqlStatement,
                        ++iParam,
                        pszGecos,
                        -1,
                        SQLITE_TRANSIENT);
    }
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_bind_int(pSqlStatement, ++iParam, dwUserInfoFlags);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (!pszHomedir)
    {
        dwError = sqlite3_bind_null(pSqlStatement, ++iParam);
    }
    else
    {
        dwError = sqlite3_bind_text(
                        pSqlStatement,
                        ++iParam,
                        pszHomedir,
                        -1,
                        SQLITE_TRANSIENT);
    }
    BAIL_ON_SAMDB_ERROR(dwError);

    if (!pszShell)
    {
        dwError = sqlite3_bind_null(pSqlStatement, ++iParam);
    }
    else
    {
        dwError = sqlite3_bind_text(
                        pSqlStatement,
                        ++iParam,
                        pszShell,
                        -1,
                        SQLITE_TRANSIENT);
    }
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_bind_int(pSqlStatement, ++iParam, dwPasswdChangeTime);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (!pszFullName)
    {
        dwError = sqlite3_bind_null(pSqlStatement, ++iParam);
    }
    else
    {
        dwError = sqlite3_bind_text(
                        pSqlStatement,
                        ++iParam,
                        pszFullName,
                        -1,
                        SQLITE_TRANSIENT);
    }
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_bind_int64(pSqlStatement, ++iParam, qwAccountExpiry);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_bind_blob(
                    pSqlStatement,
                    ++iParam,
                    &lmHash[0],
                    sizeof(lmHash),
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_bind_blob(
                    pSqlStatement,
                    ++iParam,
                    &ntHash[0],
                    sizeof(ntHash),
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_step(pSqlStatement);
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    SAM_DB_END_TRANSACTION(bTxStarted, dwError, hDirectory);

    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &pDirContext->rwLock);

    DIRECTORY_FREE_STRING(pszUserName);
    DIRECTORY_FREE_STRING(pszUserSID);
    DIRECTORY_FREE_STRING(pszDomainSID);
    DIRECTORY_FREE_STRING(pszPassword);
    DIRECTORY_FREE_STRING(pszGecos);
    DIRECTORY_FREE_STRING(pszFullName);
    DIRECTORY_FREE_STRING(pszHomedir);
    DIRECTORY_FREE_STRING(pszShell);
    DIRECTORY_FREE_STRING(pszDomainName);
    DIRECTORY_FREE_STRING(pszUserDN);

    if (ppDomainInfoList)
    {
        SamDbFreeDomainInfoList(ppDomainInfoList, dwNumDomains);
    }

    return dwError;

error:

    goto cleanup;
}
#endif


