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

/* ex: set tabstop=4 expandtab shiftwidth=4: */
#include "domainjoin.h"
#include "djfirewall.h"
#include "djdistroinfo.h"
#include "djauthconf.h"
#include "djauthinfo.h"
#include "djkrb5conf.h"
#include "ctshell.h"
#include "ctstrutils.h"
#include <glob.h>

#if !defined(__LWI_MACOSX__)
extern char** environ;
#endif

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

static
CENTERROR
BuildJoinEnvironment(
    PCSTR krb5ConfPath,
    PSTR pszPassword,
    PSTR** pppszEnv,
    PDWORD pdwNVars
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR* ppszResult = NULL;
    DWORD nVars = 0;
    DWORD iVar = 0;
    CHAR  szBuf[256];
#if !defined(__LWI_MACOSX__)
    PSTR* ppszEnvVarList = environ;
#else
    PSTR* ppszEnvVarList = (*_NSGetEnviron());
#endif
    PSTR* ppszEnv = ppszEnvVarList;

    while (ppszEnv && *ppszEnv) {
        nVars++;
        ppszEnv++;
    }

    if(krb5ConfPath != NULL)
        nVars++;
    if(pszPassword != NULL)
        nVars++;

    //One more for the null termination
    nVars++;

    ceError = CTAllocateMemory(sizeof(PSTR) * nVars, (PVOID*)&ppszResult);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ppszEnv = ppszEnvVarList;
    iVar = 0;
    while (ppszEnv && *ppszEnv) {

        ceError = CTAllocateString(*ppszEnv, ppszResult+iVar);
        BAIL_ON_CENTERIS_ERROR(ceError);

        iVar++;
        ppszEnv++;
    }

    if(pszPassword != NULL)
    {
        sprintf(szBuf, "PASSWD=%s", pszPassword);
        ceError = CTAllocateString(szBuf, ppszResult + iVar++);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if(krb5ConfPath != NULL)
    {
        sprintf(szBuf, "KRB5_CONFIG=%s", krb5ConfPath);
        ceError = CTAllocateString(szBuf, ppszResult + iVar++);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *pppszEnv = ppszResult;

    if(pdwNVars != NULL)
        *pdwNVars = nVars;

    return ceError;

error:

    if (ppszResult)
        CTFreeStringArray(ppszResult, nVars);

    *pdwNVars = 0;

    return ceError;
}

static
CENTERROR
DJExecLeaveDomain()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];
    PSTR* ppszArgs = NULL;
    PPROCINFO pProcInfo = NULL;
    DWORD nArgs = 5;
    LONG status = 0;

    sprintf(szBuf, "%s/bin/lwinet", PREFIXDIR);

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(szBuf, ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("ads", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("leave", ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("-P", ppszArgs+3);
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

static
CENTERROR
DJExecDomainJoin(
        PCSTR rootPrefix,
    PSTR pszDomainName,
    PSTR pszUserName,
    PSTR pszPassword,
    PSTR pszOU,
    PSTR* ppszWorkgroupName)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszAdmin = NULL;
    PSTR pszTmp = NULL;
    PSTR* ppszArgs = NULL;
    PSTR* nextArg = NULL;
    DWORD nArgs = 0;
    CHAR  szBuf[PATH_MAX+1];
    PSTR* ppszEnv = NULL;
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
    DistroInfo distro;
    PSTR pszOSName = NULL;
    PSTR lwiauthdConfOption = NULL;
    PSTR krb5ConfPath = NULL;

    if(rootPrefix == NULL)
        rootPrefix = "";

    memset(&distro, 0, sizeof(distro));
    ceError = DJGetDistroInfo(NULL, &distro);
    BAIL_ON_CENTERIS_ERROR(ceError);
    ceError = DJGetDistroString(distro.distro, &pszOSName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    DJ_LOG_INFO("OS Name: %s", pszOSName);
    DJ_LOG_INFO("OS Version: %s", distro.version);

    ceError = CTAllocateStringPrintf(&lwiauthdConfOption, "--configfile=%s/etc/samba/lwiauthd.conf", rootPrefix);
    BAIL_ON_CENTERIS_ERROR(ceError);
    ceError = CTAllocateStringPrintf(&krb5ConfPath, "%s/etc/krb5.conf", rootPrefix);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Join the domain and extract (screenscrape) the name of the
     * workgroup that the join utility writes to stdout.  This is
     * fragile and should be changed in the future.
     */

    /* Canonicalize the username to a UPN (user@REALM). */
    if ((pszTmp = strchr(pszUserName, '@')) == NULL) {

        ceError = CTAllocateMemory(strlen(pszUserName)+strlen(pszDomainName)+2,
                                   (PVOID*)&pszAdmin);
        BAIL_ON_CENTERIS_ERROR(ceError);

        sprintf(pszAdmin, "%s@%s", pszUserName, pszDomainName);

    } else {

        ceError = CTAllocateString(pszUserName, &pszAdmin);
        BAIL_ON_CENTERIS_ERROR(ceError);

    }

    CTStrToUpper(strchr(pszAdmin, '@'));

    nArgs = 9;
    if(!IsNullOrEmptyString(pszOU))
        nArgs++;

    if (gdjLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE)
        nArgs++;

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    nextArg = ppszArgs;

    sprintf(szBuf, "%s/bin/lwinet", PREFIXDIR);
    ceError = CTAllocateString(szBuf, nextArg++);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(lwiauthdConfOption, nextArg++);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("ads", nextArg++);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("join", nextArg++);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("-U", nextArg++);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(pszAdmin, nextArg++);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if (gdjLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE)
    {
        ceError = CTAllocateString("-d10", nextArg++);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    sprintf(szBuf, "osName=%s", pszOSName);
    ceError = CTAllocateString(szBuf, nextArg++);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(szBuf, "osVer=%s", distro.version);
    ceError = CTAllocateString(szBuf, nextArg++);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if (!IsNullOrEmptyString(pszOU)) {
       sprintf(szBuf, "createcomputer=%s", pszOU);
       ceError = CTAllocateString(szBuf, nextArg++);
       BAIL_ON_CENTERIS_ERROR(ceError);
    }


    ceError = BuildJoinEnvironment(krb5ConfPath, pszPassword, &ppszEnv, &nVars);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcessWithEnvironment(ppszArgs[0],
                                            ppszArgs,
                                            ppszEnv,
                                            -1,
                                            -1,
                                            2,
                                            &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJTimedReadData(pProcInfo,
                              &procBuffer,
                              dwTimeout,
                              &bTimedout);
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

                    memcpy(pszBuffer+iBufIdx,
                           procBuffer.szOutBuf,
                           procBuffer.dwOutBytesRead
                        );

                    iBufIdx+= procBuffer.dwOutBytesRead;
                    dwBufAvailable -= procBuffer.dwOutBytesRead;

                    *(pszBuffer+iBufIdx+1) = '\0';

                    break;

                } else {

                    /*
                     * TODO: Limit the amount of memory acquired
                     */

                    ceError = CTReallocMemory(pszBuffer,
                                              (PVOID*)&pszBuffer,
                                              dwBufLen+1024);
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

    pszTmp+=2;
    while (*pszTmp != '\0' && isspace((int) *pszTmp))
        pszTmp++;
    pszTerm = pszTmp;
    while (*pszTerm != '\0' && !isspace((int) *pszTerm))
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
    DJFreeDistroInfo(&distro);
    CT_SAFE_FREE_STRING(lwiauthdConfOption);
    CT_SAFE_FREE_STRING(krb5ConfPath);

    return ceError;
}

CENTERROR
DJRemoveCacheFiles()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pattern = "/var/lib/lwidentity/*_cache.tdb";
    BOOLEAN bFileExists = FALSE;
    glob_t matches = {.gl_pathc = 0, .gl_pathv = NULL};
    int result;
    int i;
    char *file;

    result = glob(pattern, 0, NULL, &matches);

    if (result != 0 && result != GLOB_NOMATCH)
    {
	switch (result)
	{
	case GLOB_NOSPACE:
	    BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_OUT_OF_MEMORY);
	case GLOB_ABORTED:
	    BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_ACCESS_DENIED);
	default:
	    BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_FILE_NOT_FOUND);
	}
    }

    for (i = 0; i < matches.gl_pathc; i++)
    {
	file = matches.gl_pathv[i];

	ceError = CTCheckFileExists(file, &bFileExists);
	BAIL_ON_CENTERIS_ERROR(ceError);
	
	if (bFileExists) 
	{
	    DJ_LOG_VERBOSE("Removing cache file %s", file);
	    ceError = CTRemoveFile(file);
	    BAIL_ON_CENTERIS_ERROR(ceError);
	}	
    }

error:
    if (matches.gl_pathv)
	globfree(&matches);
    return ceError;
}

CENTERROR
DJFinishJoin(
    DWORD dwSleepSeconds,
    PSTR pszShortDomainName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    LWException *inner = NULL;

    ceError = DJRemoveCacheFiles();
    BAIL_ON_CENTERIS_ERROR(ceError);

    DJManageDaemons(pszShortDomainName, TRUE, &inner);
    if(!LW_IS_OK(inner))
    {
        ceError = inner->code;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    LW_HANDLE(&inner);

    return ceError;
}

CENTERROR
PrepareForJoinOrLeaveDomain(
    PSTR    pszWorkgroupName,
    BOOLEAN bIsDomain
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszAllUpperWKGRP = NULL;
    LWException *inner = NULL;

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

    DJManageDaemons(NULL, FALSE, &inner);
    if(!LW_IS_OK(inner))
    {
        ceError = inner->code;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    DJ_LOG_INFO("Leaving domain");

    ceError = DJExecLeaveDomain();
    BAIL_ON_CENTERIS_ERROR(ceError);

    DJ_LOG_INFO("Left domain");

    ceError = DJInitSmbConfig(NULL);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bIsDomain) {

        ceError = SetWorkgroup(NULL, "WORKGROUP");
        BAIL_ON_CENTERIS_ERROR(ceError);

    } else {

        ceError = CTAllocateString(pszWorkgroupName, &pszAllUpperWKGRP);
        BAIL_ON_CENTERIS_ERROR(ceError);

        CTStrToUpper(pszAllUpperWKGRP);

        ceError = SetWorkgroup(NULL, pszAllUpperWKGRP);
        BAIL_ON_CENTERIS_ERROR(ceError);

    }

    ceError = ConfigureSambaEx(NULL, NULL);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    LW_HANDLE(&inner);
    if (pszAllUpperWKGRP)
        CTFreeString(pszAllUpperWKGRP);

    return ceError;
}

static
CENTERROR
CanonicalizeOrganizationalUnit(
    PSTR* pszCanonicalizedOrganizationalUnit,
    PSTR pszOrganizationalUnit,
    PSTR pszDomainName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int EE = 0;
    PSTR comma;
    PSTR current;
    PSTR temp = NULL;
    PSTR result = NULL;
    PSTR dnsDomain = NULL;

    if (!pszOrganizationalUnit || !pszOrganizationalUnit[0])
    {
        result = NULL;
        ceError = CENTERROR_SUCCESS;
        GOTO_CLEANUP_EE(EE);
    }

    comma = strchr(pszOrganizationalUnit, ',');
    if (!comma)
    {
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

    while (1)
    {
        PSTR equalSign;
        PSTR type;
        PSTR component;
        BOOLEAN isDc;
        BOOLEAN isOu;

        if (comma)
        {
            comma[0] = 0;
        }
        equalSign = strchr(current, '=');
        if (!equalSign)
        {
            ceError = CENTERROR_DOMAINJOIN_INVALID_OU;
            GOTO_CLEANUP_EE(EE);
        }
        equalSign[0] = 0;

        type = current;
        component = equalSign + 1;

        isDc = !strcasecmp("dc", type);
        isOu = !strcasecmp("ou", type) || !strcasecmp("cn", type);
        if (!isDc && !isOu)
        {
            ceError = CENTERROR_DOMAINJOIN_INVALID_OU;
            GOTO_CLEANUP_EE(EE);
        }
        if (!isDc)
        {
            if (dnsDomain)
            {
                ceError = CENTERROR_DOMAINJOIN_INVALID_OU;
                GOTO_CLEANUP_EE(EE);
            }
            if (result)
            {
                PSTR newResult;
                ceError = CTAllocateStringPrintf(&newResult, "%s/%s", component, result);
                GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE);
                CT_SAFE_FREE_STRING(result);
                result = newResult;
            }
            else
            {
                ceError = CTAllocateString(component, &result);
                GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE);
            }
        }
        else
        {
            if (!result)
            {
                ceError = CENTERROR_DOMAINJOIN_INVALID_OU;
                GOTO_CLEANUP_EE(EE);
            }
            if (dnsDomain)
            {
                PSTR newDnsDomain;
                ceError = CTAllocateStringPrintf(&newDnsDomain, "%s.%s", dnsDomain, component);
                GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE);
                CT_SAFE_FREE_STRING(dnsDomain);
                dnsDomain = newDnsDomain;
            }
            else
            {
                ceError = CTAllocateString(component, &dnsDomain);
                GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE);
            }
        }
        if (!comma)
        {
            break;
        }
        current = comma + 1;
        comma = strchr(current, ',');
    }

    if (IsNullOrEmptyString(dnsDomain))
    {
        ceError = CENTERROR_DOMAINJOIN_INVALID_OU;
        GOTO_CLEANUP_EE(EE);
    }

    if (IsNullOrEmptyString(result))
    {
        ceError = CENTERROR_DOMAINJOIN_INVALID_OU;
        GOTO_CLEANUP_EE(EE);
    }

    if (strcasecmp(dnsDomain, pszDomainName))
    {
        ceError = CENTERROR_DOMAINJOIN_INVALID_OU;
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (ceError)
    {
        CT_SAFE_FREE_STRING(result);
    }

    CT_SAFE_FREE_STRING(dnsDomain);
    CT_SAFE_FREE_STRING(temp);

    *pszCanonicalizedOrganizationalUnit = result;
    if (ceError)
    {
        DJ_LOG_VERBOSE("Error in CanonicalizeOrganizationalUnit: 0x%08x, EE = %d", ceError, EE);
    }
    return ceError;
}

static QueryResult QueryDoJoin(const JoinProcessOptions *options, LWException **exc)
{
    const ModuleState *state = DJGetModuleStateByName((JoinProcessOptions *)options, "join");

    if(!options->joiningDomain)
        return NotApplicable;

    if(((options->username != NULL) && (strchr(options->username, '\\') != NULL)))
    {
        LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_INVALID_USERID, "Invalid username", "The username '%s' is invalid because it contains a backslash. Please use UPN syntax (user@domain.com) if you wish to use a username from a different domain.", options->username);
        return CannotConfigure;
    }

    //This module sets its moduleData after it is finished making changes. By
    //reading it we can tell if this module has already been run.
    if(state != NULL && state->moduleData == (void *)1)
    {
        return FullyConfigured;
    }
    return NotConfigured;
}

static void DoJoin(JoinProcessOptions *options, LWException **exc)
{
    PSTR pszCanonicalizedOU = NULL;
    PSTR tempDir = NULL;
    PSTR lwiauthdPath = NULL;
    PSTR krb5Path = NULL;
    ModuleState *state = DJGetModuleStateByName(options, "join");

    if (options->ouName)
    {
        LW_CLEANUP_CTERR(exc, CanonicalizeOrganizationalUnit(&pszCanonicalizedOU,
                                                 options->ouName,
                                                 options->domainName));
    }

    LW_CLEANUP_CTERR(exc, CTCreateTempDirectory(&tempDir));
    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&lwiauthdPath, "%s/etc/samba", tempDir));
    LW_CLEANUP_CTERR(exc, CTCreateDirectory(lwiauthdPath, 0700));
    CT_SAFE_FREE_STRING(lwiauthdPath);
    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&lwiauthdPath, "%s/etc/samba/lwiauthd.conf", tempDir));
    LW_CLEANUP_CTERR(exc, CTCopyFileWithOriginalPerms("/etc/samba/lwiauthd.conf", lwiauthdPath));
    LW_CLEANUP_CTERR(exc, DJCopyKrb5ToRootDir(NULL, tempDir));

    LW_CLEANUP_CTERR(exc, DJInitSmbConfig(tempDir));
    LW_CLEANUP_CTERR(exc, SetWorkgroup(tempDir, "WORKGROUP"));

    /*
     * Setup krb5.conf with the domain as the Kerberos realm.
     * We do this before doing the join. (We should verify whether
     * it is necessary to do so before trying to join, however.)
     */
    LW_CLEANUP_CTERR(exc, DJModifyKrb5Conf(tempDir,
		    TRUE,
		    options->domainName, NULL, NULL));


    /*
     * Insert the name of the AD into the realm property.
     * samba's net command doesnt take an argument for the realm
     * or workgroup properties, so we have to patch the smb.conf
     */
    LW_CLEANUP_CTERR(exc, SetRealm(tempDir, options->domainName));
    LW_CLEANUP_CTERR(exc, DJSetSambaValue(tempDir, "security", "ads"));
    LW_CLEANUP_CTERR(exc, DJSetSambaValue(tempDir, "use kerberos keytab", "yes"));

    DJ_LOG_INFO("Executing domain join.");
    CT_SAFE_FREE_STRING(options->shortDomainName);
    LW_CLEANUP_CTERR(exc, DJExecDomainJoin(tempDir, options->domainName,
                               options->username,
                               options->password,
                               pszCanonicalizedOU,
                               &options->shortDomainName));

    //Indicate that the join was successful incase QueryDoJoin is called later
    state->moduleData = (void *)1;

cleanup:
    CT_SAFE_FREE_STRING(pszCanonicalizedOU);
    if(tempDir != NULL)
    {
        CTRemoveDirectory(tempDir);
        CT_SAFE_FREE_STRING(tempDir);
    }
    CT_SAFE_FREE_STRING(lwiauthdPath);
    CT_SAFE_FREE_STRING(krb5Path);
}

static PSTR GetJoinDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    LW_CLEANUP_CTERR(exc, CTStrdup(
"Sychronize time to DC.\n"
"Create computer account in AD.\n"
"Store machine account password on local machine for authentication daemon.\n"
"Discover pre-windows 2000 domain name.\n"
                , &ret));
cleanup:
    return ret;
}

const JoinModule DJDoJoinModule = { TRUE, "join", "join computer to AD", QueryDoJoin, DoJoin, GetJoinDescription };

static QueryResult QueryLeave(const JoinProcessOptions *options, LWException **exc)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    QueryResult result = SufficientlyConfigured;
    ModuleState *state = DJGetModuleStateByName((JoinProcessOptions *)options, "leave");

    if(options->joiningDomain)
    {
        result = NotApplicable;
        goto cleanup;
    }

    if(state->moduleData == (void *)2)
    {
        //This means a leave was attempted and it failed.
        result = NotConfigured;
    }

    if(((options->username != NULL) && (strchr(options->username, '\\') != NULL)))
    {
        LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_INVALID_USERID, "Invalid username", "The username '%s' is invalid because it contains a backslash. Please use UPN syntax (user@domain.com) if you wish to use a username from a different domain.", options->username);
        return CannotConfigure;
    }

    ceError = CTRunCommand("/usr/centeris/bin/lwinet ads testjoin >/dev/null 2>&1");
    if(ceError == CENTERROR_COMMAND_FAILED)
    {
        result = FullyConfigured;
        ceError = CENTERROR_SUCCESS;
        goto cleanup;
    }
    LW_CLEANUP_CTERR(exc, ceError);

    //If this is the first time that this module is being queried, and the
    //username is null, turn off this module. The user can override this later,
    //in which case an error will be thrown during the actual leave.
    if(options->username == NULL)
    {
        if(state->moduleData == NULL)
        {
            state->moduleData = (void *)1;
            state->runModule = FALSE;
        }
        else
        {
            result = CannotConfigure;
        }
    }

cleanup:
    return result;
}

static void DoLeave(JoinProcessOptions *options, LWException **exc)
{
    PSTR args[] = {
        PREFIXDIR "/bin/lwinet",
        "ads",
        "leave",
        "-P",
        NULL,
        NULL
    };
    int i;
    PSTR* ppszEnv = NULL;
    int fds[3] = {-1, -1, STDERR_FILENO};
    PPROCINFO pProcInfo = NULL;
    LONG status;

    if(options->username != NULL)
    {
        args[3] = "-U";
        args[4] = options->username;
    }

    fds[0] = open("/dev/zero", O_RDONLY);
    if(fds[0] < 0)
    {
        LW_CLEANUP_CTERR(exc, CTMapSystemError(errno));
    }
    fds[1] = open("/dev/zero", O_WRONLY);
    if(fds[1] < 0)
    {
        LW_CLEANUP_CTERR(exc, CTMapSystemError(errno));
    }

    LW_CLEANUP_CTERR(exc, BuildJoinEnvironment(NULL, options->password, &ppszEnv, NULL));

    LW_CLEANUP_CTERR(exc, CTSpawnProcessWithEnvironment(args[0], args,
                ppszEnv, fds[0], fds[1], fds[2], &pProcInfo));

    LW_CLEANUP_CTERR(exc, CTGetExitStatus(pProcInfo, &status));

    if(status != 0)
    {
        ModuleState *state = DJGetModuleStateByName((JoinProcessOptions *)options, "leave");

        state->moduleData = (void *)2;
        if(IsNullOrEmptyString(options->password))
        {
            LW_RAISE_EX(exc, CENTERROR_INVALID_PASSWORD, "Unable to delete computer account", "The computer account does not have sufficient permissions to remove itself. Please either provide an administrator's username and password, or the username and password of the account originally used to join the computer to AD.");
            goto cleanup;
        }
        else
        {
            LW_RAISE_EX(exc, CENTERROR_COMMAND_FAILED, "Unable to delete computer account", "Removing the computer account failed. Review the above output for more information.");
            goto cleanup;
        }
    }

cleanup:
    if (ppszEnv)
        CTFreeNullTerminatedStringArray(ppszEnv);
    for(i = 0; i < 3; i++)
    {
        if(fds[i] != -1)
            close(fds[i]);
    }
    CTFreeProcInfo(pProcInfo);
}

static PSTR GetLeaveDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    LW_CLEANUP_CTERR(exc, CTStrdup("Remove the computer account from AD", &ret));

cleanup:
    return ret;
}

const JoinModule DJDoLeaveModule = { TRUE, "leave", "delete machine account", QueryLeave, DoLeave, GetLeaveDescription };

static QueryResult QueryLwiConf(const JoinProcessOptions *options, LWException **exc)
{
    QueryResult result = NotConfigured;
    PSTR readValue = NULL;
    PSTR upperDomain = NULL;
    CENTERROR ceError;

    if(!options->joiningDomain)
    {
        LW_CLEANUP_CTERR(exc, DJGetSambaValue("workgroup", &readValue));
        if(strcmp(readValue, "WORKGROUP"))
            goto cleanup;
        CT_SAFE_FREE_STRING(readValue);

        ceError = DJGetSambaValue("realm", &readValue);
        if(ceError == CENTERROR_SUCCESS)
            goto cleanup;
        else if(ceError == CENTERROR_DOMAINJOIN_SMB_VALUE_NOT_FOUND)
            ceError = CENTERROR_SUCCESS;
        LW_CLEANUP_CTERR(exc, ceError);
        CT_SAFE_FREE_STRING(readValue);

        LW_CLEANUP_CTERR(exc, DJGetSambaValue("security", &readValue));
        if(strcmp(readValue, "user"))
            goto cleanup;
        CT_SAFE_FREE_STRING(readValue);

        result = FullyConfigured;
        goto cleanup;
    }

    if(options->shortDomainName == NULL)
        goto cleanup;

    LW_CLEANUP_CTERR(exc, DJGetSambaValue("workgroup", &readValue));
    if(strcmp(readValue, options->shortDomainName))
        goto cleanup;
    CT_SAFE_FREE_STRING(readValue);

    LW_CLEANUP_CTERR(exc, CTStrdup(options->domainName, &upperDomain));
    CTStrToUpper(upperDomain);
    LW_CLEANUP_CTERR(exc, DJGetSambaValue("realm", &readValue));
    if(strcmp(readValue, upperDomain))
        goto cleanup;
    CT_SAFE_FREE_STRING(readValue);

    LW_CLEANUP_CTERR(exc, DJGetSambaValue("security", &readValue));
    if(strcmp(readValue, "ads"))
        goto cleanup;
    CT_SAFE_FREE_STRING(readValue);

    LW_CLEANUP_CTERR(exc, DJGetSambaValue("use kerberos keytab", &readValue));
    if(strcmp(readValue, "yes"))
        goto cleanup;
    CT_SAFE_FREE_STRING(readValue);

    result = FullyConfigured;

cleanup:
    CT_SAFE_FREE_STRING(readValue);
    CT_SAFE_FREE_STRING(upperDomain);
    return result;
}

static void DoLwiConf(JoinProcessOptions *options, LWException **exc)
{
    LW_CLEANUP_CTERR(exc, DJInitSmbConfig(NULL));
    if(options->joiningDomain)
    {
        LW_CLEANUP_CTERR(exc, SetRealm(NULL, options->domainName));
        LW_CLEANUP_CTERR(exc, SetWorkgroup(NULL, options->shortDomainName));
        LW_CLEANUP_CTERR(exc, DJSetSambaValue(NULL, "security", "ads"));
        LW_CLEANUP_CTERR(exc, DJSetSambaValue(NULL, "use kerberos keytab", "yes"));
    }
    else
    {
        LW_CLEANUP_CTERR(exc, SetWorkgroup(NULL, "WORKGROUP"));
    }

cleanup:
    ;
}

static PSTR GetLwiConfDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    if(options->joiningDomain)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup(
"Edit /etc/samba/lwiauthd.conf to set the following values:\n"
"workgroup=<short domain name>\n"
"realm=<dns domain name>\n"
"security=ads\n"
"use kerberos keytab=ads\n"
                , &ret));
    }
    else
    {
        LW_CLEANUP_CTERR(exc, CTStrdup(
"Delete the realm name value pair in /etc/samba/lwiauthd.conf, and set the following values:\n"
"workgroup=WORKGROUP\n"
"security=user\n"
                , &ret));
    }
cleanup:
    return ret;
}

const JoinModule DJLwiConfModule = { TRUE, "lwiconf", "configure lwiauthd.conf", QueryLwiConf, DoLwiConf, GetLwiConfDescription };

CENTERROR
JoinDomain(
    PSTR pszDomainName,
    PSTR pszUserName,
    PSTR pszPassword,
    PSTR pszOU,
    BOOLEAN bDoNotChangeHosts
    )
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

    if (pszOU)
    {
        ceError = CanonicalizeOrganizationalUnit(&pszCanonicalizedOU,
                                                 pszOU,
                                                 pszDomainName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = DJGetSambaValue("workgroup", &pszOriginalWorkgroupName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (IsNullOrEmptyString(pszOriginalWorkgroupName)) {
        ceError = CTAllocateString("WORKGROUP", &pszOriginalWorkgroupName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /*
     * We don't want the caller to try and join with a bogus hostname
     * like linux.site or a non RFC-compliant name.
     */
    DJ_LOG_INFO("Getting computer name...");
    ceError = DJGetComputerName(&pszComputerName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    DJ_LOG_INFO("Checking validity of computer name [%s].", IsNullOrEmptyString(pszComputerName) ? "<null|empty>" : pszComputerName);
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
    if (!bDoNotChangeHosts)
    {
        DJ_LOG_INFO("Setting computer name: name %s, domain %s", pszComputerName, pszDomainName);
        ceError = DJSetComputerName(pszComputerName, pszDomainName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    DJ_LOG_INFO("Prepare for AD join.");
    ceError = PrepareForJoinOrLeaveDomain(pszDomainName, TRUE);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Open necessary firewall ports on VMWare ESX */
    ceError = DJConfigureFirewallForAuth("",
            !IsNullOrEmptyString(pszDomainName));
    BAIL_ON_CENTERIS_ERROR(ceError);

    /*
     * Setup krb5.conf with the domain as the Kerberos realm.
     * We do this before doing the join. (We should verify whether
     * it is necessary to do so before trying to join, however.)
     */
    ceError = DJModifyKrb5Conf("",
		    !IsNullOrEmptyString(pszDomainName),
		    pszDomainName, pszShortDomainName, NULL);
    BAIL_ON_CENTERIS_ERROR(ceError);


    /*
     * Insert the name of the AD into the realm property.
     * samba's net command doesnt take an argument for the realm
     * or workgroup properties, so we have to patch the smb.conf
     */
    ceError = SetRealm(NULL, pszDomainName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSetSambaValue(NULL, "security", "ads");
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSetSambaValue(NULL, "use kerberos keytab", "yes");
    BAIL_ON_CENTERIS_ERROR(ceError);

    DJ_LOG_INFO("Syncing time with DC.");
    ceError = DJSyncTimeToDC(pszDomainName, 60);
    BAIL_ON_CENTERIS_ERROR(ceError);

    DJ_LOG_INFO("Executing domain join.");
    ceError = DJExecDomainJoin(NULL, pszDomainName,
                               pszUserName,
                               pszPassword,
                               pszCanonicalizedOU,
                               &pszWorkgroupName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    DJ_LOG_INFO("Set workgroup [%s]", IsNullOrEmptyString(pszWorkgroupName) ? "" : pszWorkgroupName);
    ceError = SetWorkgroup(NULL, pszWorkgroupName);
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
JoinWorkgroup(
    PSTR pszWorkgroupName,
    PSTR pszUserName,
    PSTR pszPassword
    )
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

CENTERROR
DJSetConfiguredDescription(
    PSTR pszDescription
    )
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


CENTERROR
DJGetConfiguredDescription(
    PSTR* ppszDescription
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDescription = NULL;

    if (geteuid() != 0) {
       ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = DJGetSambaValue("server string", &pszDescription);
    if (ceError == CENTERROR_DOMAINJOIN_SMB_VALUE_NOT_FOUND)
    {
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


CENTERROR
DJGetConfiguredDomain(
    PSTR* ppszDomain
    )
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

CENTERROR
DJGetConfiguredWorkgroup(
    PSTR* ppszWorkgroup
    )
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

CENTERROR
DJGetDomainDC(PCSTR domain, PSTR *dc)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR sedPath = NULL;

    *dc = NULL;
    GCE(ceError = CTFindSed(&sedPath));
    ceError = CTShell("%prefix/bin/lwinet lookup dsgetdcname %domain 2>/dev/null | %sedPath -n %sedExpression >%dc",
            CTSHELL_STRING(prefix, PREFIXDIR),
            CTSHELL_STRING(domain, domain),
            CTSHELL_STRING(sedPath, sedPath),
            CTSHELL_STRING(sedExpression, "s/^domain_controller_name:[ \t]*\\(.*\\)$/\\1/p"),
            CTSHELL_BUFFER(dc, dc));
    GCE(ceError);
    CTStripWhitespace(*dc);
    if(*dc == NULL)
    {
        CT_SAFE_FREE_STRING(*dc);
        GCE(ceError = CENTERROR_COMMAND_FAILED);
    }

cleanup:
    CT_SAFE_FREE_STRING(sedPath);
    return ceError;
}

CENTERROR
DJGetComputerDN(PSTR *dn)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR sedPath = NULL;

    *dn = NULL;
    GCE(ceError = CTFindSed(&sedPath));
    ceError = CTShell("%prefix/bin/lwinet ads status -P 2>/dev/null | %sedPath -n %sedExpression >%dn",
            CTSHELL_STRING(prefix, PREFIXDIR),
            CTSHELL_STRING(sedPath, sedPath),
            CTSHELL_STRING(sedExpression, "s/^distinguishedName:[ \t]*\\(.*\\)$/\\1/p"),
            CTSHELL_BUFFER(dn, dn));
    GCE(ceError);
    CTStripWhitespace(*dn);
    if(*dn == NULL)
    {
        CT_SAFE_FREE_STRING(*dn);
        GCE(ceError = CENTERROR_COMMAND_FAILED);
    }

cleanup:
    CT_SAFE_FREE_STRING(sedPath);
    return ceError;
}

CENTERROR
DJGuessShortDomainName(PCSTR longName, PSTR *shortName)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR sedPath = NULL;
    PSTR dc = NULL;

    *shortName = NULL;
    GCE(ceError = CTFindSed(&sedPath));
    GCE(ceError = DJGetDomainDC(longName, &dc));
    ceError = CTShell("%prefix/bin/lwinet ads lookup -S %dc 2>/dev/null | %sedPath -n %sedExpression >%shortName",
            CTSHELL_STRING(prefix, PREFIXDIR),
            CTSHELL_STRING(dc, dc),
            CTSHELL_STRING(sedPath, sedPath),
            CTSHELL_STRING(sedExpression, "s/^Pre-Win2k Domain:[ \t]*\\(.*\\)$/\\1/p"),
            CTSHELL_BUFFER(shortName, shortName));
    GCE(ceError);
    CTStripWhitespace(*shortName);
    if(*shortName == NULL)
    {
        CT_SAFE_FREE_STRING(*shortName);
        GCE(ceError = CENTERROR_COMMAND_FAILED);
    }

cleanup:
    CT_SAFE_FREE_STRING(sedPath);
    CT_SAFE_FREE_STRING(dc);
    return ceError;
}
