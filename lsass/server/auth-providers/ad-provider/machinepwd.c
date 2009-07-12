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
 *        machinepwd.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Machine Password API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */

#include "adprovider.h"

DWORD
ADInitMachinePasswordSync(
    VOID
    )
{
    DWORD dwError = 0;
    
    dwError = LwpsOpenPasswordStore(
                    LWPS_PASSWORD_STORE_DEFAULT,
                    &ghPasswordStore);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
ADStartMachinePasswordSync(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = pthread_create(&gMachinePasswordSyncThread,
                             NULL,
                             ADSyncMachinePasswords,
                             NULL);
    BAIL_ON_LSA_ERROR(dwError);

    gpMachinePasswordSyncThread = &gMachinePasswordSyncThread;

cleanup:

    return dwError;

error:

    gpMachinePasswordSyncThread = NULL;

    goto cleanup;
}

PVOID
ADSyncMachinePasswords(
    PVOID pData
    )
{
    DWORD dwError = 0;
    DWORD dwPasswordSyncLifetime = 0;
    struct timespec timeout = {0, 0};
    PLWPS_PASSWORD_INFO pAcctInfo = NULL;    
    PSTR pszHostname = NULL;
    DWORD dwGoodUntilTime = 0;

    LSA_LOG_INFO("Machine Password Sync Thread starting");

    pthread_mutex_lock(&gMachinePasswordSyncThreadLock);
    
    do
    {
        DWORD dwReapingAge = 0;
        DWORD dwCurrentPasswordAge = 0;
        BOOLEAN bRefreshTGT = FALSE;
        
        dwError = LsaDnsGetHostInfo(&pszHostname);
        if (dwError) {
           LSA_LOG_ERROR("Error: Failed to find hostname (Error code: %ld)",
                         dwError);
           dwError = 0;
           goto lsa_wait_resync;
        }

        ADSyncTimeToDC(gpADProviderData->szDomain);
        
        dwError = LwpsGetPasswordByHostName(
                        ghPasswordStore,
                        pszHostname,
                        &pAcctInfo);
        if (dwError) {
            LSA_LOG_ERROR("Error: Failed to re-sync machine account (Error code: %ld)", dwError);
            dwError = 0;
            goto lsa_wait_resync;
        }
        
        dwCurrentPasswordAge = 
                         difftime(
                              time(NULL),
                              pAcctInfo->last_change_time);
        
        dwPasswordSyncLifetime = AD_GetMachinePasswordSyncPwdLifetime();
        dwReapingAge = dwPasswordSyncLifetime / 2;

        dwError = AD_MachineCredentialsCacheInitialize();
        if (dwError)
        {
            LSA_LOG_DEBUG("Failed to initialize credentials cache (error = %d)", dwError);
            dwError = 0;
            goto lsa_wait_resync;
        }

        if (dwCurrentPasswordAge >= dwReapingAge)
        {
            LSA_LOG_VERBOSE("Changing machine password");
            dwError = NetMachineChangePassword();           
            if (dwError) {
                LSA_LOG_ERROR("Error: Failed to re-sync machine account [Error code: %ld]", dwError);                
                
                if (AD_EventlogEnabled())
                {
                    ADLogMachinePWUpdateFailureEvent(dwError);      
                }
                
                dwError = 0;
                goto lsa_wait_resync;
            }            
            
            if (AD_EventlogEnabled())
            {
                ADLogMachinePWUpdateSuccessEvent();
            }            
            
            bRefreshTGT = TRUE;
        }
        else
        {
        	bRefreshTGT = ADShouldRefreshMachineTGT();
        }
        
        if (bRefreshTGT)
        {
		dwError = LwKrb5RefreshMachineTGT(&dwGoodUntilTime);
        	if (dwError)
        	{
                    if (AD_EventlogEnabled())
                    {
                        ADLogMachineTGTRefreshFailureEvent(dwError);
                    }

                    LSA_LOG_ERROR("Error: Failed to refresh machine TGT [Error code: %ld]", dwError);
                    dwError = 0;
                    goto lsa_wait_resync;
        	}
                ADSetMachineTGTExpiry(dwGoodUntilTime);
        	
        	LSA_LOG_VERBOSE("Machine TGT was refreshed successfully");

                if (AD_EventlogEnabled())
                {
                    ADLogMachineTGTRefreshSuccessEvent();
                }
        }
        
lsa_wait_resync:

        if (pAcctInfo) {
            LwpsFreePasswordInfo(ghPasswordStore, pAcctInfo);
            pAcctInfo = NULL;
        }

        LSA_SAFE_FREE_STRING(pszHostname);
        
        timeout.tv_sec = time(NULL) + 
                     gdwMachinePasswordSyncThreadWaitSecs;
        timeout.tv_nsec = 0;
        
retry_wait:

        dwError = pthread_cond_timedwait(&gMachinePasswordSyncThreadCondition,
                                         &gMachinePasswordSyncThreadLock,
                                         &timeout);
        
        if (ADProviderIsShuttingDown())
           break;

        if (dwError) {
           if (dwError == ETIMEDOUT) {
              dwError = 0;
              if (time(NULL) < timeout.tv_sec)
              {
                  // It didn't really timeout. Something else happened
                  goto retry_wait;
              }
           } else {
              BAIL_ON_LSA_ERROR(dwError);
           }
        }
        
    } while (1);
    
cleanup:

    if (pAcctInfo) {
        LwpsFreePasswordInfo(ghPasswordStore, pAcctInfo);
    }

    LSA_SAFE_FREE_STRING(pszHostname);

    pthread_mutex_unlock(&gMachinePasswordSyncThreadLock);
    
    LSA_LOG_INFO("Machine Password Sync Thread stopping");

    return NULL;

error:

    LSA_LOG_ERROR("Machine password sync thread exiting due to error [code: %ld]", dwError);

    goto cleanup;
}

VOID
ADSyncTimeToDC(
    PCSTR pszDomainFQDN
    )
{
    DWORD dwError = 0;
    LWNET_UNIX_TIME_T dcTime = 0;
    time_t ttDcTime = 0;

    if ( !AD_ShouldSyncSystemTime() )
    {
        goto cleanup;
    }

    BAIL_ON_INVALID_STRING(pszDomainFQDN);

    if (LsaDmIsDomainOffline(pszDomainFQDN))
    {
        goto cleanup;
    }

    dwError = LWNetGetDCTime(
                    pszDomainFQDN,
                    &dcTime);
    BAIL_ON_LSA_ERROR(dwError);
    
    ttDcTime = (time_t) dcTime;
    
    if (labs(ttDcTime - time(NULL)) > AD_GetClockDriftSeconds()) {
        dwError = LsaSetSystemTime(ttDcTime);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
cleanup:

    return;
    
error:

    LSA_LOG_ERROR("Failed to sync system time [error code: %u]", dwError);

    goto cleanup;
}

VOID
ADShutdownMachinePasswordSync(
    VOID
    )
{
    DWORD dwError = 0;
    
    if (gpMachinePasswordSyncThread)
    {   
        pthread_mutex_lock(&gMachinePasswordSyncThreadLock);
        pthread_cond_signal(&gMachinePasswordSyncThreadCondition);
        pthread_mutex_unlock(&gMachinePasswordSyncThreadLock);
        
        dwError = pthread_cancel(gMachinePasswordSyncThread);
        if (ESRCH == dwError)
        {
            dwError = 0;
        }
        if (dwError)
        {
            LSA_LOG_ERROR("Unexpected error trying to cancel thread (error = %d)", dwError);
            dwError = 0;
        }
        
        pthread_join(gMachinePasswordSyncThread, NULL);
        gpMachinePasswordSyncThread = NULL;
    }

    if (ghPasswordStore)
    {
        LwpsClosePasswordStore(ghPasswordStore);
        ghPasswordStore = (HANDLE)NULL;
    }
}

BOOLEAN
ADShouldRefreshMachineTGT()
{
	BOOLEAN bRefresh = FALSE;
	BOOLEAN bInLock = FALSE;
	
	ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);
	
	if (!gdwMachineTGTExpiry ||
            (difftime(gdwMachineTGTExpiry, time(NULL)) <= gdwMachineTGTExpiryGraceSeconds))
	{
		bRefresh = TRUE;
	}
	
	LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);
	
	return bRefresh;
}

VOID
ADSetMachineTGTExpiry(
	DWORD dwGoodUntil
	)
{
	BOOLEAN bInLock = FALSE;
	
	ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);
	
	gdwMachineTGTExpiry = dwGoodUntil;
	
	LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);
}

VOID
ADLogMachinePWUpdateSuccessEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Updated Active Directory machine password.\r\n\r\n" \
                 "     Authentication provider:   %s",
                 LSA_SAFE_LOG_STRING(gpszADProviderName));
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_SUCCESSFUL_MACHINE_ACCOUNT_PASSWORD_UPDATE,
            PASSWORD_EVENT_CATEGORY,
            pszDescription,
            NULL);

cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
ADLogMachinePWUpdateFailureEvent(
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "The Active Directory machine password failed to update.\r\n\r\n" \
                 "     Authentication provider:   %s",
                 LSA_SAFE_LOG_STRING(gpszADProviderName));
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);
      
    LsaSrvLogServiceFailureEvent(
            LSASS_EVENT_FAILED_MACHINE_ACCOUNT_PASSWORD_UPDATE,
            PASSWORD_EVENT_CATEGORY,
            pszDescription,
            pszData);
    
cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);
    LSA_SAFE_FREE_STRING(pszData);

    return;
    
error:

    goto cleanup;
}

VOID
ADLogMachineTGTRefreshSuccessEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Refreshed Active Directory machine account TGT (Ticket Granting Ticket).\r\n\r\n" \
                 "     Authentication provider:   %s",
                 LSA_SAFE_LOG_STRING(gpszADProviderName));
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_SUCCESSFUL_MACHINE_ACCOUNT_TGT_REFRESH,
            KERBEROS_EVENT_CATEGORY,
            pszDescription,
            NULL);

cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
ADLogMachineTGTRefreshFailureEvent(
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "The Active Directory machine account TGT (Ticket Granting Ticket) failed to refresh.\r\n\r\n" \
                 "     Authentication provider:   %s",
                 LSA_SAFE_LOG_STRING(gpszADProviderName));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);

    LsaSrvLogServiceFailureEvent(
            LSASS_EVENT_FAILED_MACHINE_ACCOUNT_TGT_REFRESH,
            KERBEROS_EVENT_CATEGORY,
            pszDescription,
            pszData);

cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);
    LSA_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;
}

