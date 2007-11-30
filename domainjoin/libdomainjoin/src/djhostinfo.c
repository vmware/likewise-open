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

CENTERROR DJGetComputerName(PSTR * ppszComputerName)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	CHAR szBuf[256 + 1];
	PSTR pszTmp = NULL;

	if (geteuid() != 0) {
		ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (gethostname(szBuf, 256) < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	pszTmp = szBuf;
	while (*pszTmp != '\0') {
		if (*pszTmp == '.') {
			*pszTmp = '\0';
			break;
		}
		pszTmp++;
	}

	if (IsNullOrEmptyString(szBuf)) {
		ceError = CENTERROR_DOMAINJOIN_INVALID_HOSTNAME;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = CTAllocateString(szBuf, ppszComputerName);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	return ceError;
}

static
    CENTERROR
WriteHostnameToFiles(PSTR pszComputerName, PSTR * ppszHostfilePaths)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszFilePath = (ppszHostfilePaths ? *ppszHostfilePaths : NULL);
	BOOLEAN bFileExists = FALSE;
	FILE *fp = NULL;

	while (pszFilePath != NULL && *pszFilePath != '\0') {

		ceError = CTCheckFileExists(pszFilePath, &bFileExists);
		BAIL_ON_CENTERIS_ERROR(ceError);

		if (bFileExists) {
			fp = fopen(pszFilePath, "w");
			if (fp == NULL) {
				ceError = CTMapSystemError(errno);
				BAIL_ON_CENTERIS_ERROR(ceError);
			}
			fprintf(fp, "%s\n", pszComputerName);
			fclose(fp);
			fp = NULL;
		}

		pszFilePath = *(++ppszHostfilePaths);
	}

      error:

	if (fp) {
		fclose(fp);
	}

	return ceError;
}

#if defined(_HPUX_SOURCE)
#define NETCONF "/etc/rc.config.d/netconf"
static CENTERROR SetHPUXHostname(PSTR pszComputerName)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PPROCINFO pProcInfo = NULL;
	PSTR *ppszArgs = NULL;
	DWORD nArgs = 6;
	CHAR szBuf[512];
	LONG status = 0;

	DJ_LOG_INFO("Setting hostname to [%s]", pszComputerName);

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("/bin/sh", ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("-c", ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	memset(szBuf, 0, sizeof(szBuf));
	snprintf(szBuf, sizeof(szBuf),
		 "/usr/bin/sed s/HOSTNAME=\\\"[a-zA-Z0-9].*\\\"/HOSTNAME=\\\"%s\\\"/ %s > %s.new",
		 pszComputerName, NETCONF, NETCONF);
	ceError = CTAllocateString(szBuf, ppszArgs + 2);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {
		ceError = CENTERROR_DOMAINJOIN_FAILED_SET_HOSTNAME;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	memset(szBuf, 0, sizeof(szBuf));
	snprintf(szBuf, sizeof(szBuf), "%s.new", NETCONF);

	ceError = CTMoveFile(szBuf, NETCONF);
	BAIL_ON_CENTERIS_ERROR(ceError);

	CTFreeStringArray(ppszArgs, nArgs);
	ppszArgs = NULL;
	FreeProcInfo(pProcInfo);
	pProcInfo = NULL;

	/* After updating the file, HP-UX wants us to "start" the hostname */
	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("/sbin/init.d/hostname", ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("start", ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {
		ceError = CENTERROR_DOMAINJOIN_FAILED_SET_HOSTNAME;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      error:
	if (ppszArgs)
		CTFreeStringArray(ppszArgs, nArgs);

	if (pProcInfo)
		FreeProcInfo(pProcInfo);

	return ceError;
}
#endif				/* _HPUX_SOURCE */

#if defined(_AIX)
static CENTERROR SetAIXHostname(PSTR pszComputerName)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PPROCINFO pProcInfo = NULL;
	PSTR *ppszArgs = NULL;
	DWORD nArgs = 6;
	CHAR szBuf[512];
	LONG status = 0;

	DJ_LOG_INFO("Setting hostname to [%s]", pszComputerName);

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("chdev", ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("-a", ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	sprintf(szBuf, "hostname=%s", pszComputerName);
	ceError = CTAllocateString(szBuf, ppszArgs + 2);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("-l", ppszArgs + 3);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("inet0", ppszArgs + 4);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {
		ceError = CENTERROR_DOMAINJOIN_FAILED_SET_HOSTNAME;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      error:

	if (ppszArgs)
		CTFreeStringArray(ppszArgs, nArgs);

	if (pProcInfo)
		FreeProcInfo(pProcInfo);

	return ceError;
}

#endif

#if defined(__LWI_SOLARIS__)
static CENTERROR WriteHostnameToSunFiles(PSTR pszComputerName)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	FILE *fp = NULL;
	PSTR *ppszHostfilePaths = NULL;
	DWORD nPaths = 0;
	DWORD iPath = 0;

	DJ_LOG_INFO("Setting hostname to [%s]", pszComputerName);

	fp = fopen("/etc/nodename", "w");
	if (fp == NULL) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}
	fprintf(fp, "%s\n", pszComputerName);
	fclose(fp);
	fp = NULL;

	ceError = CTGetMatchingFilePathsInFolder("/etc",
						 "hostname.*",
						 &ppszHostfilePaths, &nPaths);
	BAIL_ON_CENTERIS_ERROR(ceError);

	for (iPath = 0; iPath < nPaths; iPath++) {

		fp = fopen(*(ppszHostfilePaths + iPath), "w");
		if (fp == NULL) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}
		fprintf(fp, "%s\n", pszComputerName);
		fclose(fp);
		fp = NULL;
	}

      error:

	if (ppszHostfilePaths)
		CTFreeStringArray(ppszHostfilePaths, nPaths);

	if (fp) {
		fclose(fp);
	}

	return ceError;
}
#endif

#if defined(__LWI_MACOSX__)
static CENTERROR SetMacOsXHostName(PCSTR HostName)
{
	CENTERROR ceError;
	int EE = 0;
	char command[] = "scutil";
	/* ISSUE-2007/08/01-dalmeida -- Fix const-ness of arg array in procutils */
	PSTR args[5] = { command, "--set", "HostName", (char *)HostName };
	PPROCINFO procInfo = NULL;
	LONG status = 0;

	ceError = DJSpawnProcessSilent(command, args, &procInfo);
	GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE);

	ceError = DJGetProcessStatus(procInfo, &status);
	GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE);

	if (status != 0) {
		DJ_LOG_ERROR("%s failed [Status code: %d]", command, status);
		ceError = CENTERROR_DOMAINJOIN_FAILED_SET_HOSTNAME;
		GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE);
	}

      cleanup:
	if (procInfo) {
		FreeProcInfo(procInfo);
	}

	DJ_LOG_VERBOSE("SetMacOsXHostName LEAVE -> 0x%08x (EE = %d)", ceError,
		       EE);

	return ceError;
}
#endif

static CENTERROR DJCheckIfDHCPHost(PSTR pszPathifcfg, PBOOLEAN pbDHCPHost)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszFilter = "^[[:space:]]*BOOTPROTO.*dhcp.*$";
	BOOLEAN bDHCPHost = FALSE;

	DJ_LOG_INFO("Checking if DHCP Host...");

	// now that we have a file, we need to check out our BOOTPROTO,
	// if it's DHCP, we have to update the DHCP_HOSTNAME
	// ps: the expression should be BOOTPROTO='?dhcp'? because RH uses dhcp and SuSE 'dhcp'
	// sRun = "grep BOOTPROTO=\\'\\\\?dhcp\\'\\\\? " + sPathifcfg;
	//sRun = "grep BOOTPROTO=\\'*dhcp\\'* " + sPathifcfg;

	ceError = CTCheckFileHoldsPattern(pszPathifcfg, pszFilter, &bDHCPHost);
	BAIL_ON_CENTERIS_ERROR(ceError);

	*pbDHCPHost = bDHCPHost;

	return ceError;

      error:

	*pbDHCPHost = FALSE;

	return ceError;
}

static CENTERROR GetTmpPath(PCSTR pszOriginalPath, PSTR * ppszTmpPath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PCSTR pszSuffix = ".domainjoin";
	PSTR pszTmpPath = NULL;

	ceError =
	    CTAllocateMemory(strlen(pszOriginalPath) + strlen(pszSuffix) + 1,
			     (PVOID *) & pszTmpPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	strcpy(pszTmpPath, pszOriginalPath);
	strcat(pszTmpPath, pszSuffix);

	*ppszTmpPath = pszTmpPath;

	return ceError;

      error:

	if (pszTmpPath)
		CTFreeString(pszTmpPath);

	return ceError;
}

static BOOLEAN IsComment(PSTR pszLine)
{
	PSTR pszTmp = pszLine;

	if (IsNullOrEmptyString(pszLine))
		return TRUE;

	while (*pszTmp != '\0' && isspace((int)*pszTmp))
		pszTmp++;

	return *pszTmp == '#' || *pszTmp == '\0';
}

static
    CENTERROR
DJReplaceNameValuePair(PSTR pszFilePath, PSTR pszName, PSTR pszValue)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszTmpPath = NULL;
	FILE *fpSrc = NULL;
	FILE *fpDst = NULL;
	regex_t rx;
	CHAR szRegExp[256];
	CHAR szBuf[1024 + 1];
	BOOLEAN bRemoveFile = FALSE;

	memset(&rx, 0, sizeof(rx));

	ceError = GetTmpPath(pszFilePath, &pszTmpPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	sprintf(szRegExp, "^[[:space:]]*%s[[:space:]]*=.*$", pszName);

	if (regcomp(&rx, szRegExp, REG_EXTENDED) < 0) {
		ceError = CENTERROR_REGEX_COMPILE_FAILED;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if ((fpSrc = fopen(pszFilePath, "r")) == NULL) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if ((fpDst = fopen(pszTmpPath, "w")) == NULL) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	bRemoveFile = TRUE;

	while (1) {

		if (fgets(szBuf, 1024, fpSrc) == NULL) {
			if (feof(fpSrc)) {
				break;
			} else {
				ceError = CTMapSystemError(errno);
				BAIL_ON_CENTERIS_ERROR(ceError);
			}
		}

		if (!IsComment(szBuf) &&
		    !regexec(&rx, szBuf, (size_t) 0, NULL, 0)) {

			if (fprintf(fpDst, "%s=%s\n", pszName, pszValue) < 0) {
				ceError = CTMapSystemError(errno);
				BAIL_ON_CENTERIS_ERROR(ceError);
			}

		} else {

			if (fputs(szBuf, fpDst) == EOF) {
				ceError = CTMapSystemError(errno);
				BAIL_ON_CENTERIS_ERROR(ceError);
			}

		}

	}

	fclose(fpSrc);
	fpSrc = NULL;
	fclose(fpDst);
	fpDst = NULL;

	ceError = CTBackupFile(pszFilePath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTMoveFile(pszTmpPath, pszFilePath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	bRemoveFile = FALSE;

      error:

	if (fpSrc)
		fclose(fpSrc);

	if (fpDst)
		fclose(fpDst);

	regfree(&rx);

	if (bRemoveFile)
		CTRemoveFile(pszTmpPath);

	if (pszTmpPath)
		CTFreeString(pszTmpPath);

	return ceError;
}

static
 CENTERROR DJAppendNameValuePair(PSTR pszFilePath, PSTR pszName, PSTR pszValue)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	FILE *fp = NULL;

	if ((fp = fopen(pszFilePath, "a")) == NULL) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (fprintf(fp, "\n%s=%s\n", pszName, pszValue) < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	fclose(fp);
	fp = NULL;

      error:

	if (fp)
		fclose(fp);

	return ceError;
}

CENTERROR DJFixDHCPHost(PSTR pszPathifcfg, PSTR pszComputerName)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	BOOLEAN bPatternExists = FALSE;

	ceError = CTCheckFileHoldsPattern(pszPathifcfg,
					  "^[[:space:]]*DHCP_HOSTNAME[[:space:]]*=.*$",
					  &bPatternExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (bPatternExists) {

		ceError = DJReplaceNameValuePair(pszPathifcfg,
						 "DHCP_HOSTNAME",
						 pszComputerName);
		BAIL_ON_CENTERIS_ERROR(ceError);

	} else {

		ceError = DJAppendNameValuePair(pszPathifcfg,
						"DHCP_HOSTNAME",
						pszComputerName);
		BAIL_ON_CENTERIS_ERROR(ceError);

	}

      error:

	return ceError;
}

static CENTERROR DJFixNetworkManagerOnlineTimeout()
{
	CENTERROR ceError = CENTERROR_SUCCESS;
#if 0
	PSTR pszFilePath = "/etc/sysconfig/network/config";
	DWORD dwTimeout = 60;
#endif

	/*
	 * TODO:
	 * This is required for SLES10
	 * Postponing for now
	 */

	return ceError;
}

static CENTERROR DJRestartDHCPService(PSTR pszComputerName)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	int EE = 0;
	BOOLEAN bFileExists = FALSE;
	PSTR dhcpFilePath = "/etc/sysconfig/network/dhcp";
	PSTR dhcpFilePathNew = "/etc/sysconfig/network/dhcp.new";
	PSTR ppszArgs[] = { "/bin/sed",
		"s/^.*\\(DHCLIENT_SET_HOSTNAME\\).*=.*$/\\1=\\\"no\\\"/",
		dhcpFilePath,
		NULL
	};
	PSTR ppszNetArgs[] = {
#if defined(_AIX)
		"/etc/rc.d/init.d/network",
#else
		"/etc/init.d/network",
#endif
		"restart",
		NULL
	};
	PPROCINFO pProcInfo = NULL;
	LONG status = 0;

	ceError = CTCheckFileExists(dhcpFilePath, &bFileExists);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	if (bFileExists) {

		ceError = CTBackupFile(dhcpFilePath);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);

		ceError =
		    DJSpawnProcessOutputToFile(ppszArgs[0], ppszArgs,
					       dhcpFilePathNew, &pProcInfo);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);

		ceError = DJGetProcessStatus(pProcInfo, &status);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);

		if (status != 0) {
			ceError = CENTERROR_DOMAINJOIN_DHCPRESTART_SET_FAIL;
			CLEANUP_ON_CENTERROR_EE(ceError, EE);
		}
		// Now move temp file into place
		ceError = CTMoveFile(dhcpFilePathNew, dhcpFilePath);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);
	}

	if (pProcInfo) {
		FreeProcInfo(pProcInfo);
		pProcInfo = NULL;
	}

	ceError = DJFixNetworkManagerOnlineTimeout();
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	/* Restart network */

	ceError = DJSpawnProcess(ppszNetArgs[0], ppszNetArgs, &pProcInfo);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	if (status != 0) {
		ceError = CENTERROR_DOMAINJOIN_DHCPRESTART_FAIL;
		CLEANUP_ON_CENTERROR_EE(ceError, EE);
	}

      cleanup:

	if (pProcInfo)
		FreeProcInfo(pProcInfo);

	DJ_LOG_VERBOSE("DJRestartDHCPService LEAVE -> 0x%08x (EE = %d)",
		       ceError, EE);

	return ceError;
}

static CENTERROR DJGetMachineSID(PSTR * ppszMachineSID)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PPROCINFO pProcInfo = NULL;
	PSTR *ppszArgs = NULL;
	DWORD nArgs = 0;
	CHAR szCmd[PATH_MAX + 1];
	LONG status = 0;
	PSTR pszBuffer = NULL;
	PSTR pszMachineSID = NULL;
	PSTR pszTmp = NULL;
	PROCBUFFER procBuffer;
	DWORD iBufIdx = 0;
	DWORD dwBufLen = 0;
	DWORD dwBufAvailable = 0;

	sprintf(szCmd, "%s/bin/lwinet", PREFIXDIR);

	nArgs = 3;

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(szCmd, ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("getlocalsid", ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSpawnProcess(szCmd, ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	do {

		ceError = DJReadData(pProcInfo, &procBuffer);
		BAIL_ON_CENTERIS_ERROR(ceError);

		if (procBuffer.dwOutBytesRead) {

			while (1) {

				if (procBuffer.dwOutBytesRead < dwBufAvailable) {

					memcpy(pszBuffer + iBufIdx,
					       procBuffer.szOutBuf,
					       procBuffer.dwOutBytesRead);

					iBufIdx += procBuffer.dwOutBytesRead;
					dwBufAvailable -=
					    procBuffer.dwOutBytesRead;

					*(pszBuffer + iBufIdx + 1) = '\0';

					break;

				} else {

					/*
					 * TODO: Limit the amount of memory acquired
					 */

					ceError = CTReallocMemory(pszBuffer,
								  (PVOID *) &
								  pszBuffer,
								  dwBufLen +
								  1024);
					BAIL_ON_CENTERIS_ERROR(ceError);

					dwBufLen += 1024;
					dwBufAvailable += 1024;
				}
			}
		}
	} while (!procBuffer.bEndOfFile);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {
		ceError = CENTERROR_DOMAINJOIN_SET_MACHINESID_FAIL;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (pszBuffer == NULL ||
	    (pszTmp = strstr(pszBuffer, ": ")) == NULL ||
	    *(pszTmp + 2) == '\0') {
		ceError = CENTERROR_DOMAINJOIN_NETCONFIGCMD_FAIL;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	CTStripTrailingWhitespace(pszBuffer);

	ceError = CTAllocateString(pszTmp + 2, &pszMachineSID);
	BAIL_ON_CENTERIS_ERROR(ceError);

	*ppszMachineSID = pszMachineSID;
	pszMachineSID = NULL;

      error:

	if (ppszArgs)
		CTFreeStringArray(ppszArgs, nArgs);

	if (pszBuffer)
		CTFreeMemory(pszBuffer);

	if (pszMachineSID)
		CTFreeString(pszMachineSID);

	if (pProcInfo)
		FreeProcInfo(pProcInfo);

	return ceError;
}

static CENTERROR DJSetMachineSID(PSTR pszMachineSID)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PPROCINFO pProcInfo = NULL;
	PSTR *ppszArgs = NULL;
	DWORD nArgs = 0;
	CHAR szCmd[PATH_MAX + 1];
	LONG status = 0;

	sprintf(szCmd, "%s/bin/lwinet", PREFIXDIR);

	nArgs = 3;

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(szCmd, ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(pszMachineSID, ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSpawnProcess(szCmd, ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {
		ceError = CENTERROR_DOMAINJOIN_SET_MACHINESID_FAIL;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      error:

	if (ppszArgs)
		CTFreeStringArray(ppszArgs, nArgs);

	if (pProcInfo)
		FreeProcInfo(pProcInfo);

	return ceError;
}

static CENTERROR FixNetworkInterfaces(PSTR pszComputerName)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	int EE = 0;
	BOOLEAN bFileExists = FALSE;
	BOOLEAN bDirExists = FALSE;
	PPROCINFO pProcInfo = NULL;
	PSTR *ppszArgs = NULL;
	DWORD nArgs = 0;
	PSTR *ppszPaths = NULL;
	DWORD nPaths = 0;
	DWORD iPath = 0;
	CHAR szBuf[1024];
	LONG status = 0;
	PSTR pszDefaultPathifcfg = "/etc/sysconfig/network-scripts/ifcfg-eth0";
	PCSTR pszEthFltr_1 = "/etc/sysconfig/network/ifcfg-eth-id-";
	PCSTR pszEthFltr_2 = "/etc/sysconfig/network/ifcfg-eth0";
	PCSTR pszEthFltr_3 = "/etc/sysconfig/network/ifcfg-eth-bus";
	PSTR pszPathifcfg = NULL;
	BOOLEAN bDHCPHost = FALSE;
	PSTR pszMachineSID = NULL;
	PCSTR networkConfigPath = "/etc/sysconfig/network";

	ceError = DJGetMachineSID(&pszMachineSID);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	/*
	 * fixup HOSTNAME variable in /etc/sysconfig/network file if it exists
	 * note that 'network' is a *directory* on some dists (ie SUSE),
	 * is a *file* on others (ie Redhat). weird.
	 */
	ceError = CTCheckFileExists(networkConfigPath, &bFileExists);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	if (bFileExists) {
		ceError = CTBackupFile(networkConfigPath);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);

		nArgs = 4;
		ceError =
		    CTAllocateMemory(sizeof(PSTR) * nArgs,
				     (PVOID *) & ppszArgs);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);

		ceError = CTAllocateString("/bin/sed", ppszArgs);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);

		sprintf(szBuf, "s/^.*\\(HOSTNAME\\).*=.*$/\\1=%s/",
			pszComputerName);
		ceError = CTAllocateString(szBuf, ppszArgs + 1);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);

		ceError = CTAllocateString(networkConfigPath, ppszArgs + 2);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);

		ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);

		ceError = DJGetProcessStatus(pProcInfo, &status);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);

		if (status != 0) {
			DJ_LOG_ERROR("Failed to sed: %d", status);
			ceError = CENTERROR_DOMAINJOIN_SYSCONFIG_EDIT_FAIL;
			CLEANUP_ON_CENTERROR_EE(ceError, EE);
		}

		CTFreeStringArray(ppszArgs, nArgs);
		ppszArgs = NULL;
	}

	ceError = CTCheckDirectoryExists("/etc/sysconfig", &bDirExists);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	if (bDirExists) {

		bFileExists = FALSE;
		ceError = CTCheckFileExists(pszDefaultPathifcfg, &bFileExists);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);

		if (!bFileExists) {

			bDirExists = FALSE;
			ceError =
			    CTCheckDirectoryExists("/etc/sysconfig/network",
						   &bDirExists);
			CLEANUP_ON_CENTERROR_EE(ceError, EE);

			if (bDirExists) {
				ceError =
				    CTGetMatchingFilePathsInFolder
				    ("/etc/sysconfig/network", "ifcfg-eth*",
				     &ppszPaths, &nPaths);
				CLEANUP_ON_CENTERROR_EE(ceError, EE);

				for (iPath = 0; iPath < nPaths; iPath++) {

					if (!strncmp
					    (*(ppszPaths + iPath), pszEthFltr_1,
					     strlen(pszEthFltr_1))
					    || !strncmp(*(ppszPaths + iPath),
							pszEthFltr_2,
							strlen(pszEthFltr_2))
					    || !strncmp(*(ppszPaths + iPath),
							pszEthFltr_3,
							strlen(pszEthFltr_3))) {
						ceError =
						    CTAllocateString(*
								     (ppszPaths
								      + iPath),
								     &pszPathifcfg);
						CLEANUP_ON_CENTERROR_EE(ceError,
									EE);
						break;
					}
				}
			}

			if (IsNullOrEmptyString(pszPathifcfg)) {
				ceError =
				    CENTERROR_DOMAINJOIN_NO_ETH_ITF_CFG_FILE;
				CLEANUP_ON_CENTERROR_EE(ceError, EE);
			}

		} else {

			ceError =
			    CTAllocateString(pszDefaultPathifcfg,
					     &pszPathifcfg);
			CLEANUP_ON_CENTERROR_EE(ceError, EE);

		}

		ceError = DJCheckIfDHCPHost(pszPathifcfg, &bDHCPHost);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);

		if (bDHCPHost) {
			ceError = DJFixDHCPHost(pszPathifcfg, pszComputerName);
			CLEANUP_ON_CENTERROR_EE(ceError, EE);
		}

		if (pProcInfo) {
			FreeProcInfo(pProcInfo);
			pProcInfo = NULL;
		}

		if (ppszArgs) {
			CTFreeStringArray(ppszArgs, nArgs);
			ppszArgs = NULL;
		}
	}

	nArgs = 3;
	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	ceError = CTAllocateString("/bin/hostname", ppszArgs);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	ceError = CTAllocateString(pszComputerName, ppszArgs + 1);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	if (status != 0) {
		ceError = CENTERROR_DOMAINJOIN_HOSTS_EDIT_FAIL;
		CLEANUP_ON_CENTERROR_EE(ceError, EE);
	}
	// Only DHCP boxes need to restart their networks
	if (bDHCPHost) {
		ceError = DJRestartDHCPService(pszComputerName);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);
	}

      cleanup:

	// This ensures that we do not change the SID after a machine name
	// change.  The issue here is that Samba implements its SAM such that
	// a machine name change changes the seeding used for the machine SID.
	// Therefore, we must re-store the old SID with the new machine name
	// seed.
	if (pszMachineSID) {
		if (*pszMachineSID != '\0')
			DJSetMachineSID(pszMachineSID);
		CTFreeString(pszMachineSID);
	}

	if (pProcInfo)
		FreeProcInfo(pProcInfo);

	if (ppszArgs)
		CTFreeStringArray(ppszArgs, nArgs);

	if (ppszPaths)
		CTFreeStringArray(ppszPaths, nPaths);

	if (pszPathifcfg)
		CTFreeString(pszPathifcfg);

	DJ_LOG_VERBOSE("FixNetworkInterfaces LEAVE -> 0x%08x (EE = %d)",
		       ceError, EE);

	return ceError;
}

CENTERROR DJSetComputerName(PSTR pszComputerName, PSTR pszDnsDomainName)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	int EE = 0;
	BOOLEAN bValidComputerName = FALSE;
	PSTR oldShortHostname = NULL;
	PSTR oldFdqnHostname = NULL;
	PSTR ppszHostfilePaths[] = { "/etc/hostname", "/etc/HOSTNAME", NULL };
	struct hostent *pHostent = NULL;
	int i;

	if (geteuid() != 0) {
		ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
		CLEANUP_ON_CENTERROR_EE(ceError, EE);
	}

	ceError = DJIsValidComputerName(pszComputerName, &bValidComputerName);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	if (!bValidComputerName) {
		ceError = CENTERROR_DOMAINJOIN_INVALID_HOSTNAME;
		CLEANUP_ON_CENTERROR_EE(ceError, EE);
	}

	CTStrToLower(pszComputerName);

	/* Start spelunking for various hostname holding things. Rather
	   than trying to worry about what flavor of linux we are
	   running, we look for various files and fix them up if they
	   exist. That way we dont end up with a huge wad of repeated
	   code for each linux flavor.

	   change the repositories of the 'HOSTNAME' variable.
	   it's a string in /etc/HOSTNAME for some dists, it's a variable in
	   /etc/sysconfig/network for others

	   fixup HOSTNAME file if it exists
	   Ubuntu/Debian have /etc/hostname, so add that...
	 */

	ceError = WriteHostnameToFiles(pszComputerName, ppszHostfilePaths);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	// insert/correct the new hostname in /etc/hosts - note that this
	// has to be done *before* we update the running hostname because
	// we call hostname to get the current hostname so that we can
	// find it and replace it.
	ceError = DJGetComputerName(&oldShortHostname);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	//We have the short hostname that the hostname command returns, now we're
	//going to get the long hostname. This is the same as 'hostname -f' on
	//systems which support it.
	//Try to look it up upto 3 times
	for (i = 0; i < 3; i++) {
		pHostent = gethostbyname(oldShortHostname);
		if (pHostent == NULL) {
			if (h_errno == TRY_AGAIN) {
				sleep(1);
				continue;
			}
			break;
		}
		ceError = CTAllocateString(pHostent->h_name, &oldFdqnHostname);
		CLEANUP_ON_CENTERROR_EE(ceError, EE);
		break;
	}

	//Don't replace localhost in /etc/hosts, always add our new hostname instead
	if (oldFdqnHostname != NULL && !strcmp(oldFdqnHostname, "localhost")) {
		CTFreeString(oldFdqnHostname);
		oldFdqnHostname = NULL;
	}
	if (oldShortHostname != NULL && !strcmp(oldShortHostname, "localhost")) {
		CTFreeString(oldShortHostname);
		oldShortHostname = NULL;
	}
	ceError = DJReplaceNameInHostsFile(oldShortHostname, oldFdqnHostname,
					   pszComputerName, pszDnsDomainName);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

#if defined(__LWI_SOLARIS__)
	ceError = WriteHostnameToSunFiles(pszComputerName);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);
#endif

#if defined(_AIX)
	ceError = SetAIXHostname(pszComputerName);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);
#endif

#if defined(_HPUX_SOURCE)
	ceError = SetHPUXHostname(pszComputerName);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);
#endif

#if defined(__LWI_MACOSX__)
	ceError = SetMacOsXHostName(pszComputerName);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);
#endif

	ceError = FixNetworkInterfaces(pszComputerName);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

      cleanup:
	if (oldShortHostname)
		CTFreeString(oldShortHostname);
	if (oldFdqnHostname)
		CTFreeString(oldFdqnHostname);

	DJ_LOG_VERBOSE("DJSetComputerName LEAVE -> 0x%08x (EE = %d)", ceError,
		       EE);
	return ceError;
}

CENTERROR DJIsValidComputerName(PSTR pszComputerName, PBOOLEAN pbIsValid)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	DWORD dwLen = 0;
	PSTR pszTmp = NULL;

	if (geteuid() != 0) {
		ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (IsNullOrEmptyString(pszComputerName))
		return FALSE;

	dwLen = strlen(pszComputerName);

	if (dwLen < 1 || dwLen > 15) {

		*pbIsValid = FALSE;
		goto done;
	}

	ceError = CTAllocateString(pszComputerName, &pszTmp);
	BAIL_ON_CENTERIS_ERROR(ceError);

	CTStrToLower(pszTmp);

	if (!strcmp(pszTmp, "linux") ||
	    !strcmp(pszTmp, "localhost") ||
	    (*pszTmp == '-') || (*(pszTmp + dwLen - 1) == '-')) {
		*pbIsValid = FALSE;
		goto done;
	}

	while (*pszComputerName != '\0') {
		if (!((*pszComputerName == '-') ||
		      (*pszComputerName >= 'a' && *pszComputerName <= 'z') ||
		      (*pszComputerName >= 'A' && *pszComputerName <= 'Z') ||
		      (*pszComputerName >= '0' && *pszComputerName <= '9'))) {
			*pbIsValid = FALSE;
			goto done;
		}
		pszComputerName++;
	}

	*pbIsValid = TRUE;

      done:

	if (pszTmp)
		CTFreeString(pszTmp);

	return ceError;

      error:

	*pbIsValid = FALSE;

	if (pszTmp)
		CTFreeString(pszTmp);

	return ceError;
}

CENTERROR DJIsDomainNameResolvable(PSTR pszDomainName, PBOOLEAN pbIsResolvable)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	struct hostent *pHostent = NULL;
	int i = 0;

	if (geteuid() != 0) {
		ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	*pbIsResolvable = FALSE;

	if (IsNullOrEmptyString(pszDomainName)) {
		ceError = CENTERROR_INVALID_PARAMETER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	for (i = 0; i < 3; i++) {
		pHostent = gethostbyname(pszDomainName);
		if (pHostent == NULL) {
			if (h_errno == TRY_AGAIN) {
				continue;
			} else {
				*pbIsResolvable = FALSE;
				break;
			}
		} else {
			*pbIsResolvable =
			    !IsNullOrEmptyString(pHostent->h_name);
			break;
		}
	}

	return ceError;

      error:

	*pbIsResolvable = FALSE;

	return ceError;
}
