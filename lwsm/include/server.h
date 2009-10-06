/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        server.h
 *
 * Abstract:
 *
 *        Server private header file
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LWSM_SERVER_H__
#define __LWSM_SERVER_H__

#include "common.h"

#include <pthread.h>

struct _SM_TABLE_ENTRY;

typedef struct _SM_OBJECT_VTBL
{
    DWORD (*pfnStart)(struct _SM_TABLE_ENTRY* pEntry);
    DWORD (*pfnStop)(struct _SM_TABLE_ENTRY* pEntry);
    DWORD (*pfnGetStatus)(struct _SM_TABLE_ENTRY* pEntry, PLW_SERVICE_STATUS pStatus);
    DWORD (*pfnRefresh)(struct _SM_TABLE_ENTRY* pEntry);
    DWORD (*pfnConstruct)(struct _SM_TABLE_ENTRY* pEntry);
    VOID  (*pfnDestruct)(struct _SM_TABLE_ENTRY* pEntry);
} SM_OBJECT_VTBL, *PSM_OBJECT_VTBL;

/* Entry in the running object table */
typedef struct _SM_TABLE_ENTRY
{
    /* Details */
    PLW_SERVICE_INFO pInfo;
    /* Is entry still valid? */
    BOOL volatile bValid;
    /* Are dependencies marked? */
    BOOL volatile bDepsMarked;
    /* Lock controlling access to entry */
    pthread_mutex_t lock;
    pthread_mutex_t* pLock;
    /* State change event */
    pthread_cond_t event;
    pthread_cond_t* pEvent;
    /* Pointer to vtbl */
    PSM_OBJECT_VTBL pVtbl;
    /* Data */
    void* pData;
    /* Reverse dependency count
       
       This is the number of other running services
       which depend directly on us.  When it is > 0,
       the service cannot be safely stopped */
    DWORD volatile dwDepCount;
    /* Reference count
       
       This is the number of holders of a reference to
       this entry -- in particular, by service handles.
       
       The reference count is protected by the table lock
       and not the entry lock */
    DWORD volatile dwRefCount;
    /* Links to siblings (protected by table lock) */
    SM_LINK link;
} SM_TABLE_ENTRY, *PSM_TABLE_ENTRY;

/* Global running object table */
typedef struct _SM_TABLE
{
    pthread_mutex_t  lock;
    pthread_mutex_t* pLock; 
    SM_LINK entries;
} SM_TABLE;

/* API handle */
struct _LW_SERVICE_HANDLE
{
    /* Pointer to table entry */
    PSM_TABLE_ENTRY pEntry;
};

/* Bootstrap service definition */
typedef struct _SM_BOOTSTRAP_SERVICE
{
    PCSTR pszName;
    LW_SERVICE_TYPE type;
    PCSTR pszPath;
    CHAR const * const ppszArgs[];
} SM_BOOTSTRAP_SERVICE, *PSM_BOOTSTRAP_SERVICE;

DWORD
LwSmSrvAcquireServiceHandle(
    PCWSTR pwszName,
    PLW_SERVICE_HANDLE phHandle
    );

VOID
LwSmSrvReleaseHandle(
    LW_SERVICE_HANDLE hHandle
    );

DWORD
LwSmSrvEnumerateServices(
    PWSTR** pppwszServiceNames
    );

DWORD
LwSmSrvGetServiceStatus(
    LW_SERVICE_HANDLE hHandle,
    PLW_SERVICE_STATUS pStatus
    );

DWORD
LwSmSrvStartService(
    LW_SERVICE_HANDLE hHandle
    );

DWORD
LwSmSrvStopService(
    LW_SERVICE_HANDLE hHandle
    );

DWORD
LwSmSrvGetServiceInfo(
    LW_SERVICE_HANDLE hHandle,
    PLW_SERVICE_INFO* ppInfo
    );

LWMsgDispatchSpec*
LwSmGetDispatchSpec(
    VOID
    );

DWORD
LwSmTableGetEntry(
    PCWSTR pwszName,
    PSM_TABLE_ENTRY* ppEntry
    );

DWORD
LwSmTableEnumerateEntries(
    PWSTR** pppwszServiceNames
    );

DWORD
LwSmTableAddEntry(
    PLW_SERVICE_INFO pInfo,
    PSM_TABLE_ENTRY* ppEntry
    );

DWORD
LwSmTableUpdateEntry(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_INFO pInfo
    );

VOID
LwSmTableRetainEntry(
    PSM_TABLE_ENTRY pEntry
    );

VOID
LwSmTableReleaseEntry(
    PSM_TABLE_ENTRY pEntry
    );

DWORD
LwSmTableStartEntry(
    PSM_TABLE_ENTRY pEntry
    );

DWORD
LwSmTableStopEntry(
    PSM_TABLE_ENTRY pEntry
    );

DWORD
LwSmTableRefreshEntry(
    PSM_TABLE_ENTRY pEntry
    );

VOID
LwSmTableNotifyEntryChanged(
    PSM_TABLE_ENTRY pEntry
    );

DWORD
LwSmTableWaitEntryChanged(
    PSM_TABLE_ENTRY pEntry
    );

DWORD
LwSmTableGetEntryStatus(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_STATUS pStatus
    );

DWORD
LwSmTableGetEntryDependencyClosure(
    PSM_TABLE_ENTRY pEntry,
    PWSTR** pppwszServiceList
    );

DWORD
LwSmTableGetEntryReverseDependencyClosure(
    PSM_TABLE_ENTRY pEntry,
    PWSTR** pppwszServiceList
    );

DWORD
LwSmRegistryEnumServices(
    HANDLE hReg,
    PWSTR** pppwszNames
    );

    
DWORD
LwSmRegistryReadServiceInfo(
    HANDLE hReg,
    PCWSTR pwszName,
    PLW_SERVICE_INFO* ppInfo
    );

DWORD
LwSmBootstrap(
    VOID
    );

extern SM_OBJECT_VTBL gExecutableVtbl;

#endif
