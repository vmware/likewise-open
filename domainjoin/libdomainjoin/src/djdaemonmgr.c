/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007.  
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "domainjoin.h"

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

CENTERROR DJGetDaemonStatus(PSTR pszDaemonPath, PBOOLEAN pbStarted)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR *ppszArgs = NULL;
	DWORD nArgs = 3;
	long status = 0;
	PPROCINFO pProcInfo = NULL;

	DJ_LOG_INFO("Checking status of daemon [%s]", pszDaemonPath);

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(pszDaemonPath, ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("status", ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSpawnProcessSilent(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	// see
	// http://forgeftp.novell.com/library/SUSE%20Package%20Conventions/spc_init_scripts.html
	// and http://www.linuxbase.org/~gk4/wip-sys-init.html
	//
	// note further that some other error codes might be thrown, so
	// we choose to only pay attention to the ones that lsb says are
	// valid return codes for status that indicate that a progam
	// isnt running, otherwise, we are gonna throw it.

	DJ_LOG_INFO("Daemon [%s]: status [%d]", pszDaemonPath, status);

	if (!status) {
		*pbStarted = TRUE;
	} else if (status == 1 || status == 2 || status == 3 || status == 4)
		*pbStarted = FALSE;
	else {
		ceError = CENTERROR_DOMAINJOIN_UNEXPECTED_ERRCODE;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      error:

	if (ppszArgs)
		CTFreeStringArray(ppszArgs, nArgs);

	if (pProcInfo)
		FreeProcInfo(pProcInfo);

	return ceError;
}

CENTERROR
DJStartStopDaemon(PSTR pszDaemonPath, BOOLEAN bStatus, PSTR pszPreCommand)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR *ppszArgs = NULL;
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

		ceError =
		    CTAllocateMemory(sizeof(PSTR) * nArgs,
				     (PVOID *) & ppszArgs);
		BAIL_ON_CENTERIS_ERROR(ceError);

		ceError = CTAllocateString(pszDaemonPath, ppszArgs);
		BAIL_ON_CENTERIS_ERROR(ceError);

		ceError =
		    CTAllocateString((bStatus ? "start" : "stop"),
				     ppszArgs + 1);
		BAIL_ON_CENTERIS_ERROR(ceError);

	} else {

		ceError =
		    CTAllocateMemory(sizeof(PSTR) * nArgs,
				     (PVOID *) & ppszArgs);
		BAIL_ON_CENTERIS_ERROR(ceError);

		ceError = CTAllocateString("/bin/sh", ppszArgs);
		BAIL_ON_CENTERIS_ERROR(ceError);

		ceError = CTAllocateString("-c", ppszArgs + 1);
		BAIL_ON_CENTERIS_ERROR(ceError);

		sprintf(szBuf, "%s; %s %s ",
			pszPreCommand,
			pszDaemonPath, (bStatus ? "start" : "stop"));

		ceError = CTAllocateString(szBuf, ppszArgs + 2);
		BAIL_ON_CENTERIS_ERROR(ceError);

	}

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetDaemonStatus(pszDaemonPath, &bStarted);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (bStarted != bStatus) {

		ceError = CENTERROR_DOMAINJOIN_INCORRECT_STATUS;
		BAIL_ON_CENTERIS_ERROR(ceError);

	}

      error:

	if (ppszArgs)
		CTFreeStringArray(ppszArgs, nArgs);

	if (pProcInfo)
		FreeProcInfo(pProcInfo);

	return ceError;
}

CENTERROR
DJDoUpdateRcD(PSTR pszDaemonName,
	      BOOLEAN bStatus, PSTR pszStartPriority, PSTR pszStopPriority)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR *ppszArgs = NULL;
	DWORD nArgs = 5;
	long status = 0;
	PPROCINFO pProcInfo = NULL;
	BOOLEAN bStarted = FALSE;
	CHAR szDaemonPath[PATH_MAX + 1];
	PSTR ppszStartSpecialArgs[] = { "start",
		"90",
		"2",
		"3",
		"4",
		"5",
		".",
		"stop",
		"10",
		"0",
		"1",
		"6",
		"."
	};
	DWORD nSpecialArgs = sizeof(ppszStartSpecialArgs) / sizeof(PSTR);
	DWORD iArg = 0;

	snprintf(szDaemonPath, sizeof(szDaemonPath) - 1, "/etc/init.d/%s",
		 pszDaemonName);

	nArgs = nSpecialArgs + 3;

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(pszUpdateRcDFilePath, ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (!bStatus) {

		ceError = CTAllocateString("-f", ppszArgs + 1);
		BAIL_ON_CENTERIS_ERROR(ceError);

		ceError = CTAllocateString(pszDaemonName, ppszArgs + 2);
		BAIL_ON_CENTERIS_ERROR(ceError);

		ceError = CTAllocateString("remove", ppszArgs + 3);
		BAIL_ON_CENTERIS_ERROR(ceError);

	} else {

		ceError = CTAllocateString(pszDaemonName, ppszArgs + 1);
		BAIL_ON_CENTERIS_ERROR(ceError);

		//
		// TODO-2006/10/31-dalmeida -- Should not hardcode runlevels.
		//
		for (iArg = 0; iArg < nSpecialArgs; iArg++) {

			/* Skip NULL parameters */
			if (!ppszStartSpecialArgs[iArg])
				continue;

			ceError =
			    CTAllocateString(*(ppszStartSpecialArgs + iArg),
					     ppszArgs + iArg + 2);
			BAIL_ON_CENTERIS_ERROR(ceError);

		}

		/* FIX ME!  Use a beter way than hard coding the
		   indexes into the array here for update-rc.d  --jerry */

		if (pszStartPriority) {
			ppszStartSpecialArgs[1] = pszStartPriority;
		}

		if (pszStopPriority) {
			ppszStartSpecialArgs[8] = pszStopPriority;
		}
	}

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetDaemonStatus(szDaemonPath, &bStarted);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {

		ceError = CENTERROR_DOMAINJOIN_UPDATERCD_FAILED;
		BAIL_ON_CENTERIS_ERROR(ceError);

	}

      error:

	if (ppszArgs)
		CTFreeStringArray(ppszArgs, nArgs);

	if (pProcInfo)
		FreeProcInfo(pProcInfo);

	return ceError;
}

CENTERROR DJDoChkConfig(PSTR pszDaemonName, BOOLEAN bStatus)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	CHAR szDaemonPath[PATH_MAX + 1];
	PSTR *ppszArgs = NULL;
	DWORD nArgs = 4;
	LONG status = 0;
	PPROCINFO pProcInfo = NULL;
	FILE *fp = NULL;
	CHAR szBuf[1024 + 1];

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

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(pszChkConfigPath, ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString((bStatus ? "--add" : "--del"), ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(pszDaemonName, ppszArgs + 2);
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

				CTFreeString(*(ppszArgs + 1));

				if (pProcInfo) {
					FreeProcInfo(pProcInfo);
					pProcInfo = NULL;
				}

				ceError =
				    CTAllocateString((bStatus ? "on" : "off"),
						     ppszArgs + 1);
				BAIL_ON_CENTERIS_ERROR(ceError);

				ceError =
				    DJSpawnProcess(ppszArgs[0], ppszArgs,
						   &pProcInfo);
				BAIL_ON_CENTERIS_ERROR(ceError);

				ceError =
				    DJGetProcessStatus(pProcInfo, &status);
				BAIL_ON_CENTERIS_ERROR(ceError);

				if (status != 0) {
					ceError =
					    CENTERROR_DOMAINJOIN_CHKCONFIG_FAILED;
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

CENTERROR
DJConfigureForDaemonRestart(PSTR pszDaemonName,
			    BOOLEAN bStatus,
			    PSTR pszStartPriority, PSTR pszStopPriority)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	BOOLEAN bFileExists = FALSE;

	ceError = CTCheckFileExists(pszChkConfigPath, &bFileExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (bFileExists) {

		ceError = DJDoChkConfig(pszDaemonName, bStatus);
		BAIL_ON_CENTERIS_ERROR(ceError);

		goto done;
	}

	ceError = CTCheckFileExists(pszUpdateRcDFilePath, &bFileExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (bFileExists) {

		ceError =
		    DJDoUpdateRcD(pszDaemonName, bStatus, pszStartPriority,
				  pszStopPriority);
		BAIL_ON_CENTERIS_ERROR(ceError);

		goto done;
	}

      done:
      error:

	return ceError;
}

CENTERROR
DJManageDaemon(PSTR pszName,
	       BOOLEAN bStatus,
	       PSTR pszPreCommand, PSTR pszStartPriority, PSTR pszStopPriority)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	CHAR szBuf[PATH_MAX + 1];
	BOOLEAN bFileExists = FALSE;
	BOOLEAN bStarted = FALSE;

#if defined(_AIX)
	sprintf(szBuf, "/etc/rc.d/init.d/%s", pszName);
#elif defined(_HPUX_SOURCE)
	sprintf(szBuf, "/sbin/init.d/%s", pszName);
#else
	sprintf(szBuf, "/etc/init.d/%s", pszName);
#endif

	ceError = CTCheckFileExists(szBuf, &bFileExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (!bFileExists) {
		ceError = CENTERROR_DOMAINJOIN_MISSING_DAEMON;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}
	// check our current state prior to doing anything.  notice that
	// we are using the private version so that if we fail, our inner
	// exception will be the one that was tossed due to the failure.
	ceError = DJGetDaemonStatus(szBuf, &bStarted);
	BAIL_ON_CENTERIS_ERROR(ceError);

	// if we got this far, we have validated the existence of the
	// daemon and we have figured out if its started or stopped

	// if we are already in the desired state, do nothing.
	if (bStarted != bStatus) {

		ceError = DJStartStopDaemon(szBuf, bStatus, pszPreCommand);
		BAIL_ON_CENTERIS_ERROR(ceError);

	}

	ceError =
	    DJConfigureForDaemonRestart(pszName, bStatus, pszStartPriority,
					pszStopPriority);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	return ceError;
}

CENTERROR DJManageDaemons(PSTR pszDomainName, BOOLEAN bStart)
{
	return DJManageDaemon("likewise-open",
			      bStart,
			      NULL,
			      pszAuthdStartPriority, pszAuthdStopPriority);
}
