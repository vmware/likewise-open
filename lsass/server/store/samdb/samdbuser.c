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

#define DB_QUERY_CREATE_USERS_TABLE  \
    "create table samdbusers (       \
                UserRecordId     integer PRIMARY KEY, \
                DomainRecordId   integer,             \
                ObjectSID        text unique,         \
                Uid              integer,             \
                Name             text,                \
                Passwd           text,                \
                GroupRecordId    integer,             \
                UserInfoFlags    integer,             \
                Gecos            text,                \
                HomeDir          text,                \
                Shell            text,                \
                PasswdChangeTime integer,             \
                FullName         text,                \
                AccountExpiry    integer,             \
                LMOwf_1          integer,             \
                LMOwf_2          integer,             \
                LMOwf_3          integer,             \
                LMOwf_4          integer,             \
                NTOwf_1          integer,             \
                NTOwf_2          integer,             \
                NTOwf_3          integer,             \
                NTOwf_4          integer,             \
                CreatedTime      date,                \
                unique(DomainRecordId, Uid),          \
                unique(DomainRecordId, Name)          \
               )"

#define DB_QUERY_INSERT_USER  \
    "INSERT INTO samdbusers   \
        (                     \
            UserRecordId,     \
            DomainRecordId,   \
            ObjectSID,        \
            Uid,              \
            Name,             \
            Passwd,           \
            GroupRecordId,    \
            UserInfoFlags,    \
            Gecos,            \
            HomeDir,          \
            Shell,            \
            PasswdChangeTime, \
            FullName,         \
            AccountExpiry,    \
            LMOwf_1,          \
            LMOwf_2,          \
            LMOwf_3,          \
            LMOwf_4,          \
            NTOwf_1,          \
            NTOwf_2,          \
            NTOwf_3,          \
            NTOwf_4           \
        )                     \
     VALUES (                 \
            NULL,             \
            %d,               \
            %Q,               \
            %d,               \
            %Q,               \
            %Q,               \
            %d,               \
            %d,               \
            %Q,               \
            %Q,               \
            %Q,               \
            %d,               \
            %Q,               \
            %d,               \
            %d,               \
            %d,               \
            %d,               \
            %d,               \
            %d,               \
            %d,               \
            %d,               \
            %d                \
        )"

#define DB_QUERY_CREATE_USERS_INSERT_TRIGGER                   \
    "create trigger samdbusers_createdtime                     \
     after insert on samdbusers                                \
     begin                                                     \
          update samdbusers                                    \
          set CreatedTime = DATETIME('NOW')                    \
          where rowid = new.rowid;                             \
                                                               \
          insert into samdbgroupmembers (UserRecordId, GroupRecordId, 1) \
          values (new.UserRecordId, new.GroupRecordId, new.MemberType);  \
     end"

#define DB_QUERY_CREATE_USERS_DELETE_TRIGGER                   \
    "create trigger samdbusers_delete_record                   \
     after delete on samdbusers                                \
     begin                                                     \
          delete from samdbgroupmembers where UserRecordId = old.UserRecordId; \
     end"

#define DB_QUERY_NUM_USERS_IN_DOMAIN \
    "select count(*) \
       from samdbusers  sdu, \
            samdbdomains sdd  \
      where sdu.DomainRecordId = sdd.DomainRecordId \
        and sdd.Name = %Q"

#define DB_QUERY_MAX_UID_BY_DOMAIN \
    "select max(Uid) \
       from samdbusers sdu \
      where sdu.DomainRecordId = %d"

#define DB_QUERY_DELETE_USER \
    "delete \
       from samdbusers sdu \
      where sdu.Name = %Q \
        and sdu.DomainRecordId = %d"

DWORD
SamDbAddUserAttrLookups(
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup
    )
{
    DWORD dwError = 0;
    struct {
        PSTR pszAttrName;
        DIRECTORY_ATTR_TYPE attrType;
        SAMDB_USER_TABLE_COLUMN colType;
        BOOL bIsMandatory;
        BOOL bIsModifiable;
    } userAttrs[] =
    {
        {
            DIRECTORY_ATTR_TAG_USER_NAME,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            SAMDB_USER_TABLE_COLUMN_NAME,
            ATTR_IS_MANDATORY,
            ATTR_IS_IMMUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_USER_FULLNAME,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            SAMDB_USER_TABLE_COLUMN_FULL_NAME,
            ATTR_IS_MANDATORY,
            ATTR_IS_MUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_UID,
            DIRECTORY_ATTR_TYPE_INTEGER,
            SAMDB_USER_TABLE_COLUMN_UID,
            ATTR_IS_MANDATORY,
            ATTR_IS_IMMUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_USER_SID,
            DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR,
            SAMDB_USER_TABLE_COLUMN_SID,
            ATTR_IS_MANDATORY,
            ATTR_IS_IMMUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_USER_PRIMARY_GROUP_DN,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            SAMDB_USER_TABLE_COLUMN_PRIMARY_GROUP,
            ATTR_IS_MANDATORY,
            ATTR_IS_MUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_USER_PASSWORD,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            SAMDB_USER_TABLE_COLUMN_PASSWORD,
            ATTR_IS_NOT_MANDATORY,
            ATTR_IS_MUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_GECOS,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            SAMDB_USER_TABLE_COLUMN_GECOS,
            ATTR_IS_NOT_MANDATORY,
            ATTR_IS_MUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_HOMEDIR,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            SAMDB_USER_TABLE_COLUMN_HOMEDIR,
            ATTR_IS_MANDATORY,
            ATTR_IS_MUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_SHELL,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            SAMDB_USER_TABLE_COLUMN_SHELL,
            ATTR_IS_MANDATORY,
            ATTR_IS_MUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_PASSWORD_CHANGE_TIME,
            DIRECTORY_ATTR_TYPE_INTEGER,
            SAMDB_USER_TABLE_COLUMN_PASSWORD_CHANGE_TIME,
            ATTR_IS_NOT_MANDATORY,
            ATTR_IS_MUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_ACCOUNT_EXPIRY,
            DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            SAMDB_USER_TABLE_COLUMN_ACCOUNT_EXPIRY,
            ATTR_IS_NOT_MANDATORY,
            ATTR_IS_MUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_USER_INFO_FLAGS,
            DIRECTORY_ATTR_TYPE_INTEGER,
            SAMDB_USER_TABLE_COLUMN_USER_INFO_FLAGS,
            ATTR_IS_NOT_MANDATORY,
            ATTR_IS_MUTABLE
        }
    };
    DWORD dwNumAttrs = sizeof(userAttrs)/sizeof(userAttrs[0]);
    DWORD iAttr = 0;
    PSAMDB_ATTRIBUTE_LOOKUP_ENTRY pAttrEntry = NULL;

    for(; iAttr < dwNumAttrs; iAttr++)
    {
        dwError = DirectoryAllocateMemory(
                        sizeof(SAMDB_ATTRIBUTE_LOOKUP_ENTRY),
                        (PVOID*)&pAttrEntry);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        userAttrs[iAttr].pszAttrName,
                        &pAttrEntry->pwszAttributeName);
        BAIL_ON_SAMDB_ERROR(dwError);

        pAttrEntry->bIsMandatory = userAttrs[iAttr].bIsMandatory;
        pAttrEntry->bIsModifiable = userAttrs[iAttr].bIsModifiable;
        pAttrEntry->attrType = userAttrs[iAttr].attrType;
        pAttrEntry->dwId = userAttrs[iAttr].colType;

        dwError = LwRtlRBTreeAdd(
                        pAttrLookup->pAttrTree,
                        pAttrEntry->pwszAttributeName,
                        pAttrEntry);
        BAIL_ON_SAMDB_ERROR(dwError);

        pAttrEntry = NULL;
    }

cleanup:

    return dwError;

error:

    if (pAttrEntry)
    {
        SamDbFreeAttributeLookupEntry(pAttrEntry);
    }

    goto cleanup;
}

DWORD
SamDbAddUser(
    HANDLE        hDirectory,
    PWSTR         pwszUserDN,
    DIRECTORY_MOD modifications[]
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    DWORD dwNumMods = 0;
    DWORD iMod = 0;
    PWSTR pwszUserName = NULL;
    PWSTR pwszDomain = NULL;
    PSTR  pszDomainSID = NULL;
    PSTR  pszError = NULL;
    PSTR  pszQuery = NULL;
    PSTR  pszUserName = NULL;
    PSTR  pszUserSID = NULL;
    PSTR  pszPassword = NULL;
    PSTR  pszHomedir = NULL;
    PSTR  pszShell = NULL;
    PSTR  pszGecos = NULL;
    PSTR  pszFullName = NULL;
    DWORD dwPasswdChangeTime = 0;
    DWORD dwUID = 0;
    DWORD dwPrimaryGroupRecordId = 0;
    LONG64 qwAccountExpiry = 0;
    DWORD dwUserInfoFlags = 0;
    BYTE  lmHash[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
    BYTE  ntHash[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    PSAM_DB_DOMAIN_INFO* ppDomainInfoList = NULL;
    DWORD dwNumDomains = 0;
    BOOLEAN bInLock = FALSE;

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

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pDirContext->rwLock);

    dwError = LsaWc16sToMbs(
                    pwszUserName,
                    &pszUserName);
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

                dwUserInfoFlags = pAttrValue->uLongValue;

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

                dwPasswdChangeTime = pAttrValue->uLongValue;

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

                dwError = SamDbFindGroupRecordId_inlock(
                                hDirectory,
                                ppDomainInfoList[0],
                                pAttrValue->pwszStringValue,
                                &dwPrimaryGroupRecordId);
                BAIL_ON_SAMDB_ERROR(dwError);

            default:

                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);

                break;
        }
    }

    dwError = SamDbGetNextAvailableUID_inlock(
                    hDirectory,
                    ppDomainInfoList[0]->ulDomainRecordId,
                    &dwUID);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaWc16sToMbs(
                    ppDomainInfoList[0]->pwszDomainSID,
                    &pszDomainSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                    &pszUserSID,
                    "%s-%d",
                    pszDomainSID,
                    dwUID);
    BAIL_ON_SAMDB_ERROR(dwError);

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

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_INSERT_USER,
                    ppDomainInfoList[0]->ulDomainRecordId,
                    pszUserSID,
                    dwUID,
                    pszUserName,
                    pszPassword,
                    dwPrimaryGroupRecordId,
                    dwUserInfoFlags,
                    pszGecos,
                    pszHomedir,
                    pszShell,
                    dwPasswdChangeTime,
                    pszFullName,
                    qwAccountExpiry,
                    *((PDWORD)&lmHash[0]),
                    *((PDWORD)&lmHash[4]),
                    *((PDWORD)&lmHash[8]),
                    *((PDWORD)&lmHash[12]),
                    *((PDWORD)&ntHash[0]),
                    *((PDWORD)&ntHash[4]),
                    *((PDWORD)&ntHash[8]),
                    *((PDWORD)&ntHash[12]));

    dwError = sqlite3_exec(pDirContext->pDbContext->pDbHandle,
                           pszQuery,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    if (pszQuery)
    {
        sqlite3_free(pszQuery);
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

    if (ppDomainInfoList)
    {
        SamDbFreeDomainInfoList(ppDomainInfoList, dwNumDomains);
    }

    return dwError;

error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

DWORD
SamDbNumUsersInDomain_inlock(
    HANDLE hDirectory,
    PSTR   pszDomainName,
    PDWORD pdwNumUsers
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    int   nRows = 0;
    int   nCols = 0;
    PSTR* ppszResult = NULL;
    DWORD dwNumUsers = 0;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_NUM_USERS_IN_DOMAIN,
                    pszDomainName);

    dwError = sqlite3_get_table(
                    pDirContext->pDbContext->pDbHandle,
                    pszQuery,
                    &ppszResult,
                    &nRows,
                    &nCols,
                    &pszError);
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows)
    {
        dwNumUsers = 0;
        goto done;
    }

    if (nCols != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwNumUsers = atoi(ppszResult[1]);

done:

    *pdwNumUsers = dwNumUsers;

cleanup:

    if (pszQuery)
    {
        sqlite3_free(pszQuery);
    }

    if (ppszResult)
    {
        sqlite3_free_table(ppszResult);
    }

    return dwError;

error:

    *pdwNumUsers = 0;

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

DWORD
SamDbGetNextAvailableUID_inlock(
    HANDLE hDirectory,
    DWORD  dwDomainRecordId,
    PDWORD pdwUID
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    int   nRows = 0;
    int   nCols = 0;
    PSTR* ppszResult = NULL;
    DWORD dwMaxUID = 0;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_MAX_UID_BY_DOMAIN,
                    dwDomainRecordId);

    dwError = sqlite3_get_table(
                    pDirContext->pDbContext->pDbHandle,
                    pszQuery,
                    &ppszResult,
                    &nRows,
                    &nCols,
                    &pszError);
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows)
    {
        dwMaxUID = SAMDB_MIN_UID - 1;
        goto done;
    }

    if (nCols != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwMaxUID = atoi(ppszResult[1]);

done:

    *pdwUID = dwMaxUID + 1;

cleanup:

    if (pszQuery)
    {
        sqlite3_free(pszQuery);
    }

    if (ppszResult)
    {
        sqlite3_free_table(ppszResult);
    }

    return dwError;

error:

    *pdwUID = 0;

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

DWORD
SamDbDeleteUser(
    HANDLE hDirectory,
    PWSTR  pwszUserDN
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PWSTR pwszUserName = NULL;
    PWSTR pwszDomainName = NULL;
    PSTR  pszUserName = NULL;
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    BOOLEAN bInLock = FALSE;
    PSAM_DB_DOMAIN_INFO* ppDomainInfoList = NULL;
    DWORD dwNumDomains = 0;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    dwError = SamDbParseDN(
                    pwszUserDN,
                    &pwszUserName,
                    &pwszDomainName,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (entryType != SAMDB_ENTRY_TYPE_USER)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbFindDomains(
                    hDirectory,
                    pwszDomainName,
                    &ppDomainInfoList,
                    &dwNumDomains);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumDomains == 0)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = LsaWc16sToMbs(
                    pwszUserName,
                    &pszUserName);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pDirContext->rwLock);

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_DELETE_USER,
                    pszUserName,
                    ppDomainInfoList[0]->ulDomainRecordId);

    dwError = sqlite3_exec(
                    pDirContext->pDbContext->pDbHandle,
                    pszQuery,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    if (pszQuery)
    {
        sqlite3_free(pszQuery);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &pDirContext->rwLock);

    if (pwszUserName)
    {
        DirectoryFreeMemory(pwszUserName);
    }
    if (pwszDomainName)
    {
        DirectoryFreeMemory(pwszDomainName);
    }
    if (pszUserName)
    {
        DirectoryFreeString(pszUserName);
    }

    return dwError;

error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
