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
 *     lsadm_p.c
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

#define IsLsaDmStateFlagsOffline(Flags) \
    IsSetFlag(Flags, LSA_DM_STATE_FLAG_FORCE_OFFLINE | LSA_DM_STATE_FLAG_MEDIA_SENSE_OFFLINE)

#define IsLsaDmDomainFlagsOffline(Flags) \
    IsSetFlag(Flags, LSA_DM_DOMAIN_FLAG_OFFLINE | LSA_DM_DOMAIN_FLAG_FORCE_OFFLINE)

#define IsLsaDmDomainFlagsGcOffline(Flags) \
    IsSetFlag(Flags, LSA_DM_DOMAIN_FLAG_GC_OFFLINE | LSA_DM_DOMAIN_FLAG_FORCE_OFFLINE)

#define LOG_WRAP_BOOL(x)   ((x) ? 'Y' : 'N')
#define LOG_WRAP_STRING(x) ((x) ? (x) : "(null)")


//////////////////////////////////////////////////////////////////////
///
/// @name LSASS Domain Manager (LsaDm) Internal State
///
/// @details This keeps track of LsaDm internal state.
///
/// @{

struct _LSA_DM_LDAP_CONNECTION
{
    BOOLEAN bIsGc;
    PSTR pszDnsDomainName;
    // NULL if not connected
    HANDLE hLdapConnection;
    DWORD dwConnectionPeriod;

    PLSA_DM_LDAP_CONNECTION pNext;
};

///
/// Keeps track of offline state of a single domain.
///
typedef struct _LSA_DM_DOMAIN_STATE {

    LSA_DM_DOMAIN_FLAGS Flags;

    /// Name of domain.  Can be NULL for downlevel domain.
    PSTR pszDnsName;

    /// Short domain name.
    PSTR pszNetbiosName;

    /// NULL for primary trust.
    PSTR pszTrusteeDnsName;
    DWORD dwTrustFlags;
    DWORD dwTrustType;
    DWORD dwTrustAttributes;

    PSID pSid;
    uuid_t Guid;

    /// Typically NULL for external trusts as it is
    /// not needed in that case.
    PSTR pszForestName;

    /// Lsa internal trust category
    LSA_TRUST_DIRECTION dwTrustDirection;
    LSA_TRUST_MODE dwTrustMode;

    /// These three are NULL unless someone explictily stored DC/GC info.
    PSTR pszClientSiteName;
    PLSA_DM_DC_INFO pDcInfo;
    PLSA_DM_DC_INFO pGcInfo;

    // A free list is maintained for LDAP DC and GC connections. When a caller
    // requests a connection, the one from the top of the free list is returned. If the free list is empty, a new connection is created. When connections are
    // "closed" by callers, they are really put onto the free list and left in
    // the connected state.
    //
    // When a ldap search fails with a network error, the LSA_DM_CONNECTION
    // is reconnected (possibly to a new DC). Such failures mean that all
    // other handles are probably bad too (because the DC was rebooted or
    // reaffinitized). To compensate, this code will clear the free connection
    // list whenever such a "reconnection event" occurs. The connection period
    // marks when a connection was created. This way old connections that are
    // "closed" are rejected from entering the free list if they were connected
    // before the "reconnection event".
    DWORD dwDcConnectionPeriod;
    PLSA_DM_LDAP_CONNECTION pFreeDcConn;
    DWORD dwGcConnectionPeriod;
    PLSA_DM_LDAP_CONNECTION pFreeGcConn;

} LSA_DM_DOMAIN_STATE, *PLSA_DM_DOMAIN_STATE;

typedef struct _LSA_DM_THREAD_INFO {
    pthread_t Thread;
    pthread_t* pThread;
    pthread_mutex_t* pMutex;
    pthread_cond_t* pCondition;
    BOOLEAN bIsDone;
    BOOLEAN bTrigger;
} LSA_DM_THREAD_INFO, *PLSA_DM_THREAD_INFO;

typedef struct _LSA_DM_UNKNOWN_DOMAIN_ENTRY {
    union {
        PSID pSid;
        PSTR pszName;
    };
    LSA_LIST_LINKS Links;
    // If Time = 0, the entry is permanent (does not expire).
    time_t Time;
} LSA_DM_UNKNOWN_DOMAIN_ENTRY, *PLSA_DM_UNKNOWN_DOMAIN_ENTRY;

///
/// Keeps track of all domain state.
///
typedef struct _LSA_DM_STATE {
    /// Offline enabled, global (force or media sense) offline, etc.
    LSA_DM_STATE_FLAGS StateFlags;

    /// Points to primary domain in DomainList.
    PLSA_DM_DOMAIN_STATE pPrimaryDomain;

    /// List of domains (LSA_DM_DOMAIN_STATE).  It will contain the primary
    /// domain at the head.
    PDLINKEDLIST DomainList;

    /// Lock for general state (DomainList, etc).
    pthread_mutex_t* pMutex;

    /// Online detection thread info
    LSA_DM_THREAD_INFO OnlineDetectionThread;

    /// List of LSA_DM_UNKNOWN_DOMAIN_ENTRY.
    LSA_LIST_LINKS UnknownDomainSidList;
    LSA_LIST_LINKS UnknownDomainNameList;

    /// Domain trusts to exclude/include.
    PSTR* ppszTrustExceptionList;
    ULONG dwTrustExceptionCount;

    /// @name Parameters
    /// @{
    /// Number of seconds between checking for domains returning online
    DWORD dwCheckOnlineSeconds;
    /// Number of seconds to keep entries in the unknown domain cache
    DWORD dwUnknownDomainCacheTimeoutSeconds;
    /// @}
} LSA_DM_STATE, *PLSA_DM_STATE;

typedef struct _LSA_DM_LDAP_RECONNECT_CONTEXT
{
    PLSA_DM_LDAP_CONNECTION pLdap;
    LSA_DM_STATE_HANDLE Handle;
} LSA_DM_LDAP_RECONNECT_CONTEXT, *PLSA_DM_LDAP_RECONNECT_CONTEXT;

static
DWORD
LsaDmpLdapReconnectCallback(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    );

static
DWORD
LsaDmpDetectTransitionOnlineAllDomains(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PLSA_DM_THREAD_INFO pThreadInfo
    );

static
VOID
LsaDmpDomainDestroy(
    IN OUT PLSA_DM_DOMAIN_STATE pDomain
    );

static
VOID
LsaDmpLdapConnectionDestroy(
    IN PLSA_DM_LDAP_CONNECTION pLdap
    );

static
DWORD
LsaDmpLdapConnectionCreate(
    IN BOOLEAN bIsGc,
    IN PCSTR pszDnsDomainName,
    OUT PLSA_DM_LDAP_CONNECTION* ppConn
    );

static
VOID
LsaDmpLdapConnectionListDestroy(
    IN OUT PLSA_DM_LDAP_CONNECTION* ppList
    );

static
VOID
LsaDmpFreeUnknownDomainEntry(
    IN OUT PLSA_DM_UNKNOWN_DOMAIN_ENTRY pEntry,
    IN BOOLEAN bIsSidEntry
    );

static
VOID
LsaDmpDestroyMutex(
    IN OUT pthread_mutex_t** ppMutex
    )
{
    if (*ppMutex)
    {
        pthread_mutex_destroy(*ppMutex);
        LwFreeMemory(*ppMutex);
        *ppMutex = NULL;
    }
}

static
DWORD
LsaDmpCreateMutex(
    OUT pthread_mutex_t** ppMutex,
    IN int MutexType
    )
{
    DWORD dwError = 0;
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_t* pMutexAttr = NULL;
    pthread_mutex_t* pMutex = NULL;

    if (MutexType)
    {
        dwError = pthread_mutexattr_init(&mutexAttr);
        BAIL_ON_LSA_ERROR(dwError);

        pMutexAttr = &mutexAttr;

        dwError = pthread_mutexattr_settype(pMutexAttr, PTHREAD_MUTEX_RECURSIVE);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sizeof(*pMutex), (PVOID*)&pMutex);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pthread_mutex_init(pMutex, pMutexAttr);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pMutexAttr)
    {
        pthread_mutexattr_destroy(pMutexAttr);
    }

    *ppMutex = pMutex;
    return dwError;

error:
    // Note that we do not need to destroy as we failed to init.
    LW_SAFE_FREE_MEMORY(pMutex);
    goto cleanup;
}

// TODO-Assert macros...
static
VOID
LsaDmpAcquireMutex(
    IN pthread_mutex_t* pMutex
    )
{
    DWORD dwError = pthread_mutex_lock(pMutex);
    if (dwError)
    {
        LSA_LOG_ERROR("pthread_mutex_lock() failed: %u", dwError);
    }
}

static
VOID
LsaDmpReleaseMutex(
    IN pthread_mutex_t* pMutex
    )
{
    DWORD dwError = pthread_mutex_unlock(pMutex);
    if (dwError)
    {
        LSA_LOG_ERROR("pthread_mutex_unlock() failed: %u", dwError);
    }
}

static
VOID
LsaDmpDestroyCond(
    IN OUT pthread_cond_t** ppCond
    )
{
    if (*ppCond)
    {
        pthread_cond_destroy(*ppCond);
        LwFreeMemory(*ppCond);
        *ppCond = NULL;
    }
}

static
DWORD
LsaDmpCreateCond(
    OUT pthread_cond_t** ppCond
    )
{
    DWORD dwError = 0;
    pthread_cond_t* pCond = NULL;

    dwError = LwAllocateMemory(sizeof(*pCond), (PVOID*)&pCond);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pthread_cond_init(pCond, NULL);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *ppCond = pCond;
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pCond);
    goto cleanup;
}

/// @} lsa_om_internal

/// First time, we want to check for online soon in case there
/// was some network issue on startup.
#define LSA_DM_THREAD_FIRST_PERIOD 60
/// Minimum time interval to wait between runs.
#define LSA_DM_THREAD_MIN_PERIOD 60

static
time_t
LsaDmpGetNextCheckOnlineTime(
    IN time_t LastCheckTime,
    IN DWORD dwCheckOnlineSeconds
    )
{
    time_t nextCheckTime = 0;

    // Note that we assume nobody is setting the date
    // to before 1970...
    if (!LastCheckTime)
    {
        nextCheckTime = time(NULL) + LSA_DM_THREAD_FIRST_PERIOD;
    }
    else
    {
        //
        // We compute the next time to check based on the last
        // time we checked, making sure that we are not going
        // too often (e.g., if checking is taking a long time
        // compared to the interval).  This also handles
        // someone setting very short interval (e.g., 0 seconds).
        //

        time_t minNextCheckTime = time(NULL) + LSA_DM_THREAD_MIN_PERIOD;
        nextCheckTime = LastCheckTime + dwCheckOnlineSeconds;
        // Make sure that we are not checking more often than allowed.
        nextCheckTime = LSA_MAX(nextCheckTime, minNextCheckTime);
    }

    return nextCheckTime;
}

static
PVOID
LsaDmpThreadRoutine(
    IN PVOID pContext
    )
{
    DWORD dwError = 0;
    PLSA_DM_STATE pState = (PLSA_DM_STATE) pContext;
    PLSA_DM_THREAD_INFO pThreadInfo = &pState->OnlineDetectionThread;
    time_t lastCheckTime = 0;

    LSA_LOG_VERBOSE("Started domain manager online detection thread");

    for (;;)
    {
        DWORD dwCheckOnlineSeconds = 0;
        BOOLEAN bIsDone = FALSE;
        time_t nextCheckTime = 0;
        struct timespec wakeTime = { 0 };
        BOOLEAN bIsTriggered = FALSE;

        // Get the checking interval first.  This also synchronizes
        // thread startup wrt thread creation.
        LsaDmpAcquireMutex(pState->pMutex);
        dwCheckOnlineSeconds = pState->dwCheckOnlineSeconds;
        LsaDmpReleaseMutex(pState->pMutex);

        nextCheckTime = LsaDmpGetNextCheckOnlineTime(
                                lastCheckTime,
                                dwCheckOnlineSeconds);

        wakeTime.tv_sec = nextCheckTime;

        LsaDmpAcquireMutex(pThreadInfo->pMutex);
        bIsDone = pThreadInfo->bIsDone;
        bIsTriggered = pThreadInfo->bTrigger;
        if (!bIsDone && !bIsTriggered)
        {
            // TODO: Error code conversion
            dwError = pthread_cond_timedwait(pThreadInfo->pCondition,
                                             pThreadInfo->pMutex,
                                             &wakeTime);
            bIsDone = pThreadInfo->bIsDone;
            bIsTriggered = pThreadInfo->bTrigger;
        }
        pThreadInfo->bTrigger = FALSE;
        LsaDmpReleaseMutex(pThreadInfo->pMutex);
        if (bIsDone)
        {
            break;
        }
        if (ETIMEDOUT == dwError && !bIsTriggered)
        {
            if (time(NULL) < wakeTime.tv_sec)
            {
                // It didn't really timeout. Something else happened
                dwError = 0;
                continue;
            }
        }
        if (ETIMEDOUT == dwError || bIsTriggered)
        {
            // Mark the time so we don't try to check again too soon.
            lastCheckTime = time(NULL);
            // Do detection
            dwError = LsaDmpDetectTransitionOnlineAllDomains(pState,
                                                             pThreadInfo);
            if (dwError)
            {
                // TODO -- log something?
                dwError = 0;
            }
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    LSA_LOG_VERBOSE("Stopped domain manager online detection thread");
    return NULL;

error:
    LSA_LOG_ERROR("Unexpected error in domain manager online detection thread (%u)", dwError);
    goto cleanup;
}

static
VOID
LsaDmpForEachDomainDestroy(
    IN PVOID pData,
    IN PVOID pContext
    )
{
    PLSA_DM_DOMAIN_STATE pDomain = (PLSA_DM_DOMAIN_STATE)pData;
    LsaDmpDomainDestroy(pDomain);
}

VOID
LsaDmpStateDestroy(
    IN OUT LSA_DM_STATE_HANDLE Handle
    )
///<
/// Destroy a state object for the offline manager.
///
/// This includes stopping the online detection thread.
///
/// @param[in, out] Handle - Offline state manager object to destroy.
///
/// @return N/A
///
{
    if (Handle)
    {
        if (Handle->OnlineDetectionThread.pThread)
        {
            void* threadResult = NULL;
            LsaDmpAcquireMutex(Handle->OnlineDetectionThread.pMutex);
            Handle->OnlineDetectionThread.bIsDone = TRUE;
            LsaDmpReleaseMutex(Handle->OnlineDetectionThread.pMutex);
            pthread_cond_signal(Handle->OnlineDetectionThread.pCondition);
            pthread_join(*Handle->OnlineDetectionThread.pThread, &threadResult);
            Handle->OnlineDetectionThread.pThread = NULL;
        }
        LsaDmpDestroyCond(&Handle->OnlineDetectionThread.pCondition);
        LsaDmpDestroyMutex(&Handle->OnlineDetectionThread.pMutex);
        LsaDmpDestroyMutex(&Handle->pMutex);
        if (Handle->DomainList)
        {
            LsaDLinkedListForEach(Handle->DomainList, LsaDmpForEachDomainDestroy, NULL);
            LsaDLinkedListFree(Handle->DomainList);
        }
        while (!LsaListIsEmpty(&Handle->UnknownDomainSidList))
        {
            PLSA_LIST_LINKS pLinks = LsaListRemoveHead(&Handle->UnknownDomainSidList);
            PLSA_DM_UNKNOWN_DOMAIN_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pLinks, LSA_DM_UNKNOWN_DOMAIN_ENTRY, Links);
            LsaDmpFreeUnknownDomainEntry(pEntry, TRUE);
        }
        while (!LsaListIsEmpty(&Handle->UnknownDomainNameList))
        {
            PLSA_LIST_LINKS pLinks = LsaListRemoveHead(&Handle->UnknownDomainNameList);
            PLSA_DM_UNKNOWN_DOMAIN_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pLinks, LSA_DM_UNKNOWN_DOMAIN_ENTRY, Links);
            LsaDmpFreeUnknownDomainEntry(pEntry, FALSE);
        }
        LwFreeStringArray(Handle->ppszTrustExceptionList, Handle->dwTrustExceptionCount);
        LW_SAFE_FREE_MEMORY(Handle);
    }
}

DWORD
LsaDmpStateCreate(
    OUT PLSA_DM_STATE_HANDLE pHandle,
    IN BOOLEAN bIsOfflineBehaviorEnabled,
    IN DWORD dwCheckOnlineSeconds,
    IN DWORD dwUnknownDomainCacheTimeoutSeconds,
    IN BOOLEAN bIgnoreAllTrusts,
    IN PSTR* ppszTrustExceptionList,
    IN DWORD dwTrustExceptionCount
    )
///<
/// Create an empty state object for the offline manager.
///
/// This includes starting up the online detection thread.
///
/// @param[out] pHandle - Returns new offline state manager object.
///
/// @param[in] bIsOfflineBehaviorEnabled - Whether to enable offline behavior.
///
/// @param[in] dwCheckOnlineSeconds - How often to check whether an offline
///     domain is back online. A setting of zero disables these checks.
///
/// @param[in] dwUnknownDomainCacheTimeoutSeconds - Number of seconds to keep
///     entries in the unknown domain cache.
///
/// @param[in] bIgnoreAllTrusts - Whether to ignore all trusts (except for
///     those in ppszTrustExceptionList).
///
/// @param[in] ppszTrustExceptionList - Specific trusts to exclude/include.
///     If bIgnoreAllTrusts is not set, this is an exclusion list.
///     If bIgnoreAllTrusts is set, this is an inclusion list.
///
/// @param[in] dwTrustExceptionCount - Count of entries in
///     ppszTrustExceptionList.
///
/// @return LSA status code.
///  @arg LW_ERROR_SUCCESS on success
///  @arg !LW_ERROR_SUCCESS on failure
///
{
    DWORD dwError = 0;
    PLSA_DM_STATE pState = NULL;
    BOOLEAN bIsAcquired = FALSE;

    dwError = LwAllocateMemory(sizeof(*pState), (PVOID*)&pState);
    BAIL_ON_LSA_ERROR(dwError);

    LsaListInit(&pState->UnknownDomainSidList);
    LsaListInit(&pState->UnknownDomainNameList);

    if (bIsOfflineBehaviorEnabled)
    {
        SetFlag(pState->StateFlags, LSA_DM_STATE_FLAG_OFFLINE_ENABLED);
    }

    pState->dwCheckOnlineSeconds = dwCheckOnlineSeconds;
    pState->dwUnknownDomainCacheTimeoutSeconds = dwUnknownDomainCacheTimeoutSeconds;

    if (bIgnoreAllTrusts)
    {
        SetFlag(pState->StateFlags, LSA_DM_STATE_FLAG_IGNORE_ALL_TRUSTS);
    }

    dwError = LwDuplicateStringArray(
                    &pState->ppszTrustExceptionList,
                    &pState->dwTrustExceptionCount,
                    ppszTrustExceptionList,
                    dwTrustExceptionCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmpCreateMutex(&pState->pMutex, PTHREAD_MUTEX_RECURSIVE);
    BAIL_ON_LSA_ERROR(dwError);

    // Acquire to block the thread from checking online seconds.
    LsaDmpAcquireMutex(pState->pMutex);
    bIsAcquired = TRUE;

    dwError = LsaDmpCreateMutex(&pState->OnlineDetectionThread.pMutex, 0);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmpCreateCond(&pState->OnlineDetectionThread.pCondition);
    BAIL_ON_LSA_ERROR(dwError);

    // Now that everything is set up, we need to initialize the thread.

    dwError = pthread_create(&pState->OnlineDetectionThread.Thread, NULL,
                             LsaDmpThreadRoutine, pState);
    dwError = LwMapErrnoToLwError(dwError);
    BAIL_ON_LSA_ERROR(dwError);

    // Indicate that the thread is created
    pState->OnlineDetectionThread.pThread = &pState->OnlineDetectionThread.Thread;

cleanup:
    if (bIsAcquired)
    {
        LsaDmpReleaseMutex(pState->pMutex);
    }
    *pHandle = pState;

    return dwError;

error:
    // ISSUE-2008/09/10-dalmeida -- Good example of why error label is bad.
    // We end up having to duplicate this code under error and cleanup.
    if (bIsAcquired)
    {
        LsaDmpReleaseMutex(pState->pMutex);
        bIsAcquired = FALSE;
    }
    if (pState)
    {
        LsaDmpStateDestroy(pState);
        pState = NULL;
    }

    goto cleanup;
}

DWORD
LsaDmpQueryState(
    IN LSA_DM_STATE_HANDLE Handle,
    OUT OPTIONAL PLSA_DM_STATE_FLAGS pStateFlags,
    OUT OPTIONAL PDWORD pdwCheckOnlineSeconds,
    OUT OPTIONAL PDWORD pdwUnknownDomainCacheTimeoutSeconds
    )
{
    LsaDmpAcquireMutex(Handle->pMutex);
    if (pStateFlags)
    {
        *pStateFlags = Handle->StateFlags;
    }
    if (pdwCheckOnlineSeconds)
    {
        *pdwCheckOnlineSeconds = Handle->dwCheckOnlineSeconds;
    }
    if (pdwUnknownDomainCacheTimeoutSeconds)
    {
        *pdwUnknownDomainCacheTimeoutSeconds = Handle->dwUnknownDomainCacheTimeoutSeconds;
    }
    LsaDmpReleaseMutex(Handle->pMutex);

    return 0;
}

DWORD
LsaDmpSetState(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PBOOLEAN pbIsOfflineBehaviorEnabled,
    IN OPTIONAL PDWORD pdwCheckOnlineSeconds,
    IN OPTIONAL PDWORD pdwUnknownDomainCacheTimeoutSeconds
    )
{
    BOOLEAN bIsModified = FALSE;

    LsaDmpAcquireMutex(Handle->pMutex);

    if (pbIsOfflineBehaviorEnabled)
    {
        if (IsSetFlag(Handle->StateFlags, LSA_DM_STATE_FLAG_OFFLINE_ENABLED) !=
            !!*pbIsOfflineBehaviorEnabled)
        {
            if (*pbIsOfflineBehaviorEnabled)
            {
                SetFlag(Handle->StateFlags, LSA_DM_STATE_FLAG_OFFLINE_ENABLED);
            }
            else
            {
                ClearFlag(Handle->StateFlags, LSA_DM_STATE_FLAG_OFFLINE_ENABLED);
            }
            bIsModified = TRUE;
        }
    }

    if (pdwCheckOnlineSeconds)
    {
        if (Handle->dwCheckOnlineSeconds != *pdwCheckOnlineSeconds)
        {
            Handle->dwCheckOnlineSeconds = *pdwCheckOnlineSeconds;
            bIsModified = TRUE;
        }
    }

    if (pdwUnknownDomainCacheTimeoutSeconds)
    {
        if (Handle->dwUnknownDomainCacheTimeoutSeconds != *pdwUnknownDomainCacheTimeoutSeconds)
        {
            Handle->dwUnknownDomainCacheTimeoutSeconds = *pdwUnknownDomainCacheTimeoutSeconds;
            // Do not set bIsModified because we do not need to signal.
        }
    }


    // Allow thread to pick up changes in global state (e.g.,
    // in the check online seconds value).
    if (bIsModified)
    {
        pthread_cond_signal(Handle->OnlineDetectionThread.pCondition);
    }

    LsaDmpReleaseMutex(Handle->pMutex);
    return 0;
}

static
VOID
LsaDmpModifyStateFlags(
    IN LSA_DM_STATE_HANDLE Handle,
    IN LSA_DM_DOMAIN_FLAGS ClearFlags,
    IN LSA_DM_DOMAIN_FLAGS SetFlags
    )
{
    LSA_DM_STATE_FLAGS stateFlags = 0;
    BOOLEAN bWasOffline = FALSE;
    BOOLEAN bIsOffline = FALSE;

    LsaDmpAcquireMutex(Handle->pMutex);

    stateFlags = Handle->StateFlags;

    ClearFlag(stateFlags, ClearFlags);
    SetFlag(stateFlags, SetFlags);

    if (stateFlags != Handle->StateFlags)
    {
        bWasOffline = IsSetFlag(Handle->StateFlags, LSA_DM_STATE_FLAG_MEDIA_SENSE_OFFLINE);
        bIsOffline = IsSetFlag(stateFlags, LSA_DM_STATE_FLAG_MEDIA_SENSE_OFFLINE);

        if (bWasOffline != bIsOffline)
        {
            LSA_LOG_ALWAYS("Media sense is now %s",
                           bIsOffline ? "offline" : "online");
            if (!bIsOffline)
            {
                // Have to ignore dwError because this function returns void
                LsaSrvFlushSystemCache();
                LsaUmTriggerCheckUsersThread();
            }
        }

        bWasOffline = IsSetFlag(Handle->StateFlags, LSA_DM_STATE_FLAG_FORCE_OFFLINE);
        bIsOffline = IsSetFlag(stateFlags, LSA_DM_STATE_FLAG_FORCE_OFFLINE);

        if (bWasOffline != bIsOffline)
        {
            LSA_LOG_ALWAYS("Global force offline is now %s",
                           bIsOffline ? "enabled" : "disabled");
            if (!bIsOffline)
            {
                // Have to ignore dwError because this function returns void
                LsaSrvFlushSystemCache();
                LsaUmTriggerCheckUsersThread();
            }
        }

        Handle->StateFlags = stateFlags;
    }

    LsaDmpReleaseMutex(Handle->pMutex);
}

VOID
LsaDmpMediaSenseOffline(
    IN LSA_DM_STATE_HANDLE Handle
    )
{
    if (AD_EventlogEnabled() && AD_ShouldLogNetworkConnectionEvents())
    {
        ADLogMediaSenseOfflineEvent();
    }

    LsaDmpModifyStateFlags(Handle, 0, LSA_DM_STATE_FLAG_MEDIA_SENSE_OFFLINE);
}

VOID
LsaDmpMediaSenseOnline(
    IN LSA_DM_STATE_HANDLE Handle
    )
{
    if (AD_EventlogEnabled() && AD_ShouldLogNetworkConnectionEvents())
    {
        ADLogMediaSenseOnlineEvent();
    }

    LsaDmpModifyStateFlags(Handle, LSA_DM_STATE_FLAG_MEDIA_SENSE_OFFLINE, 0);
    LsaDmpDetectTransitionOnline(Handle, NULL);
}

DWORD
LsaDmpDuplicateSid(
    OUT PSID* ppSid,
    IN PSID pSid
    )
{
    DWORD dwError = 0;
    if (pSid)
    {
        size_t size = RtlLengthSid(pSid);
        dwError = LwAllocateMemory(size, (PVOID*)ppSid);
        BAIL_ON_LSA_ERROR(dwError);
        memcpy(*ppSid, pSid, size);
    }
    else
    {
        *ppSid = NULL;
    }
cleanup:
    return dwError;
error:
    goto cleanup;
}

static
DWORD
LsaDmpDuplicateDcInfo(
    OUT PLSA_DM_DC_INFO* ppDcInfo,
    IN PLSA_DM_DC_INFO pDcInfo
    )
{
    DWORD dwError = 0;
    PLSA_DM_DC_INFO pResultDcInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pResultDcInfo), (PVOID*)&pResultDcInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pDcInfo->pszName, &pResultDcInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pDcInfo->pszAddress, &pResultDcInfo->pszAddress);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pDcInfo->pszSiteName, &pResultDcInfo->pszSiteName);
    BAIL_ON_LSA_ERROR(dwError);

    pResultDcInfo->dwDsFlags = pDcInfo->dwDsFlags;

cleanup:
    *ppDcInfo = pResultDcInfo;
    return dwError;
error:
    LsaDmFreeDcInfo(pResultDcInfo);
    pResultDcInfo = NULL;
    goto cleanup;
}

static
DWORD
LsaDmpDomainSetDcInfoInternal(
    IN PLSA_DM_DOMAIN_STATE pDomain,
    IN BOOLEAN bSetGcInfo,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    DWORD dwError = 0;
    PLSA_DM_DC_INFO pDomainDcInfo = NULL;
    PLSA_DM_DC_INFO* ppDomainDcInfo = bSetGcInfo ? &pDomain->pGcInfo : &pDomain->pDcInfo;
    PLSA_DM_DC_INFO pCurrentDomainDcInfo = *ppDomainDcInfo;
    PSTR pszForestName = NULL;
    PSTR pszClientSiteName = NULL;
    PSTR pszDcInfoName = NULL;
    PSTR pszDcInfoAddress = NULL;
    PSTR pszDcInfoSiteName = NULL;

    if (!pDomain->pszForestName)
    {
        dwError = LwAllocateString(pDcInfo->pszDnsForestName, &pszForestName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // TODO - perhaps log if forest root result changed?

    if (!pDomain->pszClientSiteName ||
        strcasecmp(pDomain->pszClientSiteName, pDcInfo->pszClientSiteName))
    {
        if (pDomain->pszClientSiteName)
        {
            LSA_LOG_INFO("Client site name for '%s' domain changing from '%s' to '%s'",
                         pDomain->pszDnsName,
                         LOG_WRAP_STRING(pDomain->pszClientSiteName),
                         pDcInfo->pszClientSiteName);
        }

        dwError = LwAllocateString(pDcInfo->pszClientSiteName, &pszClientSiteName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pCurrentDomainDcInfo)
    {
        dwError = LwAllocateMemory(sizeof(*pDomainDcInfo), (PVOID*)&pDomainDcInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pCurrentDomainDcInfo ||
        strcasecmp(pCurrentDomainDcInfo->pszName, pDcInfo->pszDomainControllerName))
    {
        dwError = LwAllocateString(pDcInfo->pszDomainControllerName, &pszDcInfoName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pCurrentDomainDcInfo ||
        strcasecmp(pCurrentDomainDcInfo->pszAddress, pDcInfo->pszDomainControllerAddress))
    {
        dwError = LwAllocateString(pDcInfo->pszDomainControllerAddress, &pszDcInfoAddress);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pCurrentDomainDcInfo ||
        strcasecmp(pCurrentDomainDcInfo->pszSiteName, pDcInfo->pszDCSiteName))
    {
        dwError = LwAllocateString(pDcInfo->pszDCSiteName, &pszDcInfoSiteName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Everything is allocated, so we can set everything.

    if (pszForestName)
    {
        LW_SAFE_FREE_STRING(pDomain->pszForestName);
        pDomain->pszForestName = pszForestName;
        pszForestName = NULL;

        if (!strcasecmp(pDomain->pszForestName, pDomain->pszDnsName))
        {
            pDomain->Flags |= LSA_DM_DOMAIN_FLAG_FOREST_ROOT;
        }
    }

    if (pszClientSiteName)
    {
        LW_SAFE_FREE_STRING(pDomain->pszClientSiteName);
        pDomain->pszClientSiteName = pszClientSiteName;
        pszClientSiteName = NULL;
    }

    if (!pCurrentDomainDcInfo)
    {
        // Note that it could not have been set if we need it.
        pCurrentDomainDcInfo = *ppDomainDcInfo = pDomainDcInfo;
        pDomainDcInfo = NULL;
    }

    if (pszDcInfoName)
    {
        LW_SAFE_FREE_STRING(pCurrentDomainDcInfo->pszName);
        pCurrentDomainDcInfo->pszName = pszDcInfoName;
        pszDcInfoName = NULL;
    }

    if (pszDcInfoAddress)
    {
        LW_SAFE_FREE_STRING(pCurrentDomainDcInfo->pszAddress);
        pCurrentDomainDcInfo->pszAddress = pszDcInfoAddress;
        pszDcInfoAddress = NULL;
    }

    if (pszDcInfoSiteName)
    {
        LW_SAFE_FREE_STRING(pCurrentDomainDcInfo->pszSiteName);
        pCurrentDomainDcInfo->pszSiteName = pszDcInfoSiteName;
        pszDcInfoSiteName = NULL;
    }

    if (pCurrentDomainDcInfo->dwDsFlags != pDcInfo->dwFlags)
    {
        pCurrentDomainDcInfo->dwDsFlags = pDcInfo->dwFlags;
    }

cleanup:
    LW_SAFE_FREE_STRING(pszForestName);
    LW_SAFE_FREE_STRING(pszClientSiteName);
    LW_SAFE_FREE_STRING(pszDcInfoName);
    LW_SAFE_FREE_STRING(pszDcInfoAddress);
    LW_SAFE_FREE_STRING(pszDcInfoSiteName);
    LW_SAFE_FREE_MEMORY(pDomainDcInfo);
    return dwError;
error:
    goto cleanup;
}

#if 0
// Currently unused.
static
DWORD
LsaDmpDomainSetDcInfo(
    IN PLSA_DM_DOMAIN_STATE pDomain,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    return LsaDmpDomainSetDcInfoInternal(pDomain, FALSE, pDcInfo);
}

static
DWORD
LsaDmpDomainSetGcInfo(
    IN PLSA_DM_DOMAIN_STATE pDomain,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    return LsaDmpDomainSetDcInfoInternal(pDomain, TRUE, pDcInfo);
}
#endif

static
VOID
LsaDmpLdapConnectionListDestroy(
    IN OUT PLSA_DM_LDAP_CONNECTION* ppList
    )
{
    PLSA_DM_LDAP_CONNECTION pDelete = NULL;

    while (*ppList != NULL)
    {
        pDelete = *ppList;
        *ppList = pDelete->pNext;
        pDelete->pNext = NULL;
        LsaDmpLdapConnectionDestroy(pDelete);
    }
}

static
VOID
LsaDmpDomainDestroy(
    IN OUT PLSA_DM_DOMAIN_STATE pDomain
    )
{
    if (pDomain)
    {
        LW_SAFE_FREE_STRING(pDomain->pszDnsName);
        LW_SAFE_FREE_STRING(pDomain->pszNetbiosName);
        LW_SAFE_FREE_STRING(pDomain->pszTrusteeDnsName);
        LW_SAFE_FREE_MEMORY(pDomain->pSid);
        LW_SAFE_FREE_STRING(pDomain->pszForestName);
        LW_SAFE_FREE_STRING(pDomain->pszClientSiteName);
        LsaDmFreeDcInfo(pDomain->pDcInfo);
        LsaDmFreeDcInfo(pDomain->pGcInfo);
        LsaDmpLdapConnectionListDestroy(&pDomain->pFreeDcConn);
        LsaDmpLdapConnectionListDestroy(&pDomain->pFreeGcConn);
        LW_SAFE_FREE_MEMORY(pDomain);
    }
}

static
BOOLEAN
LsaDmpIsValidDnsDomainName(
    IN PCSTR pszDomainName
    )
{
    BOOLEAN bIsValid = TRUE;
    char* dot = strrchr(pszDomainName, '.');
    if (dot && !dot[1])
    {
        // If there is a dot, it cannot be the last character.
        bIsValid = FALSE;
    }
    return bIsValid;
}

static
BOOLEAN
LsaDmpDomainCreate(
    OUT PLSA_DM_DOMAIN_STATE* ppDomain,
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PSID pDomainSid,
    IN PGUID pDomainGuid,
    IN OPTIONAL PCSTR pszDnsForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo
    )
{
    DWORD dwError = 0;
    PLSA_DM_DOMAIN_STATE pDomain = NULL;

    dwError = LwAllocateMemory(sizeof(*pDomain), (PVOID*)&pDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszDnsDomainName)
    {
        if (!LsaDmpIsValidDnsDomainName(pszDnsDomainName))
        {
            LSA_LOG_ERROR("Invalid DNS domain name: '%s'", pszDnsDomainName);
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }
        dwError = LwAllocateString(pszDnsDomainName, &pDomain->pszDnsName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LsaDmIsValidNetbiosDomainName(pszNetbiosDomainName))
    {
        LSA_LOG_ERROR("Invalid NetBIOS domain name: '%s'", pszNetbiosDomainName);
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateString(pszNetbiosDomainName, &pDomain->pszNetbiosName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmpDuplicateSid(&pDomain->pSid, pDomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    if (pDomainGuid)
    {
        memcpy(&pDomain->Guid, pDomainGuid, sizeof(pDomain->Guid));
    }

    if (pszDnsForestName)
    {
        dwError = LwAllocateString(pszDnsForestName, &pDomain->pszForestName);
        BAIL_ON_LSA_ERROR(dwError);
    }

#if 0
    if (pszDnsDomainName && pDcInfo)
    {
        dwError = LsaDmpDomainSetDcInfo(pDomain, pDcInfo);
        BAIL_ON_LSA_ERROR(dwError);
        if (pDcInfo->dwFlags & DS_GC_FLAG)
        {
            dwError = LsaDmpDomainSetGcInfo(pDomain, pDcInfo);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
#endif

cleanup:
    *ppDomain = pDomain;

    return dwError;

error:
    if (pDomain)
    {
        LsaDmpDomainDestroy(pDomain);
        pDomain = NULL;
    }
    goto cleanup;
}

static
BOOLEAN
LsaDmpSlowIsDomainNameMatch(
    IN PLSA_DM_DOMAIN_STATE pDomain,
    IN PCSTR pszDomainName
    )
{
    return LsaDmIsEitherDomainNameMatch(pszDomainName,
                                        pDomain->pszDnsName,
                                        pDomain->pszNetbiosName);
}

static
BOOLEAN
LsaDmpFastIsDomainNameMatch(
    IN PLSA_DM_DOMAIN_STATE pDomain,
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszNetbiosDomainName
    )
{
    BOOLEAN bIsMatch = FALSE;

    if ((pszDnsDomainName &&
         LsaDmIsSpecificDomainNameMatch(pszDnsDomainName,
                                        pDomain->pszDnsName)) ||
        (pszNetbiosDomainName &&
         LsaDmIsSpecificDomainNameMatch(pszNetbiosDomainName,
                                        pDomain->pszNetbiosName)))
    {
        bIsMatch = TRUE;
    }

    return bIsMatch;
}

static
PLSA_DM_DOMAIN_STATE
LsaDmpFindDomain2(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszNetbiosDomainName
    )
{
    PDLINKEDLIST listEntry = NULL;
    PLSA_DM_DOMAIN_STATE pFoundDomain = NULL;

    for (listEntry = Handle->DomainList;
         listEntry;
         listEntry = listEntry->pNext)
    {
        PLSA_DM_DOMAIN_STATE pDomain = (PLSA_DM_DOMAIN_STATE)listEntry->pItem;
        if (LsaDmpFastIsDomainNameMatch(pDomain, pszDnsDomainName, pszNetbiosDomainName))
        {
            pFoundDomain = pDomain;
            break;
        }
    }

    return pFoundDomain;
}

static
PLSA_DM_DOMAIN_STATE
LsaDmpFindDomain(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    )
///<
/// Find a domain by DNS or NetBIOS name.
///
/// @param[in] Handle - Offline state.
/// @param[in] pszDomainName - Name of domain to find in offline state.
///
/// @return Domain entry found, if any.
///
/// @note The state must already be locked.
///
{
    PDLINKEDLIST listEntry = NULL;
    PLSA_DM_DOMAIN_STATE pFoundDomain = NULL;

    for (listEntry = Handle->DomainList;
         listEntry;
         listEntry = listEntry->pNext)
    {
        PLSA_DM_DOMAIN_STATE pDomain = (PLSA_DM_DOMAIN_STATE)listEntry->pItem;
        if (LsaDmpSlowIsDomainNameMatch(pDomain, pszDomainName))
        {
            pFoundDomain = pDomain;
            break;
        }
    }

    return pFoundDomain;
}

static
DWORD
LsaDmpMustFindDomain(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName,
    OUT PLSA_DM_DOMAIN_STATE* ppFoundDomain
    )
///<
/// Find a domain by DNS or NetBIOS name.
///
/// @param[in] Handle - Offline state.
/// @param[in] pszDomainName - Name of domain to find in offline state.
/// @param[in] ppFoundDomain - Found domain (or NULL on failure).
///
/// @return LW_ERROR_SUCCESS or LW_ERROR_NO_SUCH_DOMAIN if not found.
///
/// @note The state must already be locked.
///
{
    DWORD dwError = 0;
    PLSA_DM_DOMAIN_STATE pFoundDomain = NULL;

    pFoundDomain = LsaDmpFindDomain(Handle, pszDomainName);
    if (!pFoundDomain)
    {
        LSA_LOG_DEBUG("Do not know about domain '%s'", pszDomainName);
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
    }
    *ppFoundDomain = pFoundDomain;
    return dwError;
}

static
BOOLEAN
LsaDmpIsObjectSidInDomainSid(
    IN PSID pObjectSid,
    IN PSID pDomainSid
    )
{
    return ((pDomainSid->Revision == pObjectSid->Revision) &&
            !memcmp(&pDomainSid->IdentifierAuthority, &pObjectSid->IdentifierAuthority, sizeof(pObjectSid->IdentifierAuthority)) &&
            (pDomainSid->SubAuthorityCount <= pObjectSid->SubAuthorityCount) &&
            !memcmp(pDomainSid->SubAuthority, pObjectSid->SubAuthority, sizeof(pObjectSid->SubAuthority[0]) * pDomainSid->SubAuthorityCount)) ? TRUE : FALSE;
}

static
PLSA_DM_DOMAIN_STATE
LsaDmpFindDomainBySid(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PSID pObjectSid
    )
///<
/// Find a domain containing the SID.
///
/// @param[in] Handle - Offline state.
/// @param[in] pObjectSid - Sid of object for which we want to find a domain.
///
/// @return Domain entry found, if any.
///
/// @note The state must already be locked.
///
{
    PDLINKEDLIST listEntry = NULL;
    PLSA_DM_DOMAIN_STATE pFoundDomain = NULL;

    for (listEntry = Handle->DomainList;
         listEntry;
         listEntry = listEntry->pNext)
    {
        PLSA_DM_DOMAIN_STATE pDomain = (PLSA_DM_DOMAIN_STATE)listEntry->pItem;
        if (LsaDmpIsObjectSidInDomainSid(pObjectSid, pDomain->pSid))
        {
            pFoundDomain = pDomain;
            break;
        }
    }

    return pFoundDomain;
}

static
DWORD
LsaDmpMustFindDomainByObjectSid(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PSID pObjectSid,
    OUT PLSA_DM_DOMAIN_STATE* ppFoundDomain
    )
{
    DWORD dwError = 0;
    PLSA_DM_DOMAIN_STATE pFoundDomain = NULL;

    pFoundDomain = LsaDmpFindDomainBySid(Handle, pObjectSid);
    if (!pFoundDomain)
    {
        PSTR pszSid = NULL;
        dwError = LsaAllocateCStringFromSid(&pszSid, pObjectSid);
        // ignore error
        LSA_LOG_DEBUG("Do not know about domain for object SID '%s'",
                      LSA_SAFE_LOG_STRING(pszSid));
        LW_SAFE_FREE_STRING(pszSid);
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
    }
    *ppFoundDomain = pFoundDomain;
    return dwError;
}

DWORD
LsaDmpAddTrustedDomain(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PSID pDomainSid,
    IN PGUID pDomainGuid,
    IN PCSTR pszTrusteeDnsDomainName,
    IN DWORD dwTrustFlags,
    IN DWORD dwTrustType,
    IN DWORD dwTrustAttributes,
    IN LSA_TRUST_DIRECTION dwTrustDirection,
    IN LSA_TRUST_MODE dwTrustMode,
    IN BOOLEAN bIsTransitiveOnewayChild,
    IN OPTIONAL PCSTR pDnsForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsAcquired = FALSE;
    PLSA_DM_DOMAIN_STATE pFoundDomain = NULL;
    PLSA_DM_DOMAIN_STATE pDomain = NULL;

    if (!pDomainSid)
    {
        LSA_LOG_DEBUG("Missing SID for domain '%s'.", pszDnsDomainName);
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (IS_BOTH_OR_NEITHER(pszTrusteeDnsDomainName, IsSetFlag(dwTrustFlags, NETR_TRUST_FLAG_PRIMARY)))
    {
        if (pszTrusteeDnsDomainName)
        {
            LSA_LOG_DEBUG("Cannot add primary trust %s because it has a trustee (%s).", pszDnsDomainName, pszTrusteeDnsDomainName);
        }
        else
        {
            LSA_LOG_DEBUG("Cannot add non-primary trust %s because it has no trustee.", pszDnsDomainName);
        }
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaDmpAcquireMutex(Handle->pMutex);
    bIsAcquired = TRUE;

    // First, check for duplicates
    pFoundDomain = LsaDmpFindDomain2(Handle, pszDnsDomainName, pszNetbiosDomainName);
    if (pFoundDomain)
    {
        // FUTURE-2008/08/15-dalmeida -- Check for a change in trust info
        LSA_LOG_DEBUG("Duplicate trust found for %s (%s).", pszDnsDomainName, pszNetbiosDomainName);
        // TODO-document this error code.
        dwError = LW_ERROR_DUPLICATE_DOMAINNAME;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Now that we have looked for duplicates, check for trying to add
    // something where we do or do not already have a primary domain.
    // Note that we checked for duplicates first so we can return a
    // "duplicate error" if someone is trying to add the same thing
    // twice instead of returning the more serious error about violating
    // the invariant of adding domains in the appropriate order and
    // not trying to change the primary domain.
    if (!IS_BOTH_OR_NEITHER(Handle->pPrimaryDomain, pszTrusteeDnsDomainName))
    {
        if (pszTrusteeDnsDomainName)
        {
            LSA_LOG_DEBUG("Cannot add non-primary trust %s w/o first adding primary domain.", pszDnsDomainName);
        }
        else
        {
            LSA_LOG_DEBUG("Cannot add primary trust %s since a primary trust already exists.", pszDnsDomainName);
        }
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Make sure that any trustee had already been added.
    if (pszTrusteeDnsDomainName)
    {
        if (!LsaDmpIsDomainPresent(Handle, pszTrusteeDnsDomainName))
        {
            LSA_LOG_DEBUG("Trustee %s for domain %s must be added before adding the trusted domain.", pszTrusteeDnsDomainName, pszDnsDomainName);
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = LsaDmpDomainCreate(&pDomain,
                                 pszDnsDomainName,
                                 pszNetbiosDomainName,
                                 pDomainSid,
                                 pDomainGuid,
                                 pDnsForestName,
                                 pDcInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszTrusteeDnsDomainName)
    {
        dwError = LwAllocateString(pszTrusteeDnsDomainName, &pDomain->pszTrusteeDnsName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pDomain->dwTrustFlags = dwTrustFlags;
    pDomain->dwTrustType = dwTrustType;
    pDomain->dwTrustAttributes = dwTrustAttributes;
    pDomain->dwTrustDirection = dwTrustDirection;
    pDomain->dwTrustMode = dwTrustMode;

    if (bIsTransitiveOnewayChild)
    {
        SetFlag(pDomain->Flags, LSA_DM_DOMAIN_FLAG_TRANSITIVE_1WAY_CHILD);
    }

    if (IsSetFlag(pDomain->dwTrustFlags, NETR_TRUST_FLAG_PRIMARY))
    {
        SetFlag(pDomain->Flags, LSA_DM_DOMAIN_FLAG_PRIMARY);
    }

    if (IsSetFlag(dwTrustFlags, NETR_TRUST_FLAG_PRIMARY))
    {
        dwError = LsaDLinkedListPrepend(&Handle->DomainList, pDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaDLinkedListAppend(&Handle->DomainList, pDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (!pszTrusteeDnsDomainName)
    {
        Handle->pPrimaryDomain = pDomain;
    }
    pDomain = NULL;

cleanup:
    if (bIsAcquired)
    {
        LsaDmpReleaseMutex(Handle->pMutex);
    }
    if (pDomain)
    {
        LsaDmpDomainDestroy(pDomain);
    }

    return dwError;

error:
    goto cleanup;
}

BOOLEAN
LsaDmpIsDomainPresent(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    )
{
    BOOLEAN bIsPresent = FALSE;

    LsaDmpAcquireMutex(Handle->pMutex);
    bIsPresent = LsaDmpFindDomain(Handle, pszDomainName) ? TRUE : FALSE;
    LsaDmpReleaseMutex(Handle->pMutex);

    return bIsPresent;
}

static
VOID
LsaDmpFillConstDcInfo(
    IN PLSA_DM_DC_INFO pDcInfo,
    OUT PLSA_DM_CONST_DC_INFO pConstDcInfo
    )
{
    pConstDcInfo->dwDsFlags = pDcInfo->dwDsFlags;
    pConstDcInfo->pszName = pDcInfo->pszName;
    pConstDcInfo->pszAddress = pDcInfo->pszAddress;
    pConstDcInfo->pszSiteName = pDcInfo->pszSiteName;
}

VOID
LsaDmpEnumDomains(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PCSTR pszDomainName,
    IN PLSA_DM_ENUM_DOMAIN_CALLBACK pfCallback,
    IN OPTIONAL PVOID pContext
    )
{
    PDLINKEDLIST listEntry = NULL;

    LsaDmpAcquireMutex(Handle->pMutex);

    for (listEntry = Handle->DomainList;
         listEntry;
         listEntry = listEntry->pNext)
    {
        PLSA_DM_DOMAIN_STATE pDomain = (PLSA_DM_DOMAIN_STATE)listEntry->pItem;
        LSA_DM_CONST_ENUM_DOMAIN_INFO info;
        LSA_DM_CONST_DC_INFO dcInfo;
        LSA_DM_CONST_DC_INFO gcInfo;
        BOOLEAN needContinue = FALSE;

        if (pszDomainName && !LsaDmpSlowIsDomainNameMatch(pDomain, pszDomainName))
        {
            continue;
        }

        memset(&info, 0, sizeof(info));

        info.pszDnsDomainName = pDomain->pszDnsName;
        info.pszNetbiosDomainName = pDomain->pszNetbiosName;
        info.pSid = pDomain->pSid;
        info.pGuid = &pDomain->Guid;
        info.pszTrusteeDnsDomainName = pDomain->pszTrusteeDnsName;
        info.dwTrustFlags = pDomain->dwTrustFlags;
        info.dwTrustType = pDomain->dwTrustType;
        info.dwTrustAttributes = pDomain->dwTrustAttributes;
        info.dwTrustDirection = pDomain->dwTrustDirection;
        info.dwTrustMode = pDomain->dwTrustMode;
        info.pszForestName = pDomain->pszForestName;
        info.pszClientSiteName = pDomain->pszClientSiteName;
        info.Flags = pDomain->Flags;
        if (pDomain->pDcInfo)
        {
            LsaDmpFillConstDcInfo(pDomain->pDcInfo, &dcInfo);
            info.DcInfo = &dcInfo;
        }
        if (pDomain->pGcInfo)
        {
            LsaDmpFillConstDcInfo(pDomain->pGcInfo, &gcInfo);
            info.GcInfo = &gcInfo;
        }

        needContinue = pfCallback(pszDomainName, pContext, &info);
        if (pszDomainName || !needContinue)
        {
            break;
        }
    }

    LsaDmpReleaseMutex(Handle->pMutex);
}

typedef DWORD LSA_DM_ENUM_DOMAIN_FILTERED_ITEM_TYPE;

#define LSA_DM_ENUM_DOMAIN_FILTERED_ITEM_TYPE_NAME 1
#define LSA_DM_ENUM_DOMAIN_FILTERED_ITEM_TYPE_FULL 2

// IMPORTANT: The items in this union must only be pointers.
// This is so that we can cast an array of these unions to
// the appropriate array of pointers.  If we change this,
// we need to change the code that casts so that it does
// an array copy instead (based on the type).
typedef union _LSA_DM_ENUM_DOMAIN_FILTERED_ITEM {
    PSTR pszNameInfo;
    PLSA_DM_ENUM_DOMAIN_INFO pFullInfo;
} LSA_DM_ENUM_DOMAIN_FILTERED_ITEM, *PLSA_DM_ENUM_DOMAIN_FILTERED_ITEM;

typedef struct _LSA_DM_ENUM_DOMAIN_FILTER_CONTEXT {
    DWORD dwError;
    // This is the number of domains being returned.
    DWORD dwCount;
    // Capacity needs to hold dwCount + 1 (for NULL termination)
    DWORD dwCapacity;
    // Discriminator for each *pItem below.
    LSA_DM_ENUM_DOMAIN_FILTERED_ITEM_TYPE ItemType;
    // NULL-terminated array of pointers to each domain info item.
    PLSA_DM_ENUM_DOMAIN_FILTERED_ITEM pItems;
    PLSA_DM_ENUM_DOMAIN_FILTER_CALLBACK pfFilterCallback;
    PVOID pFilterContext;
} LSA_DM_ENUM_DOMAIN_FILTER_CONTEXT, *PLSA_DM_ENUM_DOMAIN_FILTER_CONTEXT;

static
VOID
LsaDmpFreeEnumDomainFilteredItems(
    IN LSA_DM_ENUM_DOMAIN_FILTERED_ITEM_TYPE ItemType,
    IN OUT PLSA_DM_ENUM_DOMAIN_FILTERED_ITEM pItems
    )
{
    if (pItems)
    {
        DWORD dwIndex;
        switch (ItemType)
        {
            case LSA_DM_ENUM_DOMAIN_FILTERED_ITEM_TYPE_NAME:
                for (dwIndex = 0; pItems[dwIndex].pszNameInfo; dwIndex++)
                {
                    LwFreeString(pItems[dwIndex].pszNameInfo);
                }
                break;
            case LSA_DM_ENUM_DOMAIN_FILTERED_ITEM_TYPE_FULL:
                for (dwIndex = 0; pItems[dwIndex].pFullInfo; dwIndex++)
                {
                    LsaDmFreeEnumDomainInfo(pItems[dwIndex].pFullInfo);
                }
                break;
            default:
                break;
        }
        LwFreeMemory(pItems);
    }
}

static
BOOLEAN
LsaDmpEnumDomainsFilteredCallback(
    IN OPTIONAL PCSTR pszEnumDomainName,
    IN OPTIONAL PVOID pContext,
    IN PLSA_DM_CONST_ENUM_DOMAIN_INFO pDomainInfo
    )
{
    DWORD dwError = 0;
    PLSA_DM_ENUM_DOMAIN_FILTER_CONTEXT pEnumContext =
        (PLSA_DM_ENUM_DOMAIN_FILTER_CONTEXT)pContext;
    PLSA_DM_ENUM_DOMAIN_FILTERED_ITEM pItems = NULL;

    if (pEnumContext->pfFilterCallback)
    {
        if (!pEnumContext->pfFilterCallback(pEnumContext->pFilterContext,
                                            pDomainInfo))
        {
            goto cleanup;
        }
    }

    // We need to make sure that we have enough room for a
    // NULL terminator too.
    if (pEnumContext->dwCapacity < (pEnumContext->dwCount + 2))
    {
        DWORD dwNewCapacity = 0;
        DWORD dwNewSize = 0;
        DWORD dwSize = 0;

        // Note that the first time needs to use at least 2.
        dwNewCapacity = LSA_MAX(2, pEnumContext->dwCapacity + 10);
        dwNewSize = sizeof(pItems[0]) * dwNewCapacity;

        dwError = LwAllocateMemory(dwNewSize, (PVOID*)&pItems);
        BAIL_ON_LSA_ERROR(dwError);

        dwSize = sizeof(pItems[0]) * pEnumContext->dwCapacity;
        memcpy(pItems, pEnumContext->pItems, dwSize);

        pEnumContext->dwCapacity = dwNewCapacity;
        LwFreeMemory(pEnumContext->pItems);
        pEnumContext->pItems = pItems;
        pItems = NULL;
    }

    switch (pEnumContext->ItemType)
    {
        case LSA_DM_ENUM_DOMAIN_FILTERED_ITEM_TYPE_NAME:
            dwError = LwAllocateString(
                        pDomainInfo->pszDnsDomainName,
                        &pEnumContext->pItems[pEnumContext->dwCount].pszNameInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_DM_ENUM_DOMAIN_FILTERED_ITEM_TYPE_FULL:
            dwError = LsaDmDuplicateConstEnumDomainInfo(
                        pDomainInfo,
                        &pEnumContext->pItems[pEnumContext->dwCount].pFullInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }

    pEnumContext->dwCount++;

cleanup:
    LW_SAFE_FREE_MEMORY(pItems);

    pEnumContext->dwError = dwError;
    return dwError ? FALSE : TRUE;

error:
    goto cleanup;
}

static
DWORD
LsaDmpEnumDomainItems(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_FILTER_CALLBACK pfFilterCallback,
    IN OPTIONAL PVOID pFilterContext,
    IN LSA_DM_ENUM_DOMAIN_FILTERED_ITEM_TYPE ItemType,
    OUT PLSA_DM_ENUM_DOMAIN_FILTERED_ITEM* ppItems,
    OUT OPTIONAL PDWORD pdwCount
    )
{
    DWORD dwError = 0;
    LSA_DM_ENUM_DOMAIN_FILTER_CONTEXT context = { 0 };

    context.ItemType = ItemType;
    context.pfFilterCallback = pfFilterCallback;
    context.pFilterContext = pFilterContext;

    LsaDmpEnumDomains(Handle,
                      NULL,
                      LsaDmpEnumDomainsFilteredCallback,
                      &context);
    dwError = context.dwError;
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *ppItems = context.pItems;
    if (pdwCount)
    {
        *pdwCount = context.dwCount;
    }

    return dwError;

error:
    LsaDmpFreeEnumDomainFilteredItems(context.ItemType, context.pItems);
    context.pItems = NULL;
    context.dwCount = 0;
    goto cleanup;
}

DWORD
LsaDmpEnumDomainNames(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_FILTER_CALLBACK pfFilterCallback,
    IN OPTIONAL PVOID pFilterContext,
    OUT PSTR** pppszDomainNames,
    OUT OPTIONAL PDWORD pdwCount
    )
{
    return LsaDmpEnumDomainItems(
            Handle,
            pfFilterCallback,
            pFilterContext,
            LSA_DM_ENUM_DOMAIN_FILTERED_ITEM_TYPE_NAME,
            (PLSA_DM_ENUM_DOMAIN_FILTERED_ITEM*)pppszDomainNames,
            pdwCount);
}

DWORD
LsaDmpEnumDomainInfo(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_FILTER_CALLBACK pfFilterCallback,
    IN OPTIONAL PVOID pFilterContext,
    OUT PLSA_DM_ENUM_DOMAIN_INFO** pppDomainInfo,
    OUT OPTIONAL PDWORD pdwCount
    )
{
    return LsaDmpEnumDomainItems(
            Handle,
            pfFilterCallback,
            pFilterContext,
            LSA_DM_ENUM_DOMAIN_FILTERED_ITEM_TYPE_FULL,
            (PLSA_DM_ENUM_DOMAIN_FILTERED_ITEM*)pppDomainInfo,
            pdwCount);
}

static
DWORD
LsaDmpQueryDomainInfoInternal(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PCSTR pszDomainName,
    IN OPTIONAL PSID pObjectSid,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName,
    OUT OPTIONAL PSID* ppSid,
    OUT OPTIONAL uuid_t* pGuid,
    OUT OPTIONAL PSTR* ppszTrusteeDnsDomainName,
    OUT OPTIONAL PDWORD pdwTrustFlags,
    OUT OPTIONAL PDWORD pdwTrustType,
    OUT OPTIONAL PDWORD pdwTrustAttributes,
    OUT OPTIONAL LSA_TRUST_DIRECTION* pdwTrustDirection,
    OUT OPTIONAL LSA_TRUST_MODE* pdwTrustMode,
    OUT OPTIONAL PSTR* ppszForestName,
    OUT OPTIONAL PSTR* ppszClientSiteName,
    OUT OPTIONAL PLSA_DM_DOMAIN_FLAGS pFlags,
    OUT OPTIONAL PLSA_DM_DC_INFO* ppDcInfo,
    OUT OPTIONAL PLSA_DM_DC_INFO* ppGcInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsAcquired = FALSE;
    PLSA_DM_DOMAIN_STATE pDomain = NULL;
    PSTR pszDnsDomainName = NULL;
    PSTR pszNetbiosDomainName = NULL;
    PSID pSid = NULL;
    PSTR pszTrusteeDnsDomainName = NULL;
    PSTR pszForestName = NULL;
    PSTR pszClientSiteName = NULL;
    PLSA_DM_DC_INFO pDcInfo = NULL;
    PLSA_DM_DC_INFO pGcInfo = NULL;
    uuid_t guid = { 0 };
    DWORD dwTrustFlags = 0;
    DWORD dwTrustType = 0;
    DWORD dwTrustAttributes = 0;
    LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
    LSA_TRUST_MODE dwTrustMode = LSA_TRUST_MODE_UNKNOWN;
    LSA_DM_DOMAIN_FLAGS Flags = 0;

    if ((pszDomainName && pObjectSid) ||
        (!pszDomainName && !pObjectSid))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaDmpAcquireMutex(Handle->pMutex);
    bIsAcquired = TRUE;

    if (pszDomainName)
    {
        dwError = LsaDmpMustFindDomain(Handle, pszDomainName, &pDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaDmpMustFindDomainByObjectSid(Handle, pObjectSid, &pDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (ppszDnsDomainName)
    {
        // Can be NULL for a down-level domain, if we were to
        // put it in the trust list.
        dwError = LwStrDupOrNull(pDomain->pszDnsName, &pszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (ppszNetbiosDomainName)
    {
        dwError = LwAllocateString(pDomain->pszNetbiosName, &pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (ppSid)
    {
        dwError = LsaDmpDuplicateSid(&pSid, pDomain->pSid);
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (ppszTrusteeDnsDomainName)
    {
        // Can be NULL for the primary domain.
        dwError = LwStrDupOrNull(pDomain->pszTrusteeDnsName, &pszTrusteeDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (ppszForestName)
    {
        // Can be NULL for an external trust.
        dwError = LwStrDupOrNull(pDomain->pszForestName, &pszForestName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (ppszClientSiteName)
    {
        dwError = LwAllocateString(pDomain->pszClientSiteName, &pszClientSiteName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    // Can be NULL
    if (ppDcInfo && pDomain->pDcInfo)
    {
        dwError = LsaDmpDuplicateDcInfo(&pDcInfo, pDomain->pDcInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }
    // Can be NULL
    if (ppGcInfo && pDomain->pGcInfo)
    {
        dwError = LsaDmpDuplicateDcInfo(&pGcInfo, pDomain->pGcInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    memcpy(&guid, &pDomain->Guid, sizeof(guid));
    dwTrustFlags = pDomain->dwTrustFlags;
    dwTrustType = pDomain->dwTrustType;
    dwTrustAttributes = pDomain->dwTrustAttributes;
    dwTrustDirection = pDomain->dwTrustDirection;
    dwTrustMode = pDomain->dwTrustMode;
    Flags = pDomain->Flags;

cleanup:
    if (bIsAcquired)
    {
        LsaDmpReleaseMutex(Handle->pMutex);
    }
    if (ppszDnsDomainName)
    {
        *ppszDnsDomainName = pszDnsDomainName;
    }
    if (ppszNetbiosDomainName)
    {
        *ppszNetbiosDomainName = pszNetbiosDomainName;
    }
    if (ppSid)
    {
        *ppSid = pSid;
    }
    if (pGuid)
    {
        memcpy(pGuid, &guid, sizeof(*pGuid));
    }
    if (ppszTrusteeDnsDomainName)
    {
        *ppszTrusteeDnsDomainName = pszTrusteeDnsDomainName;
    }
    if (pdwTrustFlags)
    {
        *pdwTrustFlags = dwTrustFlags;
    }
    if (pdwTrustType)
    {
        *pdwTrustType = dwTrustType;
    }
    if (pdwTrustAttributes)
    {
        *pdwTrustAttributes = dwTrustAttributes;
    }
    if (pdwTrustDirection)
    {
        *pdwTrustDirection = dwTrustDirection;
    }
    if (pdwTrustMode)
    {
        *pdwTrustMode = dwTrustMode;
    }
    if (ppszForestName)
    {
        *ppszForestName = pszForestName;
    }
    if (ppszClientSiteName)
    {
        *ppszClientSiteName = pszClientSiteName;
    }
    if (pFlags)
    {
        *pFlags = Flags;
    }
    if (ppDcInfo)
    {
        *ppDcInfo = pDcInfo;
    }
    if (ppGcInfo)
    {
        *ppGcInfo = pGcInfo;
    }
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszDnsDomainName);
    LW_SAFE_FREE_STRING(pszNetbiosDomainName);
    LW_SAFE_FREE_MEMORY(pSid);
    LW_SAFE_FREE_STRING(pszTrusteeDnsDomainName);
    LW_SAFE_FREE_STRING(pszForestName);
    LW_SAFE_FREE_STRING(pszClientSiteName);
    LsaDmFreeDcInfo(pDcInfo);
    pDcInfo = NULL;
    LsaDmFreeDcInfo(pGcInfo);
    pGcInfo = NULL;
    goto cleanup;
}

DWORD
LsaDmpQueryDomainInfo(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName,
    OUT OPTIONAL PSID* ppSid,
    OUT OPTIONAL uuid_t* pGuid,
    OUT OPTIONAL PSTR* ppszTrusteeDnsDomainName,
    OUT OPTIONAL PDWORD pdwTrustFlags,
    OUT OPTIONAL PDWORD pdwTrustType,
    OUT OPTIONAL PDWORD pdwTrustAttributes,
    OUT OPTIONAL LSA_TRUST_DIRECTION* pdwTrustDirection,
    OUT OPTIONAL LSA_TRUST_MODE* pdwTrustMode,
    OUT OPTIONAL PSTR* ppszForestName,
    OUT OPTIONAL PSTR* ppszClientSiteName,
    OUT OPTIONAL PLSA_DM_DOMAIN_FLAGS pFlags,
    OUT OPTIONAL PLSA_DM_DC_INFO* ppDcInfo,
    OUT OPTIONAL PLSA_DM_DC_INFO* ppGcInfo
    )
{
    return LsaDmpQueryDomainInfoInternal(
                Handle,
                pszDomainName,
                NULL,
                ppszDnsDomainName,
                ppszNetbiosDomainName,
                ppSid,
                pGuid,
                ppszTrusteeDnsDomainName,
                pdwTrustFlags,
                pdwTrustType,
                pdwTrustAttributes,
                pdwTrustDirection,
                pdwTrustMode,
                ppszForestName,
                ppszClientSiteName,
                pFlags,
                ppDcInfo,
                ppGcInfo);
}

DWORD
LsaDmpQueryDomainInfoByObjectSid(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PSID pObjectSid,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName,
    OUT OPTIONAL PSID* ppSid,
    OUT OPTIONAL uuid_t* pGuid,
    OUT OPTIONAL PSTR* ppszTrusteeDnsDomainName,
    OUT OPTIONAL PDWORD pdwTrustFlags,
    OUT OPTIONAL PDWORD pdwTrustType,
    OUT OPTIONAL PDWORD pdwTrustAttributes,
    OUT OPTIONAL LSA_TRUST_DIRECTION* pdwTrustDirection,
    OUT OPTIONAL LSA_TRUST_MODE* pdwTrustMode,
    OUT OPTIONAL PSTR* ppszForestName,
    OUT OPTIONAL PSTR* ppszClientSiteName,
    OUT OPTIONAL PLSA_DM_DOMAIN_FLAGS pFlags,
    OUT OPTIONAL PLSA_DM_DC_INFO* ppDcInfo,
    OUT OPTIONAL PLSA_DM_DC_INFO* ppGcInfo
    )
{
    return LsaDmpQueryDomainInfoInternal(
                Handle,
                NULL,
                pObjectSid,
                ppszDnsDomainName,
                ppszNetbiosDomainName,
                ppSid,
                pGuid,
                ppszTrusteeDnsDomainName,
                pdwTrustFlags,
                pdwTrustType,
                pdwTrustAttributes,
                pdwTrustDirection,
                pdwTrustMode,
                ppszForestName,
                ppszClientSiteName,
                pFlags,
                ppDcInfo,
                ppGcInfo);
}

static
DWORD
LsaDmpDomainSetDcInfoByNameInternal(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName,
    IN BOOLEAN bSetGcInfo,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsAcquired = FALSE;
    PLSA_DM_DOMAIN_STATE pDomain = NULL;

    LsaDmpAcquireMutex(Handle->pMutex);
    bIsAcquired = TRUE;

    dwError = LsaDmpMustFindDomain(Handle, pszDomainName, &pDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmpDomainSetDcInfoInternal(pDomain, bSetGcInfo, pDcInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (bIsAcquired)
    {
        LsaDmpReleaseMutex(Handle->pMutex);
    }
    return dwError;
error:
    goto cleanup;
}

DWORD
LsaDmpDomainSetDcInfoByName(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    return LsaDmpDomainSetDcInfoByNameInternal(Handle, pszDomainName, FALSE, pDcInfo);
}

DWORD
LsaDmpDomainSetGcInfoByName(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    return LsaDmpDomainSetDcInfoByNameInternal(Handle, pszDomainName, TRUE, pDcInfo);
}

static
VOID
LsaDmpModifyDomainFlagsByRef(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PLSA_DM_DOMAIN_STATE pDomain,
    IN BOOLEAN bIsSet,
    IN LSA_DM_DOMAIN_FLAGS Flags
    )
{
    BOOLEAN bWasOffline = FALSE;
    BOOLEAN bIsOffline = FALSE;
    BOOLEAN bGcWasOffline = FALSE;
    BOOLEAN bGcIsOffline = FALSE;
    BOOLEAN bNeedFlush = FALSE;

    bWasOffline = IsLsaDmDomainFlagsOffline(pDomain->Flags);
    bGcWasOffline = IsLsaDmDomainFlagsGcOffline(pDomain->Flags);
    if (bIsSet)
    {
        SetFlag(pDomain->Flags, Flags);
    }
    else
    {
        ClearFlag(pDomain->Flags, Flags);
    }
    bIsOffline = IsLsaDmDomainFlagsOffline(pDomain->Flags);
    bGcIsOffline = IsLsaDmDomainFlagsGcOffline(pDomain->Flags);

    if (bWasOffline != bIsOffline)
    {
        LSA_LOG_ALWAYS("Domain '%s' is now %sline",
                       pDomain->pszDnsName, bIsOffline ? "off" : "on");
        if (bIsOffline)
        {
            // We went from !offline -> offline.
            pDomain->dwDcConnectionPeriod++;
            LsaDmpLdapConnectionListDestroy(&pDomain->pFreeDcConn);
        }
        else
        {
            // We went from offline -> !offline.
            bNeedFlush = TRUE;
        }
    }

    if (bGcWasOffline != bGcIsOffline)
    {
        LSA_LOG_ALWAYS("Global catalog server for domain '%s' is now %sline",
                       pDomain->pszDnsName, bIsOffline ? "off" : "on");
        if (bGcIsOffline)
        {
            pDomain->dwGcConnectionPeriod++;
            LsaDmpLdapConnectionListDestroy(&pDomain->pFreeGcConn);
        }
        else
        {
            // We went from offline -> !offline.
            bNeedFlush = TRUE;
        }
    }

    if (bNeedFlush)
    {
        // Have to ignore dwError because this function returns void
        LsaSrvFlushSystemCache();

        LsaUmTriggerCheckUsersThread();
    }
}

static
DWORD
LsaDmpModifyDomainFlagsByName(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName,
    IN BOOLEAN bIsSet,
    IN LSA_DM_DOMAIN_FLAGS Flags
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsAcquired = FALSE;
    PLSA_DM_DOMAIN_STATE pFoundDomain = NULL;

    if (!pszDomainName)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaDmpAcquireMutex(Handle->pMutex);
    bIsAcquired = TRUE;

    dwError = LsaDmpMustFindDomain(Handle, pszDomainName, &pFoundDomain);
    BAIL_ON_LSA_ERROR(dwError);

    LsaDmpModifyDomainFlagsByRef(Handle, pFoundDomain, bIsSet, Flags);

cleanup:
    if (bIsAcquired)
    {
        LsaDmpReleaseMutex(Handle->pMutex);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
LsaDmpSetForceOfflineState(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PCSTR pszDomainName,
    IN BOOLEAN bIsSet
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsAcquired = FALSE;

    LsaDmpAcquireMutex(Handle->pMutex);
    bIsAcquired = TRUE;

    if (!pszDomainName)
    {
        // Handle global case.
        if (bIsSet)
        {
            LsaDmpModifyStateFlags(Handle, 0, LSA_DM_STATE_FLAG_MEDIA_SENSE_OFFLINE);
        }
        else
        {
            LsaDmpModifyStateFlags(Handle, LSA_DM_STATE_FLAG_MEDIA_SENSE_OFFLINE, 0);
        }
    }
    else
    {
        // Handle domain case.
        dwError = LsaDmpModifyDomainFlagsByName(Handle,
                                                pszDomainName,
                                                bIsSet,
                                                LSA_DM_DOMAIN_FLAG_FORCE_OFFLINE);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (bIsAcquired)
    {
        LsaDmpReleaseMutex(Handle->pMutex);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
LsaDmpTransitionOffline(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName,
    IN BOOLEAN bIsGc
    )
{
    DWORD dwFlags = 0;

    if (AD_EventlogEnabled() && AD_ShouldLogNetworkConnectionEvents())
    {
        ADLogDomainOfflineEvent(pszDomainName, bIsGc);
    }

    if (bIsGc)
    {
        dwFlags = LSA_DM_DOMAIN_FLAG_GC_OFFLINE;
    }
    else
    {
        dwFlags = LSA_DM_DOMAIN_FLAG_OFFLINE;
    }

    return LsaDmpModifyDomainFlagsByName(Handle,
                                         pszDomainName,
                                         TRUE,
                                         dwFlags);
}

DWORD
LsaDmpTransitionOnline(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    )
{
    if (AD_EventlogEnabled() && AD_ShouldLogNetworkConnectionEvents())
    {
        ADLogDomainOnlineEvent(pszDomainName);
    }

    return LsaDmpModifyDomainFlagsByName(Handle,
                                         pszDomainName,
                                         FALSE,
                                         LSA_DM_DOMAIN_FLAG_OFFLINE |
                                         LSA_DM_DOMAIN_FLAG_GC_OFFLINE);
}

BOOLEAN
LsaDmpIsDomainOffline(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PCSTR pszDomainName,
    IN BOOLEAN bIsGC
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsAcquired = FALSE;
    PLSA_DM_DOMAIN_STATE pFoundDomain = NULL;
    BOOLEAN bIsOffline = FALSE;

    LsaDmpAcquireMutex(Handle->pMutex);
    bIsAcquired = TRUE;

    if (!IsSetFlag(Handle->StateFlags, LSA_DM_STATE_FLAG_OFFLINE_ENABLED))
    {
        // Pretend that there is no offline-ness
        bIsOffline = FALSE;
    }
    else if (IsLsaDmStateFlagsOffline(Handle->StateFlags))
    {
        bIsOffline = TRUE;
    }
    else if (!pszDomainName)
    {
        // We just want global state, and that indicates online so far,
        // so we are online.
        bIsOffline = FALSE;
    }
    else
    {
        dwError = LsaDmpMustFindDomain(Handle, pszDomainName, &pFoundDomain);
        BAIL_ON_LSA_ERROR(dwError);

        if (bIsGC)
        {
            bIsOffline = IsLsaDmDomainFlagsGcOffline(pFoundDomain->Flags);
        }
        else
        {
            bIsOffline = IsLsaDmDomainFlagsOffline(pFoundDomain->Flags);
        }
    }

cleanup:
    if (bIsAcquired)
    {
        LsaDmpReleaseMutex(Handle->pMutex);
    }

    return bIsOffline;

error:
    goto cleanup;
}

DWORD
LsaDmpGetPrimaryDomainName(
    IN LSA_DM_STATE_HANDLE Handle,
    OUT PSTR* ppszPrimaryDomain
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsAcquired = FALSE;

    LsaDmpAcquireMutex(Handle->pMutex);
    bIsAcquired = TRUE;

    if (Handle->pPrimaryDomain)
    {
        dwError = LwAllocateString(Handle->pPrimaryDomain->pszDnsName, ppszPrimaryDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        *ppszPrimaryDomain = NULL;
    }

cleanup:

    if (bIsAcquired)
    {
        LsaDmpReleaseMutex(Handle->pMutex);
    }

    return dwError;

error:

    *ppszPrimaryDomain = NULL;

    goto cleanup;
}

static
DWORD
LsaDmpDetectTransitionOnlineDomain(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    PSTR pszDnsDomainName = NULL;
    PLWNET_DC_INFO pDcInfo = NULL;

    dwError = LsaDmpQueryDomainInfo(
                Handle,
                pszDomainName,
                &pszDnsDomainName,
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
                NULL,
                NULL,
                NULL,
                NULL);
    BAIL_ON_LSA_ERROR(dwError);

    //
    // Force rediscovery of DC for the domain.
    //

    dwError = LWNetGetDCName(NULL,
                             pszDnsDomainName,
                             NULL,
                             DS_FORCE_REDISCOVERY,
                             &pDcInfo);
    BAIL_ON_LSA_ERROR(dwError);

#if 0
    dwError = LsaDmpDomainSetGcInfo(pDomain, pDcInfo);
    BAIL_ON_LSA_ERROR(dwError);
#endif

    //
    // If this is a forest root, we also need to check that
    // we can find a GC.  Otherwise, our GC operations
    // will fail.
    //

    if (!strcasecmp(pDcInfo->pszDnsForestName, pDcInfo->pszFullyQualifiedDomainName) &&
        !(DS_GC_FLAG & pDcInfo->dwFlags))
    {
        LWNET_SAFE_FREE_DC_INFO(pDcInfo);

        dwError = LWNetGetDCName(NULL,
                                 pszDnsDomainName,
                                 NULL,
                                 DS_FORCE_REDISCOVERY | DS_GC_SERVER_REQUIRED,
                                 &pDcInfo);
        BAIL_ON_LSA_ERROR(dwError);

#if 0
        dwError = LsaDmpDomainSetGcInfo(pDomain, pDcInfo);
        BAIL_ON_LSA_ERROR(dwError);
#endif
    }

    //
    // This domain is online, so transition it.
    //

    dwError = LsaDmpTransitionOnline(Handle, pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);
    LW_SAFE_FREE_STRING(pszDnsDomainName);

    return dwError;

error:
    goto cleanup;
}

static
BOOLEAN
LsaDmpFilterOfflineCallback(
    IN OPTIONAL PVOID pContext,
    IN PLSA_DM_CONST_ENUM_DOMAIN_INFO pDomainInfo
    )
{
    return IsLsaDmDomainFlagsOffline(pDomainInfo->Flags) ||
        IsLsaDmDomainFlagsGcOffline(pDomainInfo->Flags);
}

static
DWORD
LsaDmpDetectTransitionOnlineAllDomains(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PLSA_DM_THREAD_INFO pThreadInfo
    )
{
    DWORD dwError = 0;
    PSTR* ppszDomainNames = NULL;
    DWORD dwCount = 0;
    DWORD dwFirstError = 0;
    DWORD dwIndex = 0;

    dwError = LsaDmpEnumDomainNames(Handle,
                                    LsaDmpFilterOfflineCallback,
                                    NULL,
                                    &ppszDomainNames,
                                    &dwCount);
    BAIL_ON_LSA_ERROR(dwError);

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        DWORD dwLocalError = 0;
        PCSTR pszCurrentDomainName = ppszDomainNames[dwIndex];

        if (pThreadInfo)
        {
            BOOLEAN bIsDone = FALSE;

            LsaDmpAcquireMutex(pThreadInfo->pMutex);
            bIsDone = pThreadInfo->bIsDone;
            LsaDmpReleaseMutex(pThreadInfo->pMutex);

            if (bIsDone)
            {
                break;
            }
        }

        dwLocalError = LsaDmpDetectTransitionOnlineDomain(
                        Handle,
                        pszCurrentDomainName);
        if (dwLocalError)
        {
            // ISSUE-2008/08/01-dalmeida -- Log something
            if (!dwFirstError)
            {
                dwFirstError = dwLocalError;
            }
        }
    }

    dwError = dwFirstError;
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING_ARRAY(ppszDomainNames);
    return dwError;

error:
    goto cleanup;
}


DWORD
LsaDmpDetectTransitionOnline(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    DWORD dwError = 0;

    if (!pszDomainName)
    {
        // Do every domain.
        dwError = LsaDmpDetectTransitionOnlineAllDomains(Handle, NULL);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaDmpDetectTransitionOnlineDomain(Handle, pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

VOID
LsaDmpTriggerOnlindeDetectionThread(
    IN LSA_DM_STATE_HANDLE Handle
    )
{
    LsaDmpAcquireMutex(Handle->OnlineDetectionThread.pMutex);
    Handle->OnlineDetectionThread.bTrigger = TRUE;
    pthread_cond_signal(Handle->OnlineDetectionThread.pCondition);
    LsaDmpReleaseMutex(Handle->OnlineDetectionThread.pMutex);
}

DWORD
LsaDmpLdapConnectionCreate(
    IN BOOLEAN bIsGc,
    IN PCSTR pszDnsDomainName,
    OUT PLSA_DM_LDAP_CONNECTION* ppConn
    )
{
    DWORD dwError = 0;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;

    dwError = LwAllocateMemory(sizeof(*pConn), (PVOID*)&pConn);
    BAIL_ON_LSA_ERROR(dwError);

    pConn->bIsGc = bIsGc;
    dwError = LwAllocateString(pszDnsDomainName, &pConn->pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    pConn->hLdapConnection = NULL;

    *ppConn = pConn;

cleanup:

    return dwError;

error:

    *ppConn = NULL;
    if (pConn != NULL)
    {
        LsaDmpLdapConnectionDestroy(pConn);
    }
    goto cleanup;
}

static
VOID
LsaDmpLdapConnectionDestroy(
    IN PLSA_DM_LDAP_CONNECTION pLdap
    )
{
    if (pLdap != NULL)
    {
        LwLdapCloseDirectory(pLdap->hLdapConnection);
        LW_SAFE_FREE_STRING(pLdap->pszDnsDomainName);
        LwFreeMemory(pLdap);
    }
}

static
DWORD
LsaDmpLdapReconnectCallback(
    IN PCSTR pszDnsDomainOrForestName,
    IN OPTIONAL PLWNET_DC_INFO pDcInfo,
    IN OPTIONAL PVOID pContext,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    HANDLE hOld = NULL;
    HANDLE hNew = NULL;
    DWORD dwFlags = 0;
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bIsAcquired = FALSE;
    PLSA_DM_DOMAIN_STATE pDomain = NULL;
    BOOLEAN bIsReconnect = FALSE;
    PLSA_DM_LDAP_RECONNECT_CONTEXT pCtx =
        (PLSA_DM_LDAP_RECONNECT_CONTEXT)pContext;
    PLSA_DM_LDAP_CONNECTION pLdap = pCtx->pLdap;
    PSTR pszPrimaryDomain = NULL;

    *pbIsNetworkError = FALSE;

    dwError = LsaDmpGetPrimaryDomainName(pCtx->Handle, &pszPrimaryDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_GetLDAPSignAndSeal())
    {
        dwFlags |= LW_LDAP_OPT_SIGN_AND_SEAL;
    }

    if (pLdap->bIsGc)
    {
        dwError = LsaLdapOpenDirectoryGc(
                        pLdap->pszDnsDomainName,
                        pszPrimaryDomain,
                        dwFlags,
                        &hNew);
    }
    else
    {
        dwError = LsaLdapOpenDirectoryDomain(
                        pLdap->pszDnsDomainName,
                        pszPrimaryDomain,
                        dwFlags,
                        &hNew);
    }
    if (dwError)
    {
        *pbIsNetworkError = TRUE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    hOld = pLdap->hLdapConnection;
    if (hOld)
    {
        LwLdapCloseDirectory(hOld);
        pLdap->hLdapConnection = NULL;
        bIsReconnect = TRUE;
    }

    LsaDmpAcquireMutex(pCtx->Handle->pMutex);
    bIsAcquired = TRUE;

    dwError = LsaDmpMustFindDomain(
                    pCtx->Handle,
                    pLdap->pszDnsDomainName,
                    &pDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (bIsReconnect)
    {
        // This is a reconnection event which marks the beginning of a new
        // connection period. The free lists need to be cleared.

        if (pLdap->bIsGc)
        {
            if (pLdap->dwConnectionPeriod == pDomain->dwGcConnectionPeriod)
            {
                LSA_LOG_INFO("Clearing ldap GC connection list for domain '%s' due to a network error.",
                        pLdap->pszDnsDomainName);
                pDomain->dwGcConnectionPeriod++;
                LsaDmpLdapConnectionListDestroy(&pDomain->pFreeGcConn);
            }
        }
        else
        {
            if (pLdap->dwConnectionPeriod == pDomain->dwDcConnectionPeriod)
            {
                LSA_LOG_INFO("Clearing ldap DC connection list for domain '%s' due to a network error.",
                        pLdap->pszDnsDomainName);
                pDomain->dwDcConnectionPeriod++;
                LsaDmpLdapConnectionListDestroy(&pDomain->pFreeDcConn);
            }
        }
    }
    if (pLdap->bIsGc)
    {
        pLdap->dwConnectionPeriod = pDomain->dwGcConnectionPeriod;
    }
    else
    {
        pLdap->dwConnectionPeriod = pDomain->dwDcConnectionPeriod;
    }

    pLdap->hLdapConnection = hNew;

cleanup:
    if (bIsAcquired)
    {
        LsaDmpReleaseMutex(pCtx->Handle->pMutex);
    }

    LW_SAFE_FREE_MEMORY(pszPrimaryDomain);

    return dwError;

error:
    if (hNew)
    {
        LwLdapCloseDirectory(hNew);
    }
    // Do not change pLdap->hLdapConnection since the old one has not been
    // freed.

    goto cleanup;
}

DWORD
LsaDmpLdapReconnect(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OUT PLSA_DM_LDAP_CONNECTION pLdap
    )
{
    LSA_DM_LDAP_RECONNECT_CONTEXT ctx;
    LSA_DM_CONNECT_DOMAIN_FLAGS flags = LSA_DM_CONNECT_DOMAIN_FLAG_AUTH;

    ctx.pLdap = pLdap;
    ctx.Handle = Handle;

    if (pLdap->bIsGc)
    {
        flags |= LSA_DM_CONNECT_DOMAIN_FLAG_GC;
    }

    return LsaDmConnectDomain(
                    pLdap->pszDnsDomainName,
                    flags,
                    NULL,
                    LsaDmpLdapReconnectCallback,
                    &ctx);
}

HANDLE
LsaDmpGetLdapHandle(
    IN PLSA_DM_LDAP_CONNECTION pConn
    )
{
    return pConn->hLdapConnection;
}

DWORD
LsaDmpLdapOpen(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDnsDomainName,
    IN BOOLEAN bUseGc,
    OUT PLSA_DM_LDAP_CONNECTION* ppConn
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsAcquired = FALSE;
    PLSA_DM_DOMAIN_STATE pDomain = NULL;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;

    BAIL_ON_INVALID_STRING(pszDnsDomainName);

    // If the global offline state says everything is offline, don't
    // return anything off the free list (the free list is only cleared when a
    // domain goes offline, not when the machine goes globally offline).
    if (LsaDmpIsDomainOffline(Handle, pszDnsDomainName, bUseGc))
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaDmpAcquireMutex(Handle->pMutex);
    bIsAcquired = TRUE;

    dwError = LsaDmpMustFindDomain(Handle, pszDnsDomainName, &pDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (bUseGc)
    {
        dwError = LsaDmpMustFindDomain(Handle, pDomain->pszForestName, &pDomain);
        BAIL_ON_LSA_ERROR(dwError);

        if (pDomain->pFreeGcConn != NULL)
        {
            pConn = pDomain->pFreeGcConn;
            pDomain->pFreeGcConn = pConn->pNext;
            pConn->pNext = NULL;
        }
    }
    else
    {
        if (pDomain->pFreeDcConn != NULL)
        {
            pConn = pDomain->pFreeDcConn;
            pDomain->pFreeDcConn = pConn->pNext;
            pConn->pNext = NULL;
        }
    }
    if (pConn == NULL)
    {
        dwError = LsaDmpLdapConnectionCreate(
                        bUseGc,
                        bUseGc ? pDomain->pszForestName : pszDnsDomainName,
                        &pConn);
        BAIL_ON_LSA_ERROR(dwError);

        // The newly created pConn will be connected to the ldap server. This
        // is done to detect a connection failure during the open call instead
        // of waiting for the caller to try to use pConn for a search.
        //
        // Since connecting pConn is a network event, the global mutex needs to
        // be released first.
        LsaDmpReleaseMutex(Handle->pMutex);
        bIsAcquired = FALSE;
        dwError = LsaDmpLdapReconnect(
                    Handle,
                    pConn);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppConn = pConn;

cleanup:
    if (bIsAcquired)
    {
        LsaDmpReleaseMutex(Handle->pMutex);
    }
    return dwError;

error:
    *ppConn = NULL;
    if (pConn != NULL)
    {
        LsaDmpLdapConnectionDestroy(pConn);
    }

    goto cleanup;
}

VOID
LsaDmpLdapClose(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PLSA_DM_LDAP_CONNECTION pConn
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsAcquired = FALSE;
    PLSA_DM_DOMAIN_STATE pDomain = NULL;

    if (pConn == NULL || Handle == NULL)
    {
        goto cleanup;
    }

    LsaDmpAcquireMutex(Handle->pMutex);
    bIsAcquired = TRUE;

    dwError = LsaDmpMustFindDomain(Handle, pConn->pszDnsDomainName, &pDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (pConn->bIsGc)
    {
        // If this isn't true, throw away the connection
        if (pConn->dwConnectionPeriod == pDomain->dwGcConnectionPeriod)
        {
            pConn->pNext = pDomain->pFreeGcConn;
            pDomain->pFreeGcConn = pConn;
            pConn = NULL;
        }
    }
    else
    {
        // If this isn't true, throw away the connection
        if (pConn->dwConnectionPeriod == pDomain->dwDcConnectionPeriod)
        {
            pConn->pNext = pDomain->pFreeDcConn;
            pDomain->pFreeDcConn = pConn;
            pConn = NULL;
        }
    }

cleanup:
    if (bIsAcquired)
    {
        LsaDmpReleaseMutex(Handle->pMutex);
    }
    if (dwError != LW_ERROR_SUCCESS)
    {
        LSA_LOG_ERROR("Error %u occurred while putting an ldap connection back in the domain free list.", dwError);
    }
    if (pConn != NULL)
    {
        LsaDmpLdapConnectionDestroy(pConn);
    }
    return;

error:

    goto cleanup;
}

BOOLEAN
LsaDmpLdapIsRetryError(
    DWORD dwError
    )
{
    switch((int)dwError)
    {
        case LW_ERROR_LDAP_SERVER_UNAVAILABLE:
        case LW_ERROR_LDAP_SERVER_DOWN:
        case LW_ERROR_LDAP_TIMEOUT:
        case LW_ERROR_LDAP_CONNECT_ERROR:
            return TRUE;
        default:
            return FALSE;
    }
}

DWORD
LsaDmpQueryForestNameFromNetlogon(
    IN PCSTR pszDnsDomainName,
    OUT PSTR* ppszDnsForestName
    )
{
    DWORD dwError = 0;
    PLWNET_DC_INFO pDcInfo = NULL;
    PSTR pszDnsForestName = NULL;

    // Try background first, then not.
    dwError = LWNetGetDCName(NULL,
                             pszDnsDomainName,
                             NULL,
                             DS_BACKGROUND_ONLY,
                             &pDcInfo);
    if (dwError)
    {
        dwError = LWNetGetDCName(NULL,
                                 pszDnsDomainName,
                                 NULL,
                                 0,
                                 &pDcInfo);
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pDcInfo->pszDnsForestName, &pszDnsForestName);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszDnsForestName = pszDnsForestName;

cleanup:
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);
    return dwError;

error:
    *ppszDnsForestName = NULL;
    LW_SAFE_FREE_STRING(pszDnsForestName);
    goto cleanup;
}

BOOLEAN
LsaDmpIsNetworkError(
    IN DWORD dwError
    )
{
    BOOLEAN bIsNetworkError = FALSE;

    switch (dwError)
    {
        case LW_ERROR_DOMAIN_IS_OFFLINE:
        case DNS_ERROR_BAD_PACKET:
        case NERR_DCNotFound:
            bIsNetworkError = TRUE;
            break;
        default:
            bIsNetworkError = FALSE;
            break;
    }

    return bIsNetworkError;
}

static
PLSA_DM_UNKNOWN_DOMAIN_ENTRY
LsaDmpFindUnknownDomainEntry(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PSID pDomainSid,
    IN OPTIONAL PCSTR pszDomainName,
    IN BOOLEAN bCanReturnExpired
    )
{
    PLSA_DM_UNKNOWN_DOMAIN_ENTRY pFoundEntry = NULL;
    PLSA_LIST_LINKS pLinks = NULL;
    PLSA_LIST_LINKS pNextLinks = NULL;
    PLSA_LIST_LINKS pHead = NULL;
    BOOLEAN bIsBySid = pDomainSid ? TRUE : FALSE;
    time_t now = time(NULL);

    LSA_ASSERT(LSA_IS_XOR(pDomainSid, pszDomainName));

    if (bIsBySid)
    {
        pHead = &Handle->UnknownDomainSidList;
    }
    else
    {
        pHead = &Handle->UnknownDomainNameList;
    }

    // NOTE: The caller must acquire the lock on the state Handle.
    // This is so that the caller can modify the entry and/or cache
    // after doing this lookup.

    for (pLinks = pHead->Next;
         pLinks != pHead;
         pLinks = pNextLinks)
    {
        PLSA_DM_UNKNOWN_DOMAIN_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pLinks, LSA_DM_UNKNOWN_DOMAIN_ENTRY, Links);
        pNextLinks = pLinks->Next;

        if ((bIsBySid && RtlEqualSid(pEntry->pSid, pDomainSid)) ||
            (!bIsBySid && !strcasecmp(pEntry->pszName, pszDomainName)))
        {
            pFoundEntry = pEntry;
            if (bCanReturnExpired)
            {
                break;
            }
        }

        // Opportunistically remove expired entries.
        if (pEntry->Time &&
            (now >= (pEntry->Time + Handle->dwUnknownDomainCacheTimeoutSeconds)))
        {
            LsaListRemove(&pEntry->Links);
            LsaDmpFreeUnknownDomainEntry(pEntry, bIsBySid);

            if (pFoundEntry)
            {
                pFoundEntry = NULL;
                break;
            }
        }
    }

    return pFoundEntry;
}

static
BOOLEAN
LsaDmpIsUnknownDomain(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PSID pDomainSid,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    PLSA_DM_UNKNOWN_DOMAIN_ENTRY pFoundEntry = NULL;

    LsaDmpAcquireMutex(Handle->pMutex);
    pFoundEntry = LsaDmpFindUnknownDomainEntry(Handle, pDomainSid, pszDomainName, FALSE);
    LsaDmpReleaseMutex(Handle->pMutex);

    return pFoundEntry ? TRUE : FALSE;
}

static
DWORD
LsaDmpCacheUnknownDomain(
    IN LSA_DM_STATE_HANDLE Handle,
    IN OPTIONAL PSID pDomainSid,
    IN OPTIONAL PCSTR pszDomainName,
    IN BOOLEAN bIsPermanent
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bIsAcquired = FALSE;
    PLSA_DM_UNKNOWN_DOMAIN_ENTRY pFoundEntry = NULL;
    PLSA_DM_UNKNOWN_DOMAIN_ENTRY pNewEntry = NULL;
    PLSA_LIST_LINKS pHead = NULL;
    BOOLEAN bIsBySid = pDomainSid ? TRUE : FALSE;

    if (bIsBySid)
    {
        pHead = &Handle->UnknownDomainSidList;
    }
    else
    {
        pHead = &Handle->UnknownDomainNameList;
    }

    LsaDmpAcquireMutex(Handle->pMutex);
    bIsAcquired = TRUE;

    pFoundEntry = LsaDmpFindUnknownDomainEntry(
                        Handle,
                        pDomainSid,
                        pszDomainName,
                        TRUE);
    if (pFoundEntry)
    {
        if (pFoundEntry->Time)
        {
            pFoundEntry->Time = time(NULL);
        }
        goto cleanup;
    }

    dwError = LwAllocateMemory(sizeof(*pNewEntry), OUT_PPVOID(&pNewEntry));
    BAIL_ON_LSA_ERROR(dwError);

    if (bIsBySid)
    {
        dwError = LsaDmpDuplicateSid(&pNewEntry->pSid, pDomainSid);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateString(pszDomainName, &pNewEntry->pszName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pNewEntry->Time = bIsPermanent ? 0 : time(NULL);

    LsaListInsertHead(pHead, &pNewEntry->Links);

cleanup:
    if (bIsAcquired)
    {
        LsaDmpReleaseMutex(Handle->pMutex);
    }

    return dwError;

error:
    if (pNewEntry)
    {
        LsaDmpFreeUnknownDomainEntry(pNewEntry, bIsBySid);
    }
    goto cleanup;
}

VOID
LsaDmpFreeUnknownDomainEntry(
    IN OUT PLSA_DM_UNKNOWN_DOMAIN_ENTRY pEntry,
    IN BOOLEAN bIsSidEntry
    )
{
    if (pEntry)
    {
        if (bIsSidEntry)
        {
            LwFreeMemory(pEntry->pSid);
        }
        else
        {
            LwFreeMemory(pEntry->pszName);
        }
        LwFreeMemory(pEntry);
    }
}

BOOLEAN
LsaDmpIsUnknownDomainSid(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PSID pDomainSid
    )
{
    return LsaDmpIsUnknownDomain(Handle, pDomainSid, NULL);
}

BOOLEAN
LsaDmpIsUnknownDomainName(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    )
{
    return LsaDmpIsUnknownDomain(Handle, NULL, pszDomainName);
}

DWORD
LsaDmpCacheUnknownDomainSid(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PSID pDomainSid
    )
{
    return LsaDmpCacheUnknownDomain(Handle, pDomainSid, NULL, FALSE);
}

DWORD
LsaDmpCacheUnknownDomainName(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    )
{
    return LsaDmpCacheUnknownDomain(Handle, NULL, pszDomainName, FALSE);
}

DWORD
LsaDmpCacheUnknownDomainSidForever(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PSID pDomainSid
    )
{
    return LsaDmpCacheUnknownDomain(Handle, pDomainSid, NULL, TRUE);
}

DWORD
LsaDmpCacheUnknownDomainNameForever(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    )
{
    return LsaDmpCacheUnknownDomain(Handle, NULL, pszDomainName, TRUE);
}

static
BOOLEAN
LsaDmpIsExceptionTrust(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    )
{
    BOOLEAN bIsException = FALSE;
    DWORD i = 0;

    for (i = 0; i < Handle->dwTrustExceptionCount; i++)
    {
        if (!strcasecmp(Handle->ppszTrustExceptionList[i], pszDomainName))
        {
            bIsException = TRUE;
            goto cleanup;
        }
    }

    bIsException = FALSE;

cleanup:

    return bIsException;
}

static
BOOLEAN
LsaDmpIsIgnoreTrustInternal(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName,
    IN OPTIONAL PCSTR pszOtherDomainName
    )
{
    BOOLEAN bIsIgnoreTrust = FALSE;
    BOOLEAN bIsAcquired = FALSE;
    BOOLEAN bDefaultIsIgnoreTrust = FALSE;
    BOOLEAN bIsException = FALSE;

    if (LW_IS_NULL_OR_EMPTY_STR(pszDomainName))
    {
        LSA_ASSERT(FALSE);
        bIsIgnoreTrust = FALSE;
        goto cleanup;
    }

    LsaDmpAcquireMutex(Handle->pMutex);
    bIsAcquired = TRUE;

    if (!pszOtherDomainName &&
        IsSetFlag(Handle->StateFlags, LSA_DM_STATE_FLAG_IGNORE_ALL_TRUSTS) &&
        (Handle->dwTrustExceptionCount > 0))
    {
        // Cannot be certain since there could be an exception (which
        // includes the domain) for the other (DNS or NetBIOS) domain name.
        bIsIgnoreTrust = FALSE;
        goto cleanup;
    }

    // If the domain is already in the domain manager, it cannot be
    // ignored no matter what.  Can lookup just one name.
    if (LsaDmpFindDomain(Handle, pszDomainName))
    {
        bIsIgnoreTrust = FALSE;
        goto cleanup;
    }

    if (IsSetFlag(Handle->StateFlags, LSA_DM_STATE_FLAG_IGNORE_ALL_TRUSTS))
    {
        bDefaultIsIgnoreTrust = TRUE;
    }
    else
    {
        bDefaultIsIgnoreTrust = FALSE;
    }

    bIsException = LsaDmpIsExceptionTrust(Handle, pszDomainName);
    if (!bIsException && pszOtherDomainName)
    {
        bIsException = LsaDmpIsExceptionTrust(
                            Handle,
                            pszOtherDomainName);
    }

    if (bIsException)
    {
        bIsIgnoreTrust = !bDefaultIsIgnoreTrust;
    }
    else
    {
        bIsIgnoreTrust = bDefaultIsIgnoreTrust;
    }

cleanup:
    if (bIsAcquired)
    {
        LsaDmpReleaseMutex(Handle->pMutex);
    }

    return bIsIgnoreTrust;
}

BOOLEAN
LsaDmpIsCertainIgnoreTrust(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDomainName
    )
{
    return LsaDmpIsIgnoreTrustInternal(
                Handle,
                pszDomainName,
                NULL);
}

BOOLEAN
LsaDmpIsIgnoreTrust(
    IN LSA_DM_STATE_HANDLE Handle,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName
    )
{
    BOOLEAN bIsIgnoreTrust = FALSE;

    if (LW_IS_NULL_OR_EMPTY_STR(pszNetbiosDomainName))
    {
        LSA_ASSERT(FALSE);
        bIsIgnoreTrust = FALSE;
    }
    else
    {
        bIsIgnoreTrust = LsaDmpIsIgnoreTrustInternal(
                                Handle,
                                pszDnsDomainName,
                                pszNetbiosDomainName);
    }

    return bIsIgnoreTrust;
}

DWORD
LsaDmpQueryExcludeTrusts(
    IN LSA_DM_STATE_HANDLE Handle,
    OUT PSTR** pppszTrustList,
    OUT PDWORD pdwTrustCount
    )
{
    DWORD dwError = 0;

    LsaDmpAcquireMutex(Handle->pMutex);
    if (IsSetFlag(Handle->StateFlags, LSA_DM_STATE_FLAG_IGNORE_ALL_TRUSTS))
    {
        *pppszTrustList = NULL;
        *pdwTrustCount = 0;
    }
    else
    {
        dwError = LwDuplicateStringArray(
                        pppszTrustList,
                        pdwTrustCount,
                        Handle->ppszTrustExceptionList,
                        Handle->dwTrustExceptionCount);
    }
    LsaDmpReleaseMutex(Handle->pMutex);

    return dwError;
}

DWORD
LsaDmpQueryIncludeTrusts(
    IN LSA_DM_STATE_HANDLE Handle,
    OUT PSTR** pppszTrustList,
    OUT PDWORD pdwTrustCount
    )
{
    DWORD dwError = 0;

    LsaDmpAcquireMutex(Handle->pMutex);
    if (!IsSetFlag(Handle->StateFlags, LSA_DM_STATE_FLAG_IGNORE_ALL_TRUSTS))
    {
        *pppszTrustList = NULL;
        *pdwTrustCount = 0;
    }
    else
    {
        dwError = LwDuplicateStringArray(
                        pppszTrustList,
                        pdwTrustCount,
                        Handle->ppszTrustExceptionList,
                        Handle->dwTrustExceptionCount);
    }
    LsaDmpReleaseMutex(Handle->pMutex);

    return dwError;
}


VOID
ADLogMediaSenseOnlineEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR  pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Media sense detected network available. Switching to online mode:\r\n\r\n" \
                 "     Authentication provider:   %s",
                 LSA_SAFE_LOG_STRING(gpszADProviderName));
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_INFO_NETWORK_DOMAIN_ONLINE_TRANSITION,
            NETWORK_EVENT_CATEGORY,
            pszDescription,
            NULL);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
ADLogMediaSenseOfflineEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR  pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Media sense detected network is not available. Switching to offline mode:\r\n\r\n" \
                 "     Authentication provider:   %s",
                 LSA_SAFE_LOG_STRING(gpszADProviderName));
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceWarningEvent(
            LSASS_EVENT_WARNING_NETWORK_DOMAIN_OFFLINE_TRANSITION,
            NETWORK_EVENT_CATEGORY,
            pszDescription,
            NULL);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
ADLogDomainOnlineEvent(
    PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    PSTR  pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Detected domain controller for Active Directory domain. Switching to online mode:\r\n\r\n" \
                 "     Authentication provider:   %s\r\n\r\n" \
                 "     Domain:                    %s",
                 LSA_SAFE_LOG_STRING(gpszADProviderName),
                 pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_INFO_NETWORK_DOMAIN_ONLINE_TRANSITION,
            NETWORK_EVENT_CATEGORY,
            pszDescription,
            NULL);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
ADLogDomainOfflineEvent(
    IN PCSTR pszDomainName,
    IN BOOLEAN bIsGc
    )
{
    DWORD dwError = 0;
    PSTR  pszDescription = NULL;

    if (bIsGc)
    {
        dwError = LwAllocateStringPrintf(
                     &pszDescription,
                     "Detected unreachable global catalog server for Active Directory forest. Switching to offline mode:\r\n\r\n" \
                     "     Authentication provider:   %s\r\n\r\n" \
                     "     Forest:                    %s",
                     LSA_SAFE_LOG_STRING(gpszADProviderName),
                     pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateStringPrintf(
                     &pszDescription,
                     "Detected unreachable domain controller for Active Directory domain. Switching to offline mode:\r\n\r\n" \
                     "     Authentication provider:   %s\r\n\r\n" \
                     "     Domain:                    %s",
                     LSA_SAFE_LOG_STRING(gpszADProviderName),
                     pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaSrvLogServiceWarningEvent(
            LSASS_EVENT_WARNING_NETWORK_DOMAIN_OFFLINE_TRANSITION,
            NETWORK_EVENT_CATEGORY,
            pszDescription,
            NULL);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

