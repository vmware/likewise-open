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

DWORD
LWNetSrvParseConfigFile(
    PCSTR pszFilePath
    )
{
    DWORD dwError = 0;
    PVOID ptr = NULL;
    dwError = LWNetParseConfigFile(
        pszFilePath,
                LWNET_CFG_OPTION_STRIP_ALL,
                &LWNetSrvCfgStartSection,
                NULL,
                &LWNetSrvCfgNameValuePair,
                NULL,
                &ptr
            );

    return dwError;
}

DWORD
LWNetSrvCfgStartSection(
    PCSTR pszSectionName,
    PVOID pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{

    DWORD dwError = 0;
    BOOLEAN bSkipSection = FALSE;
    BOOLEAN bContinue = TRUE;

    BAIL_ON_INVALID_STRING(pszSectionName);

    if(strcmp(pszSectionName, "cache") != 0)
    {
        bSkipSection = TRUE;
    }
    else 
    {
        bSkipSection = FALSE;
    }
    
    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

cleanup:
    return dwError;

error:
    *pbSkipSection = TRUE;
    goto cleanup;
}

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
    DWORD dwCacheEntryExpiry = 0;
    BOOLEAN bCacheEntryExpiryFound = FALSE;

    BAIL_ON_INVALID_STRING(pszName);
    BAIL_ON_INVALID_STRING(pszValue);

    if(strcmp(pszName, "cache-entry-expiry") == 0)
    {
        dwError = LWNetParseDateString(
                        pszValue,
                        &dwCacheEntryExpiry
                        );
        BAIL_ON_LWNET_ERROR(dwError);

        bCacheEntryExpiryFound = TRUE;
    }

    if (dwCacheEntryExpiry >= gdwLWNetCacheEntryExpirySecsMinimum &&
        dwCacheEntryExpiry <= gdwLWNetCacheEntryExpirySecsMaximum &&
        bCacheEntryExpiryFound)
    {
        dwError = LWNetSetCacheEntryExpirySeconds(dwCacheEntryExpiry);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    *pbContinue = bContinue;

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LWNetSetCacheReaperTimeoutSecs(
    DWORD dwCacheReaperTimeoutSecs
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    if (dwCacheReaperTimeoutSecs < gdwLWNetCacheReaperTimeoutSecsMinimum)
    {
        LWNET_LOG_ERROR("Failed to set CacheReaperTimeoutSecs to %u.  Minimum is %u.",
                        dwCacheReaperTimeoutSecs,
                        gdwLWNetCacheReaperTimeoutSecsMinimum
                        );
        dwError = LWNET_ERROR_INVALID_PARAMETER;
    }

    if (dwCacheReaperTimeoutSecs > gdwLWNetCacheReaperTimeoutSecsMaximum)
    {
        LWNET_LOG_ERROR("Failed to set CacheReaperTimeoutSecs to %u.  Maximum is %u.",
                        dwCacheReaperTimeoutSecs,
                        gdwLWNetCacheReaperTimeoutSecsMaximum
                        );
        dwError = LWNET_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LWNET_ERROR(dwError);

    ENTER_LWNET_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);
 
    gdwLWNetCacheReaperTimeoutSecs = dwCacheReaperTimeoutSecs;

cleanup:

    LEAVE_LWNET_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LWNetGetCacheReaperTimeoutSecs(
    VOID
    )
{
    DWORD dwCacheReaperTimeoutSecs = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_LWNET_GLOBAL_DATA_RW_READER_LOCK(bInLock);
    dwCacheReaperTimeoutSecs = gdwLWNetCacheReaperTimeoutSecs;
    LEAVE_LWNET_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwCacheReaperTimeoutSecs;
}

DWORD
LWNetSetCacheEntryExpirySeconds(
    DWORD dwExpirySecs
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    if (dwExpirySecs < gdwLWNetCacheEntryExpirySecsMinimum)
    {
        LWNET_LOG_ERROR("Failed to set CacheEntryExpiry to %u.  Minimum is %u.",
                        dwExpirySecs,
                        gdwLWNetCacheEntryExpirySecsMinimum);
        dwError = LWNET_ERROR_INVALID_PARAMETER;
    }

    if (dwExpirySecs > gdwLWNetCacheEntryExpirySecsMaximum)
    {
        LWNET_LOG_ERROR("Failed to set CacheEntryExpiry to %u.  Maximum is %u.",
                        dwExpirySecs,
                        gdwLWNetCacheEntryExpirySecsMaximum);
        dwError = LWNET_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LWNET_ERROR(dwError);
 
    ENTER_LWNET_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    gdwLWNetCacheEntryExpirySecs = dwExpirySecs;

cleanup:

    LEAVE_LWNET_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LWNetGetCacheEntryExpirySeconds(
    VOID
    )
{
    DWORD dwResult = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_LWNET_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    dwResult = gdwLWNetCacheEntryExpirySecs;

    LEAVE_LWNET_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwResult;
}

DWORD
LWNetSetCurrentSiteName(
    PCSTR pszCurrentSiteName
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszNewSiteName = NULL;

    if(!IsNullOrEmptyString(pszCurrentSiteName))
    {
        dwError = LWNetAllocateString(
                    pszCurrentSiteName,
                    &pszNewSiteName
                    );
        BAIL_ON_LWNET_ERROR(dwError);
    }

    ENTER_LWNET_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    LWNET_SAFE_FREE_STRING(gpszLWNetCurrentSiteName);
    gpszLWNetCurrentSiteName = pszNewSiteName;
    
cleanup:

    LEAVE_LWNET_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LWNetGetCurrentSiteName(
        PSTR* ppszCurrentSiteName
        )
{
    BOOLEAN bInLock = FALSE;
    PSTR pszCurrentSiteName = NULL;
    DWORD dwError = 0;
    
    ENTER_LWNET_GLOBAL_DATA_RW_READER_LOCK(bInLock);
    
    if(gpszLWNetCurrentSiteName != NULL)
    {
        dwError = LWNetAllocateString(
                    gpszLWNetCurrentSiteName,
                    &pszCurrentSiteName
                    );
        BAIL_ON_LWNET_ERROR(dwError);
    }

    LEAVE_LWNET_GLOBAL_DATA_RW_READER_LOCK(bInLock);
    
    *ppszCurrentSiteName = pszCurrentSiteName;
    
cleanup:
    return dwError;

error:
    LEAVE_LWNET_GLOBAL_DATA_RW_READER_LOCK(bInLock);
    *ppszCurrentSiteName = NULL;
    goto cleanup;
}
