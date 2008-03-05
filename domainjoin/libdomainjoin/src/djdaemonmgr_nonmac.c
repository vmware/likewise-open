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

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

static PSTR pszChkConfigPath = "/sbin/chkconfig";
static PSTR pszUpdateRcDFilePath = "/usr/sbin/update-rc.d";
static PSTR pszAuthdStartPriority = "90";
static PSTR pszAuthdStopPriority = "10";
static PSTR pszGPAgentdStartPriority = "91";
static PSTR pszGPAgentdStopPriority = "9";

// aka: CENTERROR_LICENSE_INCORRECT
static DWORD GPAGENT_LICENSE_ERROR = 0x00002001;

// CENTERROR_LICENSE_EXPIRED
static DWORD GPAGENT_LICENSE_EXPIRED_ERROR = 0x00002002;

CENTERROR
GetInitScriptDir(PSTR *store)
{
#if defined(_AIX)
    return CTStrdup("/etc/rc.d/init.d", store);
#elif defined(_HPUX_SOURCE)
    return CTStrdup("/sbin/init.d", store);
#else
    return CTStrdup("/etc/init.d", store);
#endif
}

void
DJGetDaemonStatus(
    PSTR pszDaemonPath,
    PBOOLEAN pbStarted,
    LWException **exc
    )
{
    PSTR* ppszArgs = NULL;
    PSTR prefixedPath = NULL;
    PSTR initDir = NULL;
    DWORD nArgs = 3;
    long status = 0;
    PPROCINFO pProcInfo = NULL;
    BOOLEAN daemon_installed = FALSE;

    if(pszDaemonPath[0] == '/')
        LW_CLEANUP_CTERR(exc, CTStrdup(pszDaemonPath, &prefixedPath));
    else
    {
        LW_CLEANUP_CTERR(exc, GetInitScriptDir(&initDir));
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
                &prefixedPath, "%s/%s",
                initDir, pszDaemonPath));
    }

    DJ_LOG_INFO("Checking status of daemon [%s]", prefixedPath);

    LW_CLEANUP_CTERR(exc, CTCheckFileExists(prefixedPath, &daemon_installed));

    if (!daemon_installed) {
        LW_CLEANUP_CTERR(exc, CENTERROR_DOMAINJOIN_MISSING_DAEMON);
    }

    LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs));

    LW_CLEANUP_CTERR(exc, CTAllocateString(prefixedPath, ppszArgs));

    LW_CLEANUP_CTERR(exc, CTAllocateString("status", ppszArgs+1));

    LW_CLEANUP_CTERR(exc, DJSpawnProcessSilent(ppszArgs[0], ppszArgs, &pProcInfo));

    LW_CLEANUP_CTERR(exc, DJGetProcessStatus(pProcInfo, &status));

    // see
    // http://forgeftp.novell.com/library/SUSE%20Package%20Conventions/spc_init_scripts.html
    // and http://www.linuxbase.org/~gk4/wip-sys-init.html
    //
    // note further that some other error codes might be thrown, so
    // we choose to only pay attention to the ones that lsb says are
    // valid return codes for status that indicate that a progam
    // isnt running, otherwise, we are gonna throw it.

    DJ_LOG_INFO("Daemon [%s]: status [%d]", prefixedPath, status);

    if (!status) {
        *pbStarted = TRUE;
    }
    else if (status == 1 || status == 2 || status == 3 || status == 4)
        *pbStarted = FALSE;
    else {
        LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_UNEXPECTED_ERRCODE, "Non-standard return code from init script", "According to http://forgeftp.novell.com/library/SUSE%20Package%20Conventions/spc_init_scripts.html, init scripts should return 0, 1, 2, 3, or 4. However, '%s status' returned %d.", status, prefixedPath);
        goto cleanup;
    }

cleanup:
error:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    CT_SAFE_FREE_STRING(prefixedPath);
    CT_SAFE_FREE_STRING(initDir);
}

void
DJStartStopDaemon(
    PSTR pszDaemonPath,
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
    CHAR szBuf[1024];

    if (bStatus) {
        DJ_LOG_INFO("Starting daemon [%s]", pszDaemonPath);
    } else {
        DJ_LOG_INFO("Stopping daemon [%s]", pszDaemonPath);
    }

    if (IsNullOrEmptyString(pszPreCommand)) {

        LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs));
        LW_CLEANUP_CTERR(exc, CTAllocateString(pszDaemonPath, ppszArgs));
        LW_CLEANUP_CTERR(exc, CTAllocateString((bStatus ? "start" : "stop"), ppszArgs+1));

    } else {

        LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs));
        LW_CLEANUP_CTERR(exc, CTAllocateString("/bin/sh", ppszArgs));
        LW_CLEANUP_CTERR(exc, CTAllocateString("-c", ppszArgs+1));

        sprintf(szBuf, "%s; %s %s ",
                pszPreCommand,
                pszDaemonPath,
                (bStatus ? "start" : "stop"));

        LW_CLEANUP_CTERR(exc, CTAllocateString(szBuf, ppszArgs+2));
    }

    LW_CLEANUP_CTERR(exc, DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo));
    LW_CLEANUP_CTERR(exc, DJGetProcessStatus(pProcInfo, &status));
    LW_TRY(exc, DJGetDaemonStatus(pszDaemonPath, &bStarted, &LW_EXC));

    if (bStarted != bStatus) {

        if(bStatus)
        {
            LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_INCORRECT_STATUS, "Unable to start daemon", "An attempt was made to start the '%s' daemon, but querying its status revealed that it did not start. Try running '%s start; %s status' to diagnose the issue", pszDaemonPath, pszDaemonPath, pszDaemonPath);
        }
        else
        {
            LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_INCORRECT_STATUS, "Unable to stop daemon", "An attempt was made to stop the '%s' daemon, but querying its status revealed that it did not stop. Try running '%s stop; %s status' to diagnose the issue", pszDaemonPath, pszDaemonPath, pszDaemonPath);
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
DJDoUpdateRcD(
    PSTR pszDaemonName,
    BOOLEAN bStatus,
    PSTR pszStartPriority,
    PSTR pszStopPriority
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 5;
    long status = 0;
    PPROCINFO pProcInfo = NULL;
    BOOLEAN bStarted = FALSE;
    PSTR ppszStartSpecialArgs[] =
        { "20",
          "2",
          "3",
          "4",
          "5",
          ".",
          "stop",
          "20",
          "0",
          "1",
          "6",
          "."
        };
    DWORD nSpecialArgs = sizeof(ppszStartSpecialArgs)/sizeof(PSTR);
    DWORD iArg = 0;

    nArgs = nSpecialArgs + 3;

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(pszUpdateRcDFilePath, ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bStatus) {

        ceError = CTAllocateString("-f", ppszArgs+1);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString(pszDaemonName, ppszArgs+2);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString("remove", ppszArgs+3);
        BAIL_ON_CENTERIS_ERROR(ceError);

    } else {

        ceError = CTAllocateString(pszDaemonName, ppszArgs+1);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (pszStartPriority)
	   ppszStartSpecialArgs[0] = pszStartPriority;
        if (pszStopPriority)
           ppszStartSpecialArgs[7] = pszStopPriority;

        //
        // TODO-2006/10/31-dalmeida -- Should not hardcode runlevels.
        //
        for (iArg = 0; iArg < nSpecialArgs; iArg++) {

            ceError = CTAllocateString(*(ppszStartSpecialArgs+iArg), ppszArgs+iArg+2);
            BAIL_ON_CENTERIS_ERROR(ceError);

        }
    }

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);


    // update-rc.d should not stop or start daemon, so this check
    // shouldn't be necessary (and is also incorrect)
    /*

    ceError = DJGetDaemonStatus(pszDaemonName, &bStarted);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {

        ceError = CENTERROR_DOMAINJOIN_UPDATERCD_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);

    }
    */

error:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    return ceError;
}

CENTERROR
DJDoChkConfig(
    PSTR pszDaemonName,
    BOOLEAN bStatus
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szDaemonPath[PATH_MAX+1];
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    LONG status = 0;
    PPROCINFO pProcInfo = NULL;
    FILE* fp = NULL;
    CHAR szBuf[1024+1];

#if defined(_AIX)
    sprintf(szDaemonPath, "/etc/rc.d/init.d/%s", pszDaemonName);
#elif defined(_HPUX_SOURCE)
    sprintf(szDaemonPath, "/sbin/init.d/%s", pszDaemonName);
#else
    sprintf(szDaemonPath, "/etc/init.d/%s", pszDaemonName);
#endif

    // if we got this far, we have set the daemon to the running state
    // that the caller has requested.  now we need to set it's init
    // state (whether or not it get's ran on startup) to what the
    // caller has requested. we can do this unconditionally because
    // chkconfig wont complain if we do an --add to something that is
    // already in the --add state.

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(pszChkConfigPath, ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString((bStatus ? "--add" : "--del"), ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(pszDaemonName, ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        ceError = CENTERROR_DOMAINJOIN_CHKCONFIG_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    // this step is needed in some circumstances, but not all. i
    // *think* that this step is needed for LSB-1.2 only. furthermore
    // this step appears to turn off LSB-2.0 daemons, so we have to
    // make sure that we dont try to run it on them. so we check for
    // BEGIN INIT INFO. It's slower than i would like due to the need
    // to read the file in and do stringops on it. shelling out and
    // using grep *might* be faster, but it might not be, and it has
    // all the complexity associated with running native apps.

    fp = fopen(szDaemonPath, "r");
    if (fp == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    while (1) {
        if (fgets(szBuf, 1024, fp) == NULL) {

            if (feof(fp))
                break;
            else {
                ceError = CTMapSystemError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            if (strstr(szBuf, "chkconfig:")) {

                CTFreeString(*(ppszArgs+1));

                if (pProcInfo) {
                    FreeProcInfo(pProcInfo);
                    pProcInfo = NULL;
                }

                ceError = CTAllocateString((bStatus ? "on" : "off"), ppszArgs+1);
                BAIL_ON_CENTERIS_ERROR(ceError);

                ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
                BAIL_ON_CENTERIS_ERROR(ceError);

                ceError = DJGetProcessStatus(pProcInfo, &status);
                BAIL_ON_CENTERIS_ERROR(ceError);

                if (status != 0) {
                    ceError = CENTERROR_DOMAINJOIN_CHKCONFIG_FAILED;
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

            }

            if (strstr(szBuf, "BEGIN INIT INFO"))
                break;

        }
    }

error:

    if (fp)
        fclose(fp);

    if (pProcInfo) {
        FreeProcInfo(pProcInfo);
        pProcInfo = NULL;
    }

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    return ceError;
}

void
DJConfigureForDaemonRestart(
    PSTR pszDaemonName,
    BOOLEAN bStatus,
    PSTR pszStartPriority,
    PSTR pszStopPriority,
    LWException **exc
    )
{
    BOOLEAN bFileExists = FALSE;

    DJ_LOG_VERBOSE("Looking for '%s'", pszChkConfigPath);
    LW_CLEANUP_CTERR(exc, CTCheckFileExists(pszChkConfigPath, &bFileExists));

    if (bFileExists) {

        DJ_LOG_VERBOSE("Found '%s'", pszChkConfigPath);
        LW_CLEANUP_CTERR(exc, DJDoChkConfig(pszDaemonName, bStatus));

        goto done;
    }

    DJ_LOG_VERBOSE("Looking for '%s'", pszUpdateRcDFilePath);
    LW_CLEANUP_CTERR(exc, CTCheckFileExists(pszUpdateRcDFilePath, &bFileExists));

    if (bFileExists) {

        DJ_LOG_VERBOSE("Found '%s'", pszUpdateRcDFilePath);
        LW_CLEANUP_CTERR(exc, DJDoUpdateRcD(pszDaemonName, bStatus, pszStartPriority, pszStopPriority));

        goto done;
    }

done:
cleanup:
    ;
}

void
DJManageDaemon(
    PSTR pszName,
    BOOLEAN bStatus,
    PSTR pszPreCommand,
    PSTR pszStartPriority,
    PSTR pszStopPriority,
    LWException **exc
    )
{
    CHAR szBuf[PATH_MAX+1];
    BOOLEAN bFileExists = FALSE;
    BOOLEAN bStarted = FALSE;

#if defined(_AIX)
    sprintf(szBuf, "/etc/rc.d/init.d/%s", pszName);
#elif defined(_HPUX_SOURCE)
    sprintf(szBuf, "/sbin/init.d/%s", pszName);
#else
    sprintf(szBuf, "/etc/init.d/%s", pszName);
#endif

    LW_CLEANUP_CTERR(exc, CTCheckFileExists(szBuf, &bFileExists));

    if (!bFileExists) {
        LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_MISSING_DAEMON, "Missing daemon init script", "The daemon '%s' cannot be started/stopped because '%s' is missing.", pszName, szBuf);
        goto cleanup;
    }

    // check our current state prior to doing anything.  notice that
    // we are using the private version so that if we fail, our inner
    // exception will be the one that was tossed due to the failure.
    LW_TRY(exc, DJGetDaemonStatus(szBuf, &bStarted, &LW_EXC));

    // if we got this far, we have validated the existence of the
    // daemon and we have figured out if its started or stopped

    // if we are already in the desired state, do nothing.
    if (bStarted != bStatus) {

        LW_TRY(exc, DJStartStopDaemon(szBuf, bStatus, pszPreCommand, &LW_EXC));

    }

    LW_TRY(exc, DJConfigureForDaemonRestart(pszName, bStatus, pszStartPriority, pszStopPriority, &LW_EXC));

cleanup:
    ;
}

void
DJManageAuthDaemon(
    BOOLEAN bStatus,
    PSTR pszStartPriority,
    PSTR pszStopPriority,
    LWException **exc
    )
{
    LWException *innerExc = NULL;
    DJManageDaemon("centeris.com-lwiauthd",
                             bStatus,
                             NULL,
                             pszAuthdStartPriority,
                             pszAuthdStopPriority,
                             &innerExc);

    /* If lwiauthd is not installed, check for likewise-open */
    if (!LW_IS_OK(innerExc) && innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON) {
        LW_HANDLE(&innerExc);
        DJManageDaemon("likewise-open",
                 bStatus,
                 NULL,
                 pszAuthdStartPriority,
                 pszAuthdStopPriority,
                 &innerExc);
    }

    LW_CLEANUP(exc, innerExc);

cleanup:
    LW_HANDLE(&innerExc);
}

struct
{
    PCSTR primaryName;
    PCSTR alternativeName;
    BOOLEAN required;
    int startPriority;
    int stopPriority;
} daemonList[] = {
    { "centeris.com-lwiauthd", "likewise-open", TRUE, 92, 10 },
    { "centeris.com-gpagentd", NULL, FALSE, 92, 9 },
    { NULL, NULL, FALSE, 0, 0 },
};

void
DJManageDaemons(
    PSTR pszDomainName,
    BOOLEAN bStart,
    LWException **exc
    )
{
    CHAR szPreCommand[512];
    PSTR pszDomainNameAllUpper = NULL;
    BOOLEAN bFileExists = TRUE;
    FILE* fp = NULL;
    PSTR pszErrFilePath = "/var/cache/centeris/grouppolicy/gpagentd.err";
    CHAR szBuf[256+1];
    DWORD dwGPErrCode = 0;
    LWException *innerExc = NULL;
    int daemonCount;
    int i;

#define PWGRD "/etc/rc.config.d/pwgr"
    LW_CLEANUP_CTERR(exc, CTCheckFileExists(PWGRD, &bFileExists));
    if(bFileExists)
    {
        //Shutdown pwgr (a nscd-like daemon) on HP-UX because it only handles
        //usernames up to 8 characters in length.
        LW_TRY(exc, DJStartStopDaemon("/sbin/init.d/pwgr", FALSE, NULL, &LW_EXC));
        LW_CLEANUP_CTERR(exc, CTRunSedOnFile(PWGRD, PWGRD, FALSE, "s/=1/=0/"));
    }

    //Figure out how many daemons there are
    for(daemonCount = 0; daemonList[daemonCount].primaryName != NULL; daemonCount++);

    if(bStart)
    {
        CHAR szStartPriority[32];
        CHAR szStopPriority[32];

        //Start the daemons in ascending order
        for(i = 0; i < daemonCount; i++)
        {
            sprintf(szStartPriority, "%d", daemonList[i].startPriority);
            sprintf(szStopPriority,  "%d", daemonList[i].stopPriority);
 
            DJManageDaemon(daemonList[i].primaryName,
                             bStart,
                             NULL,
                             szStartPriority,
                             szStopPriority,
                             &innerExc);

            //Try the alternate daemon name if there is one
            if (!LW_IS_OK(innerExc) &&
                    innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON &&
                    daemonList[i].alternativeName != NULL)
            {
                LW_HANDLE(&innerExc);
                DJManageDaemon(daemonList[i].alternativeName,
                                 bStart,
                                 NULL,
                                 szStartPriority,
                                 szStopPriority,
                                 &innerExc);
            }
            if (!LW_IS_OK(innerExc) &&
                    innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON &&
                    !daemonList[i].required)
            {
                LW_HANDLE(&innerExc);
            }
            if (LW_IS_OK(innerExc) && !strcmp(daemonList[i].primaryName, "centeris.com-gpagentd"))
            {
                LW_CLEANUP_CTERR(exc, CTCheckFileExists(pszErrFilePath, &bFileExists));

                if (bFileExists) {

                    LW_HANDLE(&innerExc);
                    fp = fopen(pszErrFilePath, "r");
                    if (fp != NULL) {

                        if (fgets(szBuf, 256, fp) != NULL) {

                            CTStripWhitespace(szBuf);

                            dwGPErrCode = atoi(szBuf);

                            if (dwGPErrCode == GPAGENT_LICENSE_ERROR ||
                                dwGPErrCode == GPAGENT_LICENSE_EXPIRED_ERROR) {

                                LW_RAISE(exc, CENTERROR_DOMAINJOIN_LICENSE_ERROR);
                                goto cleanup;

                            }
                        }

                    } else {

                        DJ_LOG_ERROR("Failed to open file [%s]", pszErrFilePath);

                    }
                }
            }
            LW_CLEANUP(exc, innerExc);
        }
    }
    else
    {
        CHAR szStartPriority[32];
        CHAR szStopPriority[32];

        //Stop the daemons in descending order
        for(i = daemonCount - 1; i >= 0; i--)
        {
            sprintf(szStartPriority, "%d", daemonList[i].startPriority);
            sprintf(szStopPriority,  "%d", daemonList[i].stopPriority);

            DJManageDaemon(daemonList[i].primaryName,
                             bStart,
                             NULL,
                             szStartPriority,
                             szStopPriority,
                             &innerExc);

            //Try the alternate daemon name if there is one
            if (!LW_IS_OK(innerExc) &&
                    innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON &&
                    daemonList[i].alternativeName != NULL)
            {
                LW_HANDLE(&innerExc);
                DJManageDaemon(daemonList[i].alternativeName,
                                 bStart,
                                 NULL,
                                 szStartPriority,
                                 szStopPriority,
                                 &innerExc);
            }
            if (!LW_IS_OK(innerExc) &&
                    innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON &&
                    !daemonList[i].required)
            {
                LW_HANDLE(&innerExc);
            }
            LW_CLEANUP(exc, innerExc);
        }
    }

cleanup:
    CTSafeCloseFile(&fp);

    if (pszDomainNameAllUpper)
        CTFreeString(pszDomainNameAllUpper);

    LW_HANDLE(&innerExc);
}
