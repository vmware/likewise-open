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

#if !defined(__LWI_MACOSX__)
extern char **environ;
#endif

static
 CENTERROR
BuildJoinEnvironment(PSTR pszPassword, PSTR ** pppszEnv, PDWORD pdwNVars)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR *ppszResult = NULL;
	DWORD nVars = 0;
	DWORD iVar = 0;
	CHAR szBuf[256];
#if !defined(__LWI_MACOSX__)
	PSTR *ppszEnvVarList = environ;
#else
	PSTR *ppszEnvVarList = (*_NSGetEnviron());
#endif
	PSTR *ppszEnv = ppszEnvVarList;

	while (ppszEnv && *ppszEnv) {
		nVars++;
		ppszEnv++;
	}

	nVars += 2;

	ceError =
	    CTAllocateMemory(sizeof(PSTR) * nVars, (PVOID *) & ppszResult);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ppszEnv = ppszEnvVarList;
	iVar = 0;
	while (ppszEnv && *ppszEnv) {

		ceError = CTAllocateString(*ppszEnv, ppszResult + iVar);
		BAIL_ON_CENTERIS_ERROR(ceError);

		iVar++;
		ppszEnv++;
	}

	sprintf(szBuf, "PASSWD=%s", pszPassword);
	ceError = CTAllocateString(szBuf, ppszResult + iVar);
	BAIL_ON_CENTERIS_ERROR(ceError);

	*pppszEnv = ppszResult;

	*pdwNVars = nVars;

	return ceError;

      error:

	if (ppszResult)
		CTFreeStringArray(ppszResult, nVars);

	*pdwNVars = 0;

	return ceError;
}

static
 CENTERROR DJExecLeaveDomain()
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	CHAR szBuf[PATH_MAX + 1];
	PSTR *ppszArgs = NULL;
	PPROCINFO pProcInfo = NULL;
	DWORD nArgs = 5;
	LONG status = 0;

	sprintf(szBuf, "%s/bin/lwinet", PREFIXDIR);

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(szBuf, ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("ads", ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("leave", ppszArgs + 2);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("-P", ppszArgs + 3);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {
		DJ_LOG_ERROR("Failed to leave domain. Exit code: %d", status);
		//ceError = CENTERROR_DOMAINJOIN_FAILED_TO_LEAVE_DOMAIN;
		//BAIL_ON_CENTERIS_ERROR(ceError);
	}
#ifdef __LWI_MACOSX__
	ceError = DJUnconfigureLWIDSPlugin();
	BAIL_ON_CENTERIS_ERROR(ceError);
#endif

      error:

	if (pProcInfo)
		FreeProcInfo(pProcInfo);

	if (ppszArgs)
		CTFreeStringArray(ppszArgs, nArgs);

	return ceError;
}

static CENTERROR DJGetSystemInfo(PSTR * ppszOSName, PSTR * ppszOSVer)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszOSName = NULL;
	PSTR pszOSVer = NULL;
	struct utsname utsbuf;

	memset(&utsbuf, 0, sizeof(struct utsname));

	if (uname(&utsbuf) < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = CTAllocateString(utsbuf.sysname, &pszOSName);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(utsbuf.version, &pszOSVer);
	BAIL_ON_CENTERIS_ERROR(ceError);

	*ppszOSName = pszOSName;
	*ppszOSVer = pszOSVer;

	return ceError;

      error:

	if (pszOSName)
		CTFreeString(pszOSName);

	if (pszOSVer)
		CTFreeString(pszOSVer);

	return ceError;
}

static
    CENTERROR
DJExecDomainJoin(PSTR pszDomainName,
		 PSTR pszUserName,
		 PSTR pszPassword, PSTR pszOU, PSTR * ppszWorkgroupName)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszAdmin = NULL;
	PSTR pszTmp = NULL;
	PSTR *ppszArgs = NULL;
	PSTR *nextArg = NULL;
	DWORD nArgs = 0;
	CHAR szBuf[PATH_MAX + 1];
	PSTR *ppszEnv = NULL;
	DWORD nVars = 0;
	PPROCINFO pProcInfo = NULL;
	PROCBUFFER procBuffer;
	BOOLEAN bTimedout = FALSE;
	LONG status = 0;
	DWORD dwTimeout = 5 * 30;
	PSTR pszBuffer = NULL;
	DWORD iBufIdx = 0;
	DWORD dwBufLen = 0;
	DWORD dwBufAvailable = 0;
	BOOLEAN bFirst = TRUE;
	PSTR pszTerm = NULL;
	PSTR pszWorkgroupName = NULL;
	PSTR pszOSName = NULL;
	PSTR pszOSVer = NULL;

	ceError = DJGetSystemInfo(&pszOSName, &pszOSVer);
	BAIL_ON_CENTERIS_ERROR(ceError);

	DJ_LOG_INFO("OS Name: %s", pszOSName);
	DJ_LOG_INFO("OS Version: %s", pszOSVer);

	/* join the domain and extract (screenscrape!) the name of the
	 *  workgroup that 'net' writes to stdout - this ghastly mess
	 * should work with new and existing installations.  BUGBUG This
	 * is soooo fragile! It's hosed if sambadev changes the string.
	 *
	 * make sure that we do the right thing with both old workgroup
	 * and new style AD domain account names, ie:
	 *
	 * DOMAIN\bob and bob@domain.foof.com
	 *
	 * if we are just passed 'bob', Delay lookup of group sids (domain
	 * admins, centeris admins, enterprise admins) until absolutely
	 * necessary. This is to reduce the need to call winbind
	 * immediately after a domain join (something known to be
	 * risky). Then we use the domain name to make an SPN
	 * bob@DomainToJoin
	 */

	if ((pszTmp = strchr(pszUserName, '@')) == NULL ||
	    (pszTmp = strstr(pszUserName, "\\")) == NULL) {

		ceError =
		    CTAllocateMemory(strlen(pszUserName) +
				     strlen(pszDomainName) + 2,
				     (PVOID *) & pszAdmin);
		BAIL_ON_CENTERIS_ERROR(ceError);

		sprintf(pszAdmin, "%s@%s", pszUserName, pszDomainName);

	} else {

		ceError = CTAllocateString(pszUserName, &pszAdmin);
		BAIL_ON_CENTERIS_ERROR(ceError);

	}

	CTStrToUpper(pszAdmin);

	nArgs = IsNullOrEmptyString(pszOU) ? 8 : 9;

	if (gdjLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE)
		nArgs += 1;

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	nextArg = ppszArgs;

	sprintf(szBuf, "%s/bin/lwinet", PREFIXDIR);
	ceError = CTAllocateString(szBuf, nextArg++);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("ads", nextArg++);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("join", nextArg++);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("-U", nextArg++);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(pszAdmin, nextArg++);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (gdjLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {
		ceError = CTAllocateString("-d10", nextArg++);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	sprintf(szBuf, "osName=%s", pszOSName);
	ceError = CTAllocateString(szBuf, nextArg++);
	BAIL_ON_CENTERIS_ERROR(ceError);

	sprintf(szBuf, "osVer=%s", pszOSVer);
	ceError = CTAllocateString(szBuf, nextArg++);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (!IsNullOrEmptyString(pszOU)) {
		sprintf(szBuf, "createcomputer=%s", pszOU);
		ceError = CTAllocateString(szBuf, nextArg++);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = BuildJoinEnvironment(pszPassword, &ppszEnv, &nVars);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSpawnProcessWithEnvironment(ppszArgs[0],
						ppszArgs,
						ppszEnv, -1, -1, 2, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJTimedReadData(pProcInfo,
				  &procBuffer, dwTimeout, &bTimedout);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (bTimedout) {

		ceError = CENTERROR_DOMAINJOIN_JOIN_TIMEDOUT;
		BAIL_ON_CENTERIS_ERROR(ceError);

		ceError = DJKillProcess(pProcInfo);
		BAIL_ON_CENTERIS_ERROR(ceError);

	}

	do {

		if (!bFirst) {
			ceError = DJReadData(pProcInfo, &procBuffer);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		bFirst = FALSE;

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

	if (!IsNullOrEmptyString(pszBuffer)) {
		DJ_LOG_INFO("%s", pszBuffer);
	}

	if (status != 0) {

		ceError = CENTERROR_DOMAINJOIN_JOIN_FAILED;
		BAIL_ON_CENTERIS_ERROR(ceError);

	}
	// Now do the nasty screenscrape to get the domain shortname aka
	// 'workgroup' name
	pszTmp = (pszBuffer ? strstr(pszBuffer, "--") : NULL);
	if (pszTmp == NULL) {
		ceError = CENTERROR_DOMAINJOIN_JOIN_NO_WKGRP;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	pszTmp += 2;
	while (*pszTmp != '\0' && isspace((int)*pszTmp))
		pszTmp++;
	pszTerm = pszTmp;
	while (*pszTerm != '\0' && !isspace((int)*pszTerm))
		pszTerm++;
	*pszTerm = '\0';

	if (IsNullOrEmptyString(pszTmp)) {
		ceError = CENTERROR_DOMAINJOIN_JOIN_NO_WKGRP;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = CTAllocateString(pszTmp, &pszWorkgroupName);
	BAIL_ON_CENTERIS_ERROR(ceError);

	*ppszWorkgroupName = pszWorkgroupName;
	pszWorkgroupName = NULL;

      error:

	if (pszBuffer)
		CTFreeMemory(pszBuffer);

	if (ppszArgs)
		CTFreeStringArray(ppszArgs, nArgs);

	if (ppszEnv)
		CTFreeStringArray(ppszEnv, nVars);

	if (pszAdmin)
		CTFreeString(pszAdmin);

	if (pProcInfo)
		FreeProcInfo(pProcInfo);

	if (pszWorkgroupName)
		CTFreeString(pszWorkgroupName);

	if (pszOSName)
		CTFreeString(pszOSName);

	if (pszOSVer)
		CTFreeString(pszOSVer);

	return ceError;
}

CENTERROR DJFinishJoin(DWORD dwSleepSeconds, PSTR pszShortDomainName)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszTdbPath = "/var/lib/lwidentity/winbindd_cache.tdb";
	BOOLEAN bFileExists = FALSE;

	ceError = CTCheckFileExists(pszTdbPath, &bFileExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (bFileExists) {
		ceError = CTRemoveFile(pszTdbPath);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = DJManageDaemons(pszShortDomainName, TRUE);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	return ceError;
}

CENTERROR PrepareForJoinOrLeaveDomain(PSTR pszWorkgroupName, BOOLEAN bIsDomain)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszAllUpperWKGRP = NULL;

	//
	// The basic steps are as follows:
	//
	// 1) Prepare to leave the domain.
	// 2) Try to leave the domain.
	// 3) Set up common Samba settings.
	//
	// Note that we will try to leave any configured domain
	// regardless of whether or not we are currently joined
	// to a domain.
	//
	//
	// Turn off samba before trying to leave the domain.  There were
	// problems when certain operations in winbind/smbd raced wrt
	// leaving the domain.  We do not recall the specifics.  However,
	// they likely had to do with race conditions while trying to do domain
	// operations while leaving.
	//

	DJ_LOG_INFO("stopping daemons");

	ceError = DJManageDaemons(NULL, FALSE);
	BAIL_ON_CENTERIS_ERROR(ceError);

	DJ_LOG_INFO("Leaving domain");

	ceError = DJExecLeaveDomain();
	BAIL_ON_CENTERIS_ERROR(ceError);

	DJ_LOG_INFO("Left domain");

	ceError = DJInitSmbConfig();
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (bIsDomain) {

		ceError = SetWorkgroup("WORKGROUP");
		BAIL_ON_CENTERIS_ERROR(ceError);

	} else {

		ceError = CTAllocateString(pszWorkgroupName, &pszAllUpperWKGRP);
		BAIL_ON_CENTERIS_ERROR(ceError);

		CTStrToUpper(pszAllUpperWKGRP);

		ceError = SetWorkgroup(pszAllUpperWKGRP);
		BAIL_ON_CENTERIS_ERROR(ceError);

	}

	ceError = ConfigureSambaEx(NULL, NULL);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	if (pszAllUpperWKGRP)
		CTFreeString(pszAllUpperWKGRP);

	return ceError;
}

static
    CENTERROR
CanonicalizeOrganizationalUnit(PSTR * pszCanonicalizedOrganizationalUnit,
			       PSTR pszOrganizationalUnit, PSTR pszDomainName)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	int EE = 0;
	PSTR comma;
	PSTR current;
	PSTR temp = NULL;
	PSTR result = NULL;
	PSTR dnsDomain = NULL;

	if (!pszOrganizationalUnit || !pszOrganizationalUnit[0]) {
		result = NULL;
		ceError = CENTERROR_SUCCESS;
		GOTO_CLEANUP_EE(EE);
	}

	comma = strchr(pszOrganizationalUnit, ',');
	if (!comma) {
		/* already in canonical "/" format */
		ceError = CTAllocateString(pszOrganizationalUnit, &result);
		GOTO_CLEANUP_EE(EE);
	}

	/* create a temporary buffer in which to party */
	ceError = CTAllocateString(pszOrganizationalUnit, &temp);
	CLEANUP_ON_CENTERROR_EE(ceError, EE);

	CTStripWhitespace(temp);

	current = temp;
	comma = strchr(current, ',');

	while (1) {
		PSTR equalSign;
		PSTR type;
		PSTR component;
		BOOLEAN isDc;
		BOOLEAN isOu;

		if (comma) {
			comma[0] = 0;
		}
		equalSign = strchr(current, '=');
		if (!equalSign) {
			ceError = CENTERROR_DOMAINJOIN_INVALID_OU;
			GOTO_CLEANUP_EE(EE);
		}
		equalSign[0] = 0;

		type = current;
		component = equalSign + 1;

		isDc = !strcasecmp("dc", type);
		isOu = !strcasecmp("ou", type) || !strcasecmp("cn", type);
		if (!isDc && !isOu) {
			ceError = CENTERROR_DOMAINJOIN_INVALID_OU;
			GOTO_CLEANUP_EE(EE);
		}
		if (!isDc) {
			if (dnsDomain) {
				ceError = CENTERROR_DOMAINJOIN_INVALID_OU;
				GOTO_CLEANUP_EE(EE);
			}
			if (result) {
				PSTR newResult;
				ceError =
				    CTAllocateStringPrintf(&newResult, "%s/%s",
							   component, result);
				GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE);
				CT_SAFE_FREE_STRING(result);
				result = newResult;
			} else {
				ceError = CTAllocateString(component, &result);
				GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE);
			}
		} else {
			if (!result) {
				ceError = CENTERROR_DOMAINJOIN_INVALID_OU;
				GOTO_CLEANUP_EE(EE);
			}
			if (dnsDomain) {
				PSTR newDnsDomain;
				ceError =
				    CTAllocateStringPrintf(&newDnsDomain,
							   "%s.%s", dnsDomain,
							   component);
				GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE);
				CT_SAFE_FREE_STRING(dnsDomain);
				dnsDomain = newDnsDomain;
			} else {
				ceError =
				    CTAllocateString(component, &dnsDomain);
				GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE);
			}
		}
		if (!comma) {
			break;
		}
		current = comma + 1;
		comma = strchr(current, ',');
	}

	if (IsNullOrEmptyString(dnsDomain)) {
		ceError = CENTERROR_DOMAINJOIN_INVALID_OU;
		GOTO_CLEANUP_EE(EE);
	}

	if (IsNullOrEmptyString(result)) {
		ceError = CENTERROR_DOMAINJOIN_INVALID_OU;
		GOTO_CLEANUP_EE(EE);
	}

	if (strcasecmp(dnsDomain, pszDomainName)) {
		ceError = CENTERROR_DOMAINJOIN_INVALID_OU;
		GOTO_CLEANUP_EE(EE);
	}

      cleanup:
	if (ceError) {
		CT_SAFE_FREE_STRING(result);
	}

	CT_SAFE_FREE_STRING(dnsDomain);
	CT_SAFE_FREE_STRING(temp);

	*pszCanonicalizedOrganizationalUnit = result;
	if (ceError) {
		DJ_LOG_VERBOSE
		    ("Error in CanonicalizeOrganizationalUnit: 0x%08x, EE = %d",
		     ceError, EE);
	}
	return ceError;
}

CENTERROR
JoinDomain(PSTR pszDomainName,
	   PSTR pszUserName,
	   PSTR pszPassword, PSTR pszOU, BOOLEAN bDoNotChangeHosts)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	CENTERROR ceError2 = CENTERROR_SUCCESS;
	PSTR pszComputerName = NULL;
	PSTR pszWorkgroupName = NULL;
	PSTR pszShortDomainName = NULL;
	PSTR pszOriginalWorkgroupName = NULL;
	BOOLEAN bIsValid = FALSE;
	PSTR pszCanonicalizedOU = NULL;

	if (geteuid() != 0) {
		ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (pszOU) {
		ceError = CanonicalizeOrganizationalUnit(&pszCanonicalizedOU,
							 pszOU, pszDomainName);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = DJGetSambaValue("workgroup", &pszOriginalWorkgroupName);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (IsNullOrEmptyString(pszOriginalWorkgroupName)) {
		ceError =
		    CTAllocateString("WORKGROUP", &pszOriginalWorkgroupName);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	/*
	 * We don't want the caller to try and join with a bogus hostname
	 * like linux.site or a non RFC-compliant name.
	 */
	DJ_LOG_INFO("Getting computer name...");
	ceError = DJGetComputerName(&pszComputerName);
	BAIL_ON_CENTERIS_ERROR(ceError);

	DJ_LOG_INFO("Checking validity of computer name [%s].",
		    IsNullOrEmptyString(pszComputerName) ? "<null|empty>" :
		    pszComputerName);
	ceError = DJIsValidComputerName(pszComputerName, &bIsValid);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (!bIsValid) {

		ceError = CENTERROR_DOMAINJOIN_INVALID_HOSTNAME;
		BAIL_ON_CENTERIS_ERROR(ceError);

	}

	/*
	 * Make sure that the hostname is fully and properly
	 * configured in the OS
	 */
	if (!bDoNotChangeHosts) {
		DJ_LOG_INFO("Setting computer name: name %s, domain %s",
			    pszComputerName, pszDomainName);
		ceError = DJSetComputerName(pszComputerName, pszDomainName);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	DJ_LOG_INFO("Prepare for AD join.");
	ceError = PrepareForJoinOrLeaveDomain(pszDomainName, TRUE);
	BAIL_ON_CENTERIS_ERROR(ceError);

	/*
	 * Setup krb5.conf with the domain as the Kerberos realm.
	 * We do this before doing the join. (We should verify whether
	 * it is necessary to do so before trying to join, however.)
	 */
	ceError = DJModifyKrb5Conf(pszDomainName, pszShortDomainName);
	BAIL_ON_CENTERIS_ERROR(ceError);

	/*
	 * Insert the name of the AD into the realm property.
	 * samba's net command doesnt take an argument for the realm
	 * or workgroup properties, so we have to patch the smb.conf
	 */
	ceError = SetRealm(pszDomainName);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSetSambaValue("security", "ads");
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSetSambaValue("use kerberos keytab", "yes");
	BAIL_ON_CENTERIS_ERROR(ceError);

	DJ_LOG_INFO("Syncing time with DC.");
	ceError = DJSyncTimeToDC(pszDomainName);
	BAIL_ON_CENTERIS_ERROR(ceError);

	DJ_LOG_INFO("Executing domain join.");
	ceError = DJExecDomainJoin(pszDomainName,
				   pszUserName,
				   pszPassword,
				   pszCanonicalizedOU, &pszWorkgroupName);
	BAIL_ON_CENTERIS_ERROR(ceError);

	DJ_LOG_INFO("Set workgroup [%s]",
		    IsNullOrEmptyString(pszWorkgroupName) ? "" :
		    pszWorkgroupName);
	ceError = SetWorkgroup(pszWorkgroupName);
	BAIL_ON_CENTERIS_ERROR(ceError);

	/*
	 * Now that we have joined the domain successfully, we can
	 * point our configuration files to reference winbind and
	 * start the samba daemons and turn them in the boot process.
	 */
	ceError = ConfigureSambaEx(pszDomainName, pszWorkgroupName);
	BAIL_ON_CENTERIS_ERROR(ceError);

	/*
	 * If we get this far, we want to propagate the workgroup name
	 * out to the finally block.
	 */
	ceError = CTAllocateString(pszWorkgroupName, &pszShortDomainName);
	BAIL_ON_CENTERIS_ERROR(ceError);

#ifdef __LWI_MACOSX__
	ceError = DJConfigureLWIDSPlugin();
	BAIL_ON_CENTERIS_ERROR(ceError);
#endif

      error:

	if (!CENTERROR_IS_OK(ceError)) {

		/*
		 * This code can fail, but we want the caller to get the original error
		 * not the one caused by this revert failing. note that if this fails
		 * the user will probably have to repair the smb.conf by hand before
		 * trying to join again
		 */
		DJRevertToOriginalWorkgroup(pszOriginalWorkgroupName);

	}

	/* Always run this */
	ceError2 = DJFinishJoin(IsNullOrEmptyString(pszShortDomainName) ? 0 : 5,
				pszShortDomainName);
	if (!CENTERROR_IS_OK(ceError2)) {

		DJ_LOG_ERROR("Error finishing domain join [0x%.8x]", ceError2);

		if (CENTERROR_IS_OK(ceError)) {
			ceError = ceError2;
		}
	}

	CT_SAFE_FREE_STRING(pszShortDomainName);
	CT_SAFE_FREE_STRING(pszComputerName);
	CT_SAFE_FREE_STRING(pszWorkgroupName);
	CT_SAFE_FREE_STRING(pszOriginalWorkgroupName);
	CT_SAFE_FREE_STRING(pszCanonicalizedOU);

	return ceError;
}

CENTERROR
JoinWorkgroup(PSTR pszWorkgroupName, PSTR pszUserName, PSTR pszPassword)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	if (geteuid() != 0) {
		ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = PrepareForJoinOrLeaveDomain(pszWorkgroupName, FALSE);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	if (!CENTERROR_IS_OK(ceError)) {
		DJFinishJoin(5, NULL);
	}

	return ceError;
}

CENTERROR DJSetConfiguredDescription(PSTR pszDescription)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	if (geteuid() != 0) {
		ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}
	// TODO: Should we check winbind status?
	ceError = SetDescription(pszDescription);

      error:

	return ceError;
}

CENTERROR DJGetConfiguredDescription(PSTR * ppszDescription)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszDescription = NULL;

	if (geteuid() != 0) {
		ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = DJGetSambaValue("server string", &pszDescription);
	if (ceError == CENTERROR_DOMAINJOIN_SMB_VALUE_NOT_FOUND) {
		ceError = CENTERROR_DOMAINJOIN_DESCRIPTION_NOT_FOUND;
		*ppszDescription = NULL;
		goto error;
	} else {

		BAIL_ON_CENTERIS_ERROR(ceError);

	}

	*ppszDescription = pszDescription;

	return ceError;

      error:

	if (pszDescription)
		CTFreeString(pszDescription);

	return ceError;
}

CENTERROR DJGetConfiguredDomain(PSTR * ppszDomain)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszDomain = NULL;

	if (geteuid() != 0) {
		ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = DJGetSambaValue("realm", &pszDomain);
	if (ceError == CENTERROR_DOMAINJOIN_SMB_VALUE_NOT_FOUND) {

		ceError = CENTERROR_DOMAINJOIN_DOMAIN_NOT_FOUND;
		BAIL_ON_CENTERIS_ERROR(ceError);

	} else {

		BAIL_ON_CENTERIS_ERROR(ceError);

	}

	*ppszDomain = pszDomain;

      error:

	return ceError;
}

CENTERROR DJGetConfiguredWorkgroup(PSTR * ppszWorkgroup)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszWorkgroup = NULL;

	if (geteuid() != 0) {
		ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = DJGetSambaValue("workgroup", &pszWorkgroup);
	if (ceError == CENTERROR_DOMAINJOIN_SMB_VALUE_NOT_FOUND) {

		ceError = CENTERROR_DOMAINJOIN_WORKGROUP_NOT_FOUND;
		BAIL_ON_CENTERIS_ERROR(ceError);

	} else {

		BAIL_ON_CENTERIS_ERROR(ceError);

	}

	*ppszWorkgroup = pszWorkgroup;

      error:

	return ceError;
}
