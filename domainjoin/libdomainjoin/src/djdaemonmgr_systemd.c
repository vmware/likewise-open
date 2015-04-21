/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright VMware, Inc 2015
 * All rights reserved.
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

#include "domainjoin.h"
#include "ctshell.h"
#include "djdistroinfo.h"
#include "djdaemonmgr.h"

DWORD
DJCheckIfSystemdSupported(
    BOOLEAN* pbSupported
    )
{
    DWORD dwError = 0;
    LWException *exc = NULL;
    DistroInfo distro;
    BOOLEAN bSupported = FALSE;

    memset(&distro, 0, sizeof(distro));

    if (!pbSupported)
    {
        LW_RAISE(&exc, ERROR_INVALID_PARAMETER);
    }

    LW_CLEANUP_CTERR(&exc, DJGetDistroInfo("", &distro));

    if (distro.distro == DISTRO_VMWARE_PHOTON)
    {
        bSupported = TRUE;
    }

cleanup:

    if (!LW_IS_OK(exc))
    {
        dwError = exc->code;
        LWHandle(&exc);
    }
    if (pbSupported)
    {
        *pbSupported = bSupported;
    }

    return dwError;
}

DWORD
DJGetDaemonStatusSystemd(
    PCSTR         pszDaemonName,
    PBOOLEAN      pbStarted
    )
{
    DWORD dwError = 0;
    LWException *exc = NULL;
    PSTR  pszPath = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    PPROCINFO pProcInfo = NULL;
    LONG status = 0;
    BOOLEAN bInstalled = FALSE;
    BOOLEAN bStarted = FALSE;

    if (!pbStarted)
    {
        LW_RAISE(&exc, ERROR_INVALID_PARAMETER);
    }

    LW_CLEANUP_CTERR(&exc, CTAllocateStringPrintf(
                                &pszPath,
                                "%s/%s.service",
                                SYSTEMD_PREFIX,
                                pszDaemonName));

    LW_CLEANUP_CTERR(&exc, CTCheckFileExists(pszPath, &bInstalled));

    if (!bInstalled) {
        LW_CLEANUP_CTERR(&exc, ERROR_SERVICE_NOT_FOUND);
    }

    LW_CLEANUP_CTERR(&exc, CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs)));
    LW_CLEANUP_CTERR(&exc, CTAllocateString(SYSTEMCTL_PATH, ppszArgs));
    LW_CLEANUP_CTERR(&exc, CTAllocateString("status", ppszArgs+1));
    LW_CLEANUP_CTERR(&exc, CTAllocateStringPrintf(
                                ppszArgs+2,
                                "%s.service",
                                pszDaemonName));

    LW_CLEANUP_CTERR(&exc, DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo));
    LW_CLEANUP_CTERR(&exc, DJGetProcessStatus(pProcInfo, &status));

    if (status == 0)
    {
        bStarted = TRUE;
    }

cleanup:

    if (!LW_IS_OK(exc))
    {
        dwError = exc->code;
        LWHandle(&exc);
    }
    CT_SAFE_FREE_STRING(pszPath);
    if (ppszArgs)
    {
        CTFreeStringArray(ppszArgs, nArgs);
    }
    if (pProcInfo)
    {
        FreeProcInfo(pProcInfo);
    }
    if (pbStarted)
    {
        *pbStarted = bStarted;
    }

    return dwError;
}

void
DJStartStopDaemonSystemd(
    PCSTR         pszDaemonName,
    BOOLEAN       bStatus,
    LWException** exc
    )
{
    PSTR  pszPath = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    LONG status = 0;
    PPROCINFO pProcInfo = NULL;
    BOOLEAN bInstalled = FALSE;
    BOOLEAN bStarted = FALSE;
    int retry;

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
                                &pszPath,
                                "%s/%s.service",
                                SYSTEMD_PREFIX,
                                pszDaemonName));
    LW_CLEANUP_CTERR(exc, CTCheckFileExists(pszPath, &bInstalled));

    if (!bInstalled) {
        LW_CLEANUP_CTERR(exc, ERROR_SERVICE_NOT_FOUND);
    }

    if (bStatus) {
        DJ_LOG_INFO("Starting daemon [%s.service]", pszDaemonName);
    } else {
        DJ_LOG_INFO("Stopping daemon [%s.service]", pszDaemonName);
    }

    LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs)));
    LW_CLEANUP_CTERR(exc, CTAllocateString(SYSTEMCTL_PATH, ppszArgs));
    LW_CLEANUP_CTERR(exc, CTAllocateString(
                                bStatus ? "start" : "stop",
                                ppszArgs+1));
    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
                                ppszArgs+2,
                                "%s.service",
                                pszDaemonName));

    LW_CLEANUP_CTERR(exc, DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo));
    LW_CLEANUP_CTERR(exc, DJGetProcessStatus(pProcInfo, &status));

    for(retry = 0; retry < 20; retry++)
    {
        LW_TRY(exc, DJGetDaemonStatus2(pszDaemonName, &bStarted, &LW_EXC));
        if (bStarted == bStatus)
            break;
        sleep(1);
    }

    if (bStarted != bStatus)
    {
        if(bStatus)
        {
            LW_RAISE_EX(exc,
                    ERROR_INVALID_STATE,
                    "Unable to start daemon",
                    "An attempt was made to start the '%s.service' daemon, "
                    "but querying its status revealed that it did not start. "
                    "Try running '%s start %s.service; %s status %s.service' "
                    "to diagnose the issue",
                    pszDaemonName,
                    SYSTEMCTL_PATH,
                    pszDaemonName,
                    SYSTEMCTL_PATH,
                    pszDaemonName);
        }
        else
        {
            LW_RAISE_EX(exc,
                    ERROR_INVALID_STATE,
                    "Unable to stop daemon",
                    "An attempt was made to start the '%s.service' daemon, "
                    "but querying its status revealed that it did not start. "
                    "Try running '%s stop %s.service; %s status %s.service' "
                    "to diagnose the issue",
                    pszDaemonName,
                    SYSTEMCTL_PATH,
                    pszDaemonName,
                    SYSTEMCTL_PATH,
                    pszDaemonName);
        }
        goto cleanup;
    }

cleanup:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    CT_SAFE_FREE_STRING(pszPath);
}

void
DJConfigureForDaemonRestartSystemd(
    PCSTR pszDaemonName,
    BOOLEAN bStatus,
    LWException **exc
    )
{
    PSTR  pszPath = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    LONG status = 0;
    PPROCINFO pProcInfo = NULL;
    BOOLEAN bInstalled = FALSE;

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
                                &pszPath,
                                "%s/%s.service",
                                SYSTEMD_PREFIX,
                                pszDaemonName));
    LW_CLEANUP_CTERR(exc, CTCheckFileExists(pszPath, &bInstalled));

    if (!bInstalled) {
        LW_CLEANUP_CTERR(exc, ERROR_SERVICE_NOT_FOUND);
    }

    if (bStatus) {
        DJ_LOG_INFO("Enabling daemon [%s.service]", pszDaemonName);
    } else {
        DJ_LOG_INFO("Disabling daemon [%s.service]", pszDaemonName);
    }

    LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs)));
    LW_CLEANUP_CTERR(exc, CTAllocateString(SYSTEMCTL_PATH, ppszArgs));
    LW_CLEANUP_CTERR(exc, CTAllocateString(
                                bStatus ? "enable" : "disable",
                                ppszArgs+1));
    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
                                ppszArgs+2,
                                "%s.service",
                                pszDaemonName));

    LW_CLEANUP_CTERR(exc, DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo));
    LW_CLEANUP_CTERR(exc, DJGetProcessStatus(pProcInfo, &status));

    if (status != 0)
    {
        if(bStatus)
        {
            LW_RAISE_EX(exc,
                    ERROR_INVALID_STATE,
                    "Unable to enable daemon",
                    "An attempt was made to enable the '%s.service' daemon, "
                    "Try running '%s enable %s.service' "
                    "to diagnose the issue",
                    pszDaemonName,
                    SYSTEMCTL_PATH,
                    pszDaemonName);
        }
        else
        {
            LW_RAISE_EX(exc,
                    ERROR_INVALID_STATE,
                    "Unable to disable daemon",
                    "An attempt was made to disable the '%s.service' daemon, "
                    "Try running '%s disable %s.service' "
                    "to diagnose the issue",
                    pszDaemonName,
                    SYSTEMCTL_PATH,
                    pszDaemonName);
        }
        goto cleanup;
    }

cleanup:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    CT_SAFE_FREE_STRING(pszPath);
}

