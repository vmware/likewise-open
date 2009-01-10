/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
#include "djfirewall.h"
#include "djdistroinfo.h"
#include "djauthconf.h"
#include "djauthinfo.h"
#include "djkrb5conf.h"
#include "ctshell.h"
#include "ctstrutils.h"
#include <glob.h>
#include <dlfcn.h>
#include "lsajoin.h"
#include <lwnet.h>

#define NO_TIME_SYNC_FILE "/etc/likewise-notimesync"

#if !defined(__LWI_MACOSX__)
extern char** environ;
#endif

PLSA_NET_JOIN_FUNCTION_TABLE lsaFunctions = NULL;
void *lsaHandle = NULL;

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

#define LW_RAISE_LSERR(dest, code)			\
    LWRaiseLsassError((dest), (code), __FILE__, __LINE__)

#define LW_CLEANUP_LSERR(dest, err)		\
    do						\
    {						\
	DWORD _err = (err);			\
	if (_err)				\
	{					\
	    LW_RAISE_LSERR(dest, _err);		\
	    goto cleanup;			\
	}					\
    } while (0)					\

#define HPUX_SYSTEM_RPCD_PATH "/sbin/init.d/Rpcd"

void
LWRaiseLsassError(
    LWException** dest,
    DWORD code,
    const char* file,
    unsigned int line
    )
{
    PSTR buffer = NULL;
    if(lsaFunctions != NULL)
    {
        size_t bufferSize;
        bufferSize = lsaFunctions->pfnGetErrorString(code, NULL, 0);
        LW_CLEANUP_CTERR(dest, CTAllocateMemory(bufferSize, PPCAST(&buffer)));
        if(lsaFunctions->pfnGetErrorString(code, buffer, bufferSize) == bufferSize && bufferSize > 0 && strlen(buffer) > 0)
        {
            LWRaiseEx(dest, CENTERROR_DOMAINJOIN_LSASS_ERROR, file, line, "Lsass Error", buffer);
            goto cleanup;
        }
    }

    LWRaiseEx(dest, CENTERROR_DOMAINJOIN_LSASS_ERROR, file, line, "Unable to convert lsass error", "Lsass error code %X has occurred, but an error string cannot be retrieved", code);

cleanup:
    CT_SAFE_FREE_STRING(buffer);
}

static void
DJExecWBDomainJoin(
        PCSTR rootPrefix,
    PSTR* ppszWorkgroupName,
    PCSTR osName,
    PCSTR osVer,
    JoinProcessOptions *options,
    LWException **exc);

static
CENTERROR
BuildJoinEnvironment(
    PCSTR krb5ConfPath,
    PCSTR pszPassword,
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

CENTERROR
DJRemoveCacheFiles()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;
    BOOLEAN bDirExists = FALSE;
    PSTR filePaths[] = {
        /* Likewise 4.X cache location files ... */
        "/var/lib/lwidentity/idmap_cache.tdb",
        "/var/lib/lwidentity/netsamlogon_cache.tdb",
        "/var/lib/lwidentity/winbindd_cache.tdb",
        /* Likewise 5.0 cache location files... */
        LOCALSTATEDIR "/lib/likewise/db/lsass-adcache.db",
        NULL
    };
    int i;
    const char *file;
    const char *cachePath;

    for (i = 0; filePaths[i] != NULL; i++)
    {
	file = filePaths[i];

	ceError = CTCheckFileExists(file, &bFileExists);
	BAIL_ON_CENTERIS_ERROR(ceError);
	
	if (bFileExists) 
	{
	    DJ_LOG_VERBOSE("Removing cache file %s", file);
	    ceError = CTRemoveFile(file);
	    BAIL_ON_CENTERIS_ERROR(ceError);
	}
    }

    /* Likewise 5.0 (Mac Workgroup Manager) cache files... */

    cachePath = LOCALSTATEDIR "/lib/likewise/grouppolicy/mcx";
    ceError = CTCheckDirectoryExists(cachePath, &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bDirExists)
    {
        DJ_LOG_VERBOSE("Removing Mac MCX cache files from %s", cachePath);
        ceError = CTRemoveDirectory(cachePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* Likewise 5.0 (group policy scratch) files... */

    cachePath = LOCALSTATEDIR "/lib/likewise/grouppolicy/scratch";
    ceError = CTCheckDirectoryExists(cachePath, &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bDirExists)
    {
        DJ_LOG_VERBOSE("Removing grouppolicy scratch files from %s", cachePath);
        ceError = CTRemoveDirectory(cachePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    cachePath = LOCALSTATEDIR "/lib/likewise/grouppolicy/{*}*";
    (void) CTRemoveFiles(cachePath, FALSE, TRUE);

    cachePath = LOCALSTATEDIR "/lib/likewise/grouppolicy/user-cache";
    (void) CTRemoveFiles(cachePath, FALSE, TRUE);

error:
    return ceError;
}

static
CENTERROR
CanonicalizeOrganizationalUnit(
    PSTR* pszCanonicalizedOrganizationalUnit,
    PCSTR pszOrganizationalUnit,
    PCSTR pszDomainName
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
    ModuleState *state = DJGetModuleStateByName(options, "join");
    BOOLEAN bNoTimeSyncFileExists = FALSE;


    if (options->ouName)
    {
        LW_CLEANUP_CTERR(exc, CanonicalizeOrganizationalUnit(&pszCanonicalizedOU,
                                                 options->ouName,
                                                 options->domainName));

        CT_SAFE_FREE_STRING(options->ouName);
        options->ouName = pszCanonicalizedOU;
        pszCanonicalizedOU = NULL;
    }

    LW_CLEANUP_CTERR(exc, CTCheckFileExists(NO_TIME_SYNC_FILE,
                &bNoTimeSyncFileExists));

    if (options->disableTimeSync && !bNoTimeSyncFileExists)
    {
        /* Create no time sync file */
        FILE* noTimeSyncFile = NULL;

        LW_CLEANUP_CTERR(exc, CTOpenFile(NO_TIME_SYNC_FILE,
                    "w", &noTimeSyncFile));

        CTCloseFile(noTimeSyncFile);
    }
    else if (!options->disableTimeSync && bNoTimeSyncFileExists)
    {
        /* Remove no time sync file */
        LW_CLEANUP_CTERR(exc, CTRemoveFile(NO_TIME_SYNC_FILE));
    }


    LW_TRY(exc, DJCreateComputerAccount(&options->shortDomainName, options, &LW_EXC));

    //Indicate that the join was successful incase QueryDoJoin is called later
    state->moduleData = (void *)1;

cleanup:
    CT_SAFE_FREE_STRING(pszCanonicalizedOU);
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

void
DJTestJoin(
    BOOLEAN *isValid,
    LWException **exc
    )
{
    if(lsaFunctions)
    {
        //TODO: lsajoin should set all of isValid, so it shouldn't be
        //necessary to initialize it here, but lsass's definition of BOOLEAN
        //is different from centutil's
        *isValid = FALSE;
        LW_CLEANUP_LSERR(exc, lsaFunctions->pfnNetTestJoinDomain(isValid));
    }
    else
    {
        CENTERROR ceError = CTRunCommand(PREFIXDIR "/bin/lwinet ads testjoin >/dev/null 2>&1");
        if(CENTERROR_IS_OK(ceError))
        {
            *isValid = TRUE;
        }
        else if(ceError == CENTERROR_COMMAND_FAILED)
        {
            *isValid = FALSE;
        }
        else
            LW_CLEANUP_CTERR(exc, ceError);
    }
cleanup:
    ;
}

const JoinModule DJDoJoinModule = { TRUE, "join", "join computer to AD", QueryDoJoin, DoJoin, GetJoinDescription };

static QueryResult QueryLeave(const JoinProcessOptions *options, LWException **exc)
{
    QueryResult result = SufficientlyConfigured;
    ModuleState *state = DJGetModuleStateByName((JoinProcessOptions *)options, "leave");
    BOOLEAN joinValid;

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

    LW_TRY(exc, DJTestJoin(&joinValid, &LW_EXC));
    if(!joinValid)
    {
        result = FullyConfigured;
        goto cleanup;
    }

    if(lsaFunctions != NULL)
    {
        result = NotConfigured;
    }
    else if(options->username == NULL)
    {
        //If this is the first time that this module is being queried, and
        //the username is null, turn off this module. The user can override
        //this later, in which case an error will be thrown during the actual
        //leave.
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
    LWException *inner = NULL;
    DJDisableComputerAccount(options->username, options->password, options, &inner);

    if(!LW_IS_OK(inner))
    {
        ModuleState *state = DJGetModuleStateByName((JoinProcessOptions *)options, "leave");

        state->moduleData = (void *)2;
        LW_CLEANUP(exc, inner);
    }

cleanup:
    LW_HANDLE(&inner);
}

static PSTR GetLeaveDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    LW_CLEANUP_CTERR(exc, CTStrdup("Remove the computer account from AD", &ret));

cleanup:
    return ret;
}

const JoinModule DJDoLeaveModule = { TRUE, "leave", "disable machine account", QueryLeave, DoLeave, GetLeaveDescription };

static QueryResult QueryLwiConf(const JoinProcessOptions *options, LWException **exc)
{
    QueryResult result = NotConfigured;
    PSTR readValue = NULL;
    PSTR upperDomain = NULL;
    CENTERROR ceError;
    BOOLEAN bGpagentdExists = FALSE;    
    BOOLEAN bSambaConfExists = FALSE;

    LW_CLEANUP_CTERR(exc, DJSambaConfExists(&bSambaConfExists));

    if(!bSambaConfExists && lsaFunctions)
    {
        result = NotApplicable;
        goto cleanup;
    }

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

    /*
     * Need to determine between an Enterprise and Open install.
     */

    LW_CLEANUP_CTERR(exc, CTCheckFileExists(PREFIXDIR "/sbin/gpagentd", 
					    &bGpagentdExists));

    LW_CLEANUP_CTERR(exc, DJGetSambaValue("idmap config default:backend",  &readValue));
    if (bGpagentdExists) {
        if (strcmp(readValue, "lwidentity") != 0)
	    goto cleanup;
    } else {
        if (strcmp(readValue, "lwopen") != 0)
	    goto cleanup;
    }
    CT_SAFE_FREE_STRING(readValue);
    
    LW_CLEANUP_CTERR(exc, DJGetSambaValue("winbind nss info", &readValue));
    if (bGpagentdExists) {
        if (strcmp(readValue, "lwidentity") != 0)
	    goto cleanup;
    } else {
        if (strcmp(readValue, "lwopen") != 0)
	    goto cleanup;
    }
    CT_SAFE_FREE_STRING(readValue);

    result = FullyConfigured;

cleanup:
    CT_SAFE_FREE_STRING(readValue);
    CT_SAFE_FREE_STRING(upperDomain);
    return result;
}

static void DoLwiConf(JoinProcessOptions *options, LWException **exc)
{
    BOOLEAN bGpagentdExists = FALSE;
    DistroInfo distro;

    memset(&distro, 0, sizeof(distro));
    
    LW_CLEANUP_CTERR(exc, DJInitSmbConfig(NULL));
    if(options->joiningDomain)
    {
        LW_CLEANUP_CTERR(exc, SetRealm(NULL, options->domainName));
        LW_CLEANUP_CTERR(exc, SetWorkgroup(NULL, options->shortDomainName));
        LW_CLEANUP_CTERR(exc, DJSetSambaValue(NULL, "security", "ads"));
        LW_CLEANUP_CTERR(exc, DJSetSambaValue(NULL, "use kerberos keytab", "yes"));

	LW_CLEANUP_CTERR(exc, DJGetDistroInfo(NULL, &distro));

	switch (distro.os) {
	case OS_SUNOS:
		LW_CLEANUP_CTERR(exc, DJSetSambaValue(NULL, 
						      "template homedir",
						      "/export/home/local/%D/%U"));
		LW_CLEANUP_CTERR(exc, DJSetSambaValue(NULL, 
						      "template shell",
						      "/bin/ksh"));
		break;
	case OS_DARWIN:
		LW_CLEANUP_CTERR(exc, DJSetSambaValue(NULL, 
						      "template homedir",
						      "/Users/%D/%U"));
		LW_CLEANUP_CTERR(exc, DJSetSambaValue(NULL, 
						      "template shell",
						      "/bin/bash"));
                break;
	default:
		LW_CLEANUP_CTERR(exc, DJSetSambaValue(NULL, 
						      "template homedir",
						      "/home/local/%D/%U"));
		LW_CLEANUP_CTERR(exc, DJSetSambaValue(NULL, 
						      "template shell",
						      "/bin/bash"));
	}

	/*
	 * Need to determine between an Enterprise and Open install.
	 */

	LW_CLEANUP_CTERR(exc, CTCheckFileExists(PREFIXDIR "/sbin/gpagentd", 
						&bGpagentdExists));

	if (bGpagentdExists) {
		/* Likewise Enterprise */
		LW_CLEANUP_CTERR(exc, DJSetSambaValue(NULL, 
						      "idmap config default:backend", 
						      "lwidentity"));
		LW_CLEANUP_CTERR(exc, DJSetSambaValue(NULL, 
						      "winbind nss info", 
						      "lwidentity"));
	} else {
		/* Likewise Open */
		LW_CLEANUP_CTERR(exc, DJSetSambaValue(NULL, 
						      "idmap config default:backend", 
						      "lwopen"));
		LW_CLEANUP_CTERR(exc, DJSetSambaValue(NULL, 
						      "winbind nss info", 
						      "lwopen"));
	}
    }
    else
    {
        LW_CLEANUP_CTERR(exc, SetWorkgroup(NULL, "WORKGROUP"));
    }

cleanup:
    DJFreeDistroInfo(&distro);
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
"idmap default config:backend=<lwidentity | lwopen>\n"
"winbind nss info = <lwidentity | lwopen>\n"
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
WBGetConfiguredDnsDomain(
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

void
DJGetConfiguredDnsDomain(
    PSTR* ppszDomain,
    LWException **exc
    )
{
    if(lsaFunctions)
    {
        DWORD _err = lsaFunctions->pfnGetDnsDomainName(ppszDomain);
        if(_err)
        {
	    LW_RAISE_LSERR(exc, _err);
            if(exc != NULL)
            {
                switch(_err)
                {
                    case 0x8049: //LSA_ERROR_NOT_JOINED_TO_AD
                        (*exc)->code = CENTERROR_DOMAINJOIN_DOMAIN_NOT_FOUND;
                        break;
                }
            }
	    goto cleanup;
        }
    }
    else
    {
        LW_CLEANUP_CTERR(exc, WBGetConfiguredDnsDomain(ppszDomain));
    }
cleanup:
    ;
}

CENTERROR
WBGetConfiguredShortDomain(
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

void
DJGetConfiguredShortDomain(
    PSTR* ppszWorkgroup,
    LWException **exc
    )
{
    PSTR longDomain = NULL;
    if(lsaFunctions)
    {
        LW_CLEANUP_LSERR(exc, lsaFunctions->pfnGetDnsDomainName(&longDomain));
        LW_CLEANUP_LSERR(exc, lsaFunctions->pfnGetShortDomain(longDomain, ppszWorkgroup));
    }
    else
    {
        LW_CLEANUP_CTERR(exc, WBGetConfiguredShortDomain(ppszWorkgroup));
    }
cleanup:
    CT_SAFE_FREE_STRING(longDomain);
}

static void
WBGetDomainDC(PCSTR domain, PSTR *dc, LWException **exc)
{
    PSTR sedPath = NULL;
    PSTR error = NULL;

    *dc = NULL;
    LW_CLEANUP_CTERR(exc, CTFindSed(&sedPath));
    LW_CLEANUP_CTERR(exc, CTShell("%prefix/bin/lwinet lookup dsgetdcname %domain 2>%error | %sedPath -n %sedExpression >%dc",
            CTSHELL_STRING(prefix, PREFIXDIR),
            CTSHELL_STRING(domain, domain),
            CTSHELL_STRING(sedPath, sedPath),
            CTSHELL_STRING(sedExpression, "s/^domain_controller_name:[ \t]*\\(.*\\)$/\\1/p"),
            CTSHELL_BUFFER(dc, dc),
            CTSHELL_BUFFER(error, &error)));

    if (*dc != NULL)
        CTStripWhitespace(*dc);

    if (IsNullOrEmptyString(*dc))
    {
        CT_SAFE_FREE_STRING(*dc);
        LW_RAISE_EX(exc, CENTERROR_COMMAND_FAILED, "Unable to find DC", "Calling '%s/bin/lwinet lookup dsgetdcname %s' failed. The stderr output was '%s'.",
                PREFIXDIR, domain, error);
        goto cleanup;
    }

cleanup:
    CT_SAFE_FREE_STRING(sedPath);
    CT_SAFE_FREE_STRING(error);
}

void
DJGetDomainDC(PCSTR domain, PSTR *dc, LWException **exc)
{
    if(lsaFunctions)
    {
        LW_CLEANUP_LSERR(exc, lsaFunctions->pfnGetDCName(domain, dc));
    }
    else
    {
        LW_TRY(exc, WBGetDomainDC(domain, dc, &LW_EXC));
    }
cleanup:
    ;
}

void
WBGetComputerDN(PSTR *dn, LWException **exc)
{
    PSTR sedPath = NULL;
    PSTR errors = NULL;

    *dn = NULL;
    LW_CLEANUP_CTERR(exc, CTFindSed(&sedPath));
    LW_CLEANUP_CTERR(exc, CTShell("%prefix/bin/lwinet ads status -P 2>%errors | %sedPath -n %sedExpression >%dn",
            CTSHELL_STRING(prefix, PREFIXDIR),
            CTSHELL_STRING(sedPath, sedPath),
            CTSHELL_STRING(sedExpression, "s/^distinguishedName:[ \t]*\\(.*\\)$/\\1/p"),
            CTSHELL_BUFFER(dn, dn),
            CTSHELL_BUFFER(errors, &errors)
            ));
    CTStripWhitespace(*dn);
    if(*dn == NULL || **dn == '\0')
    {
        LW_RAISE_EX(exc, CENTERROR_COMMAND_FAILED, "Unable to get distinguished name", "The computer's distinguished name could not be queried. Here is the output from 'lwinet ads status -P':\n%s", errors);
        goto cleanup;
    }

cleanup:
    CT_SAFE_FREE_STRING(sedPath);
    CT_SAFE_FREE_STRING(errors);
}

void
DJGetComputerDN(PSTR *dn, LWException **exc)
{
    if(lsaFunctions)
    {
        DWORD _err = 0;

        LW_CLEANUP_LSERR(exc, LWNetExtendEnvironmentForKrb5Affinity(FALSE));

        _err = lsaFunctions->pfnGetComputerDN(dn);
        if(_err)
        {
	    LW_RAISE_LSERR(exc, _err);
            if(exc != NULL)
            {
                switch(_err)
                {
                    case 0x804A: //LSA_ERROR_NOT_JOINED_TO_AD
                        (*exc)->code = CENTERROR_DOMAINJOIN_DOMAIN_NOT_FOUND;
                        break;
                }
            }
	    goto cleanup;
        }
    }
    else
    {
        LW_TRY(exc, WBGetComputerDN(dn, &LW_EXC));
    }
cleanup:
    ;
}

void DJNetInitialize(LWException **exc)
{
    PCSTR lsaFilename = LIBDIR "/liblsajoin" DYNLIBEXT;
    BOOLEAN lsaExists;
    PFN_LSA_NET_JOIN_INITIALIZE init = NULL;
    BOOLEAN freeLsaHandle = TRUE;
    BOOLEAN systemDcedExists = FALSE;

    DJ_LOG_INFO("Trying to load %s", lsaFilename);
    
    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(lsaFilename, &lsaExists));

    if(lsaExists)
    {
        lsaHandle = dlopen(lsaFilename, RTLD_NOW | RTLD_GLOBAL);
        if(lsaHandle == NULL)
            LW_CLEANUP_DLERROR(exc);

        init = dlsym(lsaHandle, LSA_SYMBOL_NET_JOIN_INITIALIZE);
        if(init == NULL)
            LW_CLEANUP_DLERROR(exc);

        if (geteuid() == 0) {
            LW_TRY(exc, DJManageDaemon("lwiod", TRUE,
                        92, 8, &LW_EXC));
            LW_TRY(exc, DJManageDaemon("netlogond", TRUE,
                        92, 10, &LW_EXC));

            // Use the system's dced daemon if it exists, otherwise use the
            // Likewise version.
            LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(
                        HPUX_SYSTEM_RPCD_PATH,
                        &systemDcedExists));
            if (systemDcedExists)
            {
                LW_TRY(exc, DJManageDaemon(HPUX_SYSTEM_RPCD_PATH, TRUE,
                            590, 410, &LW_EXC));
            }
            else
            {
                LW_TRY(exc, DJManageDaemon("dcerpcd", TRUE,
                            92, 11, &LW_EXC));
            }

            LW_TRY(exc, DJManageDaemon("eventlogd", TRUE,
                        92, 11, &LW_EXC));
        }

        LW_CLEANUP_LSERR(exc, init(&lsaFunctions));
        DJ_LOG_INFO("Initialized %s", lsaFilename);
#if 0
        /* Do not enable debug logging in lsajoin because
           it does not respect domainjoin logging settings
           such as logfile */
        if(gdjLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE)
            lsaFunctions->pfnEnableDebugLog();
#endif
    }

    freeLsaHandle = FALSE;

cleanup:
    if(freeLsaHandle && lsaHandle != NULL)
    {
        dlclose(lsaHandle);
        lsaHandle = NULL;
    }
}

void DJNetShutdown(LWException **exc)
{
    PFN_LSA_NET_JOIN_SHUTDOWN shutdown = NULL;
    if(lsaHandle != NULL)
    {
        if(lsaFunctions != NULL)
        {
            shutdown = dlsym(lsaHandle, LSA_SYMBOL_NET_JOIN_SHUTDOWN);
        }
        if(shutdown != NULL)
            shutdown(lsaFunctions);
        if(dlclose(lsaHandle) != 0)
            LW_CLEANUP_DLERROR(exc);
        lsaHandle = NULL;
    }
cleanup:
    ;
}

static void
DJExecWBDomainJoin(
        PCSTR rootPrefix,
    PSTR* ppszWorkgroupName,
    PCSTR osName,
    PCSTR osVer,
    JoinProcessOptions *options,
    LWException **exc)
{
    PSTR pszTmp = NULL;
    PSTR pszTerm = NULL;
    PSTR pszWorkgroupName = NULL;
    PSTR krb5ConfPath = NULL;
    PSTR outbuf = NULL;
    PSTR errbuf = NULL;
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR* ppszEnv = NULL;
    DWORD nVars = 0;

    if(rootPrefix == NULL)
        rootPrefix = "";

    /* Join the domain and extract (screenscrape) the name of the
     * workgroup that the join utility writes to stdout.  This is
     * fragile and should be changed in the future.
     */

    // The user name should already be in UPN format
    if (strchr(options->username, '@') == NULL) {
        LW_CLEANUP_CTERR(exc, CENTERROR_INVALID_PARAMETER);
    }

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&krb5ConfPath, "%s/etc/krb5.conf", rootPrefix));

    LW_CLEANUP_CTERR(exc, BuildJoinEnvironment(krb5ConfPath, options->password, &ppszEnv, &nVars));

    ceError = CTShellEx(ppszEnv, "%prefix/bin/lwinet --configfile=%rootPrefix/etc/samba/lwiauthd.conf ads join -U %pszUserName %logOption osName=%osName osVer=%osVer %createcomputerOption%pszOU %notimesyncOption >%outbuf 2>%errbuf",
            CTSHELL_STRING(prefix, PREFIXDIR),
            CTSHELL_STRING(rootPrefix, rootPrefix),
            CTSHELL_STRING(pszUserName, options->username),
            (gdjLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE? CTSHELL_STRING(logOption, "-d10") : CTSHELL_ZERO(logOption)),
            CTSHELL_STRING(osName, osName),
            CTSHELL_STRING(osVer, osVer),
            (IsNullOrEmptyString(options->ouName) ? CTSHELL_ZERO(createcomputerOption) : CTSHELL_STRING(createcomputerOption, "createcomputer=")),
            (IsNullOrEmptyString(options->ouName) ? CTSHELL_ZERO(pszOU) : CTSHELL_STRING(pszOU, options->ouName)),
            (options->disableTimeSync ? CTSHELL_STRING(notimesyncOption, "notimesync") : CTSHELL_ZERO(notimesyncOption) ),
            CTSHELL_BUFFER(outbuf, &outbuf),
            CTSHELL_BUFFER(errbuf, &errbuf));

    if (ceError == CENTERROR_COMMAND_FAILED)
    {
        LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_JOIN_FAILED, "Domain join failed",
"Creating the computer account in AD failed with the following output to stdout:\n"
"%s\n"
"And the following output to stderr:\n%s",
            outbuf, errbuf);
        goto cleanup;
    }
    LW_CLEANUP_CTERR(exc, ceError);

    if (!IsNullOrEmptyString(outbuf)) {
        DJ_LOG_INFO("%s", outbuf);
    }

    // Now do the nasty screenscrape to get the domain shortname aka
    // 'workgroup' name
    pszTmp = (outbuf ? strstr(outbuf, "--") : NULL);
    if (pszTmp == NULL) {
        LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_JOIN_NO_WKGRP,
                "Could not identity short domain name",
"The domain join appears to have succeeded but did not return the short (workgroup) name as expected. The output was:"
"%s\n"
"And the stderr output was:\n%s",
            outbuf == NULL ? "(null)" : outbuf,
            errbuf == NULL ? "(null)" : errbuf);
        goto cleanup;
    }

    pszTmp+=2;
    while (*pszTmp != '\0' && isspace((int) *pszTmp))
        pszTmp++;
    pszTerm = pszTmp;
    while (*pszTerm != '\0' && !isspace((int) *pszTerm))
        pszTerm++;
    *pszTerm = '\0';

    if (IsNullOrEmptyString(pszTmp)) {
        LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_JOIN_NO_WKGRP,
                "Could not identity short domain name",
"The domain join appears to have succeeded but did not return the short (workgroup) name as expected. The output was:"
"%s\n"
"And the stderr output was:\n%s",
            outbuf == NULL ? "(null)" : outbuf,
            errbuf == NULL ? "(null)" : errbuf);
    }

    LW_CLEANUP_CTERR(exc, CTAllocateString(pszTmp, &pszWorkgroupName));

    *ppszWorkgroupName = pszWorkgroupName;
    pszWorkgroupName = NULL;

    if(!IsNullOrEmptyString(errbuf) && options != NULL && options->warningCallback != NULL)
    {
        CTStripWhitespace(errbuf);
        options->warningCallback(options, "Lwinet ads join worked but produced the following errors", errbuf);
    }

cleanup:
    CT_SAFE_FREE_STRING(pszWorkgroupName);
    CT_SAFE_FREE_STRING(krb5ConfPath);
    CT_SAFE_FREE_STRING(outbuf);
    CT_SAFE_FREE_STRING(errbuf);

    if (ppszEnv)
        CTFreeStringArray(ppszEnv, nVars);
}

static void WBCreateComputerAccount(
                PSTR *shortDomainName,
                PCSTR osName,
                PCSTR osVer,
                JoinProcessOptions *options,
                LWException **exc)
{
    PSTR tempDir = NULL;
    PSTR lwiauthdPath = NULL;

    LW_CLEANUP_CTERR(exc, CTCreateTempDirectory(&tempDir));
    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&lwiauthdPath, "%s/etc/samba", tempDir));
    LW_CLEANUP_CTERR(exc, CTCreateDirectory(lwiauthdPath, 0700));
    CT_SAFE_FREE_STRING(lwiauthdPath);
    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&lwiauthdPath, "%s/etc/samba/lwiauthd.conf", tempDir));
    LW_CLEANUP_CTERR(exc, CTCopyFileWithOriginalPerms("/etc/samba/lwiauthd.conf", lwiauthdPath));
    LW_TRY(exc, DJCopyKrb5ToRootDir(NULL, tempDir, &LW_EXC));

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
    CT_SAFE_FREE_STRING(*shortDomainName);
    LW_TRY(exc, DJExecWBDomainJoin(tempDir,
                               shortDomainName,
                               osName,
                               osVer,
                               options,
                               &LW_EXC));

cleanup:
    if(tempDir != NULL)
    {
        CTRemoveDirectory(tempDir);
        CT_SAFE_FREE_STRING(tempDir);
    }
    CT_SAFE_FREE_STRING(lwiauthdPath);
}

void DJCreateComputerAccount(
                PSTR *shortDomainName,
                JoinProcessOptions *options,
                LWException **exc)
{
    DistroInfo distro;
    PSTR osName = NULL;
    PSTR tempDir = NULL;
    PSTR origEnv = NULL;
    CHAR krb5ConfEnv[256];
    DWORD dwFlags = 0;
    DWORD err = 0;

    PSTR likewiseVersion = NULL;
    PSTR likewiseBuild = NULL;
    PSTR likewiseRevision = NULL;

    PSTR likewiseOSServicePack = NULL;

    memset(&distro, 0, sizeof(distro));

    LW_CLEANUP_CTERR(exc, DJGetDistroInfo(NULL, &distro));
    LW_CLEANUP_CTERR(exc, DJGetDistroString(distro.distro, &osName));

    LW_CLEANUP_CTERR(exc, DJGetLikewiseVersion(&likewiseVersion,
                &likewiseBuild, &likewiseRevision));

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&likewiseOSServicePack,
                "Likewise Identity %s.%s.%s",
                likewiseVersion, likewiseBuild, likewiseRevision));

    if(lsaFunctions)
    {
        LW_CLEANUP_CTERR(exc, CTCreateTempDirectory(&tempDir));

        LW_TRY(exc, DJCopyKrb5ToRootDir(NULL, tempDir, &LW_EXC));

        /*
         * Setup krb5.conf with the domain as the Kerberos realm.
         * We do this before doing the join. This is required for any
         * authenticated LDAP connections used to acquire a TGT in order
         * to move the computer account into the right OU.
         */
        LW_CLEANUP_CTERR(exc, DJModifyKrb5Conf(
                                     tempDir,
                                     TRUE,
                                     options->domainName, NULL, NULL));

        origEnv = getenv("KRB5_CONFIG");

        sprintf(krb5ConfEnv, "KRB5_CONFIG=%s/etc/krb5.conf", tempDir);

        if (putenv(krb5ConfEnv) != 0) {
           LW_CLEANUP_CTERR(exc, CTMapSystemError(errno));
        }

        LW_CLEANUP_LSERR(exc, LWNetExtendEnvironmentForKrb5Affinity(FALSE));

        if ( options->disableTimeSync )
        {
            dwFlags |= LSA_NET_JOIN_DOMAIN_NOTIMESYNC;
        }

        err = lsaFunctions->pfnNetJoinDomain(
                  options->computerName,
                  options->domainName,
                  options->ouName,
                  options->username,
                  options->password,
                  osName,
                  distro.version,
                  likewiseOSServicePack,
                  dwFlags);
        if (err)
        {
            switch(err)
            {
                case ENOENT:
                    LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_INVALID_OU, "Lsass Error", "The OU is invalid.");
                    break;
                case EINVAL:
                    LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_INVALID_FORMAT, "Lsass Error", "The OU format is invalid.");
                    break;
                default:
                    LW_RAISE_LSERR(exc, err);
                    break;
            }
            goto cleanup;
        }

        LW_TRY(exc, DJGuessShortDomainName(
                                     options->domainName,
                                     shortDomainName, &LW_EXC));
    }
    else
    {
        LW_TRY(exc, WBCreateComputerAccount(
                                    shortDomainName,
                                    osName, distro.version,
                                    options, &LW_EXC));
    }

cleanup:

    if(tempDir != NULL)
    {
        CTRemoveDirectory(tempDir);
        CT_SAFE_FREE_STRING(tempDir);
    }

    if (origEnv && *origEnv) {
       putenv(origEnv);
    } else {
       putenv("KRB5_CONFIG=/etc/krb5.conf");
    }

    CT_SAFE_FREE_STRING(likewiseVersion);
    CT_SAFE_FREE_STRING(likewiseBuild);
    CT_SAFE_FREE_STRING(likewiseRevision);
    CT_SAFE_FREE_STRING(likewiseOSServicePack);

    DJFreeDistroInfo(&distro);
}

static void WBDisableComputerAccount(PCSTR username, PCSTR password,
    JoinProcessOptions *options, LWException **exc)
{
    PSTR outbuf = NULL;
    PSTR errbuf = NULL;
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR* ppszEnv = NULL;
    DWORD nVars = 0;

    // The user name should already be in UPN format
    if (strchr(username, '@') == NULL) {
        LW_CLEANUP_CTERR(exc, CENTERROR_INVALID_PARAMETER);
    }

    LW_CLEANUP_CTERR(exc, BuildJoinEnvironment(NULL, password, &ppszEnv, &nVars));

    ceError = CTShellEx(ppszEnv, "%prefix/bin/lwinet ads leave %credOption %username >%outbuf 2>%errbuf",
            CTSHELL_STRING(prefix, PREFIXDIR),
            CTSHELL_STRING(credOption, IsNullOrEmptyString(username) ? "-P" : "-U"),
            (IsNullOrEmptyString(username) ? CTSHELL_ZERO(username) : CTSHELL_STRING(username, username)),
            CTSHELL_BUFFER(outbuf, &outbuf),
            CTSHELL_BUFFER(errbuf, &errbuf));

    if (ceError == CENTERROR_COMMAND_FAILED)
    {
        if(IsNullOrEmptyString(password))
        {
            LW_RAISE_EX(exc, CENTERROR_INVALID_PASSWORD, "Unable to disable computer account", "The computer account does not have sufficient permissions to disable itself. Please either provide an administrator's username and password, or the username and password of the account originally used to join the computer to AD.");
            goto cleanup;
        }
        else
        {
            LW_RAISE_EX(exc, CENTERROR_COMMAND_FAILED, "Unable to disable computer account", "Disabling the computer account failed with the following output to stdout:\n"
"%s\n"
"And the following output to stderr:\n%s",
                outbuf, errbuf);
            goto cleanup;
        }
    }
    LW_CLEANUP_CTERR(exc, ceError);

    if (!IsNullOrEmptyString(outbuf)) {
        DJ_LOG_INFO("%s", outbuf);
    }

    if(!IsNullOrEmptyString(errbuf) && options != NULL && options->warningCallback != NULL)
    {
        CTStripWhitespace(errbuf);
        options->warningCallback(options, "Lwinet ads leave worked but produced the following errors", errbuf);
    }

cleanup:
    CT_SAFE_FREE_STRING(outbuf);
    CT_SAFE_FREE_STRING(errbuf);

    if (ppszEnv)
        CTFreeStringArray(ppszEnv, nVars);
}

void DJDisableComputerAccount(PCSTR username,
                PCSTR password,
                JoinProcessOptions *options,
                LWException **exc)
{
    if(lsaFunctions)
    {
        LW_CLEANUP_LSERR(exc, lsaFunctions->pfnNetLeaveDomain(username, password));
    }
    else
    {
        LW_TRY(exc, WBDisableComputerAccount(username, password, options, &LW_EXC));
    }
cleanup:
    ;
}

static void
WBGuessShortDomainName(PCSTR longName, PSTR *shortName, LWException **exc)
{
    PSTR sedPath = NULL;
    PSTR dc = NULL;

    *shortName = NULL;
    LW_CLEANUP_CTERR(exc, CTFindSed(&sedPath));
    LW_TRY(exc, DJGetDomainDC(longName, &dc, &LW_EXC));
    LW_CLEANUP_CTERR(exc, CTShell("%prefix/bin/lwinet ads lookup -S %dc 2>/dev/null | %sedPath -n %sedExpression >%shortName",
            CTSHELL_STRING(prefix, PREFIXDIR),
            CTSHELL_STRING(dc, dc),
            CTSHELL_STRING(sedPath, sedPath),
            CTSHELL_STRING(sedExpression, "s/^Pre-Win2k Domain:[ \t]*\\(.*\\)$/\\1/p"),
            CTSHELL_BUFFER(shortName, shortName)));
    CTStripWhitespace(*shortName);
    if(*shortName == NULL)
    {
        CT_SAFE_FREE_STRING(*shortName);
        LW_CLEANUP_CTERR(exc, CENTERROR_COMMAND_FAILED);
    }

cleanup:
    CT_SAFE_FREE_STRING(sedPath);
    CT_SAFE_FREE_STRING(dc);
}

void DJGuessShortDomainName(PCSTR longName,
                PSTR *shortName,
                LWException **exc)
{
    if(lsaFunctions)
    {
        LW_CLEANUP_LSERR(exc, lsaFunctions->pfnGetShortDomain(longName, shortName));
    }
    else
    {
        LW_TRY(exc, WBGuessShortDomainName(longName, shortName, &LW_EXC));
    }
cleanup:
    ;
}

CENTERROR
WBGetMachineSID(
    PSTR* ppszMachineSID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 0;
    CHAR  szCmd[PATH_MAX+1];
    LONG  status = 0;
    PSTR  pszBuffer = NULL;
    PSTR  pszMachineSID = NULL;
    PSTR  pszTmp = NULL;
    PROCBUFFER procBuffer;
    DWORD iBufIdx = 0;
    DWORD dwBufLen = 0;
    DWORD dwBufAvailable = 0;

    sprintf(szCmd, "%s/bin/lwinet", PREFIXDIR);

    nArgs = 3;

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs));
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(szCmd, ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("getlocalsid", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(szCmd, ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    do {

        ceError = DJReadData(pProcInfo, &procBuffer);
        BAIL_ON_CENTERIS_ERROR(ceError);

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

    if (status != 0) {
        ceError = CENTERROR_DOMAINJOIN_SET_MACHINESID_FAIL;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pszBuffer == NULL ||
        (pszTmp = strstr(pszBuffer, ": ")) == NULL ||
        *(pszTmp+2) == '\0') {
        ceError = CENTERROR_DOMAINJOIN_NETCONFIGCMD_FAIL;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    CTStripTrailingWhitespace(pszBuffer);

    ceError = CTAllocateString(pszTmp+2, &pszMachineSID);
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

CENTERROR
DJGetMachineSID(
    PSTR* ppszMachineSID
    )
{
    if(lsaFunctions)
    {
        *ppszMachineSID = NULL;
        return CENTERROR_SUCCESS;
    }
    else
    {
        return WBGetMachineSID(ppszMachineSID);
    }
}

static
CENTERROR
WBSetMachineSID(
    PSTR pszMachineSID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 0;
    CHAR  szCmd[PATH_MAX+1];
    LONG  status = 0;

    sprintf(szCmd, "%s/bin/lwinet", PREFIXDIR);

    nArgs = 4;

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs));
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(szCmd, ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("setlocalsid", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(pszMachineSID, ppszArgs+2);
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

CENTERROR
DJSetMachineSID(
    PSTR pszMachineSID
    )
{
    if(lsaFunctions)
    {
        return CENTERROR_SUCCESS;
    }
    else
    {
        return WBSetMachineSID(pszMachineSID);
    }
}
