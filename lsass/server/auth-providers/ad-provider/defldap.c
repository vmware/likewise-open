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
 *        provider-main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "adprovider.h"

DWORD
DefaultModeFindNSSArtefactByKey(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PCSTR          pszKeyName,
    PCSTR          pszMapName,
    DWORD          dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID*         ppNSSArtefactInfo
    )
{
    DWORD  dwError = 0;
    PVOID  pNSSArtefactInfo = NULL;

    ADConfigurationMode adConfMode = NonSchemaMode;

    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    BAIL_ON_LSA_ERROR(dwError);

    switch (adConfMode)
    {
       case SchemaMode:

           dwError = DefaultModeSchemaFindNSSArtefactByKey(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pszKeyName,
                       pszMapName,
                       dwInfoLevel,
                       dwFlags,
                       &pNSSArtefactInfo);
           break;

       case NonSchemaMode:

           dwError = DefaultModeNonSchemaFindNSSArtefactByKey(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pszKeyName,
                       pszMapName,
                       dwInfoLevel,
                       dwFlags,
                       &pNSSArtefactInfo);
           break;
       case UnknownMode:
           dwError = LSA_ERROR_NOT_SUPPORTED;
           break;
    }
    BAIL_ON_LSA_ERROR(dwError);

    *ppNSSArtefactInfo = pNSSArtefactInfo;

cleanup:

    return dwError;

error:

    *ppNSSArtefactInfo = NULL;

    if (pNSSArtefactInfo) {
      LsaFreeNSSArtefactInfo(dwInfoLevel, pNSSArtefactInfo);
    }

    goto cleanup;
}

DWORD
DefaultModeSchemaFindNSSArtefactByKey(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PCSTR          pszKeyName,
    PCSTR          pszMapName,
    DWORD          dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID*         ppNSSArtefactInfo
    )
{
    DWORD  dwError = 0;
    DWORD  dwCount = 0;
    PSTR   pszQuery = NULL;
    PSTR   pszDN = NULL;
    PSTR   pszEscapedDN = NULL;
    PSTR szAttributeList[] =
        {
             AD_LDAP_NAME_TAG,
             AD_LDAP_KEYWORDS_TAG,
             NULL
        };

    LDAPMessage *pMessagePseudo = NULL;
    PVOID* ppArtefactInfos = NULL;
    DWORD dwNumInfos = 0;
    BOOLEAN bMapExists = FALSE;
    LDAP *pLd = NULL;

    pLd = LsaLdapGetSession(hDirectory);

    BAIL_ON_INVALID_STRING(pszMapName);
    BAIL_ON_INVALID_STRING(pszKeyName);

    dwError = LsaAllocateStringPrintf(
                    &pszDN,
                    "CN=%s,CN=Maps,%s",
                    pszMapName,
                    pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapEscapeString(
                   &pszEscapedDN,
                   pszDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_IsValidDN(
                    hDirectory,
                    pszEscapedDN,
                    &bMapExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bMapExists)
    {
        dwError = LSA_ERROR_NO_SUCH_NSS_MAP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateStringPrintf(
                    &pszQuery,
                    "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseMapEntry)(name=%s))",
                    pszKeyName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapDirectorySearch(
                   hDirectory,
                   pszEscapedDN,
                   LDAP_SCOPE_ONELEVEL,
                   pszQuery,
                   szAttributeList,
                   &pMessagePseudo);
    BAIL_ON_LSA_ERROR(dwError);

    dwCount = ldap_count_entries(
                      pLd,
                      pMessagePseudo);
    if (dwCount < 0) {
       dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
       dwError = LSA_ERROR_NO_SUCH_NSS_KEY;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADSchemaMarshalNSSArtefactInfoList(
                    hDirectory,
                    pszNetBIOSDomainName,
                    pMessagePseudo,
                    dwInfoLevel,
                    dwFlags,
                    &ppArtefactInfos,
                    &dwNumInfos);
    BAIL_ON_LSA_ERROR(dwError);

    *ppNSSArtefactInfo = *ppArtefactInfos;
    *ppArtefactInfos = NULL;

cleanup:

    if (pMessagePseudo) {
        ldap_msgfree(pMessagePseudo);
    }

    if (ppArtefactInfos)
    {
        LsaFreeNSSArtefactInfoList(dwInfoLevel, ppArtefactInfos, dwNumInfos);
    }

    LSA_SAFE_FREE_STRING(pszDN);
    LSA_SAFE_FREE_STRING(pszEscapedDN);
    LSA_SAFE_FREE_STRING(pszQuery);

    return dwError;

error:

    *ppNSSArtefactInfo = NULL;

    if (dwError == LDAP_NO_SUCH_OBJECT)
    {
        dwError = LSA_ERROR_NO_SUCH_NSS_KEY;
    }

    goto cleanup;
}

DWORD
DefaultModeNonSchemaFindNSSArtefactByKey(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PCSTR          pszKeyName,
    PCSTR          pszMapName,
    DWORD          dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID*         ppNSSArtefactInfo
    )
{
    DWORD  dwError = 0;
    DWORD  dwCount = 0;
    PSTR   pszQuery = NULL;
    PSTR   pszDN = NULL;
    PSTR   pszEscapedDN = NULL;
    PSTR szAttributeList[] =
        {
             AD_LDAP_NAME_TAG,
             AD_LDAP_KEYWORDS_TAG,
             NULL
        };

    LDAPMessage *pMessagePseudo = NULL;
    PVOID* ppArtefactInfos = NULL;
    DWORD dwNumInfos = 0;
    BOOLEAN bMapExists = FALSE;
    LDAP *pLd = NULL;

    pLd = LsaLdapGetSession(hDirectory);

    BAIL_ON_INVALID_STRING(pszMapName);
    BAIL_ON_INVALID_STRING(pszKeyName);

    dwError = LsaAllocateStringPrintf(
                    &pszDN,
                    "CN=%s,CN=Maps,%s",
                    pszMapName,
                    pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapEscapeString(
                   &pszEscapedDN,
                   pszDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_IsValidDN(
                    hDirectory,
                    pszEscapedDN,
                    &bMapExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bMapExists)
    {
        dwError = LSA_ERROR_NO_SUCH_NSS_MAP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateStringPrintf(
                    &pszQuery,
                    "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseMapEntry)(name=%s))",
                    pszKeyName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapDirectorySearch(
                   hDirectory,
                   pszEscapedDN,
                   LDAP_SCOPE_ONELEVEL,
                   pszQuery,
                   szAttributeList,
                   &pMessagePseudo);
    BAIL_ON_LSA_ERROR(dwError);

    dwCount = ldap_count_entries(
                      pLd,
                      pMessagePseudo);
    if (dwCount < 0) {
       dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
       dwError = LSA_ERROR_NO_SUCH_NSS_KEY;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADNonSchemaMarshalNSSArtefactInfoList(
                    hDirectory,
                    pszNetBIOSDomainName,
                    pMessagePseudo,
                    dwFlags,
                    dwInfoLevel,
                    &ppArtefactInfos,
                    &dwNumInfos);
    BAIL_ON_LSA_ERROR(dwError);

    *ppNSSArtefactInfo = *ppArtefactInfos;
    *ppArtefactInfos = NULL;

cleanup:

    if (pMessagePseudo) {
        ldap_msgfree(pMessagePseudo);
    }

    if (ppArtefactInfos)
    {
        LsaFreeNSSArtefactInfoList(dwInfoLevel, ppArtefactInfos, dwNumInfos);
    }

    LSA_SAFE_FREE_STRING(pszDN);
    LSA_SAFE_FREE_STRING(pszEscapedDN);
    LSA_SAFE_FREE_STRING(pszQuery);

    return dwError;

error:

    *ppNSSArtefactInfo = NULL;

    if (dwError == LDAP_NO_SUCH_OBJECT)
    {
        dwError = LSA_ERROR_NO_SUCH_NSS_KEY;
    }

    goto cleanup;
}

DWORD
DefaultModeEnumNSSArtefacts(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumNSSArtefacts,
    PDWORD         pdwNumNSSArtefactsFound,
    PVOID**        pppNSSArtefactInfoList
    )
{
    DWORD  dwError = 0;
    DWORD  dwNumNSSArtefactsFound = 0;
    PVOID* ppNSSArtefactInfoList = NULL;

    ADConfigurationMode adConfMode = NonSchemaMode;

    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    BAIL_ON_LSA_ERROR(dwError);

    switch (adConfMode)
    {
       case SchemaMode:
           dwError = DefaultModeSchemaEnumNSSArtefacts(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pEnumState,
                       dwMaxNumNSSArtefacts,
                       &dwNumNSSArtefactsFound,
                       &ppNSSArtefactInfoList
                       );
           break;

       case NonSchemaMode:
           dwError = DefaultModeNonSchemaEnumNSSArtefacts(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pEnumState,
                       dwMaxNumNSSArtefacts,
                       &dwNumNSSArtefactsFound,
                       &ppNSSArtefactInfoList
                       );
           break;
       case UnknownMode:
           dwError = LSA_ERROR_NOT_SUPPORTED;
           break;
    }
    BAIL_ON_LSA_ERROR(dwError);

    *pppNSSArtefactInfoList = ppNSSArtefactInfoList;
    *pdwNumNSSArtefactsFound = dwNumNSSArtefactsFound;

cleanup:

    return dwError;

error:

    *pppNSSArtefactInfoList = NULL;
    *pdwNumNSSArtefactsFound = 0;

    if (ppNSSArtefactInfoList) {
      LsaFreeNSSArtefactInfoList(pEnumState->dwInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }

    goto cleanup;
}

DWORD
DefaultModeSchemaEnumNSSArtefacts(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumNSSArtefacts,
    PDWORD         pdwNumNSSArtefactsFound,
    PVOID**        pppNSSArtefactInfoList
    )
{
    DWORD  dwError = 0;
    DWORD  dwCount = 0;
    PCSTR   pszQuery = "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseMapEntry))";
    PSTR   pszDN = NULL;
    PSTR   pszEscapedDN = NULL;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD  dwNumNSSArtefactsFound = 0;
    PSTR   szAttributeList[] =
               {
                 AD_LDAP_NAME_TAG,
                 AD_LDAP_KEYWORDS_TAG,
                 NULL
               };
    LDAPMessage *pMessagePseudo = NULL;

    LDAP *pLd = LsaLdapGetSession(hDirectory);

    BAIL_ON_INVALID_STRING(pEnumState->pszMapName);

    dwError = LsaAllocateStringPrintf(
                    &pszDN,
                    "CN=%s,CN=Maps,%s",
                    pEnumState->pszMapName,
                    pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapEscapeString(
                   &pszEscapedDN,
                   pszDN);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pEnumState->bMorePages){
        dwError = LSA_ERROR_NO_MORE_NSS_ARTEFACTS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapDirectoryOnePagedSearch(
                       hDirectory,
                       pszEscapedDN,
                       pszQuery,
                       szAttributeList,
                       dwMaxNumNSSArtefacts,
                       &pEnumState->pCookie,
                       LDAP_SCOPE_ONELEVEL,
                       &pMessagePseudo,
                       &pEnumState->bMorePages);

    BAIL_ON_LSA_ERROR(dwError);

    dwCount = ldap_count_entries(
                          pLd,
                          pMessagePseudo
                          );
    if (dwCount < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
        dwError = LSA_ERROR_NO_MORE_NSS_ARTEFACTS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADSchemaMarshalNSSArtefactInfoList(
                    hDirectory,
                    pszNetBIOSDomainName,
                    pMessagePseudo,
                    pEnumState->dwInfoLevel,
                    pEnumState->dwMapFlags,
                    &ppNSSArtefactInfoList,
                    &dwNumNSSArtefactsFound);
    BAIL_ON_LSA_ERROR(dwError);

    *pppNSSArtefactInfoList = ppNSSArtefactInfoList;
    *pdwNumNSSArtefactsFound = dwNumNSSArtefactsFound;

cleanup:

    if (pMessagePseudo)
    {
        ldap_msgfree(pMessagePseudo);
    }

    LSA_SAFE_FREE_STRING(pszDN);
    LSA_SAFE_FREE_STRING(pszEscapedDN);

    return dwError;

error:

    *pppNSSArtefactInfoList = NULL;
    *pdwNumNSSArtefactsFound = 0;

    if (ppNSSArtefactInfoList) {
          LsaFreeNSSArtefactInfoList(pEnumState->dwInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }

    if (dwError == LDAP_NO_SUCH_OBJECT)
    {
        dwError = LSA_ERROR_NO_MORE_NSS_ARTEFACTS;
    }

    goto cleanup;
}

DWORD
DefaultModeNonSchemaEnumNSSArtefacts(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumNSSArtefacts,
    PDWORD         pdwNumNSSArtefactsFound,
    PVOID**        pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    PCSTR pszQuery = "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseMapEntry))";
    PSTR pszDN = NULL;
    PSTR pszEscapedDN = NULL;
    PVOID* ppNSSArtefactInfoList = NULL;
    PVOID* ppNSSArtefactInfoList_accumulate = NULL;
    DWORD  dwTotalNumNSSArtefactsFound = 0;
    DWORD  dwNumNSSArtefactsFound = 0;
    DWORD  dwNSSArtefactInfoLevel = 0;
    PSTR szAttributeList[] =
               {
                 AD_LDAP_NAME_TAG,
                 AD_LDAP_KEYWORDS_TAG,
                 NULL
               };

    LDAPMessage *pMessagePseudo = NULL;
    LDAP *pLd = NULL;
    DWORD dwNumNSSArtefactsWanted = dwMaxNumNSSArtefacts;

    dwNSSArtefactInfoLevel = pEnumState->dwInfoLevel;

    pLd = LsaLdapGetSession(hDirectory);

    BAIL_ON_INVALID_STRING(pEnumState->pszMapName);

    dwError = LsaAllocateStringPrintf(
                    &pszDN,
                    "CN=%s,CN=Maps,%s",
                    pEnumState->pszMapName,
                    pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapEscapeString(
                    &pszEscapedDN,
                    pszDN);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pEnumState->bMorePages){
            dwError = LSA_ERROR_NO_MORE_NSS_ARTEFACTS;
            BAIL_ON_LSA_ERROR(dwError);
    }

    do
    {
        dwError = LsaLdapDirectoryOnePagedSearch(
                       hDirectory,
                       pszEscapedDN,
                       pszQuery,
                       szAttributeList,
                       dwNumNSSArtefactsWanted,
                       &pEnumState->pCookie,
                       LDAP_SCOPE_ONELEVEL,
                       &pMessagePseudo,
                       &pEnumState->bMorePages);
        BAIL_ON_LSA_ERROR(dwError);

        dwCount = ldap_count_entries(
                          pLd,
                          pMessagePseudo);
        if (dwCount < 0) {
           dwError = LSA_ERROR_LDAP_ERROR;
        } else if (dwCount == 0) {
           dwError = LSA_ERROR_NO_MORE_NSS_ARTEFACTS;
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADNonSchemaMarshalNSSArtefactInfoList(
                        hDirectory,
                        pszNetBIOSDomainName,
                        pMessagePseudo,
                        pEnumState->dwMapFlags,
                        dwNSSArtefactInfoLevel,
                        &ppNSSArtefactInfoList,
                        &dwNumNSSArtefactsFound
                        );
        BAIL_ON_LSA_ERROR(dwError);

        dwNumNSSArtefactsWanted -= dwNumNSSArtefactsFound;

        dwError = LsaCoalesceNSSArtefactInfoList(
                        &ppNSSArtefactInfoList,
                        &dwNumNSSArtefactsFound,
                        &ppNSSArtefactInfoList_accumulate,
                        &dwTotalNumNSSArtefactsFound
                        );
        BAIL_ON_LSA_ERROR(dwError);

        if (pMessagePseudo) {
               ldap_msgfree(pMessagePseudo);
               pMessagePseudo = NULL;
        }
    } while (pEnumState->bMorePages && dwNumNSSArtefactsWanted);

    *pppNSSArtefactInfoList = ppNSSArtefactInfoList_accumulate;
    *pdwNumNSSArtefactsFound = dwTotalNumNSSArtefactsFound;

cleanup:

    LSA_SAFE_FREE_STRING(pszDN);
    LSA_SAFE_FREE_STRING(pszEscapedDN);

    return dwError;

error:

    *pppNSSArtefactInfoList = NULL;
    *pdwNumNSSArtefactsFound = 0;

    if (ppNSSArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }

    if (ppNSSArtefactInfoList_accumulate) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList_accumulate, dwTotalNumNSSArtefactsFound);
    }

    if (dwError == LDAP_NO_SUCH_OBJECT)
    {
        dwError = LSA_ERROR_NO_MORE_NSS_ARTEFACTS;
    }

    goto cleanup;
}
