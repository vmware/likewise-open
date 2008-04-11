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
#include "djdistroinfo.h"
#include "djdaemonmgr.h"

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

static PSTR pszChkConfigPath = "/sbin/chkconfig";
static PSTR pszUpdateRcDFilePath = "/usr/sbin/update-rc.d";

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

static CENTERROR FindDaemonScript(PCSTR name, PSTR *path);

void
DJGetDaemonStatus(
    PCSTR pszDaemonName,
    PBOOLEAN pbStarted,
    LWException **exc
    )
{
    PSTR* ppszArgs = NULL;
    PSTR prefixedPath = NULL;
    PSTR initDir = NULL;
    PSTR daemonPath = NULL;
    PSTR altDaemonName = NULL;
    DWORD nArgs = 3;
    long status = 0;
    PPROCINFO pProcInfo = NULL;
    BOOLEAN daemon_installed = FALSE;
    CENTERROR ceError;

    if(pszDaemonName[0] == '/')
        LW_CLEANUP_CTERR(exc, CTStrdup(pszDaemonName, &prefixedPath));
    else
    {
        LW_CLEANUP_CTERR(exc, FindDaemonScript(pszDaemonName, &prefixedPath));
    }

    DJ_LOG_INFO("Checking status of daemon [%s]", prefixedPath);

    LW_CLEANUP_CTERR(exc, CTCheckFileExists(prefixedPath, &daemon_installed));

    if (!daemon_installed) {
        LW_CLEANUP_CTERR(exc, CENTERROR_DOMAINJOIN_MISSING_DAEMON);
    }

    /* AIX has /etc/rc.dt. When '/etc/rc.dt status' is run, it tries to
     * start XWindows. So to be safe, we won't call that.
     */
    if(!strcmp(prefixedPath, "/etc/rc.dt"))
    {
        status = 1;
    }
    else
    {
        LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs)));

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
    }

    if (!status) {
        *pbStarted = TRUE;
    }
    else if (status == 2 || status == 3 || status == 4)
        *pbStarted = FALSE;
    else if (status == 1)
    {
        // An unknown error occurred. Most likely the init script doesn't
        // support the query option. We'll have to query it with ps.

        pid_t daemonPid;
        const char *daemonBaseName = strrchr(pszDaemonName, '/');
        if(daemonBaseName == NULL)
            daemonBaseName = pszDaemonName;

        DJ_LOG_VERBOSE("Looking for %s", daemonBaseName);
        ceError = CTFindFileInPath(daemonBaseName, "/usr/local/sbin:/usr/local/bin:/usr/dt/bin:/usr/sbin:/usr/bin:/sbin:/bin", &daemonPath);
        if(ceError == CENTERROR_FILE_NOT_FOUND)
        {
            CT_SAFE_FREE_STRING(altDaemonName);
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&altDaemonName,
                    "%sd", daemonBaseName));
            ceError = CTFindFileInPath(altDaemonName, "/usr/local/sbin:/usr/local/bin:/usr/dt/bin:/usr/sbin:/usr/bin:/sbin:/bin", &daemonPath);
        }
        LW_CLEANUP_CTERR(exc, ceError);

        DJ_LOG_VERBOSE("Found %s", daemonPath);
        ceError = CTGetPidOfCmdLine(NULL, daemonPath, NULL, 0, &daemonPid, NULL);
        if(ceError == CENTERROR_NO_SUCH_PROCESS || ceError == CENTERROR_NOT_IMPLEMENTED)
        {
            //Nope, couldn't find the daemon running
            *pbStarted = FALSE;
        }
        else
        {
            LW_CLEANUP_CTERR(exc, ceError);
            DJ_LOG_INFO("Even though '%s status' exited with code 1, '%s' is running with pid %d.", prefixedPath, daemonPath, daemonPid);
            *pbStarted = TRUE;
        }
    }
    else {
        LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_UNEXPECTED_ERRCODE, "Non-standard return code from init script", "According to http://forgeftp.novell.com/library/SUSE%20Package%20Conventions/spc_init_scripts.html, init scripts should return 0, 1, 2, 3, or 4. However, '%s status' returned %d.", status, prefixedPath);
        goto cleanup;
    }

cleanup:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    CT_SAFE_FREE_STRING(prefixedPath);
    CT_SAFE_FREE_STRING(initDir);
    CT_SAFE_FREE_STRING(daemonPath);
    CT_SAFE_FREE_STRING(altDaemonName);
}

static CENTERROR FindDaemonScript(PCSTR name, PSTR *path)
{
    PSTR altName = NULL;
    CENTERROR ceError;

    *path = NULL;

    ceError = CTFindFileInPath(name, "/etc/init.d:/etc/rc.d/init.d:/sbin/init.d", path);
    if(ceError == CENTERROR_FILE_NOT_FOUND)
    {
        CT_SAFE_FREE_STRING(altName);
        GCE(ceError = CTAllocateStringPrintf(&altName,
                    "%s.rc", name));
        ceError = CTFindFileInPath(altName, "/etc/init.d:/etc/rc.d/init.d:/sbin/init.d", path);
    }
    if(ceError == CENTERROR_FILE_NOT_FOUND && !strcmp(name, "dtlogin"))
    {
        CT_SAFE_FREE_STRING(altName);
        GCE(ceError = CTAllocateStringPrintf(&altName,
                    "rc.dt"));
        ceError = CTFindFileInPath(altName, "/etc:/etc/init.d:/etc/rc.d/init.d:/sbin/init.d", path);
    }
    if(ceError == CENTERROR_FILE_NOT_FOUND)
        GCE(ceError = CENTERROR_DOMAINJOIN_MISSING_DAEMON);
    GCE(ceError);

cleanup:
    CT_SAFE_FREE_STRING(altName);
    return ceError;
}

void
DJStartStopDaemon(
    PCSTR pszDaemonName,
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
    PSTR pszDaemonPath = NULL;
    CENTERROR ceError;
    int retry;

    ceError = FindDaemonScript(pszDaemonName, &pszDaemonPath);
    if(ceError == CENTERROR_DOMAINJOIN_MISSING_DAEMON)
    {
        LW_RAISE_EX(exc, ceError, "Unable to find daemon", "The '%s' daemon could not be found in /etc/rc.d/init.d, /etc/init.d, or /sbin/init.d");
    }
    LW_CLEANUP_CTERR(exc, ceError);

    if (bStatus) {
        DJ_LOG_INFO("Starting daemon [%s]", pszDaemonPath);
    } else {
        DJ_LOG_INFO("Stopping daemon [%s]", pszDaemonPath);
    }

    if (!strcmp(pszDaemonPath, "/etc/rc.dt") && !bStatus)
    {
        // The dtlogin init script is broken on AIX. It always starts the
        // process.

        LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs)));
        LW_CLEANUP_CTERR(exc, CTAllocateString("/bin/sh", ppszArgs));
        LW_CLEANUP_CTERR(exc, CTAllocateString("-c", ppszArgs+1));

        LW_CLEANUP_CTERR(exc, CTAllocateString("kill `cat /var/dt/Xpid`", ppszArgs+2));
    }
    else if (IsNullOrEmptyString(pszPreCommand)) {

        LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs)));
        LW_CLEANUP_CTERR(exc, CTAllocateString(pszDaemonPath, ppszArgs));
        LW_CLEANUP_CTERR(exc, CTAllocateString((bStatus ? "start" : "stop"), ppszArgs+1));

    } else {

        LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs)));
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

    for(retry = 0; retry < 3; retry++)
    {
        LW_TRY(exc, DJGetDaemonStatus(pszDaemonName, &bStarted, &LW_EXC));
        if (bStarted == bStatus)
            break;
        sleep(1);
    }

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

    CT_SAFE_FREE_STRING(pszDaemonPath);
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

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs));
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

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs));
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
    DistroInfo distro;
    PSTR searchExpression = NULL;
    PSTR *matchingPaths = NULL;
    DWORD matchCount;
    PSTR symlinkTarget = NULL;
    PSTR symlinkName = NULL;

    memset(&distro, 0, sizeof(distro));

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

    LW_CLEANUP_CTERR(exc, DJGetDistroInfo("", &distro));

    if(distro.os == OS_AIX)
    {
        /* Programs on AIX may store their init scripts in /etc/rc.d/init.d .
         * The symlinks for specific runlevels are stored in /etc/rc.d/rc2.d,
         * /etc/rc.d/rc3.d, upto /etc/rc9.d . Only runlevels 0-2 are used.
         * Init scripts can only be started in 0 or 1 by editing inittab. Run
         * levels 3 and higher are left for the user to define.
         *
         * During startup, the /etc/rc.d/rc script runs all of the kill
         * scripts in /etc/rc.d/rc2.d followed by all of the start scripts in
         * that directory. Since the system goes to runlevel 2 at start up,
         * only scripts in that directory are run.
         *
         * During shutdown, the /sbin/shutdown program directly (not through
         * init or rc) runs all of the kill scripts in /etc/rc.d/rc*.d .
         *
         * So in order for a daemon to correctly be started once when the
         * machine boots, and only be shutdown once when the machine shuts
         * down, it should only create start and kill symlink in
         * /etc/rc.d/rc2.d.
         */
        if(bStatus)
        {
            LW_CLEANUP_CTERR(exc, FindDaemonScript(pszDaemonName,
                        &symlinkTarget));
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&symlinkName,
                    "/etc/rc.d/rc2.d/K%s%s", pszStopPriority, pszDaemonName));
            LW_CLEANUP_CTERR(exc, CTCreateSymLink(symlinkTarget,
                        symlinkName));
            CT_SAFE_FREE_STRING(symlinkName);
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&symlinkName,
                    "/etc/rc.d/rc2.d/S%s%s", pszStartPriority, pszDaemonName));
            LW_CLEANUP_CTERR(exc, CTCreateSymLink(symlinkTarget,
                        symlinkName));
        }
        else
        {
            /* Remove any symlinks to the daemon if they exist */
            PCSTR directories[] = {
                "/etc/rc.d/rc2.d",
                "/etc/rc.d/rc3.d",
                "/etc/rc.d/rc4.d",
                "/etc/rc.d/rc5.d",
                "/etc/rc.d/rc6.d",
                "/etc/rc.d/rc7.d",
                "/etc/rc.d/rc8.d",
                "/etc/rc.d/rc9.d",
                NULL };
            int i, j;

            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&searchExpression,
                    "^.*%s$", pszDaemonName));
            for(i = 0; directories[i] != NULL; i++)
            {
                LW_CLEANUP_CTERR(exc, CTGetMatchingFilePathsInFolder(
                    directories[i], searchExpression, &matchingPaths,
                    &matchCount));
                for(j = 0; j < matchCount; j++)
                {
                    LW_CLEANUP_CTERR(exc, CTRemoveFile(matchingPaths[j]));
                }
                //CTFreeStringArray will ignore matchingPaths if it is NULL
                CTFreeStringArray(matchingPaths, matchCount);
                matchingPaths = NULL;
            }
        }
    }

done:
cleanup:
    CT_SAFE_FREE_STRING(searchExpression);
    CT_SAFE_FREE_STRING(symlinkTarget);
    CT_SAFE_FREE_STRING(symlinkName);
    CTFreeStringArray(matchingPaths, matchCount);
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

    LW_TRY(exc, DJConfigureForDaemonRestart(pszName, bStatus, pszStartPriority, pszStopPriority, &LW_EXC));

cleanup:
    ;
}

struct _DaemonList daemonList[] = {
    { "centeris.com-lwiauthd", "likewise-open", TRUE, 92, 10 },
    { "centeris.com-gpagentd", NULL, FALSE, 92, 9 },
    { NULL, NULL, FALSE, 0, 0 },
};

