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
#include "djauditing.h"
#include "djdistroinfo.h"
#include "djauthconf.h"
#include "djauthinfo.h"
#include "djkrb5conf.h"
#include "djregistry.h"
#include "djservicemgr.h"
#include "ctshell.h"
#include "ctstrutils.h"
#include <glob.h>
#include <dlfcn.h>
#include <lsa/lsa.h>
#include <lsa/ad.h>
#include <lwnet.h>
#include <eventlog.h>
#include <lwstr.h>
#include <lwsm/lwsm.h>

#define DOMAINJOIN_EVENT_CATEGORY   "Domain join"

#define SUCCESS_AUDIT_EVENT_TYPE    "Success Audit"
#define FAILURE_AUDIT_EVENT_TYPE    "Failure Audit"
#define INFORMATION_EVENT_TYPE      "Information"
#define WARNING_EVENT_TYPE          "Warning"
#define ERROR_EVENT_TYPE            "Error"

#define DOMAINJOIN_EVENT_INFO_JOINED_DOMAIN            1000
#define DOMAINJOIN_EVENT_ERROR_DOMAIN_JOIN_FAILURE     1001
#define DOMAINJOIN_EVENT_INFO_LEFT_DOMAIN              1002
#define DOMAINJOIN_EVENT_ERROR_DOMAIN_LEAVE_FAILURE    1003

#define NO_TIME_SYNC_FILE "/etc/likewise-notimesync"

#if !defined(__LWI_MACOSX__)
extern char** environ;
#endif

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

#define LW_RAISE_LSERR(dest, code)			\
    LW_RAISE(dest, code);

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

static
QueryResult QueryCache(
    const JoinProcessOptions *options,
    LWException **ppException
    );

static
VOID
DoCache(
    JoinProcessOptions * pJoinOptions,
    LWException** ppException
    );

static
PSTR
GetCacheDescription(
    const JoinProcessOptions *pJoinOptions,
    LWException** ppException
    );

static
DWORD
RemoveCacheFiles();

VOID
FixCfgString(
    PSTR pszString
    )
{
    size_t len = 0;

    CTStripWhitespace(pszString);

    len = strlen(pszString);

    if(pszString[len - 1] == ';')
        len--;
    if(pszString[len - 1] == '"')
        len--;

    pszString[len] = 0;

    if(pszString[0] == '"')
    {
        //Since the string is shrinking by one character, copying len
        //characters will move the null too.
        memmove(pszString, pszString + 1, len);
    }
}

const JoinModule DJCacheModule =
{
    TRUE,
    "cache",
    "manage caches for this host",
    QueryCache,
    DoCache,
    GetCacheDescription
};

static
QueryResult QueryCache(
    const JoinProcessOptions *options,
    LWException **ppException
    )
{
    ModuleState *state = DJGetModuleStateByName(options, "cache");

    if(!options->joiningDomain || options->enableMultipleJoins)
        return NotApplicable;

    //This module sets its moduleData after it is finished making changes. By
    //reading it we can tell if this module has already been run.
    if(state != NULL && state->moduleData == (void *)1)
    {
        return FullyConfigured;
    }
    return NotConfigured;
}

static
VOID
DoCache(
    JoinProcessOptions * options,
    LWException** ppException
    )
{
    ModuleState *state = DJGetModuleStateByName(options, "cache");

    LW_CLEANUP_CTERR(ppException, RemoveCacheFiles());

    //Indicate that the operation was successful incase QueryCache is called later
    state->moduleData = (void *)1;

cleanup:

    return;

}

static
PSTR
GetCacheDescription(
    const JoinProcessOptions *pJoinOptions,
    LWException** ppException
    )
{
    PSTR pszDescription = NULL;

    LW_CLEANUP_CTERR(ppException,
                     CTAllocateString(
                         "Manages caches for this host",
                         &pszDescription));

cleanup:

    return pszDescription;

}

static
DWORD
RemoveCacheFiles()
{
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;
    BOOLEAN bDirExists = FALSE;
    CHAR szSudoOrigFile[PATH_MAX+1];
    PSTR pszSearchPath = NULL;
    PSTR pszSudoersPath = NULL;
    int i = 0;
    PSTR file = NULL;
    PSTR pszCachePath = NULL;

    PSTR filePaths[] = {
        /* Likewise 4.X cache location files ... */
        "/var/lib/lwidentity/idmap_cache.tdb",
        "/var/lib/lwidentity/netsamlogon_cache.tdb",
        "/var/lib/lwidentity/winbindd_cache.tdb",
        /* Likewise 5.0 cache location files... */
        NULL
    };

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

    pszCachePath = LOCALSTATEDIR "/lib/likewise/grouppolicy/mcx";
    ceError = CTCheckDirectoryExists(pszCachePath, &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bDirExists)
    {
        DJ_LOG_VERBOSE("Removing Mac MCX cache files from %s", pszCachePath);
        ceError = CTRemoveDirectory(pszCachePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* /Library/Preferences/com.apple.loginwindow.plist */
    ceError = CTCheckFileExists("/Library/Preferences/com.apple.loginwindow.plist.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /Library/Preferences/com.apple.loginwindow.plist.lwidentity.orig file to /Library/Preferences/com.apple.loginwindow.plist");
        ceError = CTMoveFile("/Library/Preferences/com.apple.loginwindow.plist.lwidentity.orig", "/Library/Preferences/com.apple.loginwindow.plist");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* Likewise 5.0 (group policy scratch) files... */

    pszCachePath = LOCALSTATEDIR "/lib/likewise/grouppolicy/scratch";
    ceError = CTCheckDirectoryExists(pszCachePath, &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bDirExists)
    {
        DJ_LOG_VERBOSE("Removing grouppolicy scratch files from %s", pszCachePath);
        ceError = CTRemoveDirectory(pszCachePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pszCachePath = LOCALSTATEDIR "/lib/likewise/grouppolicy/{*}*";
    (void) CTRemoveFiles(pszCachePath, FALSE, TRUE);

    pszCachePath = LOCALSTATEDIR "/lib/likewise/grouppolicy/user-cache";
    (void) CTRemoveFiles(pszCachePath, FALSE, TRUE);

    pszCachePath = LOCALSTATEDIR "/lib/likewise/lwedsplugin/user-cache";
    (void) CTRemoveFiles(pszCachePath, FALSE, TRUE);

    /* Revert any system configuration files that may have been changed by previous domain GPOs */
    ceError = CTCheckDirectoryExists( "/var/lib/likewise/grouppolicy",
                                      &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);
    if (bDirExists) {
        ceError = CTRemoveDirectory("/var/lib/likewise/grouppolicy");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = DeleteTreeFromRegistry("Policy");
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* /etc/krb5.conf */
    ceError = CTCheckFileExists("/etc/krb5.conf.lwidentity-gp.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/krb5.conf.lwidentity-gp.orig file to /etc/krb5.conf");
        ceError = CTMoveFile("/etc/krb5.conf.lwidentity-gp.orig", "/etc/krb5.conf");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* /etc/sudoers */
    ceError = CTAllocateString( "/usr/local/etc:/usr/etc:/etc:/opt/sudo/etc:/opt/csw/etc",
                                &pszSearchPath);

    FixCfgString(pszSearchPath);

    ceError = CTFindFileInPath( "sudoers",
                                 pszSearchPath,
                                 &pszSudoersPath);

    if( ceError == ERROR_FILE_NOT_FOUND )
        ceError = ERROR_SUCCESS;

    if(pszSudoersPath)
    {
        sprintf( szSudoOrigFile,
                 "%s.lwidentity.orig",
                 pszSudoersPath);

        ceError = CTCheckFileExists( szSudoOrigFile,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);
        if (bFileExists)
        {
            DJ_LOG_VERBOSE("Restoring %s file to %s",szSudoOrigFile,pszSudoersPath);
            ceError = CTMoveFile(szSudoOrigFile, pszSudoersPath);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

    }

    /* /etc/motd */
    ceError = CTCheckFileExists("/etc/motd.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/motd.lwidentity.orig file to /etc/motd");
        ceError = CTMoveFile("/etc/motd.lwidentity.orig", "/etc/motd");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* /etc/syslog.conf */
    ceError = CTCheckFileExists("/etc/syslog.conf.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/syslog.conf.lwidentity.orig file to /etc/syslog.conf");
        ceError = CTMoveFile("/etc/syslog.conf.lwidentity.orig", "/etc/syslog.conf");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        /* /etc/syslog-ng.conf */
        ceError = CTCheckFileExists("/etc/syslog-ng/syslog-ng.conf.lwidentity.orig", &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists)
        {
            DJ_LOG_VERBOSE("Restoring /etc/syslog-ng/syslog-ng.conf.lwidentity.orig file to /etc/syslog-ng/syslog-ng.conf");
            ceError = CTMoveFile("/etc/syslog-ng/syslog-ng.conf.lwidentity.orig", "/etc/syslog-ng/syslog-ng.conf");
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else
        {

            /* /etc/rsyslog.conf */
            ceError = CTCheckFileExists("/etc/rsyslog.conf.lwidentity.orig", &bFileExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (bFileExists)
            {
                DJ_LOG_VERBOSE("Restoring /etc/rsyslog.conf.lwidentity.orig file to /etc/rsyslog.conf");
                ceError = CTMoveFile("/etc/rsyslog.conf.lwidentity.orig", "/etc/rsyslog.conf");
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }
    }

    /* /etc/crontab */
    ceError = CTCheckFileExists("/etc/crontab.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/crontab.lwidentity.orig file to /etc/crontab");
        ceError = CTMoveFile("/etc/crontab.lwidentity.orig", "/etc/crontab");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* /etc/logrotate.conf */
    ceError = CTCheckFileExists("/etc/logrotate.conf.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/logrotate.conf.lwidentity.orig file to /etc/logrotate.conf");
        ceError = CTMoveFile("/etc/logrotate.conf.lwidentity.orig", "/etc/logrotate.conf");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* /etc/issue */
    ceError = CTCheckFileExists("/etc/issue.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/issue.lwidentity.orig file to /etc/issue");
        ceError = CTMoveFile("/etc/issue.lwidentity.orig", "/etc/issue");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* Revert selinux/config */
    ceError = CTCheckFileExists("/etc/selinux/config.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/selinux/config.lwidentity.orig file to /etc/selinux/config.lwidentity.orig");
        ceError = CTMoveFile("/etc/selinux/config.lwidentity.orig", "/etc/selinux/config");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* Revert fstab */
    ceError = CTCheckFileExists("/etc/fstab.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/fstab.lwidentity.orig file to /etc/fstab");
        ceError = CTMoveFile("/etc/fstab.lwidentity.orig", "/etc/fstab");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {

        ceError = CTCheckFileExists("/etc/vfstab.lwidentity.orig", &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists)
        {
            DJ_LOG_VERBOSE("Restoring /etc/vfstab.lwidentity.orig file to /etc/vfstab");
            ceError = CTMoveFile("/etc/vfstab.lwidentity.orig", "/etc/vfstab");
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else
        {
            ceError = CTCheckFileExists("/etc/filesystems.lwidentity.orig", &bFileExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (bFileExists)
            {
                DJ_LOG_VERBOSE("Restoring /etc/filesystems.lwidentity.orig file to /etc/filesystems");
                ceError = CTMoveFile("/etc/filesystems.lwidentity.orig", "/etc/filesystems");
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }
    }

    /* Revert auto.master */
    ceError = CTCheckFileExists("/etc/auto.master.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/auto.master.lwidentity.orig file to /etc/auto.master");
        ceError = CTMoveFile("/etc/auto.master.lwidentity.orig", "/etc/auto.master");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = CTCheckFileExists("/etc/auto_master.lwidentity.orig", &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists)
        {
            DJ_LOG_VERBOSE("Restoring /etc/auto_master.lwidentity.orig file to /etc/auto_master");
            ceError = CTMoveFile("/etc/auto_master.lwidentity.orig", "/etc/auto_master");
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

error:

    CT_SAFE_FREE_STRING(pszSearchPath);
    CT_SAFE_FREE_STRING(pszSudoersPath);

    return ceError;
}

static
DWORD
CanonicalizeOrganizationalUnit(
    PSTR* pszCanonicalizedOrganizationalUnit,
    PCSTR pszOrganizationalUnit,
    PCSTR pszDomainName
    )
{
    DWORD ceError = ERROR_SUCCESS;
    int EE = 0;
    PSTR comma;
    PSTR current;
    PSTR temp = NULL;
    PSTR result = NULL;
    PSTR dnsDomain = NULL;

    if (!pszOrganizationalUnit || !pszOrganizationalUnit[0])
    {
        result = NULL;
        ceError = ERROR_SUCCESS;
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
    CLEANUP_ON_DWORD_EE(ceError, EE);

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
            ceError = LW_ERROR_INVALID_OU;
            GOTO_CLEANUP_EE(EE);
        }
        equalSign[0] = 0;

        type = current;
        component = equalSign + 1;

        isDc = !strcasecmp("dc", type);
        isOu = !strcasecmp("ou", type) || !strcasecmp("cn", type);
        if (!isDc && !isOu)
        {
            ceError = LW_ERROR_INVALID_OU;
            GOTO_CLEANUP_EE(EE);
        }
        if (!isDc)
        {
            if (dnsDomain)
            {
                ceError = LW_ERROR_INVALID_OU;
                GOTO_CLEANUP_EE(EE);
            }
            if (result)
            {
                PSTR newResult;
                ceError = CTAllocateStringPrintf(&newResult, "%s/%s", component, result);
                GOTO_CLEANUP_ON_DWORD_EE(ceError, EE);
                CT_SAFE_FREE_STRING(result);
                result = newResult;
            }
            else
            {
                ceError = CTAllocateString(component, &result);
                GOTO_CLEANUP_ON_DWORD_EE(ceError, EE);
            }
        }
        else
        {
            if (!result)
            {
                ceError = LW_ERROR_INVALID_OU;
                GOTO_CLEANUP_EE(EE);
            }
            if (dnsDomain)
            {
                PSTR newDnsDomain;
                ceError = CTAllocateStringPrintf(&newDnsDomain, "%s.%s", dnsDomain, component);
                GOTO_CLEANUP_ON_DWORD_EE(ceError, EE);
                CT_SAFE_FREE_STRING(dnsDomain);
                dnsDomain = newDnsDomain;
            }
            else
            {
                ceError = CTAllocateString(component, &dnsDomain);
                GOTO_CLEANUP_ON_DWORD_EE(ceError, EE);
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
        ceError = LW_ERROR_INVALID_OU;
        GOTO_CLEANUP_EE(EE);
    }

    if (IsNullOrEmptyString(result))
    {
        ceError = LW_ERROR_INVALID_OU;
        GOTO_CLEANUP_EE(EE);
    }

    if (strcasecmp(dnsDomain, pszDomainName))
    {
        ceError = LW_ERROR_INVALID_OU;
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
        LW_RAISE_EX(exc, ERROR_BAD_FORMAT, "Invalid username", "The username '%s' is invalid because it contains a backslash. Please use UPN syntax (user@domain.com) if you wish to use a username from a different domain.", options->username);
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

    if (!IsNullOrEmptyString(getenv("LD_LIBRARY_PATH")) ||
        !IsNullOrEmptyString(getenv("LD_PRELOAD")))
    {
        if (options->warningCallback != NULL)
        {
            options->warningCallback(options, "Unsupported loader flags set",
                                 "LD_LIBRARY_PATH and/or LD_PRELOAD are currently set on your system. Best practices for Unix and Linux administration strongly recommend not to use these environmental variables. Likewise does not support environments where either variable is set.\n"
                                  "\n"
                                 "If this operation fails you should stop all likewise daemons, clear the environmental variable, then retry the join operation.\n"
                                 "\n"
                                 "For more information, see the Likewise guide online at:\n"
                                 "http://www.likewise.com/resources/documentation_library/manuals/open/likewise-open-guide.html#AgentRequirements\n"
                                 "\n"
                                 "Or a local PDF file is available in:\n"
                                 "/opt/likewise/docs/likewise-open-guide.pdf (See section 4.2 Requirements for the Agent");
        }
    }

    if (options->ouName)
    {
        LW_CLEANUP_CTERR(exc, CanonicalizeOrganizationalUnit(&pszCanonicalizedOU,
                                                 options->ouName,
                                                 options->domainName));

        CT_SAFE_FREE_STRING(options->ouName);
        options->ouName = pszCanonicalizedOU;
        pszCanonicalizedOU = NULL;
    }

    DJStartService("lsass");

    LW_TRY(exc, SetLsassTimeSync(!options->disableTimeSync, &LW_EXC));

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
    PCSTR pszDomainName,
    BOOLEAN *isValid,
    LWException **exc
    )
{
    DWORD dwError = 0;
    BOOLEAN isJoined = FALSE;
    HANDLE hLsaConnection = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    LW_CLEANUP_LSERR(exc, dwError);

    dwError = LsaAdGetMachineAccountInfo(hLsaConnection, pszDomainName, &pAccountInfo);
    switch (dwError)
    {
        case NERR_SetupNotJoined:
            isJoined = FALSE;
            dwError = 0;
            break;
        case 0:
            isJoined = TRUE;
            break;
        default:
            LW_CLEANUP_LSERR(exc, dwError);
    }

cleanup:
    if (dwError)
    {
        isJoined = FALSE;
    }

    if (pAccountInfo)
    {
        LsaAdFreeMachineAccountInfo(pAccountInfo);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    *isValid = isJoined;
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
        LW_RAISE_EX(exc, ERROR_BAD_FORMAT, "Invalid username", "The username '%s' is invalid because it contains a backslash. Please use UPN syntax (user@domain.com) if you wish to use a username from a different domain.", options->username);
        return CannotConfigure;
    }

    LW_TRY(exc, DJTestJoin(options->domainName, &joinValid, &LW_EXC));
    if(!joinValid)
    {
        result = FullyConfigured;
        goto cleanup;
    }

    result = NotConfigured;

cleanup:
    return result;
}

static void DoLeave(JoinProcessOptions *options, LWException **exc)
{
    LWException *inner = NULL;
    DJDisableComputerAccount(options->username, options->password, options, &inner);
#ifndef ENABLE_MINIMAL
    BOOLEAN nscdRunning = FALSE;
#endif

    if(!LW_IS_OK(inner))
    {
        ModuleState *state = DJGetModuleStateByName((JoinProcessOptions *)options, "leave");

        state->moduleData = (void *)2;
        LW_CLEANUP(exc, inner);
    }

#ifndef ENABLE_MINIMAL
    DJGetDaemonStatus("nscd", &nscdRunning, &inner);
    if(!LW_IS_OK(inner) && (inner->code == ERROR_SERVICE_NOT_FOUND ||
            inner->code == ERROR_FILE_NOT_FOUND))
    {
        //The daemon isn't installed
        LW_HANDLE(&inner);
        nscdRunning = FALSE;
    }
    LW_CLEANUP(exc, inner);

    if (nscdRunning)
    {
        LW_CLEANUP_CTERR(exc, CTRunCommand("/usr/sbin/nscd -i passwd"));
        LW_CLEANUP_CTERR(exc, CTRunCommand("/usr/sbin/nscd -i group"));
    }
#endif

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

void
DJGetConfiguredDnsDomain(
    PSTR* ppszDomain,
    LWException **exc
    )
{
    DWORD dwError = 0;
    PSTR pszDnsDomainName = NULL;
    HANDLE hLsaConnection = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    LW_CLEANUP_LSERR(exc, dwError);

    dwError = LsaAdGetMachineAccountInfo(hLsaConnection, NULL, &pAccountInfo);
    LW_CLEANUP_LSERR(exc, dwError);

    dwError = LwAllocateString(pAccountInfo->DnsDomainName, &pszDnsDomainName);
    LW_CLEANUP_LSERR(exc, dwError);

cleanup:
    if (dwError)
    {
        LW_SAFE_FREE_STRING(pszDnsDomainName);
    }

    if (pAccountInfo)
    {
        LsaAdFreeMachineAccountInfo(pAccountInfo);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    *ppszDomain = pszDnsDomainName;
}

void
DJGetConfiguredShortDomain(
    PSTR* ppszWorkgroup,
    LWException **exc
    )
{
    DWORD dwError = 0;
    PSTR pszNetbiosDomainName = NULL;
    HANDLE hLsaConnection = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    LW_CLEANUP_LSERR(exc, dwError);

    dwError = LsaAdGetMachineAccountInfo(hLsaConnection, NULL, &pAccountInfo);
    LW_CLEANUP_LSERR(exc, dwError);

    dwError = LwAllocateString(pAccountInfo->NetbiosDomainName, &pszNetbiosDomainName);
    LW_CLEANUP_LSERR(exc, dwError);

cleanup:
    if (dwError)
    {
        LW_SAFE_FREE_STRING(pszNetbiosDomainName);
    }

    if (pAccountInfo)
    {
        LsaAdFreeMachineAccountInfo(pAccountInfo);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    *ppszWorkgroup = pszNetbiosDomainName;
}

void
DJGetComputerDN(PSTR *dn, LWException **exc)
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    LW_CLEANUP_LSERR(exc, dwError);

    dwError = LsaAdGetComputerDn(hLsaConnection, NULL, dn);
    LW_CLEANUP_LSERR(exc, dwError);

cleanup:
    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }
}

void DJNetInitialize(BOOLEAN bEnableDcerpcd, LWException **exc)
{
}

void DJNetShutdown(LWException **exc)
{
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
    // Do not free origEnvVarStr
    PSTR origEnvVarStr = NULL;
    PSTR shortHostname = NULL;
    PSTR hostFqdn = NULL;
    // Do not free dnsDomain
    PSTR dnsDomain = NULL;
    CHAR krb5ConfEnv[256];
    DWORD dwFlags = 0;
    DWORD dwError = 0;

    PSTR likewiseProduct = NULL;
    PSTR likewiseVersion = NULL;
    PSTR likewiseBuild = NULL;
    PSTR likewiseRevision = NULL;

    PSTR likewiseOSServicePack = NULL;
    HANDLE lsa = NULL;

    memset(&distro, 0, sizeof(distro));

    LW_CLEANUP_CTERR(exc, DJGetDistroInfo(NULL, &distro));
    LW_CLEANUP_CTERR(exc, DJGetDistroString(distro.distro, &osName));

#ifdef MINIMAL_JOIN
    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&likewiseOSServicePack,
                "Likewise Open %s",
                BUILD_VERSION));
#else
    LW_CLEANUP_CTERR(exc, DJGetLikewiseVersion(&likewiseProduct,
                &likewiseVersion, &likewiseBuild, &likewiseRevision));

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&likewiseOSServicePack,
                "Likewise %s %s.%s.%s",
                likewiseProduct, likewiseVersion, likewiseBuild, likewiseRevision));
#endif

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
    if (origEnv && *origEnv)
    {
        LW_CLEANUP_CTERR(exc,
                         CTAllocateStringPrintf(
                         &origEnvVarStr,
                         "KRB5_CONFIG=%s",
                         origEnv));
    }

    sprintf(krb5ConfEnv, "KRB5_CONFIG=%s/etc/krb5.conf", tempDir);

    if (putenv(krb5ConfEnv) != 0) {
       LW_CLEANUP_CTERR(exc, LwMapErrnoToLwError(errno));
    }

    LW_CLEANUP_LSERR(exc, LWNetExtendEnvironmentForKrb5Affinity(TRUE));

    if ( options->disableTimeSync || options->enableMultipleJoins )
    {
        dwFlags |= LSA_NET_JOIN_DOMAIN_NOTIMESYNC;
    }

    if ( options->enableMultipleJoins )
    {
        dwFlags |= LSA_NET_JOIN_DOMAIN_MULTIPLE;
    }

    LW_CLEANUP_CTERR(exc, DJGetFQDN(&shortHostname, &hostFqdn));

    if (hostFqdn && (strlen(hostFqdn) > (strlen(shortHostname) + 1)))
    {
        dnsDomain = hostFqdn + strlen(shortHostname) + 1;
    }

    dwError = SetBooleanRegistryValue("Services\\lsass\\Parameters\\Providers\\ActiveDirectory",
                                   "AssumeDefaultDomain",
                                   options->assumeDefaultDomain);
    if (dwError)
    {
        LW_RAISE_LSERR(exc, dwError);
        goto cleanup;
    }

    dwError = SetStringRegistryValue("Services\\lsass\\Parameters\\Providers\\ActiveDirectory",
                                     "UserDomainPrefix",
                                     options->userDomainPrefix);
    if (dwError)
    {
        LW_RAISE_LSERR(exc, dwError);
        goto cleanup;
    }

    LW_CLEANUP_LSERR(exc, LsaOpenServer(&lsa));

    dwError = LsaAdJoinDomain(
                 lsa,
                 options->computerName,
                 dnsDomain,
                 options->domainName,
                 options->ouName,
                 options->username,
                 options->password,
                 osName,
                 distro.version,
                 likewiseOSServicePack,
                 dwFlags);
    if (dwError)
    {
        switch(dwError)
        {
            case ERROR_FILE_NOT_FOUND:
                LW_RAISE_EX(exc, LW_ERROR_INVALID_OU, "Lsass Error", "The OU is invalid.");
                break;
            case ERROR_CRC:
                LW_RAISE_EX(exc, LW_ERROR_BAD_LICENSE_KEY, "Lsass Error", "An invalid license key exists in AD");
                break;
            case ERROR_INVALID_PARAMETER:
                LW_RAISE_EX(exc, ERROR_BAD_FORMAT, "Lsass Error", "The OU format is invalid.");
                break;
            case ERROR_DS_NAME_ERROR_NO_MAPPING:
                LW_RAISE_EX(exc, ERROR_INVALID_COMPUTERNAME, "Lsass Error", "The dnsHostName attribute cannot be set on the computer account because your user account does not have permission to write arbitrary values, and your computer's domain name is not listed in the msDS-AllowedDNSSuffixes attribute.");
                break;
            case ERROR_BAD_NET_NAME:
                if (strlen(dnsDomain) > sizeof(".local") - 1 &&
                    !strcasecmp(dnsDomain + strlen(dnsDomain) -
                        sizeof(".local") + 1, ".local"))
                {
                    LW_RAISE_EX(exc, dwError, "Lsass Error", "%s. Failure to lookup a domain name ending in \".local\" may be the result of configuring the local system's hostname resolution (or equivalent) to use Multi-cast DNS. Please refer to the Likewise manual at http://www.likewise.com/resources/documentation_library/manuals/open/likewise-open-guide.html#ConfigNsswitch for more information.", LwWin32ExtErrorToDescription(dwError));
                }
                else
                {
                    LW_RAISE_LSERR(exc, dwError);
                }
                break;
            case ERROR_NOT_ENOUGH_QUOTA:
                LW_RAISE_EX(exc, ERROR_NOT_ENOUGH_QUOTA, "Lsass Error", "The account's computer join limit has been exceeded. Talk to your Windows administrators about the limits assigned to your account.");
                break;
            default:
                LW_RAISE_LSERR(exc, dwError);
                break;
        }
        goto cleanup;
    }

    LW_TRY(exc, DJGuessShortDomainName(
                                 options->domainName,
                                 shortDomainName, &LW_EXC));
cleanup:

    if (lsa)
    {
        LsaCloseServer(lsa);
    }

    if (exc && LW_IS_OK(*exc))
    {
        DJLogDomainJoinSucceededEvent(options, osName, distro.version, likewiseOSServicePack);
    }
    else
    {
        DJLogDomainJoinFailedEvent(options, osName, distro.version, likewiseOSServicePack, *exc);
    }

    if(tempDir != NULL)
    {
        CTRemoveDirectory(tempDir);
        CT_SAFE_FREE_STRING(tempDir);
    }

    if (origEnvVarStr) {
       putenv(origEnvVarStr);
    } else {
       putenv("KRB5_CONFIG=/etc/krb5.conf");
    }

    CT_SAFE_FREE_STRING(likewiseProduct);
    CT_SAFE_FREE_STRING(likewiseVersion);
    CT_SAFE_FREE_STRING(likewiseBuild);
    CT_SAFE_FREE_STRING(likewiseRevision);
    CT_SAFE_FREE_STRING(likewiseOSServicePack);
    CT_SAFE_FREE_STRING(shortHostname);
    CT_SAFE_FREE_STRING(hostFqdn);

    DJFreeDistroInfo(&distro);
}

void DJDisableComputerAccount(PCSTR username,
                PCSTR password,
                JoinProcessOptions *options,
                LWException **exc)
{
    HANDLE lsa = NULL;

    LW_CLEANUP_LSERR(exc, LsaOpenServer(&lsa));
    LW_CLEANUP_LSERR(exc, LsaAdLeaveDomain2(lsa, username, password, options->domainName, 0));

cleanup:

    if (lsa)
    {
        LsaCloseServer(lsa);
    }

    if (exc && LW_IS_OK(*exc))
    {
        DJLogDomainLeaveSucceededEvent(options);
    }
    else
    {
        DJLogDomainLeaveFailedEvent(options, *exc);
    }
}

void DJGuessShortDomainName(PCSTR longName,
                PSTR *shortName,
                LWException **exc)
{
    DWORD dwError = 0;
    PSTR pszNetbiosDomainName = NULL;
    PCSTR pszDnsDomainName = NULL;
    PSTR pszFreeDnsDomainName = NULL;
    PLWNET_DC_INFO pDcInfo = NULL;

    if (LW_IS_NULL_OR_EMPTY_STR(longName))
    {
        LW_TRY(exc, DJGetConfiguredDnsDomain(&pszFreeDnsDomainName, &LW_EXC));
        pszDnsDomainName = pszFreeDnsDomainName;
    }
    else
    {
        pszDnsDomainName = longName;
    }

    dwError = LWNetGetDCName(
                    NULL,
                    pszDnsDomainName,
                    NULL,
                    0,
                    &pDcInfo);
    LW_CLEANUP_LSERR(exc, dwError);

    if (LW_IS_NULL_OR_EMPTY_STR(pDcInfo->pszNetBIOSDomainName))
    {
        dwError = LW_ERROR_NO_NETBIOS_NAME;
        LW_CLEANUP_LSERR(exc, dwError);
    }

    dwError = LwAllocateString(
                    pDcInfo->pszNetBIOSDomainName,
                    &pszNetbiosDomainName);
    LW_CLEANUP_LSERR(exc, dwError);

cleanup:
    if (dwError)
    {
        LW_SAFE_FREE_STRING(pszNetbiosDomainName);
    }

    LW_SAFE_FREE_STRING(pszFreeDnsDomainName);
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);

    *shortName = pszNetbiosDomainName;
}

DWORD
DJGetMachineSID(
    PSTR* ppszMachineSID
    )
{
    *ppszMachineSID = NULL;
    return ERROR_SUCCESS;
}

DWORD
DJSetMachineSID(
    PSTR pszMachineSID
    )
{
    return ERROR_SUCCESS;
}

DWORD
DJOpenEventLog(
    PSTR pszCategoryType,
    PHANDLE phEventLog
    )
{
#if defined(MINIMAL_JOIN)
    return 0;
#else
    return LWIOpenEventLogEx(
                  NULL,             // Server name (defaults to local computer eventlogd)
                  pszCategoryType,  // Table Category ID (Security, System, ...)
                  "Likewise DomainJoin", // Source
                  0,                // Source Event ID
                  "SYSTEM",         // User
                  NULL,             // Computer (defaults to assigning local hostname)
                  phEventLog);
#endif
}

DWORD
DJCloseEventLog(
    HANDLE hEventLog
    )
{
#if defined(MINIMAL_JOIN)
    return 0;
#else
    return LWICloseEventLog(hEventLog);
#endif
}

DWORD
DJLogInformationEvent(
    HANDLE hEventLog,
    DWORD  dwEventID,
    PCSTR  pszUser, // NULL defaults to SYSTEM
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
#if defined(MINIMAL_JOIN)
    return 0;
#else
    EVENT_LOG_RECORD event = {0};

    event.dwEventRecordId = 0;
    event.pszEventTableCategoryId = NULL;
    event.pszEventType = INFORMATION_EVENT_TYPE;
    event.dwEventDateTime = 0;
    event.pszEventSource = NULL;
    event.pszEventCategory = (PSTR) pszCategory;
    event.dwEventSourceId = dwEventID;
    event.pszUser = (PSTR) pszUser;
    event.pszComputer = NULL;
    event.pszDescription = (PSTR) pszDescription;
    event.pszData = (PSTR) pszData;

    return LWIWriteEventLogBase(
                   hEventLog,
                   event);
#endif
}

DWORD
DJLogWarningEvent(
    HANDLE hEventLog,
    DWORD  dwEventID,
    PCSTR  pszUser, // NULL defaults to SYSTEM
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
#if defined(MINIMAL_JOIN)
    return 0;
#else
    EVENT_LOG_RECORD event = {0};

    event.dwEventRecordId = 0;
    event.pszEventTableCategoryId = NULL;
    event.pszEventType = WARNING_EVENT_TYPE;
    event.dwEventDateTime = 0;
    event.pszEventSource = NULL;
    event.pszEventCategory = (PSTR) pszCategory;
    event.dwEventSourceId = dwEventID;
    event.pszUser = (PSTR) pszUser;
    event.pszComputer = NULL;
    event.pszDescription = (PSTR) pszDescription;
    event.pszData = (PSTR) pszData;

    return LWIWriteEventLogBase(
                   hEventLog,
                   event);
#endif
}

DWORD
DJLogErrorEvent(
    HANDLE hEventLog,
    DWORD  dwEventID,
    PCSTR  pszUser, // NULL defaults to SYSTEM
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
#if defined(MINIMAL_JOIN)
    return 0;
#else
    EVENT_LOG_RECORD event = {0};

    event.dwEventRecordId = 0;
    event.pszEventTableCategoryId = NULL;
    event.pszEventType = ERROR_EVENT_TYPE;
    event.dwEventDateTime = 0;
    event.pszEventSource = NULL;
    event.pszEventCategory = (PSTR) pszCategory;
    event.dwEventSourceId = dwEventID;
    event.pszUser = (PSTR) pszUser;
    event.pszComputer = NULL;
    event.pszDescription = (PSTR) pszDescription;
    event.pszData = (PSTR) pszData;

    return LWIWriteEventLogBase(
                   hEventLog,
                   event);
#endif
}

VOID
DJLogDomainJoinSucceededEvent(
    JoinProcessOptions * JoinOptions,
    PSTR pszOSName,
    PSTR pszDistroVersion,
    PSTR pszLikewiseVersion
    )
{
    DWORD ceError = ERROR_SUCCESS;
    HANDLE hEventLog = NULL;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    ceError = DJOpenEventLog("System", &hEventLog);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateStringPrintf(
                 &pszDescription,
                 "Domain join successful.\r\n\r\n" \
                 "     Domain name:             %s\r\n" \
                 "     Domain name (short):     %s\r\n" \
                 "     Computer name:           %s\r\n" \
                 "     Organizational unit:     %s\r\n" \
                 "     Assume default domain:   %s\r\n" \
                 "     User domain prefix:      %s\r\n" \
                 "     User name:               %s\r\n" \
                 "     Operating system:        %s\r\n" \
                 "     Distribution version:    %s\r\n" \
                 "     Likewise version:        %s",
                 JoinOptions->domainName ? JoinOptions->domainName : "<not set>",
                 JoinOptions->shortDomainName ? JoinOptions->shortDomainName : "<not set>",
                 JoinOptions->computerName ? JoinOptions->computerName : "<not set>",
                 JoinOptions->ouName ? JoinOptions->ouName : "<not set>",
                 JoinOptions->assumeDefaultDomain ? "true" : "false",
                 JoinOptions->userDomainPrefix ? JoinOptions->userDomainPrefix : "<not set>",
                 JoinOptions->username ? JoinOptions->username : "<not set>",
                 pszOSName ? pszOSName : "<not set>",
                 pszDistroVersion ? pszDistroVersion : "<not set>",
                 pszLikewiseVersion ? pszLikewiseVersion : "<not set>");
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJLogInformationEvent(
                    hEventLog,
                    DOMAINJOIN_EVENT_INFO_JOINED_DOMAIN,
                    JoinOptions->username,
                    DOMAINJOIN_EVENT_CATEGORY,
                    pszDescription,
                    pszData);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    DJCloseEventLog(hEventLog);
    CT_SAFE_FREE_STRING(pszDescription);
    CT_SAFE_FREE_STRING(pszData);

    return;
}

VOID
DJLogDomainJoinFailedEvent(
    JoinProcessOptions * JoinOptions,
    PSTR pszOSName,
    PSTR pszDistroVersion,
    PSTR pszLikewiseVersion,
    LWException *exc
    )
{
    DWORD ceError = ERROR_SUCCESS;
    HANDLE hEventLog = NULL;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    ceError = DJOpenEventLog("System", &hEventLog);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateStringPrintf(
                 &pszDescription,
                 "Domain join failed.\r\n\r\n" \
                 "     Reason message:          %s\r\n" \
                 "     Reason message (long):   %s\r\n" \
                 "     Reason code:             0x%8x\r\n\r\n" \
                 "     Domain name:             %s\r\n" \
                 "     Domain name (short):     %s\r\n" \
                 "     Computer name:           %s\r\n" \
                 "     Organizational unit:     %s\r\n" \
                 "     Assume default domain:   %s\r\n" \
                 "     User domain prefix:      %s\r\n" \
                 "     User name:               %s\r\n" \
                 "     Operating system:        %s\r\n" \
                 "     Distribution version:    %s\r\n" \
                 "     Likewise version:        %s",
                 exc && exc->shortMsg ? exc->shortMsg : "<not set>",
                 exc && exc->longMsg ? exc->longMsg : "<not set>",
                 exc && exc->code ? exc->code : 0,
                 JoinOptions->domainName ? JoinOptions->domainName : "<not set>",
                 JoinOptions->shortDomainName ? JoinOptions->shortDomainName : "<not set>",
                 JoinOptions->computerName ? JoinOptions->computerName : "<not set>",
                 JoinOptions->ouName ? JoinOptions->ouName : "<not set>",
                 JoinOptions->assumeDefaultDomain ? "true" : "false",
                 JoinOptions->userDomainPrefix ? JoinOptions->userDomainPrefix : "<not set>",
                 JoinOptions->username ? JoinOptions->username : "<not set>",
                 pszOSName ? pszOSName : "<not set>",
                 pszDistroVersion ? pszDistroVersion : "<not set>",
                 pszLikewiseVersion ? pszLikewiseVersion : "<not set>");
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJLogErrorEvent(
                    hEventLog,
                    DOMAINJOIN_EVENT_ERROR_DOMAIN_JOIN_FAILURE,
                    JoinOptions->username,
                    DOMAINJOIN_EVENT_CATEGORY,
                    pszDescription,
                    pszData);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    DJCloseEventLog(hEventLog);
    CT_SAFE_FREE_STRING(pszDescription);
    CT_SAFE_FREE_STRING(pszData);

    return;
}

VOID
DJLogDomainLeaveSucceededEvent(
    JoinProcessOptions * JoinOptions
    )
{
    DWORD ceError = ERROR_SUCCESS;
    HANDLE hEventLog = NULL;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    ceError = DJOpenEventLog("System", &hEventLog);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateStringPrintf(
                 &pszDescription,
                 "Domain leave succeeded.\r\n\r\n" \
                 "     Domain name:             %s\r\n" \
                 "     Domain name (short):     %s\r\n" \
                 "     Computer name:           %s\r\n" \
                 "     Organizational unit:     %s\r\n" \
                 "     User name:               %s\r\n",
                 JoinOptions->domainName ? JoinOptions->domainName : "<not set>",
                 JoinOptions->shortDomainName ? JoinOptions->shortDomainName : "<not set>",
                 JoinOptions->computerName ? JoinOptions->computerName : "<not set>",
                 JoinOptions->ouName ? JoinOptions->ouName : "<not set>",
                 JoinOptions->username ? JoinOptions->username : "<not set>");
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJLogInformationEvent(
                    hEventLog,
                    DOMAINJOIN_EVENT_INFO_LEFT_DOMAIN,
                    JoinOptions->username,
                    DOMAINJOIN_EVENT_CATEGORY,
                    pszDescription,
                    pszData);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    DJCloseEventLog(hEventLog);
    CT_SAFE_FREE_STRING(pszDescription);
    CT_SAFE_FREE_STRING(pszData);

    return;
}

VOID
DJLogDomainLeaveFailedEvent(
    JoinProcessOptions * JoinOptions,
    LWException *exc
    )
{
    DWORD ceError = ERROR_SUCCESS;
    HANDLE hEventLog = NULL;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    ceError = DJOpenEventLog("System", &hEventLog);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateStringPrintf(
                 &pszDescription,
                 "Domain leave failed.\r\n\r\n" \
                 "     Reason message:          %s\r\n" \
                 "     Reason message (long):   %s\r\n" \
                 "     Reason code:             0x%8x\r\n\r\n" \
                 "     Domain name:             %s\r\n" \
                 "     Domain name (short):     %s\r\n" \
                 "     Computer name:           %s\r\n" \
                 "     Organizational unit:     %s\r\n" \
                 "     User name:               %s",
                 exc && exc->shortMsg ? exc->shortMsg : "<not set>",
                 exc && exc->longMsg ? exc->longMsg : "<not set>",
                 exc && exc->code ? exc->code : 0,
                 JoinOptions->domainName ? JoinOptions->domainName : "<not set>",
                 JoinOptions->shortDomainName ? JoinOptions->shortDomainName : "<not set>",
                 JoinOptions->computerName ? JoinOptions->computerName : "<not set>",
                 JoinOptions->ouName ? JoinOptions->ouName : "<not set>",
                 JoinOptions->username ? JoinOptions->username : "<not set>");
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJLogErrorEvent(
                    hEventLog,
                    DOMAINJOIN_EVENT_ERROR_DOMAIN_LEAVE_FAILURE,
                    JoinOptions->username,
                    DOMAINJOIN_EVENT_CATEGORY,
                    pszDescription,
                    pszData);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    DJCloseEventLog(hEventLog);
    CT_SAFE_FREE_STRING(pszDescription);
    CT_SAFE_FREE_STRING(pszData);

    return;
}

void SetLsassTimeSync(BOOLEAN sync, LWException **exc)
{
    LW_CLEANUP_LSERR(
        exc,
        SetBooleanRegistryValue(
            "Services\\lsass\\Parameters\\Providers\\ActiveDirectory",
            "SyncSystemTime",
            sync));
cleanup:
    return;
}
