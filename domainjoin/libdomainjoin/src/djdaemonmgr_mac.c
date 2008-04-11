/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#include "domainjoin.h"
#include "djdaemonmgr.h"

// aka: CENTERROR_LICENSE_INCORRECT
static DWORD GPAGENT_LICENSE_ERROR = 0x00002001;

// CENTERROR_LICENSE_EXPIRED
static DWORD GPAGENT_LICENSE_EXPIRED_ERROR = 0x00002002;

static PSTR pszAuthdStartPriority = "90";
static PSTR pszAuthdStopPriority = "10";
static PSTR pszGPAgentdStartPriority = "91";
static PSTR pszGPAgentdStopPriority = "9";

void
DJGetDaemonStatus(
    PCSTR pszDaemonPath,
    PBOOLEAN pbStarted,
    LWException **exc
    )
{
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 7;
    long status = 0;
    PPROCINFO pProcInfo = NULL;
    CHAR  szBuf[1024+1];
    CHAR  szCmd[128];
    FILE* fp = NULL;

    /* TODO: Cleanup
     * Currently, I don't know of a way to get service status
     * using launchctl. We can get the pid from the pidfile
     * and use it in the ps command - then, we have to map the
     * service name to the pid file.
     */
    if (!strcmp(pszDaemonPath, "com.centeris.gpagentd") ||
	    !strcmp(pszDaemonPath, "centeris.com-gpagentd"))
        strcpy(szCmd, "centeris-gpagentd");
    else if (!strcmp(pszDaemonPath, "com.likewise.open") ||
	    !strcmp(pszDaemonPath, "likewise-open"))
        strcpy(szCmd, "likewise-winbindd");
    else {
        DJ_LOG_ERROR("Checking status of daemon [%s] failed: Missing", pszDaemonPath);
        LW_CLEANUP_CTERR(exc, CENTERROR_DOMAINJOIN_MISSING_DAEMON);
    }

    DJ_LOG_INFO("Checking status of daemon [%s]", pszDaemonPath);

    LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs));

    LW_CLEANUP_CTERR(exc, CTAllocateString("/bin/ps", ppszArgs));

    LW_CLEANUP_CTERR(exc, CTAllocateString("-U", ppszArgs+1));

    LW_CLEANUP_CTERR(exc, CTAllocateString("root", ppszArgs+2));

    LW_CLEANUP_CTERR(exc, CTAllocateString("-c", ppszArgs+3));

    LW_CLEANUP_CTERR(exc, CTAllocateString("-o", ppszArgs+4));

    LW_CLEANUP_CTERR(exc, CTAllocateString("command=", ppszArgs+5));

    LW_CLEANUP_CTERR(exc, DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo));

    LW_CLEANUP_CTERR(exc, DJGetProcessStatus(pProcInfo, &status));

    *pbStarted = FALSE;
    if (!status) {

        fp = fdopen(pProcInfo->fdout, "r");
        if (!fp) {
            LW_CLEANUP_CTERR(exc, CTMapSystemError(errno));
        }

        while (1) {

            if (fgets(szBuf, 1024, fp) == NULL) {
                if (!feof(fp)) {
                    LW_CLEANUP_CTERR(exc, CTMapSystemError(errno));
                }
                else
                    break;
            }

            CTStripWhitespace(szBuf);

            if (IsNullOrEmptyString(szBuf))
                continue;

            if (!strcmp(szBuf, szCmd)) {
                *pbStarted = TRUE;
                break;
            }
        }
    }

cleanup:

    if (fp)
        fclose(fp);

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);
}

void
DJStartStopDaemon(
    PCSTR pszDaemonPath,
    BOOLEAN bStatus,
    PSTR pszPreCommand,
    LWException **exc
    )
{
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    long status = 0;
    PPROCINFO pProcInfo = NULL;
    BOOLEAN bStarted = FALSE;
    int count;

    if (bStatus) {
        DJ_LOG_INFO("Starting daemon [%s]", pszDaemonPath);
    } else {
        DJ_LOG_INFO("Stopping daemon [%s]", pszDaemonPath);
    }

    LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs));
    LW_CLEANUP_CTERR(exc, CTAllocateString("/bin/launchctl", ppszArgs));

    if (bStatus)
    {
       LW_CLEANUP_CTERR(exc, CTAllocateString("start", ppszArgs+1));
       LW_CLEANUP_CTERR(exc, CTAllocateString(pszDaemonPath, ppszArgs+2));
    }
    else
    {
       LW_CLEANUP_CTERR(exc, CTAllocateString("unload", ppszArgs+1));
       LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(ppszArgs+2, "/System/Library/LaunchDaemons/%s.plist", pszDaemonPath));
    }

    LW_CLEANUP_CTERR(exc, DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo));
    LW_CLEANUP_CTERR(exc, DJGetProcessStatus(pProcInfo, &status));

    for (count = 0; count < 5; count++) {

	LW_TRY(exc, DJGetDaemonStatus(pszDaemonPath, &bStarted, &LW_EXC));

        if (bStarted == bStatus) {
            break;
        }

        sleep(1);
    }

    if (bStarted != bStatus) {

        if(bStatus)
        {
            LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_INCORRECT_STATUS, "Unable to start daemon", "An attempt was made to start the '%s' daemon, but querying its status revealed that it did not start.", pszDaemonPath);
        }
        else
        {
            LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_INCORRECT_STATUS, "Unable to start daemon", "An attempt was made to stop the '%s' daemon, but querying its status revealed that it did not stop.", pszDaemonPath);
        }
        goto cleanup;
    }

cleanup:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);
}

CENTERROR
DJConfigureForDaemonRestart(
    PSTR pszDaemonName,
    BOOLEAN bStatus,
    PSTR pszStartPriority,
    PSTR pszStopPriority
    )
{
    return CENTERROR_SUCCESS;
}

static
CENTERROR
DJExistsInLaunchCTL(
    PSTR pszName,
    PBOOLEAN pbExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    LONG  status = 0;
    DWORD nArgs = 3;
    FILE* fp = NULL;
    CHAR szBuf[1024+1];
    BOOLEAN bExists = FALSE;

    ceError = CTAllocateMemory(nArgs*sizeof(PSTR), (PVOID*)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/bin/launchctl", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("list", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        ceError = CENTERROR_DOMAINJOIN_LISTSVCS_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fp = fdopen(pProcInfo->fdout, "r");
    if (fp == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    for(;;)
    {
        if (fgets(szBuf, 1024, fp) == NULL)
        {
            if (feof(fp))
            {
                break;
            }
            else
            {
                ceError = CTMapSystemError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        CTStripWhitespace(szBuf);

        if (!strcmp(szBuf, pszName))
        {
            bExists = TRUE;
            break;
        }
    }

error:

    *pbExists = bExists;

    if (fp)
        fclose(fp);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    return ceError;
}

static
CENTERROR
DJPrepareServiceLaunchScript(
    PSTR pszName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    LONG  status = 0;
    DWORD nArgs = 5;
    CHAR szBuf[1024+1];

    ceError = CTAllocateMemory(nArgs*sizeof(PSTR), (PVOID*)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/bin/cp", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("-f", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(szBuf, "/etc/centeris/LaunchDaemons/%s.plist", pszName);
    ceError = CTAllocateString(szBuf, ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(szBuf, "/System/Library/LaunchDaemons/%s.plist", pszName);
    ceError = CTAllocateString(szBuf, ppszArgs+3);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        ceError = CENTERROR_DOMAINJOIN_PREPSVC_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    return ceError;
}

static
CENTERROR
DJRemoveFromLaunchCTL(
    PSTR pszName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bExistsInLaunchCTL = FALSE;
    CHAR    szBuf[1024+1];
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    LONG  status = 0;

    ceError = DJExistsInLaunchCTL(pszName, &bExistsInLaunchCTL);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bExistsInLaunchCTL) {

        sprintf(szBuf, "/System/Library/LaunchDaemons/%s.plist", pszName);

        ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString("/bin/launchctl", ppszArgs);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString("unload", ppszArgs+1);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString(szBuf, ppszArgs+2);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = DJGetProcessStatus(pProcInfo, &status);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (status != 0) {
            ceError = CENTERROR_DOMAINJOIN_SVC_UNLOAD_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        sprintf(szBuf, "/System/Library/LaunchDaemons/%s.plist", pszName);
        ceError = CTRemoveFile(szBuf);
        if (!CENTERROR_IS_OK(ceError)) {
            DJ_LOG_WARNING("Failed to remove file [%s]", szBuf);
            ceError = CENTERROR_SUCCESS;
        }
    }

error:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    return ceError;
}

static
CENTERROR
DJAddToLaunchCTL(
    PSTR pszName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bExistsInLaunchCTL = FALSE;
    BOOLEAN bFileExists = FALSE;
    CHAR    szBuf[1024+1];
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    LONG  status = 0;

    ceError = DJExistsInLaunchCTL(pszName, &bExistsInLaunchCTL);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bExistsInLaunchCTL) {

        ceError = DJRemoveFromLaunchCTL(pszName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        bExistsInLaunchCTL = FALSE;
    }

    ceError = DJPrepareServiceLaunchScript(pszName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(szBuf, "/System/Library/LaunchDaemons/%s.plist", pszName);

    ceError = CTCheckFileExists(szBuf, &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bFileExists) {
        ceError = CENTERROR_DOMAINJOIN_MISSING_DAEMON;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    sprintf(szBuf, "/System/Library/LaunchDaemons/%s.plist", pszName);

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/bin/launchctl", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("load", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(szBuf, ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        ceError = CENTERROR_DOMAINJOIN_SVC_LOAD_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    return ceError;
}

void
DJManageDaemon(
    PCSTR pszName,
    BOOLEAN bStatus,
    PSTR pszPreCommand,
    PSTR pszStartPriority,
    PSTR pszStopPriority,
    LWException **exc
    )
{
    BOOLEAN bStarted = FALSE;

    if (bStatus)
    {
        LW_CLEANUP_CTERR(exc, DJAddToLaunchCTL(pszName));
    }

    // check our current state prior to doing anything.  notice that
    // we are using the private version so that if we fail, our inner
    // exception will be the one that was tossed due to the failure.
    LW_TRY(exc, DJGetDaemonStatus(pszName, &bStarted, &LW_EXC));

    // if we got this far, we have validated the existence of the
    // daemon and we have figured out if its started or stopped

    // if we are already in the desired state, do nothing.
    if (bStarted != bStatus) {

        LW_TRY(exc, DJStartStopDaemon(pszName, bStatus, pszPreCommand, &LW_EXC));

    }
    else
    {
        DJ_LOG_INFO("daemon '%s' is already %s", pszName, bStarted ? "started" : "stopped");
    }

    LW_CLEANUP_CTERR(exc, DJConfigureForDaemonRestart(pszName, bStatus, pszStartPriority, pszStopPriority));

cleanup:
    ;
}

struct _DaemonList daemonList[] = {
    { "com.likewise.open", NULL, TRUE, 92, 10 },
    { "com.centeris.gpagentd", NULL, FALSE, 92, 9 },
    { NULL, NULL, FALSE, 0, 0 },
};
