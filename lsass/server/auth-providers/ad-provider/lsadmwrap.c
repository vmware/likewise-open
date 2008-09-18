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

/**
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * @file
 *
 *     lsadmwrap.h
 *
 * @brief
 *
 *     LSASS Domain Manager (LsaDm) Wrapper (Helper) API Implementation
 *
 * @details
 *
 *     This module wraps calls to LsaDm for the convenience of the
 *     AD provider code.
 *
 * @author Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */

#include "adprovider.h"

typedef struct _LSA_DM_WRAP_ENUM_CONTEXT {
    DWORD dwError;
    // Capacity needs to hold dwCount + 1 (for NULL termination)
    DWORD dwCapacity;
    // This is the number of domains being returned.
    DWORD dwCount;
    // NULL-terminated array of strings
    PSTR* ppszDomainNames;
} LSA_DM_WRAP_ENUM_CONTEXT, *PLSA_DM_WRAP_ENUM_CONTEXT;

BOOLEAN
LsaDmWrappEnumExtraForestDomainsCallback(
    IN OPTIONAL PCSTR pszEnumDomainName,
    IN OPTIONAL PVOID pContext,
    IN PLSA_DM_ENUM_DOMAIN_CALLBACK_INFO pDomainInfo
    )
{
    DWORD dwError = 0;
    PLSA_DM_WRAP_ENUM_CONTEXT pEnumContext = (PLSA_DM_WRAP_ENUM_CONTEXT)pContext;
    PSTR* ppszDomainNames = NULL;

    // Find a "two-way across forest trust".  This is two-way trust to an external
    // trust to a domain in another forest or a forest trust.
    if (!(pDomainInfo->dwTrustFlags & NETR_TRUST_FLAG_IN_FOREST) &&
        (pDomainInfo->dwTrustFlags & NETR_TRUST_FLAG_OUTBOUND) && 
        (pDomainInfo->dwTrustFlags & NETR_TRUST_FLAG_INBOUND))
    {
        // We need to make sure that we have enough room for a
        // NULL terminator too.
        if (pEnumContext->dwCapacity < (pEnumContext->dwCount + 2))
        {
            DWORD dwNewCapacity = 0;
            DWORD dwNewSize = 0;
            DWORD dwSize = 0;

            // Note that the first time needs to use at least 2.
            dwNewCapacity = LSA_MAX(2, pEnumContext->dwCapacity + 10);
            dwNewSize = sizeof(ppszDomainNames[0]) * dwNewCapacity;

            dwError = LsaAllocateMemory(dwNewSize, (PVOID*)&ppszDomainNames);
            BAIL_ON_LSA_ERROR(dwError);

            dwSize = sizeof(ppszDomainNames[0]) * pEnumContext->dwCapacity;
            memcpy(ppszDomainNames, pEnumContext->ppszDomainNames, dwSize);

            pEnumContext->dwCapacity = dwNewCapacity;
            LsaFreeMemory(pEnumContext->ppszDomainNames);
            pEnumContext->ppszDomainNames = ppszDomainNames;
            ppszDomainNames = NULL;
        }
        dwError = LsaAllocateString(pDomainInfo->pszDnsDomainName,
                                    &pEnumContext->ppszDomainNames[pEnumContext->dwCount]);
        BAIL_ON_LSA_ERROR(dwError);

        pEnumContext->ppszDomainNames[pEnumContext->dwCount + 1] = NULL;
        pEnumContext->dwCount++;
    }

cleanup:
    LSA_SAFE_FREE_MEMORY(ppszDomainNames);

    pEnumContext->dwError = dwError;
    return dwError ? FALSE : TRUE;
error:
    goto cleanup;
}

// ISSUE-2008/08/15-dalmeida -- The old code looked for
// two-way trusts across forest boundaries (external or forest trust).
// However, this is not necessarily correct.
DWORD
LsaDmWrapEnumExtraForestTrustDomains(
    OUT PSTR** pppszDomainNames,
    OUT PDWORD pdwCount
    )
{
    DWORD dwError = 0;
    LSA_DM_WRAP_ENUM_CONTEXT context = { 0 };

    LsaDmEnumDomains(NULL,
                     LsaDmWrappEnumExtraForestDomainsCallback,
                     &context);
    dwError = context.dwError;
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *pppszDomainNames = context.ppszDomainNames;
    *pdwCount = context.dwCount;

    return dwError;
error:
    LSA_SAFE_FREE_STRING_ARRAY(context.ppszDomainNames);
    context.dwCount = 0;
    goto cleanup;
}

DWORD
LsaDmWrapGetForestName(
    IN PCSTR pszDomainName,
    OUT PSTR* ppszDnsForestName
    )
{
    return LsaDmQueryDomainInfo(pszDomainName,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                ppszDnsForestName,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
}

DWORD
LsaDmWrapGetDomainName(
    IN PCSTR pszDomainName,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName
    )
{
    return LsaDmQueryDomainInfo(pszDomainName,
                                ppszDnsDomainName,
                                ppszNetbiosDomainName,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
}

#define IsSetFlag(Variable, Flags) (((Variable) & (Flags)) != 0)

typedef DWORD LSA_DM_WRAP_CONNECT_DOMAIN_FLAGS;

#define LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_GC           0x00000001
#define LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_DC_INFO      0x00000002
#define LSA_DM_WRAP_CONNECT_DOMAIN_NO_OFFLINE_HACK   0x00000004
#define LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_AUTH         0x00000008

typedef DWORD (*PFLSA_DM_WRAP_CONNECT_CALLBACK)(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    );

static
DWORD
LsaDmWrappConnectDomain(
    IN PCSTR pszDnsDomainName,
    IN LSA_DM_WRAP_CONNECT_DOMAIN_FLAGS dwConnectFlags,
    IN PLWNET_DC_INFO pDcInfo,
    IN PFLSA_DM_WRAP_CONNECT_CALLBACK pfConnectCallback,
    IN OPTIONAL PVOID pContext
    )
{
    DWORD dwError = 0;
    PSTR pszDnsForestName = NULL;
    PCSTR pszDnsDomainOrForestName = pszDnsDomainName;
    PLWNET_DC_INFO pLocalDcInfo = NULL;
    PLWNET_DC_INFO pActualDcInfo = pDcInfo;
    DWORD dwGetDcNameFlags = 0;
    BOOLEAN bIsNetworkError = FALSE;
    BOOLEAN bUseGc = IsSetFlag(dwConnectFlags, LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_GC);
    BOOLEAN bUseDcInfo = IsSetFlag(dwConnectFlags, LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_DC_INFO);

    if (bUseGc)
    {
        dwError = LsaDmWrapGetForestName(pszDnsDomainName,
                                         &pszDnsForestName);
        BAIL_ON_LSA_ERROR(dwError);
        pszDnsDomainOrForestName = pszDnsForestName;
        dwGetDcNameFlags |= DS_GC_SERVER_REQUIRED;
    }

    if (!IsSetFlag(dwConnectFlags, LSA_DM_WRAP_CONNECT_DOMAIN_NO_OFFLINE_HACK) &&
        LsaDmIsDomainOffline(pszDnsDomainOrForestName))
    {
        dwError = LSA_ERROR_DOMAIN_IS_OFFLINE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (IsSetFlag(dwConnectFlags, LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_AUTH))
    {
        dwError = AD_MachineCredentialsCacheInitialize();
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (bUseDcInfo && !pActualDcInfo)
    {
        dwError = LWNetGetDCName(NULL,
                                 pszDnsDomainOrForestName,
                                 NULL,
                                 dwGetDcNameFlags,
                                 &pLocalDcInfo);
        BAIL_ON_LSA_ERROR(dwError);
        pActualDcInfo = pLocalDcInfo;
    }

    dwError = pfConnectCallback(pszDnsDomainOrForestName,
                                pActualDcInfo,
                                pContext,
                                &bIsNetworkError);
    if (!dwError)
    {
        goto cleanup;
    }
    if (!bIsNetworkError)
    {
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (!bUseDcInfo)
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    LWNET_SAFE_FREE_DC_INFO(pLocalDcInfo);
    pActualDcInfo = NULL;
    dwError = LWNetGetDCName(NULL,
                             pszDnsDomainOrForestName,
                             NULL,
                             dwGetDcNameFlags | DS_FORCE_REDISCOVERY,
                             &pLocalDcInfo);
    BAIL_ON_LSA_ERROR(dwError);
    pActualDcInfo = pLocalDcInfo;

    dwError = pfConnectCallback(pszDnsDomainOrForestName,
                                pActualDcInfo,
                                pContext,
                                &bIsNetworkError);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LWNET_SAFE_FREE_DC_INFO(pLocalDcInfo);
    LSA_SAFE_FREE_STRING(pszDnsForestName);
    return dwError;

error:
    if (bIsNetworkError &&
        !IsSetFlag(dwConnectFlags, LSA_DM_WRAP_CONNECT_DOMAIN_NO_OFFLINE_HACK))
    {
        DWORD dwLocalError = LsaDmTransitionOffline(pszDnsDomainOrForestName);
        if (dwLocalError)
        {
            LSA_LOG_DEBUG("Error %d transitioning %s offline",
                          dwLocalError, pszDnsDomainOrForestName);
        }
        dwError = LSA_ERROR_DOMAIN_IS_OFFLINE;
    }
    goto cleanup;
}

///
/// Callback contexts
///

typedef struct _LSA_DM_WRAP_LDAP_OPEN_DIRECORY_CALLBACK_CONTEXT {
    IN DWORD dwFlags;
    OUT HANDLE hDirectory;
} LSA_DM_WRAP_LDAP_OPEN_DIRECORY_CALLBACK_CONTEXT, *PLSA_DM_WRAP_LDAP_OPEN_DIRECORY_CALLBACK_CONTEXT;

typedef struct _LSA_DM_WRAP_LOOKUP_SID_BY_NAME_CALLBACK_CONTEXT {
    IN PCSTR pszName;
    OUT PSTR pszSid;
} LSA_DM_WRAP_LOOKUP_SID_BY_NAME_CALLBACK_CONTEXT, *PLSA_DM_WRAP_LOOKUP_SID_BY_NAME_CALLBACK_CONTEXT;

typedef struct _LSA_DM_WRAP_LOOKUP_NAME_BY_SID_CALLBACK_CONTEXT {
    IN PCSTR pszSid;
    OUT PSTR pszName;
} LSA_DM_WRAP_LOOKUP_NAME_BY_SID_CALLBACK_CONTEXT, *PLSA_DM_WRAP_LOOKUP_NAME_BY_SID_CALLBACK_CONTEXT;

typedef struct _LSA_DM_WRAP_ENUM_DOMAIN_TRUSTS_CALLBACK_CONTEXT {
    IN DWORD dwFlags;
    OUT NetrDomainTrust* pTrusts;
    OUT DWORD dwCount;
} LSA_DM_WRAP_ENUM_DOMAIN_TRUSTS_CALLBACK_CONTEXT, *PLSA_DM_WRAP_ENUM_DOMAIN_TRUSTS_CALLBACK_CONTEXT;

///
/// Callback functions
///

static
DWORD
LsaDmWrappLdapPingTcpCallback(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;

    dwError = LsaLdapPingTcp(pDcInfo->pszDomainControllerAddress, 5);
    *pbIsNetworkError = dwError ? TRUE : FALSE;
    return dwError;
}

static
DWORD
LsaDmWrappLdapOpenDirectoryDomainCallback(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PLSA_DM_WRAP_LDAP_OPEN_DIRECORY_CALLBACK_CONTEXT pCtx = (PLSA_DM_WRAP_LDAP_OPEN_DIRECORY_CALLBACK_CONTEXT) pContext;

    dwError = LsaLdapOpenDirectoryDomain(pszDnsDomainOrForestName,
                                         pCtx->dwFlags,
                                         &pCtx->hDirectory);
    *pbIsNetworkError = dwError ? TRUE : FALSE;
    return dwError;
}

static
DWORD
LsaDmWrappLdapOpenDirectoryGcCallback(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PLSA_DM_WRAP_LDAP_OPEN_DIRECORY_CALLBACK_CONTEXT pCtx = (PLSA_DM_WRAP_LDAP_OPEN_DIRECORY_CALLBACK_CONTEXT) pContext;

    dwError = LsaLdapOpenDirectoryGc(pszDnsDomainOrForestName,
                                     pCtx->dwFlags,
                                     &pCtx->hDirectory);
    *pbIsNetworkError = dwError ? TRUE : FALSE;
    return dwError;
}

static
DWORD
LsaDmWrappLookupSidByNameCallback(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PLSA_DM_WRAP_LOOKUP_SID_BY_NAME_CALLBACK_CONTEXT pCtx = (PLSA_DM_WRAP_LOOKUP_SID_BY_NAME_CALLBACK_CONTEXT) pContext;

    dwError = AD_NetLookupObjectSidByName(pDcInfo->pszDomainControllerName,
                                          pCtx->pszName,
                                          &pCtx->pszSid,
                                          pbIsNetworkError);
    return dwError;
}

static
DWORD
LsaDmWrappLookupNameBySidCallback(
    IN PCSTR     pszDnsDomainOrForestName,
    IN OPTIONAL  PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL  PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    PLSA_DM_WRAP_LOOKUP_NAME_BY_SID_CALLBACK_CONTEXT pCtx = (PLSA_DM_WRAP_LOOKUP_NAME_BY_SID_CALLBACK_CONTEXT) pContext;

    return AD_NetLookupObjectNameBySid(
                    pDcInfo->pszDomainControllerName,
                    pCtx->pszSid,
                    &pCtx->pszName,
                    pbIsNetworkError);
}

static
DWORD
LsaDmWrappDsEnumerateDomainTrustsCallback(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PLSA_DM_WRAP_ENUM_DOMAIN_TRUSTS_CALLBACK_CONTEXT pCtx = (PLSA_DM_WRAP_ENUM_DOMAIN_TRUSTS_CALLBACK_CONTEXT) pContext;

    dwError = AD_DsEnumerateDomainTrusts(pDcInfo->pszDomainControllerName,
                                         pCtx->dwFlags,
                                         &pCtx->pTrusts,
                                         &pCtx->dwCount,
                                         pbIsNetworkError);
    return dwError;
}

///
/// Connect wrap functions
///

DWORD
LsaDmWrapLdapPingTcp(
    IN PCSTR pszDnsDomainName
    )
{
    DWORD dwError = 0;

    dwError = LsaDmWrappConnectDomain(pszDnsDomainName,
                                      LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_DC_INFO |
                                      LSA_DM_WRAP_CONNECT_DOMAIN_NO_OFFLINE_HACK,
                                      NULL,
                                      LsaDmWrappLdapPingTcpCallback,
                                      NULL);
    return dwError;
}

DWORD
LsaDmWrapLdapOpenDirectoryDomain(
    IN PCSTR pszDnsDomainName,
    OUT PHANDLE phDirectory
    )
{
    DWORD dwError = 0;
    LSA_DM_WRAP_LDAP_OPEN_DIRECORY_CALLBACK_CONTEXT context = { 0 };

    if (AD_GetLDAPSignAndSeal())
    {
        context.dwFlags |= LSA_LDAP_OPT_SIGN_AND_SEAL;
    }

    dwError = LsaDmWrappConnectDomain(pszDnsDomainName,
                                      LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_AUTH,
                                      NULL,
                                      LsaDmWrappLdapOpenDirectoryDomainCallback,
                                      &context);

    *phDirectory = context.hDirectory;

    return dwError;
}

DWORD
LsaDmWrapLdapOpenDirectoryGc(
    IN PCSTR pszDnsDomainName,
    OUT PHANDLE phDirectory
    )
{
    DWORD dwError = 0;
    LSA_DM_WRAP_LDAP_OPEN_DIRECORY_CALLBACK_CONTEXT context = { 0 };

    if (AD_GetLDAPSignAndSeal())
    {
        context.dwFlags |= LSA_LDAP_OPT_SIGN_AND_SEAL;
    }

    dwError = LsaDmWrappConnectDomain(pszDnsDomainName,
                                      LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_AUTH |
                                      LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_GC,
                                      NULL,
                                      LsaDmWrappLdapOpenDirectoryGcCallback,
                                      &context);

    *phDirectory = context.hDirectory;

    return dwError;
}

DWORD
LsaDmWrapNetLookupObjectSidByName(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszName,
    OUT PSTR* ppszSid
    )
{
    DWORD dwError = 0;
    LSA_DM_WRAP_LOOKUP_SID_BY_NAME_CALLBACK_CONTEXT context = { 0 };

    context.pszName = pszName;

    dwError = LsaDmWrappConnectDomain(pszDnsDomainName,
                                      LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_AUTH |
                                      LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_DC_INFO,
                                      NULL,
                                      LsaDmWrappLookupSidByNameCallback,
                                      &context);

    *ppszSid = context.pszSid;

    return dwError;
}

DWORD
LsaDmWrapNetLookupNameByObjectSid(
    IN  PCSTR pszDnsDomainName,
    IN  PCSTR pszSid,
    OUT PSTR* ppszName
    )
{
    DWORD dwError = 0;
    LSA_DM_WRAP_LOOKUP_NAME_BY_SID_CALLBACK_CONTEXT context = { 0 };

    context.pszSid = pszSid;

    dwError = LsaDmWrappConnectDomain(pszDnsDomainName,
                                      LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_AUTH |
                                      LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_DC_INFO,
                                      NULL,
                                      LsaDmWrappLookupNameBySidCallback,
                                      &context);

    *ppszName = context.pszName;

    return dwError;
}

DWORD
LsaDmWrapDsEnumerateDomainTrusts(
    IN PCSTR pszDnsDomainName,
    IN DWORD dwFlags,
    OUT NetrDomainTrust** ppTrusts,
    OUT PDWORD pdwCount
    )
{
    DWORD dwError = 0;
    LSA_DM_WRAP_ENUM_DOMAIN_TRUSTS_CALLBACK_CONTEXT context = { 0 };

    context.dwFlags = dwFlags;

    dwError = LsaDmWrappConnectDomain(pszDnsDomainName,
                                      LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_AUTH |
                                      LSA_DM_WRAP_CONNECT_DOMAIN_FLAG_DC_INFO,
                                      NULL,
                                      LsaDmWrappDsEnumerateDomainTrustsCallback,
                                      &context);

    *ppTrusts = context.pTrusts;
    *pdwCount = context.dwCount;

    return dwError;
}

