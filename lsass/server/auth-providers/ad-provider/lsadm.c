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
 *     lsadm.c
 *
 * @brief
 *
 *     LSASS Domain Manager (LsaDm) Implementation
 *
 * @details
 *
 *     This module keeps track of the state of each domain.  In addition
 *     to keeping track of domain names, SIDs, trust info, and affinity,
 *     it also keeps track of which domains are considered unreachable
 *     (and thus "offline").  A thread will try to transition each offline
 *     domain back to online by periodically checking the reachability
 *     of offline domains.
 *
 * @author Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */

#include "adprovider.h"
#include "lsadm_p.h"

///
/// LSASS offline state.
///
static LSA_DM_STATE_HANDLE gLsaDmState;

DWORD
LsaDmInitialize(
    IN BOOLEAN bIsOfflineBehaviorEnabled,
    IN DWORD dwCheckOnlineSeconds
    )
{
    DWORD dwError = 0;
    LSA_DM_STATE_HANDLE pState = NULL;

    dwError = LsaDmpStateCreate(&pState,
                                bIsOfflineBehaviorEnabled,
                                dwCheckOnlineSeconds);
    BAIL_ON_LSA_ERROR(dwError);

    if (gLsaDmState)
    {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    gLsaDmState = pState;
    pState = NULL;
    dwError = 0;

cleanup:
    if (pState)
    {
        LsaDmpStateDestroy(pState);
    }

    return dwError;

error:
    goto cleanup;
}

VOID
LsaDmCleanup(
    )
{
    if (gLsaDmState)
    {
        LsaDmpStateDestroy(gLsaDmState);
        gLsaDmState = NULL;
    }
}

DWORD
LsaDmQueryState(
    OUT OPTIONAL PDWORD pdwCheckOnlineSeconds,
    OUT OPTIONAL PLSA_DM_STATE_FLAGS pStateFlags
    )
{
    return LsaDmpQueryState(gLsaDmState, pdwCheckOnlineSeconds, pStateFlags);
}

DWORD
LsaDmSetState(
    IN OPTIONAL PDWORD pdwCheckOnlineSeconds,
    IN OPTIONAL PBOOLEAN pbIsOfflineBehaviorEnabled
    )
{
    return LsaDmpSetState(gLsaDmState, pdwCheckOnlineSeconds, pbIsOfflineBehaviorEnabled);
}

VOID
LsaDmMediaSenseOffline(
    VOID
    )
{
    LsaDmpMediaSenseOffline(gLsaDmState);
}

VOID
LsaDmMediaSenseOnline(
    VOID
    )
{
    LsaDmpMediaSenseOnline(gLsaDmState);
}

DWORD
LsaDmAddTrustedDomain(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PSID pDomainSid,
    IN uuid_t* pDomainGuid,
    IN PCSTR pszTrusteeDnsDomainName,
    IN DWORD dwTrustFlags,
    IN DWORD dwTrustType,
    IN DWORD dwTrustAttributes,
    IN OPTIONAL PCSTR pszDnsForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo
    )
{
    return LsaDmpAddTrustedDomain(gLsaDmState,
                                  pszDnsDomainName,
                                  pszNetbiosDomainName,
                                  pDomainSid,
                                  pDomainGuid,
                                  pszTrusteeDnsDomainName,
                                  dwTrustFlags,
                                  dwTrustType,
                                  dwTrustAttributes,
                                  pszDnsForestName,
                                  pDcInfo);
}

BOOLEAN
LsaDmIsDomainPresent(
    IN PCSTR pszDomainName
    )
{
    return LsaDmpIsDomainPresent(gLsaDmState, pszDomainName);
}

DWORD
LsaDmEnumDomainNames(
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_FILTER_CALLBACK pfFilterCallback,
    IN OPTIONAL PVOID pFilterContext,
    OUT PSTR** pppszDomainNames,
    OUT OPTIONAL PDWORD pdwCount
    )
{
    return LsaDmpEnumDomainNames(gLsaDmState,
                                 pfFilterCallback,
                                 pFilterContext,
                                 pppszDomainNames,
                                 pdwCount);
}

DWORD
LsaDmEnumDomainInfo(
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_FILTER_CALLBACK pfFilterCallback,
    IN OPTIONAL PVOID pFilterContext,
    OUT PLSA_DM_ENUM_DOMAIN_INFO** pppDomainInfo,
    OUT OPTIONAL PDWORD pdwCount
    )
{
    return LsaDmpEnumDomainInfo(gLsaDmState,
                                pfFilterCallback,
                                pFilterContext,
                                pppDomainInfo,
                                pdwCount);
}

DWORD
LsaDmQueryDomainInfo(
    IN PCSTR pszDomainName,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName,
    OUT OPTIONAL PSID* ppSid,
    OUT OPTIONAL uuid_t* pGuid,
    OUT OPTIONAL PSTR* ppszTrusteeDnsDomainName,
    OUT OPTIONAL PDWORD pdwTrustFlags,
    OUT OPTIONAL PDWORD pdwTrustType,
    OUT OPTIONAL PDWORD pdwTrustAttributes,
    OUT OPTIONAL PSTR* ppszForestName,
    OUT OPTIONAL PSTR* ppszClientSiteName,
    OUT OPTIONAL PLSA_DM_DOMAIN_FLAGS pFlags,
    OUT OPTIONAL PLSA_DM_DC_INFO* ppDcInfo,
    OUT OPTIONAL PLSA_DM_DC_INFO* ppGcInfo
    )
{
    return LsaDmpQueryDomainInfo(gLsaDmState,
                                 pszDomainName,
                                 ppszDnsDomainName,
                                 ppszNetbiosDomainName,
                                 ppSid,
                                 pGuid,
                                 ppszTrusteeDnsDomainName,
                                 pdwTrustFlags,
                                 pdwTrustType,
                                 pdwTrustAttributes,
                                 ppszForestName,
                                 ppszClientSiteName,
                                 pFlags,
                                 ppDcInfo,
                                 ppGcInfo);
}

VOID
LsaDmFreeDcInfo(
    IN OUT PLSA_DM_DC_INFO pDcInfo
    )
{
    if (pDcInfo)
    {
        LSA_SAFE_FREE_STRING(pDcInfo->pszName);
        LSA_SAFE_FREE_STRING(pDcInfo->pszAddress);
        LSA_SAFE_FREE_STRING(pDcInfo->pszSiteName);
        LsaFreeMemory(pDcInfo);
    }
}

DWORD
LsaDmSetDomainDcInfo(
    IN PCSTR pszDomainName,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    return LsaDmpDomainSetDcInfoByName(gLsaDmState, pszDomainName, pDcInfo);
}

DWORD
LsaDmSetDomainGcInfo(
    IN PCSTR pszDomainName,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    return LsaDmpDomainSetGcInfoByName(gLsaDmState, pszDomainName, pDcInfo);
}

DWORD
LsaDmSetForceOfflineState(
    IN OPTIONAL PCSTR pszDomainName,
    IN BOOLEAN bIsSet
    )
{
    return LsaDmpSetForceOfflineState(gLsaDmState, pszDomainName, bIsSet);
}

DWORD
LsaDmTransitionOffline(
    IN PCSTR pszDomainName
    )
{
    return LsaDmpTransitionOffline(gLsaDmState, pszDomainName);
}

DWORD
LsaDmTransitionOnline(
    IN PCSTR pszDomainName
    )
{
    return LsaDmpTransitionOnline(gLsaDmState, pszDomainName);
}

BOOLEAN
LsaDmIsDomainOffline(
    IN OPTIONAL PCSTR pszDomainName
    )
{
    return LsaDmpIsDomainOffline(gLsaDmState, pszDomainName);
}

DWORD
LsaDmDetectTransitionOnline(
    IN OPTIONAL PCSTR pszDomainName
    )
{
    return LsaDmpDetectTransitionOnline(gLsaDmState, pszDomainName);
}

VOID
LsaDmTriggerOnlindeDetectionThread(
    )
{
    LsaDmpTriggerOnlindeDetectionThread(gLsaDmState);
}

BOOLEAN
LsaDmIsSpecificDomainNameMatch(
    IN PCSTR pszDomainNameQuery,
    IN PCSTR pszDomainName
    )
{
    BOOLEAN bIsMatch = FALSE;

    if (pszDomainName &&
        !strcasecmp(pszDomainNameQuery, pszDomainName))
    {
        bIsMatch = TRUE;
    }

    return bIsMatch;
}

BOOLEAN
LsaDmIsEitherDomainNameMatch(
    IN PCSTR pszDomainNameQuery,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName
    )
{
    BOOLEAN bIsMatch = FALSE;

    if (LsaDmIsSpecificDomainNameMatch(pszDomainNameQuery, pszDnsDomainName) ||
        LsaDmIsSpecificDomainNameMatch(pszDomainNameQuery, pszNetbiosDomainName))
    {
        bIsMatch = TRUE;
    }

    return bIsMatch;
}

BOOLEAN
LsaDmIsNetbiosDomainName(
    IN PCSTR pszDomainName
    )
{
    BOOLEAN bIsValid = FALSE;
    char* dot = strchr(pszDomainName, '.');
    if (!dot)
    {
        // Must not have dot.
        bIsValid = TRUE;
    }
    return bIsValid;
}

DWORD
LsaDmDuplicateConstEnumDomainInfo(
    IN PLSA_DM_CONST_ENUM_DOMAIN_INFO pSrc,
    OUT PLSA_DM_ENUM_DOMAIN_INFO* ppDest
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PLSA_DM_ENUM_DOMAIN_INFO pDest = NULL;

    dwError = LsaAllocateMemory(sizeof(*pDest), (PVOID*)&pDest);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(
                pSrc->pszDnsDomainName,
                &pDest->pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(
                pSrc->pszNetbiosDomainName,
                &pDest->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmpDuplicateSid(
                &pDest->pSid,
                pSrc->pSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                sizeof(*pDest->pGuid),
                (PVOID *)&pDest->pGuid);
    BAIL_ON_LSA_ERROR(dwError);
    memcpy(pDest->pGuid, pSrc->pGuid, sizeof(*pSrc->pGuid));

    dwError = LsaStrDupOrNull(
                pSrc->pszTrusteeDnsDomainName,
                &pDest->pszTrusteeDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    pDest->dwTrustFlags = pSrc->dwTrustFlags;
    pDest->dwTrustType = pSrc->dwTrustType;
    pDest->dwTrustAttributes = pSrc->dwTrustAttributes;

    dwError = LsaAllocateString(
                pSrc->pszForestName,
                &pDest->pszForestName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaStrDupOrNull(
                pSrc->pszClientSiteName,
                &pDest->pszClientSiteName);
    BAIL_ON_LSA_ERROR(dwError);

    pDest->Flags = pSrc->Flags;

    // ISSUE-2008/09/10-dalmeida -- Never duplicate DC info (for now, at least).
    // We currently never populate this information.
    pDest->DcInfo = NULL;
    pDest->GcInfo = NULL;

    *ppDest = pDest;

cleanup:
    return dwError;

error:
    if (pDest != NULL)
    {
        LsaDmFreeEnumDomainInfo(pDest);
    }

    *ppDest = NULL;
    goto cleanup;
}

VOID
LsaDmFreeEnumDomainInfo(
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
            LsaDmFreeDcInfo(pDomainInfo->DcInfo);
        }
        if (pDomainInfo->GcInfo)
        {
            LsaDmFreeDcInfo(pDomainInfo->GcInfo);
        }
        LsaFreeMemory(pDomainInfo);
    }
}

VOID
LsaDmFreeEnumDomainInfoArray(
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo
    )
{
    if (ppDomainInfo)
    {
        DWORD dwIndex;
        for (dwIndex = 0; ppDomainInfo[dwIndex]; dwIndex++)
        {
            LsaDmFreeEnumDomainInfo(ppDomainInfo[dwIndex]);
        }
        LsaFreeMemory(ppDomainInfo);
    }
}

