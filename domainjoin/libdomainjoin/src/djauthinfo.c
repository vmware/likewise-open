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
#include <lsa/join.h>
#include <lsa/lsa.h>
#include <lwnet.h>
#include <eventlog.h>

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

    size_t bufferSize;
    bufferSize = LsaGetErrorString(code, NULL, 0);
    LW_CLEANUP_CTERR(dest, CTAllocateMemory(bufferSize, PPCAST(&buffer)));
    if (LsaGetErrorString(code, buffer, bufferSize) == bufferSize && bufferSize > 0 && strlen(buffer) > 0)
    {
        DWORD err = CENTERROR_DOMAINJOIN_LSASS_ERROR;

        switch (code)
        {
            case 0x806B: //LSA_ERROR_FAILED_TO_LOOKUP_DC
                err = CENTERROR_DOMAINJOIN_UNRESOLVED_DOMAIN_NAME;
                break;
        }

        LWRaiseEx(dest, err, file, line, "Lsass Error", buffer);
        (*dest)->subcode = code;
        goto cleanup;
    }

    LWRaiseEx(dest, CENTERROR_DOMAINJOIN_LSASS_ERROR, file, line, "Unable to convert lsass error", "Lsass error code %X has occurred, but an error string cannot be retrieved", code);
    (*dest)->subcode = code;

cleanup:
    CT_SAFE_FREE_STRING(buffer);
}

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

CENTERROR
DJRemoveCacheFiles()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;
    BOOLEAN bDirExists = FALSE;
    CHAR szSudoOrigFile[PATH_MAX+1];
    PSTR pszSearchPath = NULL;
    PSTR pszSudoersPath = NULL;
    int i = 0;
    PSTR file = NULL;
    PSTR pszCachePath = NULL;;

    PSTR filePaths[] = {
        /* Likewise 4.X cache location files ... */
        "/var/lib/lwidentity/idmap_cache.tdb",
        "/var/lib/lwidentity/netsamlogon_cache.tdb",
        "/var/lib/lwidentity/winbindd_cache.tdb",
        /* Likewise 5.0 cache location files... */
        LOCALSTATEDIR "/lib/likewise/db/lsass-adcache.db",
        LOCALSTATEDIR "/lib/likewise/db/lsass-adstate.db",
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

    /* Revert any system configuration files that may have been changed by previous domain GPOs */

    /* /etc/likewise/lsassd.conf */
    ceError = CTCheckFileExists("/etc/likewise/lsassd.conf.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/likewise/lsassd.conf.lwidentity.orig file to /etc/likewise/lsassd.conf");
        ceError = CTMoveFile("/etc/likewise/lsassd.conf.lwidentity.orig", "/etc/likewise/lsassd.conf");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* /etc/likewise/lwedsplugin.conf */
    ceError = CTCheckFileExists("/etc/likewise/lwedsplugin.conf.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/likewise/lwedsplugin.conf.lwidentity.orig file to /etc/likewise/lwedsplugin.conf");
        ceError = CTMoveFile("/etc/likewise/lwedsplugin.conf.lwidentity.orig", "/etc/likewise/lwedsplugin.conf");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* /etc/likewise/grouppolicy-settings.conf */
    ceError = CTCheckFileExists("/etc/likewise/grouppolicy-settings.conf.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/likewise/grouppolicy-settings.conf.lwidentity.orig file to /etc/likewise/grouppolicy-settings.conf");
        ceError = CTMoveFile("/etc/likewise/grouppolicy-settings.conf.lwidentity.orig", "/etc/likewise/grouppolicy-settings.conf");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* /etc/likewise/eventlogd.conf */
    ceError = CTCheckFileExists("/etc/likewise/eventlogd.conf.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/likewise/eventlogd.conf.lwidentity.orig file to /etc/likewise/eventlogd.conf");
        ceError = CTMoveFile("/etc/likewise/eventlogd.conf.lwidentity.orig", "/etc/likewise/eventlogd.conf");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }


    /* /etc/sudoers */
    ceError = CTAllocateString( "/usr/local/etc:/usr/etc:/etc:/opt/sudo/etc:/opt/csw/etc",
                                &pszSearchPath);

    FixCfgString(pszSearchPath);

    ceError = CTFindFileInPath( "sudoers",
                                 pszSearchPath,
                                 &pszSudoersPath);

    if( ceError == CENTERROR_FILE_NOT_FOUND )
        ceError = CENTERROR_SUCCESS;

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

    /* Revert login.defs */
    ceError = CTCheckFileExists("/etc/login.defs.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/login.defs.lwidentity.orig file to /etc/login.defs");
        ceError = CTMoveFile("/etc/login.defs.lwidentity.orig", "/etc/login.defs");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

#if defined (__LWI_SOLARIS__)
    /*Revert shadow file -- only for Solaris*/
    ceError = CTCheckFileExists("/etc/shadow.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/shadow.lwidentity.orig file to /etc/shadow");
        ceError = CTMoveFile("/etc/shadow.lwidentity.orig", "/etc/shadow");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTCheckFileExists("/etc/default/passwd.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/default/passwd.lwidentity.orig file to /etc/default/passwd");
        ceError = CTMoveFile("/etc/default/passwd.lwidentity.orig", "/etc/default/passwd");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
#endif

#if defined (__LWI_AIX__)
    /*Revert /etc/passwd and /etc/security/user -- only for AIX*/
    ceError = CTCheckFileExists("/etc/security/user.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/security/user.lwidentity.orig file to /etc/security/user");
        ceError = CTMoveFile("/etc/security/user.lwidentity.orig", "/etc/security/user");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTCheckFileExists("/etc/security/passwd.lwidentity.orig", &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Restoring /etc/security/passwd.lwidentity.orig file to /etc/security/passwd");
        ceError = CTMoveFile("/etc/security/passwd.lwidentity.orig", "/etc/security/passwd");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
#endif

error:

    CT_SAFE_FREE_STRING(pszSearchPath);
    CT_SAFE_FREE_STRING(pszSudoersPath);

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
    LW_CLEANUP_LSERR(exc, LsaNetTestJoinDomain(isValid));
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

    result = NotConfigured;

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

void
DJGetConfiguredDnsDomain(
    PSTR* ppszDomain,
    LWException **exc
    )
{
    DWORD _err = LsaGetDnsDomainName(ppszDomain);
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
cleanup:
    ;
}

void
DJGetConfiguredShortDomain(
    PSTR* ppszWorkgroup,
    LWException **exc
    )
{
    PSTR longDomain = NULL;
    LW_CLEANUP_LSERR(exc, LsaGetDnsDomainName(&longDomain));
    LW_CLEANUP_LSERR(exc, LsaNetGetShortDomainName(longDomain, ppszWorkgroup));

cleanup:
    CT_SAFE_FREE_STRING(longDomain);
}

void
DJGetDomainDC(PCSTR domain, PSTR *dc, LWException **exc)
{
    LW_CLEANUP_LSERR(exc, LsaNetGetDCName(domain, dc));
cleanup:
    ;
}

void
DJGetComputerDN(PSTR *dn, LWException **exc)
{
    DWORD _err = 0;

    LW_CLEANUP_LSERR(exc, LWNetExtendEnvironmentForKrb5Affinity(FALSE));

    _err = LsaGetComputerDN(dn);
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
cleanup:
    ;
}

void DJNetInitialize(BOOLEAN bEnableDcerpcd, LWException **exc)
{
    BOOLEAN systemDcedExists = FALSE;
    LWException *innerExc = NULL;

#ifndef MINIMAL_JOIN
    if (geteuid() == 0)
    {
        LW_TRY(exc, DJManageDaemon("netlogond", TRUE,
                    92, 8, &LW_EXC));
        LW_TRY(exc, DJManageDaemon("lwiod", TRUE,
                    92, 10, &LW_EXC));

        if (bEnableDcerpcd)
        {
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

            DJManageDaemon("eventlogd", TRUE, 92, 11, &innerExc);
            if (!LW_IS_OK(innerExc) && innerExc->code != CENTERROR_DOMAINJOIN_MISSING_DAEMON)
            {
                DJLogException(LOG_LEVEL_WARNING, innerExc);
            }
        }
    }

#if 0
        LW_TRY(exc, DJManageDaemon("srvsvcd", TRUE,
                    92, 12, &LW_EXC));
#endif
#endif

        LW_CLEANUP_LSERR(exc, LsaNetJoinInitialize());
#if 0
        /* Do not enable debug logging in lsajoin because
           it does not respect domainjoin logging settings
           such as logfile */
        if(gdjLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE)
            lsaFunctions->pfnEnableDebugLog();
#endif

cleanup:
    LWHandle(&innerExc);
}

void DJNetShutdown(LWException **exc)
{
    LsaNetJoinShutdown();
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
    PSTR shortHostname = NULL;
    PSTR hostFqdn = NULL;
    // Do not free dnsDomain
    PSTR dnsDomain = NULL;
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

    LW_CLEANUP_CTERR(exc, DJGetFQDN(&shortHostname, &hostFqdn));

    if (hostFqdn && (strlen(hostFqdn) > (strlen(shortHostname) + 1)))
    {
        dnsDomain = hostFqdn + strlen(shortHostname) + 1;
    }

    err = LsaNetJoinDomain(
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
cleanup:

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

    if (origEnv && *origEnv) {
       putenv(origEnv);
    } else {
       putenv("KRB5_CONFIG=/etc/krb5.conf");
    }

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
    LW_CLEANUP_LSERR(exc, LsaNetLeaveDomain(username, password));

cleanup:

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
    LW_CLEANUP_LSERR(exc, LsaNetGetShortDomainName(longName, shortName));
cleanup:
    ;
}

CENTERROR
DJGetMachineSID(
    PSTR* ppszMachineSID
    )
{
    *ppszMachineSID = NULL;
    return CENTERROR_SUCCESS;
}

CENTERROR
DJSetMachineSID(
    PSTR pszMachineSID
    )
{
    return CENTERROR_SUCCESS;
}

DWORD
DJOpenEventLog(
    PSTR pszCategoryType,
    PHANDLE phEventLog
    )
{
    return LWIOpenEventLogEx(
                  NULL,             // Server name (defaults to local computer eventlogd)
                  pszCategoryType,  // Table Category ID (Security, System, ...)
                  "Likewise DomainJoin", // Source
                  0,                // Source Event ID
                  "SYSTEM",         // User
                  NULL,             // Computer (defaults to assigning local hostname)
                  phEventLog);
}

DWORD
DJCloseEventLog(
    HANDLE hEventLog
    )
{
    return LWICloseEventLog(hEventLog);
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
}

VOID
DJLogDomainJoinSucceededEvent(
    JoinProcessOptions * JoinOptions,
    PSTR pszOSName,
    PSTR pszDistroVersion,
    PSTR pszLikewiseVersion
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
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
                 "     User name:               %s\r\n" \
                 "     Operating system:        %s\r\n" \
                 "     Distribution version:    %s\r\n" \
                 "     Likewise version:        %s",
                 JoinOptions->domainName ? JoinOptions->domainName : "<not set>",
                 JoinOptions->shortDomainName ? JoinOptions->shortDomainName : "<not set>",
                 JoinOptions->computerName ? JoinOptions->computerName : "<not set>",
                 JoinOptions->ouName ? JoinOptions->ouName : "<not set>",
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
    CENTERROR ceError = CENTERROR_SUCCESS;
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
    CENTERROR ceError = CENTERROR_SUCCESS;
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
    CENTERROR ceError = CENTERROR_SUCCESS;
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
