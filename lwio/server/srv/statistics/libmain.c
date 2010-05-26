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
 *        libmain.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Statistics Logging Module
 *
 *        Module Entry Points
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
SrvStatsLoadProvider(
    VOID
    );

static
NTSTATUS
SrvStatisticsValidateProviderTable(
    PLWIO_SRV_STAT_PROVIDER_FUNCTION_TABLE pStatFnTable
    );

inline
static
VOID
SrvStatisticsFreeInfo(
    PSRV_STAT_INFO pStatInfo
    );

NTSTATUS
SrvStatisticsInitialize(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_STATISTICS_CONFIG config = {0};

    pthread_rwlock_init(&gSrvStatGlobals.mutex, NULL);
    gSrvStatGlobals.pMutex = &gSrvStatGlobals.mutex;

    ntStatus = SrvStatsConfigInitContents(&config);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvStatsConfigRead(&config);
    if (ntStatus == STATUS_DEVICE_CONFIGURATION_ERROR)
    {
        ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvStatsConfigTransferContents(&config, &gSrvStatGlobals.config);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvStatsLoadProvider();
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SrvStatsConfigFreeContents(&config);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvStatsLoadProvider(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;
    PVOID    hModule  = NULL;
    PFN_INIT_SRV_STAT_PROVIDER     pfnInitStatProvider  = NULL;
    PFN_SHUTDOWN_SRV_STAT_PROVIDER pfnCloseStatProvider = NULL;
    PLWIO_SRV_STAT_PROVIDER_FUNCTION_TABLE pStatFnTable = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSrvStatGlobals.mutex);

    if (!IsNullOrEmptyString(gSrvStatGlobals.config.pszProviderPath))
    {
        PCSTR pszError = NULL;

        dlerror();

        hModule = dlopen(gSrvStatGlobals.config.pszProviderPath,
                         RTLD_NOW | RTLD_GLOBAL);
        if (!hModule)
        {
            pszError = dlerror();

            LWIO_LOG_ERROR("Failed to load statistics provider from '%s' (%s)",
                            gSrvStatGlobals.config.pszProviderPath,
                            pszError ? pszError : "");

            ntStatus = STATUS_DLL_NOT_FOUND;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        dlerror();

        pfnInitStatProvider =
                (PFN_INIT_SRV_STAT_PROVIDER)dlsym(
                                                hModule,
                                                LWIO_SYMBOL_NAME_INIT_SRV_STAT_PROVIDER);
        if (!pfnInitStatProvider)
        {
            pszError = dlerror();

            LWIO_LOG_ERROR( "Failed to load "
                            LWIO_SYMBOL_NAME_INIT_SRV_STAT_PROVIDER
                            " function for statistics provider from %s (%s)",
                            gSrvStatGlobals.config.pszProviderPath,
                            pszError ? pszError : "");

            ntStatus = STATUS_BAD_DLL_ENTRYPOINT;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pfnCloseStatProvider =
                (PFN_SHUTDOWN_SRV_STAT_PROVIDER)dlsym(
                                                    hModule,
                                                    LWIO_SYMBOL_NAME_CLOSE_SRV_STAT_PROVIDER);
        if (!pfnCloseStatProvider)
        {
            pszError = dlerror();

            LWIO_LOG_ERROR( "Failed to load "
                            LWIO_SYMBOL_NAME_CLOSE_SRV_STAT_PROVIDER
                            " function for statistics provider from %s (%s)",
                            gSrvStatGlobals.config.pszProviderPath,
                            pszError ? pszError : "");

            ntStatus = STATUS_BAD_DLL_ENTRYPOINT;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = pfnInitStatProvider(&pStatFnTable);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvStatisticsValidateProviderTable(pStatFnTable);
        BAIL_ON_NT_STATUS(ntStatus);

        gSrvStatGlobals.hModule = hModule;
        gSrvStatGlobals.pfnShutdownStatProvider = pfnCloseStatProvider;
        gSrvStatGlobals.pStatFnTable = pStatFnTable;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvStatGlobals.mutex);

    return ntStatus;

error:

    if (hModule)
    {
        if (pfnCloseStatProvider)
        {
            pfnCloseStatProvider(pStatFnTable);
        }

        dlclose(hModule);
    }

    goto cleanup;
}

static
NTSTATUS
SrvStatisticsValidateProviderTable(
    PLWIO_SRV_STAT_PROVIDER_FUNCTION_TABLE pStatFnTable
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!pStatFnTable ||
        !pStatFnTable->pfnCreateRequestContext ||
        !pStatFnTable->pfnPushMessage ||
        !pStatFnTable->pfnSetSubOpCode ||
		!pStatFnTable->pfnSetIOCTL ||
        !pStatFnTable->pfnSetSessionInfo ||
        !pStatFnTable->pfnPopMessage ||
        !pStatFnTable->pfnSetResponseInfo ||
        !pStatFnTable->pfnCloseRequestContext)
    {
        ntStatus = STATUS_BAD_DLL_ENTRYPOINT;
    }

    return ntStatus;
}


inline
BOOLEAN
SrvStatisticsLoggingEnabled(
    VOID
    )
{
    return SrvStatsConfigLoggingEnabled();
}

inline
NTSTATUS
SrvStatisticsCreateRequestContext(
    PSRV_STAT_CONNECTION_INFO pConnection,        /* IN              */
    SRV_STAT_SMB_VERSION      protocolVersion,    /* IN              */
    ULONG                     ulRequestLength,    /* IN              */
    PSRV_STAT_INFO*           ppStatInfo          /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;
    PSRV_STAT_INFO pStatInfo = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSrvStatGlobals.mutex);

    if (gSrvStatGlobals.config.bEnableLogging && gSrvStatGlobals.pStatFnTable)
    {
        ntStatus = SrvAllocateMemory(sizeof(SRV_STAT_INFO), (PVOID*)&pStatInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pStatInfo->refCount = 1;

        pthread_mutex_init(&pStatInfo->mutex, NULL);
        pStatInfo->pMutex = &pStatInfo->mutex;

        ntStatus = gSrvStatGlobals.pStatFnTable->pfnCreateRequestContext(
                        pConnection,
                        protocolVersion,
                        ulRequestLength,
                        &pStatInfo->hContext);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppStatInfo = pStatInfo;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvStatGlobals.mutex);

    return ntStatus;

error:

    *ppStatInfo = NULL;

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvStatGlobals.mutex);

    if (pStatInfo)
    {
        SrvStatisticsRelease(pStatInfo);
    }

    goto cleanup;
}

inline
PSRV_STAT_INFO
SrvStatisticsAcquire(
    PSRV_STAT_INFO pStatInfo
    )
{
    InterlockedIncrement(&pStatInfo->refCount);

    return pStatInfo;
}

inline
VOID
SrvStatisticsRelease(
    PSRV_STAT_INFO pStatInfo
    )
{
    if (InterlockedDecrement(&pStatInfo->refCount) == 0)
    {
        SrvStatisticsCloseRequestContext(pStatInfo);

        SrvStatisticsFreeInfo(pStatInfo);
    }
}

inline
NTSTATUS
SrvStatisticsPushMessage(
    PSRV_STAT_INFO               pStatInfo,        /* IN              */
    ULONG                        ulOpcode,         /* IN              */
    ULONG                        ulMessageLen      /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSrvStatGlobals.mutex);

    if (gSrvStatGlobals.config.bEnableLogging && gSrvStatGlobals.pStatFnTable)
    {
        BOOLEAN  bStatInfoInLock = FALSE;

        LWIO_LOCK_MUTEX(bStatInfoInLock, &pStatInfo->mutex);

        if (!LwIsSetFlag(pStatInfo->ulFlags, SRV_STATISTICS_USAGE_FLAG_MESSAGE))
        {
            ntStatus = gSrvStatGlobals.pStatFnTable->pfnPushMessage(
                            pStatInfo->hContext,
                            ulOpcode,
                            ulMessageLen);

            if (ntStatus == STATUS_SUCCESS)
            {
                LwSetFlag(pStatInfo->ulFlags, SRV_STATISTICS_USAGE_FLAG_MESSAGE);
            }
        }

        LWIO_UNLOCK_MUTEX(bStatInfoInLock, &pStatInfo->mutex);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvStatGlobals.mutex);

    return ntStatus;
}

inline
NTSTATUS
SrvStatisticsSetSubOpcode(
    PSRV_STAT_INFO            pStatInfo,           /* IN              */
    ULONG                     ulSubOpcode          /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSrvStatGlobals.mutex);

    if (gSrvStatGlobals.config.bEnableLogging && gSrvStatGlobals.pStatFnTable)
    {
        BOOLEAN  bStatInfoInLock = FALSE;

        LWIO_LOCK_MUTEX(bStatInfoInLock, &pStatInfo->mutex);

        if (!LwIsSetFlag(pStatInfo->ulFlags, SRV_STATISTICS_USAGE_FLAG_OPCODE))
        {
            ntStatus = gSrvStatGlobals.pStatFnTable->pfnSetSubOpCode(
                            pStatInfo->hContext,
                            ulSubOpcode);

            if (ntStatus == STATUS_SUCCESS)
            {
                LwSetFlag(pStatInfo->ulFlags, SRV_STATISTICS_USAGE_FLAG_OPCODE);
            }
        }

        LWIO_UNLOCK_MUTEX(bStatInfoInLock, &pStatInfo->mutex);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvStatGlobals.mutex);

    return ntStatus;
}

inline
NTSTATUS
SrvStatisticsSetIOCTL(
    PSRV_STAT_INFO            pStatInfo,           /* IN              */
    ULONG                     ulIoCtlCode          /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSrvStatGlobals.mutex);

    if (gSrvStatGlobals.config.bEnableLogging && gSrvStatGlobals.pStatFnTable)
    {
        BOOLEAN  bStatInfoInLock = FALSE;

        LWIO_LOCK_MUTEX(bStatInfoInLock, &pStatInfo->mutex);

        if (!LwIsSetFlag(pStatInfo->ulFlags, SRV_STATISTICS_USAGE_FLAG_IOCTL))
        {
            ntStatus = gSrvStatGlobals.pStatFnTable->pfnSetIOCTL(
                            pStatInfo->hContext,
                            ulIoCtlCode);

            if (ntStatus == STATUS_SUCCESS)
            {
                LwSetFlag(pStatInfo->ulFlags, SRV_STATISTICS_USAGE_FLAG_IOCTL);
            }
        }

        LWIO_UNLOCK_MUTEX(bStatInfoInLock, &pStatInfo->mutex);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvStatGlobals.mutex);

    return ntStatus;
}

inline
NTSTATUS
SrvStatisticsSetSessionInfo(
    PSRV_STAT_INFO            pStatInfo,           /* IN              */
    PSRV_STAT_SESSION_INFO    pSessionInfo         /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSrvStatGlobals.mutex);

    if (gSrvStatGlobals.config.bEnableLogging &&
        gSrvStatGlobals.pStatFnTable)
    {
        BOOLEAN  bStatInfoInLock = FALSE;

        LWIO_LOCK_MUTEX(bStatInfoInLock, &pStatInfo->mutex);

        if (!LwIsSetFlag(pStatInfo->ulFlags, SRV_STATISTICS_USAGE_FLAG_SESSION))
        {
            ntStatus = gSrvStatGlobals.pStatFnTable->pfnSetSessionInfo(
                            pStatInfo->hContext,
                            pSessionInfo);
            if (ntStatus == STATUS_SUCCESS)
            {
                LwSetFlag(pStatInfo->ulFlags, SRV_STATISTICS_USAGE_FLAG_SESSION);
            }
        }

        LWIO_UNLOCK_MUTEX(bStatInfoInLock, &pStatInfo->mutex);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvStatGlobals.mutex);

    return ntStatus;
}

inline
NTSTATUS
SrvStatisticsPopMessage(
    PSRV_STAT_INFO            pStatInfo,           /* IN              */
    ULONG                     ulOpCode,            /* IN              */
    ULONG                     ulResponseLength,    /* IN              */
    NTSTATUS                  msgStatus            /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSrvStatGlobals.mutex);

    if (gSrvStatGlobals.config.bEnableLogging &&
        gSrvStatGlobals.pStatFnTable)
    {
        BOOLEAN  bStatInfoInLock = FALSE;

        LWIO_LOCK_MUTEX(bStatInfoInLock, &pStatInfo->mutex);

        ntStatus = gSrvStatGlobals.pStatFnTable->pfnPopMessage(
                        pStatInfo->hContext,
                        ulOpCode,
                        ulResponseLength,
                        msgStatus);

        pStatInfo->ulFlags = 0;

        LWIO_UNLOCK_MUTEX(bStatInfoInLock, &pStatInfo->mutex);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvStatGlobals.mutex);

    return ntStatus;
}

inline
NTSTATUS
SrvStatisticsSetResponseInfo(
    PSRV_STAT_INFO            pStatInfo,           /* IN              */
    ULONG                     ulResponseLength     /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSrvStatGlobals.mutex);

    if (gSrvStatGlobals.config.bEnableLogging &&
        gSrvStatGlobals.pStatFnTable)
    {
        BOOLEAN  bStatInfoInLock = FALSE;

        LWIO_LOCK_MUTEX(bStatInfoInLock, &pStatInfo->mutex);

        ntStatus = gSrvStatGlobals.pStatFnTable->pfnSetResponseInfo(
                        pStatInfo->hContext,
                        ulResponseLength);

        LWIO_UNLOCK_MUTEX(bStatInfoInLock, &pStatInfo->mutex);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvStatGlobals.mutex);

    return ntStatus;
}

inline
NTSTATUS
SrvStatisticsCloseRequestContext(
    PSRV_STAT_INFO            pStatInfo            /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSrvStatGlobals.mutex);

    // NOTE:
    // If logging was disabled just before this call, we still need to free
    // the memory that was given out earlier. Once the statistics module is
    // loaded, it is not recommended that we unload it before accounting for
    // all the contexts (memory) that were issued.
    if (gSrvStatGlobals.pStatFnTable)
    {
        BOOLEAN  bStatInfoInLock = FALSE;

        if (pStatInfo->pMutex)
        {
            LWIO_LOCK_MUTEX(bStatInfoInLock, &pStatInfo->mutex);
        }

        if (pStatInfo->hContext)
        {
            ntStatus = gSrvStatGlobals.pStatFnTable->pfnCloseRequestContext(
                            pStatInfo->hContext);

            pStatInfo->hContext = NULL;
        }

        if (pStatInfo->pMutex)
        {
            LWIO_UNLOCK_MUTEX(bStatInfoInLock, &pStatInfo->mutex);
        }
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvStatGlobals.mutex);

    return ntStatus;
}

inline
static
VOID
SrvStatisticsFreeInfo(
    PSRV_STAT_INFO pStatInfo
    )
{
    if (pStatInfo->pMutex)
    {
        pthread_mutex_destroy(&pStatInfo->mutex);
    }

    SrvFreeMemory(pStatInfo);
}

NTSTATUS
SrvStatisticsShutdown(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    if (gSrvStatGlobals.pMutex)
    {
        LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSrvStatGlobals.mutex);
    }

    if (gSrvStatGlobals.hModule)
    {
        if (gSrvStatGlobals.pfnShutdownStatProvider)
        {
            gSrvStatGlobals.pfnShutdownStatProvider(
                                gSrvStatGlobals.pStatFnTable);

            gSrvStatGlobals.pfnShutdownStatProvider = NULL;
            gSrvStatGlobals.pStatFnTable = NULL;
        }

        dlclose(gSrvStatGlobals.hModule);
        gSrvStatGlobals.hModule = NULL;
    }

    SrvStatsConfigFreeContents(&gSrvStatGlobals.config);

    if (gSrvStatGlobals.pMutex)
    {
        LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvStatGlobals.mutex);

        pthread_rwlock_destroy(&gSrvStatGlobals.mutex);
        gSrvStatGlobals.pMutex = NULL;
    }

    return ntStatus;
}
