/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        config.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Configuration API (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "api.h"

DWORD
LsaSrvRefreshConfiguration(
    HANDLE hServer
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PSTR pszConfigFilePath = NULL;
    BOOLEAN bUnlockConfigLock = FALSE;
    LSA_SRV_API_CONFIG apiConfig;

    if (pServerState->peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvApiInitConfig(&apiConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvApiGetConfigFilePath(&pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvApiReadConfig(
                    pszConfigFilePath,
                    &apiConfig);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_mutex_lock(&gAPIConfigLock);
    bUnlockConfigLock = TRUE;

    dwError = LsaSrvApiTransferConfigContents(
                    &apiConfig,
                    &gAPIConfig);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_mutex_unlock(&gAPIConfigLock);
    bUnlockConfigLock = FALSE;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnRefreshConfiguration(
                                        hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        LsaSrvCloseProvider(pProvider, hProvider);
        hProvider = (HANDLE)NULL;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszConfigFilePath);

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LsaSrvApiFreeConfigContents(&apiConfig);

    if (bUnlockConfigLock)
    {
        pthread_mutex_unlock(&gAPIConfigLock);
    }

    return(dwError);

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "refresh configuration");

    goto cleanup;

}

DWORD
LsaSrvApiGetConfigFilePath(
    PSTR* ppszConfigFilePath
    )
{
    DWORD dwError = 0;
    PSTR  pszConfigFilePath = NULL;

    pthread_mutex_lock(&gAPIConfigLock);

    BAIL_ON_INVALID_STRING(gpszConfigFilePath);

    dwError = LwAllocateString(
                    gpszConfigFilePath,
                    &pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszConfigFilePath = pszConfigFilePath;

cleanup:

    pthread_mutex_unlock(&gAPIConfigLock);

    return dwError;

error:

    *ppszConfigFilePath = NULL;

    goto cleanup;
}

DWORD
LsaSrvApiInitConfig(
    PLSA_SRV_API_CONFIG pConfig
    )
{
    LsaSrvApiFreeConfigContents(pConfig);

    pConfig->bEnableEventLog = FALSE;
    pConfig->bLogNetworkConnectionEvents = TRUE;

    return 0;
}

DWORD
LsaSrvApiReadConfig(
    PCSTR pszConfigFilePath,
    PLSA_SRV_API_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    LSA_SRV_API_CONFIG apiConfig;

    BAIL_ON_INVALID_STRING(pszConfigFilePath);

    dwError = LsaSrvApiInitConfig(&apiConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaParseConfigFile(
                    pszConfigFilePath,
                    LSA_CFG_OPTION_STRIP_ALL,
                    &LsaSrvApiConfigStartSection,
                    NULL,
                    &LsaSrvApiConfigNameValuePair,
                    NULL,
                    &apiConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvApiTransferConfigContents(
                    &apiConfig,
                    pConfig);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaSrvApiFreeConfigContents(&apiConfig);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvApiConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bSkipSection = FALSE;

    if (LW_IS_NULL_OR_EMPTY_STR(pszSectionName) ||
        (strncasecmp(pszSectionName, "global", sizeof("global")-1)))
    {
        bSkipSection = TRUE;
    }

    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

    return dwError;
}

DWORD
LsaSrvApiConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_CONFIG pConfig = (PLSA_SRV_API_CONFIG)pData;
    LSA_LOG_INFO LogInfo = {};

    BAIL_ON_INVALID_POINTER(pConfig);
    BAIL_ON_INVALID_STRING(pszName);

    if (!strcasecmp(pszName, "enable-eventlog"))
    {
        if (!LW_IS_NULL_OR_EMPTY_STR(pszValue) &&
            (!strcasecmp(pszValue, "true") ||
             !strcasecmp(pszValue, "1") ||
             (*pszValue == 'y') ||
             (*pszValue == 'Y')))
        {
            pConfig->bEnableEventLog = TRUE;
        }
        else
        {
            pConfig->bEnableEventLog = FALSE;
        }
    }
    else if (!strcasecmp(pszName, "log-network-connection-events"))
    {
        if (!LW_IS_NULL_OR_EMPTY_STR(pszValue) &&
            (!strcasecmp(pszValue, "false") ||
             !strcasecmp(pszValue, "0") ||
             (*pszValue == 'n') ||
             (*pszValue == 'N')))
        {
            pConfig->bLogNetworkConnectionEvents = FALSE;
        }
        else
        {
            pConfig->bLogNetworkConnectionEvents = TRUE;
        }
    }
    else if (!strcasecmp(pszName, "log-level"))
    {
        if (!strcasecmp(pszValue, "error"))
            LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_ERROR;
        else if (!strcasecmp(pszValue, "warning"))
            LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_WARNING;
        else if (!strcasecmp(pszValue, "info"))
            LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_INFO;
        else if (!strcasecmp(pszValue, "verbose"))
            LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_VERBOSE;
        else if (!strcasecmp(pszValue, "debug"))
            LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_DEBUG;
        else if (!strcasecmp(pszValue, "trace"))
            LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_TRACE;
        else
        {
            dwError = LW_ERROR_INVALID_LOG_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaLogSetInfo_r(&LogInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pbContinue = TRUE;

cleanup:

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LsaSrvApiTransferConfigContents(
    PLSA_SRV_API_CONFIG pSrc,
    PLSA_SRV_API_CONFIG pDest
    )
{
    LsaSrvApiFreeConfigContents(pDest);

    *pDest = *pSrc;

    LsaSrvApiFreeConfigContents(pSrc);

    return 0;
}

VOID
LsaSrvApiFreeConfigContents(
    PLSA_SRV_API_CONFIG pConfig
    )
{
    // Nothing to free right now
    memset(pConfig, 0, sizeof(*pConfig));
}

BOOLEAN
LsaSrvEventlogEnabled(
    VOID
    )
{
    BOOLEAN bResult = FALSE;

    pthread_mutex_lock(&gAPIConfigLock);

    bResult = gAPIConfig.bEnableEventLog;

    pthread_mutex_unlock(&gAPIConfigLock);

    return bResult;
}

VOID
LsaSrvEnableEventlog(
    BOOLEAN bValue
    )
{
    pthread_mutex_lock(&gAPIConfigLock);

    gAPIConfig.bEnableEventLog = bValue;

    pthread_mutex_unlock(&gAPIConfigLock);
}

BOOLEAN
LsaSrvShouldLogNetworkConnectionEvents(
    VOID
    )
{
    BOOLEAN bResult = TRUE;

    pthread_mutex_lock(&gAPIConfigLock);

    bResult = gAPIConfig.bLogNetworkConnectionEvents;

    pthread_mutex_unlock(&gAPIConfigLock);

    return bResult;
}

VOID
LsaSrvSetLogNetworkConnectionEvents(
    BOOLEAN bValue
    )
{
    pthread_mutex_lock(&gAPIConfigLock);

    gAPIConfig.bLogNetworkConnectionEvents = bValue;

    pthread_mutex_unlock(&gAPIConfigLock);
}


DWORD
LsaSrvSetMachineSid(
    HANDLE hServer,
    PCSTR pszSID
    )
{
    const wchar_t wszDomainFilterFmt[] = L"%ws=%d OR %ws=%d";
    const wchar_t wszAccountFilterFmt[] = L"%ws='%ws' AND (%ws=%d OR %ws=%d)";
    const DWORD dwInt32StrSize = 10;

    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSA_SRV_API_STATE pSrvState = (PLSA_SRV_API_STATE)hServer;
    PWSTR pwszNewDomainSid = NULL;
    PSID pNewDomainSid = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PWSTR pwszDomainFilter = NULL;
    DWORD dwDomainFilterLen = 0;
    PWSTR pwszBase = NULL;
    ULONG ulScope = 0;
    ULONG ulAttributesOnly = 0;
    PDIRECTORY_ENTRY pDomainEntries = NULL;
    PDIRECTORY_ENTRY pDomainEntry = NULL;
    DWORD dwNumDomainEntries = 0;
    DWORD iEntry = 0;
    DWORD dwObjectClass = DIR_OBJECT_CLASS_UNKNOWN;
    PWSTR pwszDomainSid = NULL;
    PSID pDomainSid = NULL;
    PWSTR pwszDomainObjectDN = NULL;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszAccountFilter = NULL;
    DWORD dwAccountFilterLen = 0;
    PDIRECTORY_ENTRY pAccountEntries = NULL;
    PDIRECTORY_ENTRY pAccountEntry = NULL;
    DWORD dwNumAccountEntries = 0;
    PWSTR pwszAccountSid = NULL;
    PSID pAccountSid = NULL;
    PWSTR pwszAccountObjectDN = NULL;
    ULONG ulSidLength = 0;
    ULONG ulRid = 0;
    PSID pNewAccountSid = NULL;
    PWSTR pwszNewAccountSid = NULL;

    WCHAR wszAttrObjectDN[] = DIRECTORY_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectClass[] = DIRECTORY_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectSID[] = DIRECTORY_ATTR_OBJECT_SID;
    WCHAR wszAttrDomainName[] = DIRECTORY_ATTR_DOMAIN_NAME;

    PWSTR wszAttributes[] = {
        &wszAttrObjectClass[0],
        &wszAttrObjectSID[0],
        &wszAttrDomainName[0],
        &wszAttrObjectDN[0],
        NULL
    };

    enum AttrValueIndex {
        ATTR_IDX_OBJECT_SID = 0,
        ATTR_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        }
    };

    DIRECTORY_MOD modObjectSID = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrObjectSID[0],
        1,
        &AttrValues[ATTR_IDX_OBJECT_SID]
    };

    DIRECTORY_MOD mods[ATTR_IDX_SENTINEL + 1];

    if (pSrvState->peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaMbsToWc16s(pszSID,
                            &pwszNewDomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlAllocateSidFromWC16String(&pNewDomainSid,
                                            pwszNewDomainSid);
    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = DirectoryOpen(&hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwDomainFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                         dwInt32StrSize +
                         (sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                         dwInt32StrSize +
                         (sizeof(wszDomainFilterFmt)/
                          sizeof(wszDomainFilterFmt[0])));
    dwError = LwAllocateMemory(dwDomainFilterLen * sizeof(WCHAR),
                                (PVOID*)&pwszDomainFilter);
    BAIL_ON_LSA_ERROR(dwError);

    sw16printfw(pwszDomainFilter, dwDomainFilterLen, wszDomainFilterFmt,
                &wszAttrObjectClass[0],
                DIR_OBJECT_CLASS_DOMAIN,
                &wszAttrObjectClass[0],
                DIR_OBJECT_CLASS_BUILTIN_DOMAIN);

    dwError = DirectorySearch(hDirectory,
                              pwszBase,
                              ulScope,
                              pwszDomainFilter,
                              wszAttributes,
                              ulAttributesOnly,
                              &pDomainEntries,
                              &dwNumDomainEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumDomainEntries != 2)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (iEntry = 0; iEntry < dwNumDomainEntries; iEntry++)
    {
        pDomainEntry = &(pDomainEntries[iEntry]);

        dwError = DirectoryGetEntryAttrValueByName(
                                    pDomainEntry,
                                    wszAttrObjectClass,
                                    DIRECTORY_ATTR_TYPE_INTEGER,
                                    &dwObjectClass);
        BAIL_ON_LSA_ERROR(dwError);

        if (dwObjectClass != DIR_OBJECT_CLASS_DOMAIN)
        {
            continue;
        }

        dwError = DirectoryGetEntryAttrValueByName(
                                    pDomainEntry,
                                    wszAttrObjectSID,
                                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                    &pwszDomainSid);
        BAIL_ON_LSA_ERROR(dwError);

        ntStatus = RtlAllocateSidFromWC16String(&pDomainSid,
                                                pwszDomainSid);
        if (ntStatus != STATUS_SUCCESS)
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = DirectoryGetEntryAttrValueByName(
                                    pDomainEntry,
                                    wszAttrObjectDN,
                                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                    &pwszDomainObjectDN);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetEntryAttrValueByName(
                                    pDomainEntry,
                                    wszAttrDomainName,
                                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                    &pwszDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    memset(&mods[0], 0, sizeof(mods));

    AttrValues[ATTR_IDX_OBJECT_SID].data.pwszStringValue = pwszNewDomainSid;
    mods[0] = modObjectSID;

    dwError = DirectoryModifyObject(hDirectory,
                                    pwszDomainObjectDN,
                                    mods);
    BAIL_ON_LSA_ERROR(dwError);

    dwAccountFilterLen = ((sizeof(wszAttrDomainName)/sizeof(WCHAR) - 1) +
                          wc16slen(pwszDomainName) +
                          (sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                          dwInt32StrSize +
                          (sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                          dwInt32StrSize +
                          (sizeof(wszAccountFilterFmt)/
                           sizeof(wszAccountFilterFmt[0])));
    dwError = LwAllocateMemory(dwAccountFilterLen * sizeof(WCHAR),
                                (PVOID*)&pwszAccountFilter);
    BAIL_ON_LSA_ERROR(dwError);

    sw16printfw(pwszAccountFilter, dwAccountFilterLen, wszAccountFilterFmt,
                &wszAttrDomainName[0],
                pwszDomainName,
                &wszAttrObjectClass[0],
                DIR_OBJECT_CLASS_USER,
                &wszAttrObjectClass[0],
                DIR_OBJECT_CLASS_LOCAL_GROUP);

    dwError = DirectorySearch(hDirectory,
                              pwszBase,
                              ulScope,
                              pwszAccountFilter,
                              wszAttributes,
                              ulAttributesOnly,
                              &pAccountEntries,
                              &dwNumAccountEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumAccountEntries == 0)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (iEntry = 0; iEntry < dwNumAccountEntries; iEntry++)
    {
        pAccountEntry = &(pAccountEntries[iEntry]);

        dwError = DirectoryGetEntryAttrValueByName(
                                     pAccountEntry,
                                     wszAttrObjectSID,
                                     DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                     &pwszAccountSid);
        BAIL_ON_LSA_ERROR(dwError);

        /* Account SID has to be valid ... */
        ntStatus = RtlAllocateSidFromWC16String(&pAccountSid,
                                                pwszAccountSid);
        if (ntStatus != STATUS_SUCCESS)
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        /* ... and it has to be in the same domain as machine SID */
        if (!RtlIsPrefixSid(pDomainSid,
                            pAccountSid))
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        ulSidLength = RtlLengthSid(pAccountSid);
        dwError = LwAllocateMemory(ulSidLength,
                                    (PVOID*)&pNewAccountSid);
        BAIL_ON_LSA_ERROR(dwError);

        ntStatus = RtlCopySid(ulSidLength,
                              pNewAccountSid,
                              pNewDomainSid);
        if (ntStatus)
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        ulRid = pAccountSid->SubAuthority[pAccountSid->SubAuthorityCount - 1];
        ntStatus = RtlAppendRidSid(ulSidLength,
                                   pNewAccountSid,
                                   ulRid);
        if (ntStatus)
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        ntStatus = RtlAllocateWC16StringFromSid(&pwszNewAccountSid,
                                                pNewAccountSid);
        if (ntStatus)
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = DirectoryGetEntryAttrValueByName(
                                     pAccountEntry,
                                     wszAttrObjectDN,
                                     DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                     &pwszAccountObjectDN);
        BAIL_ON_LSA_ERROR(dwError);

        memset(&mods[0], 0, sizeof(mods));

        AttrValues[ATTR_IDX_OBJECT_SID].data.pwszStringValue = pwszNewAccountSid;
        mods[0] = modObjectSID;

        dwError = DirectoryModifyObject(hDirectory,
                                        pwszAccountObjectDN,
                                        mods);
        BAIL_ON_LSA_ERROR(dwError);

        if (pAccountSid)
        {
            RTL_FREE(&pAccountSid);
        }

        if (pNewAccountSid)
        {
            RTL_FREE(&pNewAccountSid);
        }

        if (pwszNewAccountSid)
        {
            LW_SAFE_FREE_MEMORY(pwszNewAccountSid);
            pwszNewAccountSid = NULL;
        }
    }

cleanup:
    if (pDomainEntries)
    {
        DirectoryFreeEntries(pDomainEntries,
                             dwNumDomainEntries);
    }

    if (pAccountEntries)
    {
        DirectoryFreeEntries(pAccountEntries,
                             dwNumAccountEntries);
    }

    if (hDirectory)
    {
        DirectoryClose(hDirectory);
    }

    LW_SAFE_FREE_MEMORY(pwszDomainFilter);
    LW_SAFE_FREE_MEMORY(pwszAccountFilter);
    LW_SAFE_FREE_MEMORY(pwszNewDomainSid);
    LW_SAFE_FREE_MEMORY(pwszNewAccountSid);

    if (pDomainSid)
    {
        RTL_FREE(&pDomainSid);
    }

    if (pNewDomainSid)
    {
        RTL_FREE(&pNewDomainSid);
    }

    if (pAccountSid)
    {
        RTL_FREE(&pAccountSid);
    }

    if (pNewAccountSid)
    {
        RTL_FREE(&pNewAccountSid);
    }

    return dwError;

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "set machine sid (new sid = '%s')", LSA_SAFE_LOG_STRING(pszSID));

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
