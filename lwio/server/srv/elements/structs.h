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
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */
#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _SRV_SESSION_ENUM_QUERY
{
    PWSTR    pwszUsername;
    ULONG64  ullSessionCount;

} SRV_SESSION_ENUM_QUERY, *PSRV_SESSION_ENUM_QUERY;

typedef struct _SRV_SESSION_DEL_QUERY
{
    PLWRTL_RB_TREE pSessionCollection;
    PWSTR          pwszUsername;
} SRV_SESSION_DEL_QUERY, *PSRV_SESSION_DEL_QUERY;

typedef struct _SRV_TIMER_REQUEST
{
    LONG                   refCount;

    LONG64                 llExpiry;
    PVOID                  pUserData;
    PFN_SRV_TIMER_CALLBACK pfnTimerExpiredCB;

    struct _SRV_TIMER_REQUEST* pNext;
    struct _SRV_TIMER_REQUEST* pPrev;

} SRV_TIMER_REQUEST;

typedef struct _SRV_TIMER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    pthread_cond_t   event;
    pthread_cond_t*  pEvent;

    PSRV_TIMER_REQUEST pRequests;

    BOOLEAN bStop;

} SRV_TIMER_CONTEXT, *PSRV_TIMER_CONTEXT;

typedef struct _SRV_TIMER
{
    pthread_t  timerThread;
    pthread_t* pTimerThread;

    SRV_TIMER_CONTEXT context;

} SRV_TIMER, *PSRV_TIMER;

typedef struct _SRV_ELEMENTS_RESOURCES
{
    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    PLWRTL_RB_TREE    pResources;

    ULONG             ulNextAvailableId;

} SRV_ELEMENTS_RESOURCES, *PSRV_ELEMENTS_RESOURCES;

typedef struct _SRV_ELEMENTS_ENUM_RESOURCES
{
    SRV_RESOURCE_TYPE  resourceType;
    PFN_ENUM_RESOURCES pfnEnumResourcesCB;
    PVOID              pUserData;
    BOOLEAN            bContinue;

} SRV_ELEMENTS_ENUM_RESOURCES, *PSRV_ELEMENTS_ENUM_RESOURCES;

typedef struct _SRV_DEBITOR
{
    ULONG64 ullSequence;

    struct _SRV_DEBITOR* pPrev;
    struct _SRV_DEBITOR* pNext;

} SRV_DEBITOR, *PSRV_DEBITOR;

typedef struct _SRV_CREDITOR
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    ULONG64 ullNextAvblId;

    BOOLEAN bInitialized;

    PSRV_DEBITOR pAvbl_head;    // Available sequences
    PSRV_DEBITOR pAvbl_tail;

    PSRV_DEBITOR pInterim_head; // Interim sequences
    PSRV_DEBITOR pInterim_tail;

    PSRV_DEBITOR pInUse_head;   // Non-interim sequences being processed
    PSRV_DEBITOR pInUse_tail;

    USHORT  usCreditLimit;  // how many credits can be acquired from global pool
    USHORT  usTotalCredits; // total number of currently available credits

} SRV_CREDITOR;

typedef struct _SRV_ELEMENTS_CONFIG
{
    BOOLEAN bShareNameEcpEnabled;
    BOOLEAN bClientAddressEcpEnabled;
    BOOLEAN bOEMSessionEcpEnabled;

    ULONG  ulGlobalCreditLimit;
    ULONG  usClientCreditLimit;

} SRV_ELEMENTS_CONFIG, *PSRV_ELEMENTS_CONFIG;

typedef struct _SRV_ELEMENTS_GLOBALS
{
    pthread_mutex_t  mutex;

    SRV_TIMER timer;

    PBYTE pHintsBuffer;
    ULONG ulHintsLength;

    LONG64 llBootTime;

    ULONG  ulGlobalCreditLimit;

    pthread_rwlock_t    configLock;
    pthread_rwlock_t*   pConfigLock;
    SRV_ELEMENTS_CONFIG config;

    pthread_rwlock_t        statsLock;
    pthread_rwlock_t*       pStatsLock;
    SRV_ELEMENTS_STATISTICS stats;

    SRV_ELEMENTS_RESOURCES  resources;

} SRV_ELEMENTS_GLOBALS, *PSRV_ELEMENTS_GLOBALS;

typedef enum _SRV_OBJECT_TYPE {
    SRV_OBJECT_TYPE_INVALID,
    SRV_OBJECT_TYPE_CONNECTION,
    SRV_OBJECT_TYPE_SESSION,
    SRV_OBJECT_TYPE_TREE,
    SRV_OBJECT_TYPE_FILE,
    SRV_OBJECT_TYPE_SESSION_2,
    SRV_OBJECT_TYPE_TREE_2,
    SRV_OBJECT_TYPE_FILE_2,
    SRV_OBJECT_TYPE_ASYNC_STATE
} SRV_OBJECT_TYPE, *PSRV_OBJECT_TYPE;

typedef struct _SRV_OBJECT_CALLBACK_INFO {
    SRV_OBJECT_TYPE objectType;
    union {
        PFN_SRV_CONNECTION_ENUM_CALLBACK pfnConnection;
        PFN_SRV_SESSION_ENUM_CALLBACK pfnSession;
        PFN_SRV_TREE_ENUM_CALLBACK pfnTree;
        PFN_SRV_FILE_ENUM_CALLBACK pfnFile;
        PFN_SRV_SESSION_2_ENUM_CALLBACK pfnSession2;
        PFN_SRV_TREE_2_ENUM_CALLBACK pfnTree2;
        PFN_SRV_FILE_2_ENUM_CALLBACK pfnFile2;
        PFN_SRV_ASYNC_STATE_ENUM_CALLBACK pfnAsyncState;
    } callback;
} SRV_OBJECT_CALLBACK_INFO, *PSRV_OBJECT_CALLBACK_INFO;

typedef struct _SRV_ENUM_CALLBACK_CONTEXT {
    PSRV_OBJECT_CALLBACK_INFO pCallbackInfo;
    PVOID pContext;
} SRV_ENUM_CALLBACK_CONTEXT, *PSRV_ENUM_CALLBACK_CONTEXT;

#endif /* __STRUCTS_H__ */



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/


