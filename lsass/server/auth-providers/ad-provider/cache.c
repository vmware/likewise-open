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
 *        cache.c
 *
 * Abstract:
 *
 *        Caching for AD Provider Database Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */

#include "adprovider.h"

DWORD
ADInitCacheReaper(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = pthread_create(&gCacheReaperThread,
                             NULL,
                             ADReapCache,
                             NULL);
    BAIL_ON_LSA_ERROR(dwError);

    gpCacheReaperThread = &gCacheReaperThread;

cleanup:

    return dwError;

error:

    gpCacheReaperThread = NULL;

    goto cleanup;
}

PVOID
ADReapCache(
    PVOID pData
    )
{
    DWORD dwError = 0;
    DWORD dwCacheReaperTimeoutSecs = 0;
    struct timespec timeout = {0, 0};

    LSA_LOG_INFO("Cache Reaper starting");
        
    pthread_mutex_lock(&gCacheReaperThreadLock);
    
    do
    {
        dwCacheReaperTimeoutSecs = AD_GetCacheReaperTimeoutSecs();

        timeout.tv_sec = time(NULL) + dwCacheReaperTimeoutSecs;
        timeout.tv_nsec = 0;
        
        dwError = pthread_cond_timedwait(&gCacheReaperThreadCondition,
                                         &gCacheReaperThreadLock,
                                         &timeout);
        
        if (ADProviderIsShuttingDown())
           break;

        if (dwError) {
           if (dwError == ETIMEDOUT) {
              dwError = 0;
           } else {
              BAIL_ON_LSA_ERROR(dwError);
           }
        }
        
    } while (1);
    
cleanup:

    pthread_mutex_unlock(&gCacheReaperThreadLock);
    
    LSA_LOG_INFO("Cache Reaper stopping");

    return NULL;

error:

    LSA_LOG_ERROR("Cache Reaper exiting due to error [code: %ld]", dwError);

    goto cleanup;
}

DWORD
ADShutdownCacheReaper(
    VOID
    )
{
    DWORD dwError = 0;
    
    if (gpCacheReaperThread) {
        
        pthread_mutex_lock(&gCacheReaperThreadLock);
        pthread_cond_signal(&gCacheReaperThreadCondition);
        pthread_mutex_unlock(&gCacheReaperThreadLock);
        
        dwError = pthread_cancel(gCacheReaperThread);
        if (ESRCH == dwError)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);
        
        pthread_join(gCacheReaperThread, NULL);
        gpCacheReaperThread = NULL;
    }

cleanup:
    
    return dwError;
    
error:

    goto cleanup;
}
