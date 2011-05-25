/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2011
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
 *        main-svcm.c
 *
 * Abstract:
 *
 *        Likewise Server Service (SRVSVC)
 *
 *        Service module implementation
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"


NTSTATUS
SrvsSvcmInit(
    PCWSTR ServiceName,
    PLW_SVCM_INSTANCE pInstance
    )
{
    return STATUS_SUCCESS;
}


VOID
SrvsSvcmDestroy(
    PLW_SVCM_INSTANCE pInstance
    )
{
}


NTSTATUS
SrvsSvcmStart(
    PLW_SVCM_INSTANCE pInstance,
    ULONG ArgCount,
    PWSTR* pArgs,
    ULONG FdCount,
    int* pFds
    )
{
    DWORD err = ERROR_SUCCESS;

    err = SrvsInitialiseConfig(&gpSrvsServerInfo->config);
    BAIL_ON_SRVSVC_ERROR(err);

    err = SrvSvcReadConfigSettings(&gpSrvsServerInfo->config);
    BAIL_ON_SRVSVC_ERROR(err);

    err = SrvSvcInitSecurity();
    BAIL_ON_SRVSVC_ERROR(err);

    err = SrvSvcRpcInitialize();
    BAIL_ON_SRVSVC_ERROR(err);

    err = SrvSvcDSNotify();
    BAIL_ON_SRVSVC_ERROR(err);

    err = SrvSvcSMNotify();
    BAIL_ON_SRVSVC_ERROR(err);

error:
    return LwWin32ErrorToNtStatus(err);
}


NTSTATUS
SrvsSvcmStop(
    PLW_SVCM_INSTANCE pInstance
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    SrvSvcDSShutdown();
    SrvSvcRpcShutdown();

    return ntStatus;
}


NTSTATUS
SrvsSvcmRefresh(
    PLW_SVCM_INSTANCE pInstance
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    return ntStatus;
}


static LW_SVCM_MODULE gSrvsService =
{
    .Size = sizeof(gSrvsService),
    .Init = SrvsSvcmInit,
    .Destroy = SrvsSvcmDestroy,
    .Start = SrvsSvcmStart,
    .Stop = SrvsSvcmStop,
    .Refresh = SrvsSvcmRefresh
};


#define SVCM_ENTRY_POINT LW_RTL_SVCM_ENTRY_POINT_NAME(srvsvc)

PLW_SVCM_MODULE
SVCM_ENTRY_POINT(
    VOID
    )
{
    return &gSrvsService;
}


DWORD
SrvSvcRpcInitialize(
    VOID
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwBindAttempts = 0;
    BOOLEAN bInLock = FALSE;
    static const DWORD dwMaxBindAttempts = 5;
    static const DWORD dwBindSleepSeconds = 5;

    SRVSVC_LOCK_MUTEX(bInLock, &gSrvsServerInfo.mutex);

    /* Binding to our RPC end-point might fail if dcerpcd is not
       yet ready when we start, so attempt it in a loop with
       a small delay between attempts */
    for (dwBindAttempts = 0; dwBindAttempts < dwMaxBindAttempts; dwBindAttempts++)
    {
        dwError = SrvSvcRegisterForRPC(
                        "Likewise Server Service",
                        &gSrvsServerInfo.pServerBinding);
        if (dwError)
        {
            SRVSVC_LOG_INFO("Failed to bind srvsvc endpoint; retrying in "
                            "%i seconds...",
                            (int) dwBindSleepSeconds);
            sleep(dwBindSleepSeconds);
        }
        else
        {
            break;
        }
    }
    /* Bail if we still haven't succeeded after several attempts */
    BAIL_ON_SRVSVC_ERROR(dwError);

    /* Now register the winreg pipe */

    for (dwBindAttempts = 0; dwBindAttempts < dwMaxBindAttempts; dwBindAttempts++)
    {
        dwError = WinRegRegisterForRPC(
                        "Likewise Registry Service",
                        &gSrvsServerInfo.pRegistryBinding);
        if (dwError)
        {
            SRVSVC_LOG_INFO("Failed to bind wkssvc endpoint; retrying in "
                            "%i seconds...",
                            (int) dwBindSleepSeconds);
            sleep(dwBindSleepSeconds);
        }
        else
        {
            break;
        }
    }
    /* Bail if we still haven't succeeded after several attempts */
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwMapErrnoToLwError(dcethread_create(
                                        &gSrvsServerInfo.pRpcListenerThread,
                                        NULL,
                                        &SrvSvcListenForRPC,
                                        NULL));
    BAIL_ON_SRVSVC_ERROR(dwError);

    while (!SrvSvcRpcIsListening())
    {
    }

error:
    SRVSVC_UNLOCK_MUTEX(bInLock, &gSrvsServerInfo.mutex);

    return dwError;
}


VOID
SrvSvcRpcShutdown(
    VOID
    )
{
    DWORD dwError = ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;

    SRVSVC_LOCK_MUTEX(bInLock, &gSrvsServerInfo.mutex);

    if (gSrvsServerInfo.pServerBinding)
    {
        dwError = SrvSvcUnregisterForRPC(gSrvsServerInfo.pServerBinding);
        BAIL_ON_SRVSVC_ERROR(dwError);

        gSrvsServerInfo.pServerBinding = NULL;
    }

    if (gSrvsServerInfo.pRegistryBinding)
    {
        dwError = WinRegUnregisterForRPC(gSrvsServerInfo.pRegistryBinding);
        BAIL_ON_SRVSVC_ERROR(dwError);

        gSrvsServerInfo.pRegistryBinding = NULL;
    }

    if (gSrvsServerInfo.pRpcListenerThread)
    {
        dwError = LwMapErrnoToLwError(dcethread_interrupt(gSrvsServerInfo.pRpcListenerThread));
        BAIL_ON_SRVSVC_ERROR(dwError);

        dwError = LwMapErrnoToLwError(dcethread_join(gSrvsServerInfo.pRpcListenerThread, NULL));
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    if (gSrvsServerInfo.pSessionSecDesc)
    {
        SrvSvcSrvDestroyServerSecurityDescriptor(gSrvsServerInfo.pSessionSecDesc);
        gSrvsServerInfo.pSessionSecDesc = NULL;
    }

error:
    SRVSVC_UNLOCK_MUTEX(bInLock, &gSrvsServerInfo.mutex);

    return;
}


DWORD
SrvSvcDSNotify(
    VOID
    )
{
    DWORD dwError = ERROR_SUCCESS;

    dwError = LwDsCacheAddPidException(getpid());
    if (dwError == LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK)
    {
        SRVSVC_LOG_ERROR("Could not register process pid (%d) "
                         "with Mac DirectoryService Cache plugin",
                         (int) getpid());
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

error:
    return dwError;
}


VOID
SrvSvcDSShutdown(
    VOID
    )
{
    LwDsCacheRemovePidException(getpid());
}


DWORD
SrvSvcSMNotify(
    VOID
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PCSTR pszSmNotify = NULL;
    int notifyFd = -1;

    if ((pszSmNotify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        int ret = 0;
        char notifyCode = 0;

        notifyFd = atoi(pszSmNotify);

        do
        {
            ret = write(notifyFd, &notifyCode, sizeof(notifyCode));
        } while(ret != sizeof(notifyCode) && errno == EINTR);

        if (ret < 0)
        {
            SRVSVC_LOG_ERROR("Could not notify service manager: %s (%i)",
                                strerror(errno),
                                errno);

            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_SRVSVC_ERROR(dwError);
        }
    }

error:
    if (notifyFd >= 0)
    {
        close(notifyFd);
    }

    return dwError;
}
