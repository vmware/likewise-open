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
 *        lwnet-ldap.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        LDAP API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *
 */
#include "includes.h"

DWORD
LWNetCLdapOpenDirectory(
    IN PCSTR pszServerName,
    OUT PHANDLE phDirectory
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    LDAP * ld = NULL;
    PAD_DIRECTORY_CONTEXT pDirectory = NULL;
    int rc = LDAP_VERSION3;
    PSTR pszURL = NULL;

    if (IsNullOrEmptyString(pszServerName))
    {
        dwError = LWNET_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dwError = LWNetAllocateStringPrintf(&pszURL, "cldap://%s",
                                        pszServerName);
    BAIL_ON_LWNET_ERROR(dwError);

    // ISSUE-2008/07/02-dalmeida -- Error code conversion...
    dwError = ldap_initialize(&ld, pszURL);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &rc);
    if (dwError)
    {
        // ISSUE-2008/07/02-dalmeida -- Error code conversion...
        LWNET_LOG_ERROR("Failed to set LDAP option protocol version");
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = ldap_set_option(ld, LDAP_OPT_REFERRALS, (void *)LDAP_OPT_OFF);
    if (dwError)
    {
        // ISSUE-2008/07/02-dalmeida -- Error code conversion...
        LWNET_LOG_ERROR("Failed to set LDAP option to not follow referrals");
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetAllocateMemory(sizeof(AD_DIRECTORY_CONTEXT), (PVOID *)&pDirectory);
    BAIL_ON_LWNET_ERROR(dwError);

    pDirectory->ld = ld;

error:
    LWNET_SAFE_FREE_STRING(pszURL);
    if (dwError)
    {
        if (pDirectory)
        {
            LWNetLdapCloseDirectory((HANDLE)pDirectory);
            pDirectory = NULL;
        }
    }

    *phDirectory = (HANDLE)pDirectory;

    return dwError;
}



DWORD
LWNetLdapBindDirectoryAnonymous(
    HANDLE hDirectory
    )
{
    DWORD dwError = 0;
    PAD_DIRECTORY_CONTEXT pDirectory = (PAD_DIRECTORY_CONTEXT)hDirectory;
    
    BAIL_ON_INVALID_HANDLE(hDirectory);
    
    dwError = ldap_bind_s(
                    pDirectory->ld,
                    NULL,
                    NULL,
                    LDAP_AUTH_SIMPLE);
    BAIL_ON_LWNET_ERROR(dwError);
    
cleanup:

    return dwError;
    
error:

    LWNET_LOG_ERROR("Failed on LDAP simple bind (Error code: %u)", dwError);
    
    if(pDirectory->ld != NULL)
    {
        ldap_unbind_s(pDirectory->ld);
        pDirectory->ld = NULL;
    }

    goto cleanup;
}

DWORD
LWNetLdapConvertDomainToDN(
    PCSTR pszDomainName,
    PSTR* ppszDomainDN
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    PSTR pszChar = NULL;
    PSTR pszDomainDN = NULL;
    CHAR  pszBuffer[256];

    memset(pszBuffer, 0, sizeof(pszBuffer));
    while ((pszChar = strchr(pszDomainName, '.'))) {
        strcat(pszBuffer,"dc=");
        strncat(pszBuffer, pszDomainName, pszChar - pszDomainName);
        strcat(pszBuffer,",");
        pszDomainName = pszChar+1;
    }
    strcat(pszBuffer, "dc=");
    strcat(pszBuffer, pszDomainName);

    dwError = LWNetAllocateString(pszBuffer, &pszDomainDN);
    BAIL_ON_LWNET_ERROR(dwError);

    *ppszDomainDN = pszDomainDN;

    return(dwError);

error:

    *ppszDomainDN = NULL;
    return(dwError);
}

void
LWNetLdapCloseDirectory(
    HANDLE hDirectory
    )
{
    PAD_DIRECTORY_CONTEXT pDirectory = (PAD_DIRECTORY_CONTEXT)hDirectory;

    if (pDirectory) {
        if(pDirectory->ld)
        {
            ldap_unbind_s(pDirectory->ld);
        }
        LWNetFreeMemory(pDirectory);
    }
    return;
}

DWORD
LWNetLdapReadObject(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    PSTR*  ppszAttributeList,
    LDAPMessage** ppMessage
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    PAD_DIRECTORY_CONTEXT pDirectory = NULL;
    struct timeval timeout = {0};
    LDAPMessage* pMessage = NULL;

    timeout.tv_sec = 15;
    timeout.tv_usec = 0;

    pDirectory = (PAD_DIRECTORY_CONTEXT)hDirectory;

    dwError = ldap_search_st(pDirectory->ld,
                             pszObjectDN,
                             LDAP_SCOPE_BASE,
                             "(objectClass=*)",
                             ppszAttributeList,
                             0,
                             &timeout,
                             &pMessage);
    BAIL_ON_LWNET_ERROR(dwError);
    
    *ppMessage = pMessage;
    
cleanup:

    return(dwError);

error:

    *ppMessage = NULL;
    
    goto cleanup;
}

DWORD
LWNetLdapGetParentDN(
    PCSTR pszObjectDN,
    PSTR* ppszParentDN
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    PSTR pszParentDN = NULL;
    PSTR pComma = NULL;

    if (!pszObjectDN || !*pszObjectDN) {
        dwError = LWNET_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    pComma = strchr(pszObjectDN,',');
    if (!pComma) {
        dwError = LWNET_ERROR_LDAP_NO_PARENT_DN;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    pComma++;

    dwError= LWNetAllocateString(pComma, &pszParentDN);
    BAIL_ON_LWNET_ERROR(dwError);

    *ppszParentDN = pszParentDN;

    return(dwError);

error:

    *ppszParentDN = NULL;
    return(dwError);
}

DWORD
LWNetLdapDirectorySearch(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    int    scope,
    PCSTR  pszQuery,
    PSTR*  ppszAttributeList,
    LDAPMessage** ppMessage
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    PAD_DIRECTORY_CONTEXT pDirectory = (PAD_DIRECTORY_CONTEXT)hDirectory;
    struct timeval timeout = {0};
    LDAPMessage* pMessage = NULL;

    timeout.tv_sec = 15;
    timeout.tv_usec = 0;

    dwError = ldap_search_st(pDirectory->ld,
                             pszObjectDN,
                             scope,
                             pszQuery,
                             ppszAttributeList,
                             0,
                             &timeout,
                             &pMessage);
    if (dwError) {
       if (dwError==LDAP_NO_SUCH_OBJECT) {
          LWNET_LOG_VERBOSE("Caught LDAP_NO_SUCH_OBJECT Error on ldap search");
          goto error;
       }
       if (dwError == LDAP_REFERRAL) {
          LWNET_LOG_ERROR("Caught LDAP_REFERRAL Error on ldap search");
          LWNET_LOG_ERROR("LDAP Search Info: DN: [%s]", IsNullOrEmptyString(pszObjectDN) ? "<null>" : pszObjectDN);
          LWNET_LOG_ERROR("LDAP Search Info: scope: [%d]", scope);
          LWNET_LOG_ERROR("LDAP Search Info: query: [%s]", IsNullOrEmptyString(pszQuery) ? "<null>" : pszQuery);
          if (ppszAttributeList) {
             size_t i;
             for (i = 0; ppszAttributeList[i] != NULL; i++) {
                 LWNET_LOG_ERROR("LDAP Search Info: attribute: [%s]", ppszAttributeList[i]);
             }
          }
          else {
             LWNET_LOG_ERROR("Error: LDAP Search Info: no attributes were specified");
          }
       }
       BAIL_ON_LWNET_ERROR(dwError);
    }
    
    *ppMessage = pMessage;
    
cleanup:

    return(dwError);

error:

    *ppMessage = NULL;
    
    goto cleanup;
}

DWORD
LWNetLdapDirectorySearchEx(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    int    scope,
    PCSTR  pszQuery,
    PSTR*  ppszAttributeList,
    DWORD  dwNumMaxEntries,
    LDAPMessage** ppMessage
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    PAD_DIRECTORY_CONTEXT pDirectory = (PAD_DIRECTORY_CONTEXT)hDirectory;
    struct timeval timeout = {0};
    LDAPMessage* pMessage = NULL;

    timeout.tv_sec = 15;
    timeout.tv_usec = 0;

    dwError = ldap_search_ext_s(
                    pDirectory->ld,
                    pszObjectDN,
                    scope,
                    pszQuery,
                    ppszAttributeList,
                    0,
                    NULL,
                    NULL,
                    &timeout,
                    dwNumMaxEntries,
                    &pMessage);
    if (dwError) {
       if (dwError==LDAP_NO_SUCH_OBJECT) {
          LWNET_LOG_VERBOSE("Caught LDAP_NO_SUCH_OBJECT Error on ldap search");
          goto error;
       }
       if (dwError == LDAP_REFERRAL) {
          LWNET_LOG_ERROR("Caught LDAP_REFERRAL Error on ldap search");
          LWNET_LOG_ERROR("LDAP Search Info: DN: [%s]", IsNullOrEmptyString(pszObjectDN) ? "<null>" : pszObjectDN);
          LWNET_LOG_ERROR("LDAP Search Info: scope: [%d]", scope);
          LWNET_LOG_ERROR("LDAP Search Info: query: [%s]", IsNullOrEmptyString(pszQuery) ? "<null>" : pszQuery);
          if (ppszAttributeList) {
             size_t i;
             for (i = 0; ppszAttributeList[i] != NULL; i++) {
                 LWNET_LOG_ERROR("LDAP Search Info: attribute: [%s]", ppszAttributeList[i]);
             }
          }
          else {
             LWNET_LOG_ERROR("Error: LDAP Search Info: no attributes were specified");
          }
       }
       BAIL_ON_LWNET_ERROR(dwError);
    }
    
    *ppMessage = pMessage;
    
cleanup:

    return(dwError);

error:

    *ppMessage = NULL;
    
    goto cleanup;
}

LDAPMessage*
LWNetLdapFirstEntry(
    HANDLE hDirectory,
    LDAPMessage* pMessage
    )
{
    PAD_DIRECTORY_CONTEXT pDirectory = NULL;

    pDirectory = (PAD_DIRECTORY_CONTEXT)hDirectory;

    return ldap_first_entry(pDirectory->ld, pMessage);
}

LDAPMessage*
LWNetLdapNextEntry(
    HANDLE hDirectory,
    LDAPMessage* pMessage
    )
{
    PAD_DIRECTORY_CONTEXT pDirectory = NULL;

    pDirectory = (PAD_DIRECTORY_CONTEXT)hDirectory;

    return ldap_next_entry(pDirectory->ld, pMessage);
}

LDAP *
LWNetLdapGetSession(
    HANDLE hDirectory
    )
{
    PAD_DIRECTORY_CONTEXT pDirectory = (PAD_DIRECTORY_CONTEXT)hDirectory;
    return(pDirectory->ld);
}

DWORD
LWNetLdapGetBytes(
        HANDLE hDirectory,
        LDAPMessage* pMessage,
        PSTR pszFieldName,
        PBYTE* ppszByteValue,
        PDWORD pszByteLen        
        )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    PAD_DIRECTORY_CONTEXT pDirectory = NULL;
    struct berval **ppszValues = NULL;
    PBYTE pszByteValue = NULL;
    DWORD szByteLen = 0;

    pDirectory = (PAD_DIRECTORY_CONTEXT)hDirectory;
    
    ppszValues = ldap_get_values_len(pDirectory->ld, pMessage, pszFieldName);
    
    if (ppszValues && ppszValues[0]){
        if (ppszValues[0]->bv_len != 0){
            LWNetAllocateMemory(
                    sizeof(BYTE) * ppszValues[0]->bv_len,
                    (PVOID *)&pszByteValue);
            memcpy (pszByteValue, ppszValues[0]->bv_val, ppszValues[0]->bv_len * sizeof (BYTE)); 
            szByteLen = ppszValues[0]->bv_len;
        }
    }
    
    *ppszByteValue = pszByteValue;
    *pszByteLen = szByteLen;
    
    if (ppszValues) {
            ldap_value_free_len(ppszValues);
    }

    return dwError;
}


DWORD
LWNetLdapGetString(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PCSTR pszFieldName,
    PSTR* ppszValue
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    PAD_DIRECTORY_CONTEXT pDirectory = NULL;
    PSTR *ppszValues = NULL;
    PSTR pszValue = NULL;

    pDirectory = (PAD_DIRECTORY_CONTEXT)hDirectory;

    ppszValues = (PSTR*)ldap_get_values(pDirectory->ld, pMessage, pszFieldName);
    if (ppszValues && ppszValues[0]) {
        dwError = LWNetAllocateString(ppszValues[0], &pszValue);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    *ppszValue = pszValue;
    
cleanup:
    if (ppszValues) {
        ldap_value_free(ppszValues);
    }
    return dwError;
    
error:
    *ppszValue = NULL;
    
    if (pszValue){
        LWNET_SAFE_FREE_STRING(pszValue);
    }
    
    goto cleanup;    
}

DWORD
LWNetLdapGetUInt32(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PCSTR pszFieldName,
    PDWORD pdwValue
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    PSTR pszValue = NULL;

    dwError = LWNetLdapGetString(hDirectory, pMessage, pszFieldName, &pszValue);
    BAIL_ON_LWNET_ERROR(dwError);

    if (pszValue) {
        *pdwValue = atoi(pszValue);
    }

error:

    if (pszValue) {
        LWNetFreeString(pszValue);
    }

    return dwError;
}

DWORD
LWNetLdapGetStrings(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PCSTR pszFieldName,
    PSTR** pppszValues,
    PDWORD pdwNumValues
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    PAD_DIRECTORY_CONTEXT pDirectory = NULL;
    PSTR *ppszLDAPValues = NULL;
    PSTR *ppszValues = NULL;
    PSTR pszValue = NULL;
    DWORD dwNumValues = 0;
    int   iValue = 0;

    pDirectory = (PAD_DIRECTORY_CONTEXT)hDirectory;

    ppszLDAPValues = (PSTR*)ldap_get_values(pDirectory->ld, pMessage, pszFieldName);
    if (ppszLDAPValues) {
        dwNumValues = ldap_count_values(ppszLDAPValues);
        if (dwNumValues < 0) {

            dwError = LWNET_ERROR_LDAP_ERROR;
            BAIL_ON_LWNET_ERROR(dwError);

        } else if (dwNumValues > 0) {

            dwError = LWNetAllocateMemory((dwNumValues+1)*sizeof(PSTR),
                                       (PVOID*)&ppszValues);
            BAIL_ON_LWNET_ERROR(dwError);

            for (iValue = 0; iValue < dwNumValues; iValue++) {

                dwError = LWNetAllocateString(ppszLDAPValues[iValue], &pszValue);
                BAIL_ON_LWNET_ERROR(dwError);

                ppszValues[iValue] = pszValue;

                pszValue = NULL;
            }
            ppszValues[iValue] = NULL;
        }
    }

    if (ppszLDAPValues) {
        ldap_value_free(ppszLDAPValues);
    }

    *pppszValues = ppszValues;
    *pdwNumValues = dwNumValues;

    return dwError;

error:

    if (ppszValues && dwNumValues > 0) {
        for (iValue = 0; iValue < dwNumValues; iValue++) {
            if (ppszValues[iValue]) {
                LWNetFreeString(ppszValues[iValue]);
            }
        }
        LWNetFreeMemory(ppszValues);
    }

    if (ppszLDAPValues) {
        ldap_value_free(ppszLDAPValues);
    }

    *pppszValues = NULL;
    *pdwNumValues = 0;

    return dwError;
}

