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
 *        event.h
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Eventlog API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

VOID
LWNetSrvInitEventlogInterface(
    VOID
    )
{
    DWORD dwError = 0;
    PEVENTLOG_INTERFACE pEventLogItf = NULL;
    PVOID pLibHandle = NULL;
    PCSTR pszError = NULL;
    PSTR pszLibDirPath = NULL;
    CHAR szEventLogLibPath[PATH_MAX+1];
    BOOLEAN bExists = FALSE;
    PEVENTAPIFUNCTIONTABLE  pFuncTable = NULL;
    PFN_INITIALIZE_EVENTAPI pfnInitEventApi = NULL;
    PFN_SHUTDOWN_EVENTAPI   pfnShutdownEventApi = NULL;
    BOOLEAN bInLock = FALSE;
    
    ENTER_EVENTLOG_WRITER_LOCK(bInLock);
    
    dwError = LWNetGetLibDirPath(&pszLibDirPath);
    BAIL_ON_LWNET_ERROR(dwError);
    
    sprintf(szEventLogLibPath, "%s/libeventdlapi.so", pszLibDirPath);
    
    dwError = LWNetCheckFileExists(szEventLogLibPath, &bExists);
    BAIL_ON_LWNET_ERROR(dwError);
    
    if (!bExists) {
       ghEventLogItf = (HANDLE)NULL;
       goto cleanup;
    }
    
    dlerror();
    pLibHandle = dlopen(szEventLogLibPath, RTLD_NOW | RTLD_GLOBAL);
    if (pLibHandle == NULL) {
        
        pszError = dlerror();
                
        LWNET_LOG_ERROR("Error: Failed to load Likewise Eventlog Module [%s]",
             IsNullOrEmptyString(pszError) ? "" : pszError);
        
        dwError = LWNET_ERROR_LOAD_LIBRARY_FAILED;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dlerror();
    pfnInitEventApi = (PFN_INITIALIZE_EVENTAPI)dlsym(
                                pLibHandle, 
                                EVENTAPI_INITIALIZE_FUNCTION);
    if (pfnInitEventApi == NULL) {
        pszError = dlerror();
        
        LWNET_LOG_ERROR("Error: Failed to lookup symbol %s in Eventlog Module [%s]",
                      IsNullOrEmptyString(pszError) ? "" : pszError);
        
        dwError = LWNET_ERROR_LOOKUP_SYMBOL_FAILED;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dlerror();
    pfnShutdownEventApi = (PFN_SHUTDOWN_EVENTAPI)dlsym(
                                pLibHandle,
                                EVENTAPI_SHUTDOWN_FUNCTION);
    if (pfnShutdownEventApi == NULL) {
        pszError = dlerror();
        
        LWNET_LOG_ERROR("Error: Failed to lookup symbol %s in Eventlog Module [%s]",
                        IsNullOrEmptyString(pszError) ? "" : pszError);
        
        dwError = LWNET_ERROR_LOOKUP_SYMBOL_FAILED;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dwError = pfnInitEventApi(&pFuncTable);
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LWNetSrvValidateEventlogInterface(pFuncTable);
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LWNetAllocateMemory(
                    sizeof(EVENTLOG_INTERFACE),
                    (PVOID*)&pEventLogItf);
    BAIL_ON_LWNET_ERROR(dwError);
    
    pEventLogItf->pFuncTable = pFuncTable;
    pEventLogItf->pLibHandle = pLibHandle;
    pEventLogItf->pfnShutdownEventApi = pfnShutdownEventApi;
    
    ghEventLogItf = (HANDLE)pEventLogItf;
    
cleanup:

    LEAVE_EVENTLOG_WRITER_LOCK(bInLock);

    LWNET_SAFE_FREE_STRING(pszLibDirPath);

    return;
    
error:

    if (pfnShutdownEventApi) {
        pfnShutdownEventApi();
    }
    
    if (pLibHandle) {
        dlclose(pLibHandle);
    }
    
    ghEventLogItf = (HANDLE)NULL;
    
    LWNET_LOG_ERROR("Failed to load eventlog module [Error code: %d]", dwError);

    goto cleanup;
}

DWORD
LWNetSrvValidateEventlogInterface(
    PEVENTAPIFUNCTIONTABLE pFuncTable
    )
{
    DWORD dwError = 0;
    
    if (!pFuncTable ||
        !pFuncTable->pfnClearEventLog ||
        !pFuncTable->pfnCloseEventLog ||
        !pFuncTable->pfnCountEventLog ||
        !pFuncTable->pfnDeleteFromEventLog ||
        !pFuncTable->pfnFreeEventRecord ||
        !pFuncTable->pfnOpenEventLog ||
        !pFuncTable->pfnOpenEventLogEx ||
        !pFuncTable->pfnReadEventLog ||
        !pFuncTable->pfnSetEventLogComputer ||
        !pFuncTable->pfnSetEventLogSource ||
        !pFuncTable->pfnSetEventLogTableCategory ||
        !pFuncTable->pfnSetEventLogTableCategoryId ||
        !pFuncTable->pfnSetEventLogType ||
        !pFuncTable->pfnSetEventLogUser ||
        !pFuncTable->pfnWriteEventLog ||
        !pFuncTable->pfnWriteEventLogBase)
    {
       dwError = LWNET_ERROR_INVALID_EVENTLOG;
    }
    
    return dwError;
}

VOID
LWNetSrvShutdownEventlogInterface(
    VOID
    )
{
    BOOLEAN bInLock = FALSE;
    
    ENTER_EVENTLOG_WRITER_LOCK(bInLock);
    
    PEVENTLOG_INTERFACE pEventLogItf = (PEVENTLOG_INTERFACE)ghEventLogItf;
    if (pEventLogItf) {
       if (pEventLogItf->pfnShutdownEventApi) {
          pEventLogItf->pfnShutdownEventApi();
       }
       if (pEventLogItf->pLibHandle) {
           dlclose(pEventLogItf->pLibHandle);
       }
       LWNetFreeMemory(pEventLogItf);
    }
    
    LEAVE_EVENTLOG_WRITER_LOCK(bInLock);
}

DWORD
LWNetSrvOpenEventLog(
    HANDLE  hServer,
    PHANDLE phEventLog
    )
{
    DWORD dwError = 0;
    PLWNET_SRV_API_STATE pServerState = (PLWNET_SRV_API_STATE)hServer;
    PEVENTLOG_INTERFACE pEventLogItf = NULL;
    BOOLEAN bInLock = FALSE;
    
    ENTER_EVENTLOG_READER_LOCK(bInLock);
    
    if (ghEventLogItf == (HANDLE)NULL) {
       *phEventLog = (HANDLE)NULL;
       goto cleanup;
    }
    
    pEventLogItf = (PEVENTLOG_INTERFACE)ghEventLogItf;

    if (pServerState->hEventLog == (HANDLE)NULL) {
       dwError = pEventLogItf->pFuncTable->pfnOpenEventLogEx(
                      NULL,
                      TableCategorySystem,
                      "Likewise Site Manager",
                      0x9000,
                      "likewise-netlogond",
                      NULL,
                      &pServerState->hEventLog);
       BAIL_ON_LWNET_ERROR(dwError);
    }

    *phEventLog = pServerState->hEventLog;

cleanup:

    LEAVE_EVENTLOG_READER_LOCK(bInLock);

    return dwError; 

error:

    *phEventLog = (HANDLE)NULL;

    goto cleanup;
}

DWORD
LWNetSrvCloseEventLog(
    HANDLE hEventLog
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    
    ENTER_EVENTLOG_READER_LOCK(bInLock);
    
    if (ghEventLogItf != (HANDLE)NULL) {
       PEVENTLOG_INTERFACE pEventLogItf = 
           (PEVENTLOG_INTERFACE)(ghEventLogItf);
       dwError = pEventLogItf->pFuncTable->pfnCloseEventLog(hEventLog);
       BAIL_ON_LWNET_ERROR(dwError);
    }
    
cleanup:

    LEAVE_EVENTLOG_READER_LOCK(bInLock);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWNetSrvLogInfoEvent(
    HANDLE hServer,
    PCSTR  pszEventType,
    PCSTR  pszCategory,
    PCSTR  pszDescription
    )
{
    DWORD dwError = 0;
    HANDLE hEventLog = (HANDLE)NULL;
    BOOLEAN bInLock = FALSE;
    PEVENTLOG_INTERFACE pEventLogItf = NULL;
    
    ENTER_EVENTLOG_READER_LOCK(bInLock);
    
    if (ghEventLogItf == (HANDLE)NULL) {
       goto cleanup;
    }
    
    pEventLogItf = (PEVENTLOG_INTERFACE)ghEventLogItf;

    dwError = LWNetSrvOpenEventLog(
                   hServer,
                   &hEventLog);
    BAIL_ON_LWNET_ERROR(dwError); 

    dwError = pEventLogItf->pFuncTable->pfnWriteEventLog(
                   hEventLog,
                   pszEventType,
                   pszCategory,
                   pszDescription);
    BAIL_ON_LWNET_ERROR(dwError);

cleanup:

    LEAVE_EVENTLOG_READER_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

