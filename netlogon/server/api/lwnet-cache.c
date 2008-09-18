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
 *        lwnet-cache.c
 *
 * Abstract:
 *
 *        Likewise Netlogon
 * 
 *        
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */

#include "includes.h"

DWORD
LWNetInitCacheReaper(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = pthread_create(&gLWNetCacheReaperThread,
                             NULL,
                             LWNetReapCache,
                             NULL);
    BAIL_ON_LWNET_ERROR(dwError);

    gpLWNetCacheReaperThread = &gLWNetCacheReaperThread;

cleanup:

    return dwError;

error:

    gpLWNetCacheReaperThread = NULL;

    goto cleanup;
}

PVOID
LWNetReapCache(
    PVOID pData
    )
{
    DWORD dwError = 0;
    DWORD dwLWNetCacheReaperTimeoutSecs = 0;
    struct timespec timeout = {0, 0};
    DWORD dwMaxCacheEntryAge = 0;

    LWNET_LOG_INFO("LWNet Cache Reaper starting");
        
    pthread_mutex_lock(&gLWNetCacheReaperThreadLock);
    
    do
    {
        dwLWNetCacheReaperTimeoutSecs = LWNetGetCacheReaperTimeoutSecs();

        timeout.tv_sec = time(NULL) + dwLWNetCacheReaperTimeoutSecs;
        timeout.tv_nsec = 0;
        
        dwError = pthread_cond_timedwait(&gLWNetCacheReaperThreadCondition,
                                         &gLWNetCacheReaperThreadLock,
                                         &timeout);
        if (dwError == ETIMEDOUT) 
        {
            dwError = 0;
        } 
        BAIL_ON_LWNET_ERROR(dwError);

        dwMaxCacheEntryAge = LWNetGetCacheEntryExpirySeconds();

        dwError = LWNetCacheScavenge(dwMaxCacheEntryAge, 0);
        BAIL_ON_LWNET_ERROR(dwError);

    } while (1);
    
cleanup:

    pthread_mutex_unlock(&gLWNetCacheReaperThreadLock);
    
    LWNET_LOG_INFO("LWNet Cache Reaper stopping");

    return NULL;

error:

    LWNET_LOG_ERROR("LWNetCache Reaper exiting due to error [code: %ld]", dwError);

    goto cleanup;
}

DWORD
LWNetShutdownCacheReaper(
    VOID
    )
{
    DWORD dwError = 0;
    
    if (gpLWNetCacheReaperThread) {
        
        pthread_mutex_lock(&gLWNetCacheReaperThreadLock);
        pthread_cond_signal(&gLWNetCacheReaperThreadCondition);
        pthread_mutex_unlock(&gLWNetCacheReaperThreadLock);
        
        dwError = pthread_cancel(gLWNetCacheReaperThread);
        BAIL_ON_LWNET_ERROR(dwError);
        
        pthread_join(gLWNetCacheReaperThread, NULL);
        gpLWNetCacheReaperThread = NULL;
    }

cleanup:
    
    return dwError;
    
error:

    goto cleanup;
}
