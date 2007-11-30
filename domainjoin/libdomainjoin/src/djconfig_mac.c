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

typedef enum {
	CSPSearchPath = 0,
	NSPSearchPath
} SearchPolicyType;

#define LWIDSPLUGIN_SYMLINK_PATH "/System/Library/Frameworks/DirectoryService.framework/Versions/Current/Resources/Plugins/LWIDSPlugin.dsplug"
#define LWIDSPLUGIN_INSTALL_PATH "/opt/centeris/lib/LWIDSPlugin.dsplug"
#define LWIDSPLUGIN_NAME         "/Likewise Identity/Active Directory"
#define PID_FILE_CONTENTS_SIZE   ((9 * 2) + 2)
#define CONFIGD_PID_FILE         "/var/run/configd.pid"

static
 CENTERROR DJGetConfigDPID(pid_t * ppid)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	BOOLEAN bFileExists = FALSE;
	char contents[PID_FILE_CONTENTS_SIZE];
	int fd = -1;
	int len = 0;

	ceError = CTCheckFileExists(CONFIGD_PID_FILE, &bFileExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (!bFileExists) {
		*ppid = 0;
		goto error;
	}

	fd = open(CONFIGD_PID_FILE, O_RDONLY, 0644);
	if (fd < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if ((len = read(fd, contents, sizeof(contents) - 1)) < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	} else if (len == 0) {
		*ppid = 0;
		goto error;
	}
	contents[len - 1] = 0;

	*ppid = atoi(contents);

      error:

	if (fd >= 0) {
		close(fd);
	}

	return ceError;
}

static CENTERROR DJNotifyConfigDaemon()
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	pid_t pid = 0;

	ceError = DJGetConfigDPID(&pid);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (pid > 0) {
		if (kill(pid, SIGHUP) < 0) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}
	}

      error:

	return ceError;
}

static CENTERROR DJAddToKicker()
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PPROCINFO pProcInfo = NULL;
	PSTR *ppszArgs = NULL;
	DWORD nArgs = 3;
	LONG status = 0;

	DJ_LOG_INFO("Setting up Network Change Prompt in Kicker...");

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("/opt/centeris/bin/lwikickercfg", ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("-install", ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {
		ceError = CENTERROR_DOMAINJOIN_FAILED_KICKER_INSTALL;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = DJNotifyConfigDaemon();
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	if (ppszArgs) {
		CTFreeStringArray(ppszArgs, nArgs);
	}

	if (pProcInfo) {
		FreeProcInfo(pProcInfo);
	}

	return ceError;
}

static CENTERROR DJRemoveFromKicker()
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PPROCINFO pProcInfo = NULL;
	PSTR *ppszArgs = NULL;
	DWORD nArgs = 3;
	LONG status = 0;

	DJ_LOG_INFO("Removing Network Change Prompt from Kicker...");

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("/opt/centeris/bin/lwikickercfg", ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("-uninstall", ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {
		ceError = CENTERROR_DOMAINJOIN_FAILED_KICKER_UNINSTALL;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = DJNotifyConfigDaemon();
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	if (ppszArgs) {
		CTFreeStringArray(ppszArgs, nArgs);
	}

	if (pProcInfo) {
		FreeProcInfo(pProcInfo);
	}

	return ceError;
}

static CENTERROR DJSetSearchPath(SearchPolicyType searchPolicyType)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PPROCINFO pProcInfo = NULL;
	PSTR *ppszArgs = NULL;
	DWORD nArgs = 7;
	LONG status = 0;

	DJ_LOG_INFO("Setting search policy to %s",
		    searchPolicyType ==
		    CSPSearchPath ? "Custom path" : "Automatic");

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("/usr/bin/dscl", ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("/Search", ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("-create", ppszArgs + 2);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("/", ppszArgs + 3);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("SearchPolicy", ppszArgs + 4);
	BAIL_ON_CENTERIS_ERROR(ceError);

	switch (searchPolicyType) {
	case CSPSearchPath:
		{
			ceError =
			    CTAllocateString("CSPSearchPath", ppszArgs + 5);
			BAIL_ON_CENTERIS_ERROR(ceError);

			break;
		}
	case NSPSearchPath:
		{
			ceError =
			    CTAllocateString("NSPSearchPath", ppszArgs + 5);
			BAIL_ON_CENTERIS_ERROR(ceError);

			break;
		}
	}

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {
		ceError = CENTERROR_DOMAINJOIN_FAILED_SET_SEARCHPATH;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      error:

	if (ppszArgs) {
		CTFreeStringArray(ppszArgs, nArgs);
	}

	if (pProcInfo) {
		FreeProcInfo(pProcInfo);
	}

	return ceError;
}

/* Use dscl to place the LWIDSPlugin in the authenticator list */
static CENTERROR DJRegisterLWIDSPlugin()
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PPROCINFO pProcInfo = NULL;
	PSTR *ppszArgs = NULL;
	DWORD nArgs = 7;
	LONG status = 0;

	DJ_LOG_INFO
	    ("Registering LWIDSPlugin for Open Directory Authentication");

	ceError = DJSetSearchPath(CSPSearchPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("/usr/bin/dscl", ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("/Search", ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("-append", ppszArgs + 2);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("/", ppszArgs + 3);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("CSPSearchPath", ppszArgs + 4);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(LWIDSPLUGIN_NAME, ppszArgs + 5);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {
		ceError = CENTERROR_DOMAINJOIN_FAILED_REG_OPENDIR;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      error:

	if (ppszArgs) {
		CTFreeStringArray(ppszArgs, nArgs);
	}

	if (pProcInfo) {
		FreeProcInfo(pProcInfo);
	}

	return ceError;
}

/* Remove LWIDSPlugin from the authenticator list */
static CENTERROR DJUnregisterLWIDSPlugin()
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PPROCINFO pProcInfo = NULL;
	PSTR *ppszArgs = NULL;
	DWORD nArgs = 7;
	LONG status = 0;

	DJ_LOG_INFO
	    ("Unregistering LWIDSPlugin from Open Directory Authentication");

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("/usr/bin/dscl", ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("/Search", ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("-delete", ppszArgs + 2);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("/", ppszArgs + 3);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("CSPSearchPath", ppszArgs + 4);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(LWIDSPLUGIN_NAME, ppszArgs + 5);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {
		ceError = CENTERROR_DOMAINJOIN_FAILED_UNREG_OPENDIR;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = DJSetSearchPath(NSPSearchPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	if (ppszArgs) {
		CTFreeStringArray(ppszArgs, nArgs);
	}

	if (pProcInfo) {
		FreeProcInfo(pProcInfo);
	}

	return ceError;
}

/*
   The LWIDSPlugin is saved to /opt/centeris/lib/LWIDSPlugin.dsplug upon installation

   In order to participate in Open Directory Services, we need to create a symbolic link

   from

   /System/Library/Frameworks/DirectoryService.framework/Versions/Current/Resources/Plugins/LWIDSPlugin.dsplug

   to

   /opt/centeris/lib/LWIDSPlugin.dsplug
*/
#if 0
static CENTERROR DJEngageLWIDSPlugin()
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	BOOLEAN bDirExists = FALSE;
	BOOLEAN bLinkExists = FALSE;
	BOOLEAN bCreateSymlink = TRUE;
	PSTR pszTargetPath = NULL;

	ceError = CTCheckDirectoryExists(LWIDSPLUGIN_INSTALL_PATH, &bDirExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (!bDirExists) {
		DJ_LOG_ERROR("LWIDSPlugin folder [%s] does not exist",
			     LWIDSPLUGIN_INSTALL_PATH);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = CTCheckLinkExists(LWIDSPLUGIN_SYMLINK_PATH, &bLinkExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (bLinkExists) {
		ceError =
		    CTGetSymLinkTarget(LWIDSPLUGIN_SYMLINK_PATH,
				       &pszTargetPath);
		BAIL_ON_CENTERIS_ERROR(ceError);

		if (strcmp(pszTargetPath, LWIDSPLUGIN_INSTALL_PATH)) {
			DJ_LOG_INFO("Removing symbolic link at [%s]",
				    LWIDSPLUGIN_SYMLINK_PATH);
			ceError = CTRemoveFile(LWIDSPLUGIN_SYMLINK_PATH);
			BAIL_ON_CENTERIS_ERROR(ceError);
		} else {
			bCreateSymlink = FALSE;
		}
	}

	if (bCreateSymlink) {
		ceError =
		    CTCreateSymLink(LWIDSPLUGIN_SYMLINK_PATH,
				    LWIDSPLUGIN_INSTALL_PATH);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      error:

	if (pszTargetPath) {
		CTFreeString(pszTargetPath);
	}

	return ceError;
}

static CENTERROR DJDisengageLWIDSPlugin()
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	BOOLEAN bLinkExists = FALSE;
	BOOLEAN bDirExists = FALSE;

	ceError = CTCheckLinkExists(LWIDSPLUGIN_SYMLINK_PATH, &bLinkExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (bLinkExists) {
		DJ_LOG_INFO("Removing symbolic link at [%s]",
			    LWIDSPLUGIN_SYMLINK_PATH);

		ceError = CTRemoveFile(LWIDSPLUGIN_SYMLINK_PATH);
		BAIL_ON_CENTERIS_ERROR(ceError);

		goto done;
	}

	/* If a directory exists in the place of the symlink, remove that instead */
	ceError = CTCheckDirectoryExists(LWIDSPLUGIN_SYMLINK_PATH, &bDirExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (bDirExists) {
		ceError = CTRemoveDirectory(LWIDSPLUGIN_SYMLINK_PATH);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      done:
      error:

	return ceError;
}
#endif

CENTERROR DJConfigureLWIDSPlugin()
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	ceError = DJRegisterLWIDSPlugin();
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJAddToKicker();
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	return ceError;
}

CENTERROR DJUnconfigureLWIDSPlugin()
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	ceError = DJRemoveFromKicker();
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJUnregisterLWIDSPlugin();
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	return ceError;
}
