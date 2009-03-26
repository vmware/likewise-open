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
 *        samdbdomain.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      SamDbInitDomainTable
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "includes.h"


#define DB_QUERY_CREATE_DOMAINS_INSERT_TRIGGER                 \
    "create trigger samdbdomains_createdtime                   \
     after insert on samdbdomains                              \
     begin                                                     \
          update samdbdomains                                  \
          set CreatedTime = DATETIME('NOW')                    \
          where rowid = new.rowid;                             \
     end"

#define DB_QUERY_FIND_DOMAIN \
    "select DomainRecordId,  \
            ObjectSID,       \
            Name,            \
            NetbiosName      \
       from samdbdomains     \
      where Name = %Q"

#define DB_QUERY_FIND_ALL_DOMAINS \
    "select DomainRecordId,  \
            ObjectSID,       \
            Name,            \
            NetbiosName      \
       from samdbdomains"

#define DB_QUERY_DELETE_DOMAIN \
    "delete from samdbdomains where Name = %Q"

DWORD
SamDbAddDomainAttrLookups(
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup
    )
{
    DWORD dwError = 0;
    struct {
        PSTR pszAttrName;
        DIRECTORY_ATTR_TYPE      attrType;
        SAMDB_DOMAIN_TABLE_COLUMN colType;
        BOOL bIsMandatory;
        BOOL bIsModifiable;
    } domainAttrs[] =
    {
        {
            DIRECTORY_ATTR_TAG_DOMAIN_NAME,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            SAMDB_DOMAIN_TABLE_COLUMN_DOMAIN_NAME,
            ATTR_IS_MANDATORY,
            ATTR_IS_IMMUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_DOMAIN_SID,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            SAMDB_DOMAIN_TABLE_COLUMN_MACHINE_SID,
            ATTR_IS_MANDATORY,
            ATTR_IS_IMMUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_DOMAIN_NETBIOS_NAME,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            SAMDB_DOMAIN_TABLE_COLUMN_NETBIOS_NAME,
            ATTR_IS_MANDATORY,
            ATTR_IS_MUTABLE
        }
    };
    DWORD dwNumAttrs = sizeof(domainAttrs)/sizeof(domainAttrs[0]);
    DWORD iAttr = 0;
    PSAMDB_ATTRIBUTE_LOOKUP_ENTRY pAttrEntry = NULL;

    for(; iAttr < dwNumAttrs; iAttr++)
    {
        dwError = DirectoryAllocateMemory(
                        sizeof(SAMDB_ATTRIBUTE_LOOKUP_ENTRY),
                        (PVOID*)&pAttrEntry);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        domainAttrs[iAttr].pszAttrName,
                        &pAttrEntry->pwszAttributeName);
        BAIL_ON_SAMDB_ERROR(dwError);

        pAttrEntry->bIsMandatory = domainAttrs[iAttr].bIsMandatory;
        pAttrEntry->bIsModifiable = domainAttrs[iAttr].bIsModifiable;
        pAttrEntry->attrType = domainAttrs[iAttr].attrType;
        pAttrEntry->dwId = domainAttrs[iAttr].colType;

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
SamDbAddDomain(
    HANDLE hDirectory,
    PWSTR  pwszObjectDN,
    DIRECTORY_MOD Modifications[]
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PWSTR pwszObjectName = NULL;
    PWSTR pwszDomainName = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszDomainSID = NULL;
    PSTR pszNetBIOSName = NULL;
    PSTR pszObjectDN = NULL;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    BOOLEAN bInLock = FALSE;
    DWORD dwNumMods = 0;
    DWORD iMod = 0;
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    PCSTR pszQueryTemplate = "INSERT INTO samdbobjects\n" \
                                "(ObjectRecordId,"    \
                                " ObjectSID,"         \
                                " DistinguishedName," \
                                " ParentDN,"          \
                                " ObjectClass,"       \
                                " Domain,"            \
                                " CommonName,"        \
                                " SamAccountName,"    \
                                " Description,"       \
                                " UID,"               \
                                " UserPasswd,"        \
                                " UserInfoFlags,"     \
                                " Gecos,"             \
                                " HomeDir,"           \
                                " Shell,"             \
                                " PasswdChangeTime,"  \
                                " FullName,"          \
                                " AccountExpiry,"     \
                                " LMOwf_1,"           \
                                " LMOwf_2,"           \
                                " LMOwf_3,"           \
                                " LMOwf_4,"           \
                                " NTOwf_1,"           \
                                " NTOwf_2,"           \
                                " NTOwf_3,"           \
                                " NTOwf_4,"           \
                                " GID,"               \
                                " GroupPasswd,"       \
                                " CreatedTime"        \
                                ")\n"                 \
                                "VALUES ("            \
   /* ObjectRecordId */         "NULL,"               \
   /* ObjectSID      */         "%Q,"                 \
   /* DistinguishedName */      "%Q,"                 \
   /* ParentDN */               "NULL,"               \
   /* ObjectClass */            "%d,"                 \
   /* Domain */                 "%Q,"                 \
   /* CommonName */             "NULL,"               \
   /* SamAccountName */         "%Q,"                 \
   /* Description */            "NULL,"               \
   /* UID */                    "0,"                  \
   /* UserPasswd */             "NULL,"               \
   /* UserInfoFlags */          "0,"                  \
   /* Gecos */                  "NULL,"               \
   /* HomeDir */                "NULL,"               \
   /* Shell */                  "NULL,"               \
   /* PasswdChangeTime */       "0,"                  \
   /* FullName */               "NULL,"               \
   /* AccountExpiry */          "0,"                  \
   /* LMOwf_1 */                "0,"                  \
                                "0,"                  \
                                "0,"                  \
                                "0,"                  \
   /* NTOwf_1 */                "0,"                  \
                                "0,"                  \
                                "0,"                  \
                                "0,"                  \
   /* GID */                    "0,"                  \
   /* GroupPasswd */            "NULL,"               \
   /* CreatedTime */            "DATETIME('NOW')"     \
                                ")";

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    while (Modifications[dwNumMods].pwszAttrName &&
           Modifications[dwNumMods].pAttrValues) {
        dwNumMods++;
    }

    dwError = SamDbParseDN(
                    pwszObjectDN,
                    &pwszObjectName,
                    &pwszDomainName,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (entryType != SAMDB_ENTRY_TYPE_DOMAIN)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pDirContext->rwLock);

    dwError = LsaWc16sToMbs(
                    pwszDomainName,
                    &pszDomainName);
    BAIL_ON_SAMDB_ERROR(dwError);

    for (; iMod < dwNumMods; iMod++)
    {
        NTSTATUS ntStatus = 0;
        PSAMDB_ATTRIBUTE_LOOKUP_ENTRY pLookupEntry = NULL;
        PATTRIBUTE_VALUE pAttrValue = NULL;

        ntStatus = LwRtlRBTreeFind(
                        pDirContext->pAttrLookup->pAttrTree,
                        Modifications[iMod].pwszAttrName,
                        (PVOID*)&pLookupEntry);
        if (ntStatus)
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        switch (pLookupEntry->dwId)
        {
            case SAMDB_DOMAIN_TABLE_COLUMN_NETBIOS_NAME:

                if ((Modifications[iMod].ulNumValues != 1) ||
                    (Modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &Modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttrValue->pwszStringValue,
                                &pszNetBIOSName);
                BAIL_ON_SAMDB_ERROR(dwError);

                break;

            case SAMDB_DOMAIN_TABLE_COLUMN_MACHINE_SID:

                if ((Modifications[iMod].ulNumValues != 1) ||
                    (Modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &Modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttrValue->pwszStringValue,
                                &pszDomainSID);
                BAIL_ON_SAMDB_ERROR(dwError);

                break;

            default:

                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);

                break;
        }
    }

    if (IsNullOrEmptyString(pszDomainName) ||
        IsNullOrEmptyString(pszNetBIOSName) ||
        IsNullOrEmptyString(pszDomainSID))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }


    dwError = LsaWc16sToMbs(
                    pwszObjectDN,
                    &pszObjectDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    pszQuery = sqlite3_mprintf(
                    pszQueryTemplate,
                    pszDomainSID,
                    pszObjectDN,
                    SAMDB_OBJECT_CLASS_DOMAIN,
                    pszDomainName,
                    pszNetBIOSName);

    dwError = sqlite3_exec(pDirContext->pDbContext->pDbHandle,
                           pszQuery,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    SAMDB_UNLOCK_RWMUTEX(bInLock, &pDirContext->rwLock);

    if (pszDomainName)
    {
        DirectoryFreeMemory(pszDomainName);
    }
    if (pszObjectDN)
    {
        DirectoryFreeMemory(pszObjectDN);
    }
    if (pszDomainSID)
    {
        DirectoryFreeMemory(pszDomainSID);
    }
    if (pszNetBIOSName)
    {
        DirectoryFreeMemory(pszNetBIOSName);
    }
    if (pwszObjectName)
    {
        DirectoryFreeMemory(pwszObjectName);
    }
    if (pwszDomainName)
    {
        DirectoryFreeMemory(pwszDomainName);
    }
    if (pszQuery)
    {
        sqlite3_free(pszQuery);
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
SamDbFindDomains(
    HANDLE                hDirectory,
    PWSTR                 pwszDomainName,
    PSAM_DB_DOMAIN_INFO** pppDomainInfoList,
    PDWORD                pdwNumDomains
    )
{
    DWORD   dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR    pszDomainName = NULL;
    PSTR    pszQuery = NULL;
    PSTR    pszError = NULL;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PSAM_DB_DOMAIN_INFO*   ppDomainInfoList = NULL;
    DWORD   dwNumDomainsFound = 0;
    int nRows = 0;
    int nCols = 0;
    int nExpectedCols = 4;
    PSTR* ppszResult = NULL;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    if (pwszDomainName)
    {
        dwError = LsaWc16sToMbs(
                        pwszDomainName,
                        &pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        LsaStrToUpper(pszDomainName);
    }

    SAMDB_LOCK_RWMUTEX_SHARED(bInLock, &pDirContext->rwLock);

    if (pszDomainName)
    {
        pszQuery = sqlite3_mprintf(
                        DB_QUERY_FIND_DOMAIN,
                        pszDomainName);
    }
    else
    {
        pszQuery = sqlite3_mprintf(
                        DB_QUERY_FIND_ALL_DOMAINS);
    }

    dwError = sqlite3_get_table(
                    pDirContext->pDbContext->pDbHandle,
                    pszQuery,
                    &ppszResult,
                    &nRows,
                    &nCols,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (nRows)
    {
        dwError = SamDbBuildDomainInfo(
                        ppszResult,
                        nRows,
                        nCols,
                        nExpectedCols,
                        &ppDomainInfoList,
                        &dwNumDomainsFound);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *pppDomainInfoList = ppDomainInfoList;
    *pdwNumDomains = dwNumDomainsFound;

cleanup:

    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &pDirContext->rwLock);

    if (pszDomainName)
    {
        DirectoryFreeMemory(pszDomainName);
    }

    return dwError;

error:

    *pppDomainInfoList = NULL;
    *pdwNumDomains = 0;

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    if (ppDomainInfoList)
    {
        SamDbFreeDomainInfoList(ppDomainInfoList, dwNumDomainsFound);
    }

    goto cleanup;
}

DWORD
SamDbBuildDomainInfo(
    PSTR*  ppszResult,
    int    nRows,
    int    nCols,
    int    nHeaderColsToSkip,
    PSAM_DB_DOMAIN_INFO** pppDomainInfoList,
    PDWORD pdwNumDomainsFound
    )
{
    DWORD dwError = 0;
    DWORD iCol = 0, iRow = 0;
    DWORD iVal = nHeaderColsToSkip;
    PSAM_DB_DOMAIN_INFO* ppDomainInfoList = NULL;
    PSAM_DB_DOMAIN_INFO pDomainInfo = NULL;
    DWORD dwNumDomainsFound = nRows;

    dwError = DirectoryAllocateMemory(
                    sizeof(PSAM_DB_DOMAIN_INFO) * nRows,
                    (PVOID*)&ppDomainInfoList);
    BAIL_ON_SAMDB_ERROR(dwError);

    for (iRow = 0; iRow < nRows; iRow++)
    {
        dwError = DirectoryAllocateMemory(
                        sizeof(SAM_DB_DOMAIN_INFO),
                        (PVOID*)&pDomainInfo);
        BAIL_ON_SAMDB_ERROR(dwError);

        for (iCol = 0; iCol < nCols; iCol++)
        {
            switch(iCol)
            {
                case 0: /* DomainRecordId */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal]))
                    {
                       pDomainInfo->ulDomainRecordId = atol(ppszResult[iVal]);
                    }
                    break;
                }
                case 1: /* Domain SID */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal]))
                    {
                       dwError = LsaMbsToWc16s(
                                       ppszResult[iVal],
                                       &pDomainInfo->pwszDomainSID);
                       BAIL_ON_SAMDB_ERROR(dwError);
                    }
                    break;
                }
                case 2: /* Name */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal]))
                    {
                       dwError = LsaMbsToWc16s(
                                       ppszResult[iVal],
                                       &pDomainInfo->pwszDomainName);
                       BAIL_ON_SAMDB_ERROR(dwError);
                    }
                    break;
                }
                case 3: /* NetBIOS Name */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal]))
                    {
                       dwError = LsaMbsToWc16s(
                                       ppszResult[iVal],
                                       &pDomainInfo->pwszNetBIOSName);
                       BAIL_ON_SAMDB_ERROR(dwError);
                    }
                    break;
                }
            }
            iVal++;
        }

        *(ppDomainInfoList + iRow) = pDomainInfo;
        pDomainInfo = NULL;
    }

    *pppDomainInfoList = ppDomainInfoList;
    *pdwNumDomainsFound = dwNumDomainsFound;

cleanup:

    return dwError;

error:

    if (ppDomainInfoList)
    {
        SamDbFreeDomainInfoList(ppDomainInfoList, dwNumDomainsFound);
    }

    if (pDomainInfo)
    {
        SamDbFreeDomainInfo(pDomainInfo);
    }

    *pppDomainInfoList = NULL;
    *pdwNumDomainsFound = 0;

    goto cleanup;
}

DWORD
SamDbModifyDomain(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    )
{
    DWORD dwError = 0;
    // PSAM_DIRECTORY_CONTEXT pContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;
    PSTR    pszDomainName = NULL;
    PWSTR   pwszObjectName = NULL;
    PWSTR   pwszDomainName = NULL;
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    DWORD   dwNumMods = 0;
    DWORD   iMod = 0;

    while (modifications[dwNumMods].pwszAttrName &&
           modifications[dwNumMods].pAttrValues)
    {
        dwNumMods++;
    }

    dwError = SamDbParseDN(
                    pwszObjectDN,
                    &pwszObjectName,
                    &pwszDomainName,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (entryType != SAMDB_ENTRY_TYPE_DOMAIN)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = LsaWc16sToMbs(
                    pwszDomainName,
                    &pszDomainName);
    BAIL_ON_SAMDB_ERROR(dwError);

    LsaStrToUpper(pszDomainName);

    for (; iMod < dwNumMods; iMod++)
    {
        // PDIRECTORY_MOD pMod = &modifications[iMod];

        // TODO:
    }

cleanup:

    if (pszDomainName)
    {
        DirectoryFreeString(pszDomainName);
    }
    if (pwszObjectName)
    {
        DirectoryFreeMemory(pwszObjectName);
    }
    if (pwszDomainName)
    {
        DirectoryFreeMemory(pwszDomainName);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
SamDbDeleteDomain(
    HANDLE hDirectory,
    PWSTR  pwszObjectDN
    )
{
    DWORD   dwError = 0;
    PSAM_DIRECTORY_CONTEXT pContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;
    PWSTR   pwszObjectName = NULL;
    PWSTR   pwszDomainName = NULL;
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    PSTR    pszDomainName = NULL;
    BOOLEAN bInLock = FALSE;
    DWORD   dwNumGroups = 0;
    DWORD   dwNumUsers = 0;
    PSTR    pszQuery = NULL;
    PSTR    pszError = NULL;

    dwError = SamDbParseDN(
                    pwszObjectDN,
                    &pwszObjectName,
                    &pwszDomainName,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (entryType != SAMDB_ENTRY_TYPE_DOMAIN)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = LsaWc16sToMbs(
                    pwszDomainName,
                    &pszDomainName);
    BAIL_ON_SAMDB_ERROR(dwError);

    LsaStrToUpper(pszDomainName);

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pContext->rwLock);

    dwError = SamDbNumGroupsInDomain_inlock(
                    hDirectory,
                    pszDomainName,
                    &dwNumGroups);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumGroups)
    {
        dwError = LSA_ERROR_DOMAIN_IN_USE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = SamDbNumUsersInDomain_inlock(
                    hDirectory,
                    pszDomainName,
                    &dwNumUsers);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumUsers)
    {
        dwError = LSA_ERROR_DOMAIN_IN_USE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_DELETE_DOMAIN,
                    pszDomainName);

    dwError = sqlite3_exec(
                    pContext->pDbContext->pDbHandle,
                    pszQuery,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    SAMDB_UNLOCK_RWMUTEX(bInLock, &pContext->rwLock);

    if (pszDomainName)
    {
        DirectoryFreeString(pszDomainName);
    }
    if (pwszObjectName)
    {
        DirectoryFreeMemory(pwszObjectName);
    }
    if (pwszDomainName)
    {
        DirectoryFreeMemory(pwszDomainName);
    }
    if (pszQuery)
    {
        sqlite3_free(pszQuery);
    }

    return dwError;

error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

VOID
SamDbFreeDomainInfoList(
    PSAM_DB_DOMAIN_INFO* ppDomainInfoList,
    DWORD dwNumDomains
    )
{
    DWORD iInfo = 0;

    for (; iInfo < dwNumDomains; iInfo++)
    {
        PSAM_DB_DOMAIN_INFO pInfo = ppDomainInfoList[iInfo];

        if (pInfo)
        {
            SamDbFreeDomainInfo(pInfo);
        }
    }

    DirectoryFreeMemory(ppDomainInfoList);
}

VOID
SamDbFreeDomainInfo(
    PSAM_DB_DOMAIN_INFO pDomainInfo
    )
{
    if (pDomainInfo->pwszDomainName)
    {
        DirectoryFreeMemory(pDomainInfo->pwszDomainName);
    }
    if (pDomainInfo->pwszDomainSID)
    {
        DirectoryFreeMemory(pDomainInfo->pwszDomainSID);
    }
    if (pDomainInfo->pwszNetBIOSName)
    {
        DirectoryFreeMemory(pDomainInfo->pwszNetBIOSName);
    }
    DirectoryFreeMemory(pDomainInfo);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
