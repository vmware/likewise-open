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
 *        lwnet-server-cfg.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#include "includes.h"

//
// Internal Module Globals
//

typedef struct _LWNET_SERVER_CONFIG {
    PSTR pszPluginPath;
    DWORD dwPingAgainTimeoutSeconds;
    DWORD dwNegativeCacheTimeoutSeconds;
    DWORD dwWritableRediscoveryTimeoutSeconds;
    DWORD dwWritableTimestampMinimumChangeSeconds;
} LWNET_SERVER_CONFIG, *PLWNET_SERVER_CONFIG;

#define LWNET_PING_AGAIN_TIMEOUT_SECONDS (15 * 60)
#define LWNET_NEGATIVE_CACHE_TIMEOUT_SECONDS (1 * 60)

#define LWNET_WRITABLE_REDISCOVERY_TIMEOUT_SECONDS (30 * 60)
#define LWNET_WRITABLE_TIMESTAMP_MINIMUM_CHANGE_SECONDS (0 * 60)

LWNET_SERVER_CONFIG gLWNetServerConfig = {
    .dwPingAgainTimeoutSeconds = LWNET_PING_AGAIN_TIMEOUT_SECONDS,
    .dwNegativeCacheTimeoutSeconds = LWNET_NEGATIVE_CACHE_TIMEOUT_SECONDS,
    .dwWritableRediscoveryTimeoutSeconds = LWNET_WRITABLE_REDISCOVERY_TIMEOUT_SECONDS,
    .dwWritableTimestampMinimumChangeSeconds = LWNET_WRITABLE_TIMESTAMP_MINIMUM_CHANGE_SECONDS,
};

//
// Local Prototypes
//

static
DWORD
LWNetSrvCfgStartSection(
    PCSTR pszSectionName,
    PVOID pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    );

static
DWORD
LWNetSrvCfgNameValuePair(
    PCSTR pszName,
    PCSTR pszValue,
    PVOID pData,
    PBOOLEAN pbContinue
    );

//
// Implementation
//

DWORD
LWNetSrvParseConfigFile(
    PCSTR pszFilePath
    )
{
    DWORD dwError = 0;

    dwError = LWNetParseConfigFile(
                    pszFilePath,
                    LWNET_CFG_OPTION_STRIP_ALL,
                    &LWNetSrvCfgStartSection,
                    NULL,
                    &LWNetSrvCfgNameValuePair,
                    NULL,
                    NULL);

    return dwError;
}

static
DWORD
LWNetSrvCfgStartSection(
    PCSTR pszSectionName,
    PVOID pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{

    DWORD dwError = 0;
    BOOLEAN bSkipSection = TRUE;
    BOOLEAN bContinue = TRUE;

    BAIL_ON_INVALID_STRING(pszSectionName);

    if (!strcmp(pszSectionName, "netlogond"))
    {
        bSkipSection = FALSE;
    }
    
cleanup:
    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

    return dwError;

error:
    bSkipSection = TRUE;
    bContinue = TRUE;

    goto cleanup;
}

static
DWORD
LWNetSrvCfgNameValuePair(
    PCSTR pszName,
    PCSTR pszValue,
    PVOID pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;

    BAIL_ON_INVALID_STRING(pszName);
    BAIL_ON_INVALID_STRING(pszValue);

    if (!strcmp(pszName, "plugin-path"))
    {
        LWNET_SAFE_FREE_STRING(gLWNetServerConfig.pszPluginPath);
        if (!IsNullOrEmptyString(pszValue))
        {
            dwError = LWNetAllocateString(pszValue, &gLWNetServerConfig.pszPluginPath);
            BAIL_ON_LWNET_ERROR(dwError);
        }
    }
    else if (!strcmp(pszName, "ping-again-timeout"))
    {
        if (!IsNullOrEmptyString(pszValue))
        {
            dwError = LWNetParseDateString(pszValue, &gLWNetServerConfig.dwPingAgainTimeoutSeconds);
            BAIL_ON_LWNET_ERROR(dwError);
        }
    }
    else if (!strcmp(pszName, "negative-cache-timeout"))
    {
        if (!IsNullOrEmptyString(pszValue))
        {
            dwError = LWNetParseDateString(pszValue, &gLWNetServerConfig.dwNegativeCacheTimeoutSeconds);
            BAIL_ON_LWNET_ERROR(dwError);
        }
    }
    else if (!strcmp(pszName, "writable-rediscovery-timeout"))
    {
        if (!IsNullOrEmptyString(pszValue))
        {
            dwError = LWNetParseDateString(pszValue, &gLWNetServerConfig.dwWritableRediscoveryTimeoutSeconds);
            BAIL_ON_LWNET_ERROR(dwError);
        }
    }
    else if (!strcmp(pszName, "writable-timestamp-minimum-change"))
    {
        if (!IsNullOrEmptyString(pszValue))
        {
            dwError = LWNetParseDateString(pszValue, &gLWNetServerConfig.dwWritableTimestampMinimumChangeSeconds);
            BAIL_ON_LWNET_ERROR(dwError);
        }
    }

cleanup:
    *pbContinue = bContinue;

    return dwError;

error:
    goto cleanup;
}

PCSTR
LWNetConfigGetPluginPath(
    VOID
    )
{
    return gLWNetServerConfig.pszPluginPath;
}

DWORD
LWNetConfigGetPingAgainTimeoutSeconds(
    VOID
    )
{
    return gLWNetServerConfig.dwPingAgainTimeoutSeconds;
}

DWORD
LWNetConfigGetNegativeCacheTimeoutSeconds(
    VOID
    )
{
    return gLWNetServerConfig.dwNegativeCacheTimeoutSeconds;
}

DWORD
LWNetConfigGetWritableRediscoveryTimeoutSeconds(
    VOID
    )
{
    return gLWNetServerConfig.dwWritableRediscoveryTimeoutSeconds;
}
DWORD
LWNetConfigGetWritableTimestampMinimumChangeSeconds(
    VOID
    )
{
    return gLWNetServerConfig.dwWritableTimestampMinimumChangeSeconds;
}
