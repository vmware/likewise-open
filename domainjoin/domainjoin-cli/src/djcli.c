/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

/* ex: set tabstop=8 softtabstop=4 expandtab shiftwidth=4: */
#include "domainjoin.h"
#include "djcli.h"
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <stdlib.h>

typedef enum
{
    PARSE_STATE_OPEN = 0,
    PARSE_STATE_SETNAME,
    PARSE_STATE_JOIN,
    PARSE_STATE_SYNC_TIME,
    PARSE_STATE_LOG,
    PARSE_STATE_LOG_LEVEL,
    PARSE_STATE_CONFIGURE,
    PARSE_STATE_TEST_PREFIX,
    PARSE_STATE_LONG_DOMAIN,
    PARSE_STATE_SHORT_DOMAIN,
    PARSE_STATE_ERROR,
    PARSE_STATE_PROGRAM,
    PARSE_STATE_CMD,
    PARSE_STATE_OWNER,
    PARSE_STATE_DONE
} ParseState;

typedef enum
{
   PARSE_STATE_JOIN_OPEN = 0,
   PARSE_STATE_JOIN_OU,
   PARSE_STATE_JOIN_ENABLE_MODULE,
   PARSE_STATE_JOIN_DISABLE_MODULE,
   PARSE_STATE_JOIN_DOMAINNAME,
   PARSE_STATE_JOIN_USERNAME,
   PARSE_STATE_JOIN_PASSWORD,
   PARSE_STATE_JOIN_DETAILS
} JoinParseState;

typedef enum
{
   PARSE_STATE_TIME_SERVER = 0,
   PARSE_STATE_TIME_DRIFT,
} TimeParseState;

void
FreeTaskInfo(
    PTASKINFO pTaskInfo
    )
{
    if (pTaskInfo) {

        if (pTaskInfo->pszComputerName)
            CTFreeString(pTaskInfo->pszComputerName);

        if (pTaskInfo->pszUserName)
            CTFreeString(pTaskInfo->pszUserName);

        if (pTaskInfo->pszPassword)
            CTFreeString(pTaskInfo->pszPassword);

        if (pTaskInfo->pszDomainName)
            CTFreeString(pTaskInfo->pszDomainName);

        if (pTaskInfo->pszOU)
           CTFreeString(pTaskInfo->pszOU);

        if (pTaskInfo->pszLogFilePath)
            CTFreeString(pTaskInfo->pszLogFilePath);

        CT_SAFE_FREE_STRING(pTaskInfo->pszTestPrefix);
        CT_SAFE_FREE_STRING(pTaskInfo->pszProgramName);
        CT_SAFE_FREE_STRING(pTaskInfo->pszCmdName);
        CT_SAFE_FREE_STRING(pTaskInfo->pszShortName);

        CTArrayFree(&pTaskInfo->enableModules);
        CTArrayFree(&pTaskInfo->disableModules);

        CTFreeMemory(pTaskInfo);
    }
}

CENTERROR
GetTaskInfo(
    int argc,
    char* argv[],
    PTASKINFO* ppTaskInfo
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PTASKINFO pTaskInfo = NULL;
    ParseState pState = PARSE_STATE_OPEN;
    JoinParseState joinPState = PARSE_STATE_JOIN_OPEN;
    TimeParseState timePState = PARSE_STATE_TIME_SERVER;
    DWORD     iArg = 1;
    PSTR      pArg = NULL;

    ceError = CTAllocateMemory(sizeof(TASKINFO), (PVOID*)&pTaskInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pTaskInfo->bNoLog = FALSE;
    pTaskInfo->dwLogLevel = LOG_LEVEL_WARNING;
    pTaskInfo->dwEnable = ENABLE_TYPE_AUTO;
    pTaskInfo->owner = -1;

    while (iArg < argc && pState != PARSE_STATE_DONE) {

        pArg = argv[iArg++];

        switch(pState)
        {
        case PARSE_STATE_OPEN:
	{
            if (!strcmp(pArg, "setname")) {

                pTaskInfo->dwTaskType = TASK_TYPE_SETNAME;
                pState = PARSE_STATE_SETNAME;

            } else if (!strcmp(pArg, "sync_time")) {

                pTaskInfo->dwTaskType = TASK_TYPE_SYNC_TIME;
                pState = PARSE_STATE_SYNC_TIME;
                timePState = PARSE_STATE_TIME_SERVER;

            } else if (!strcmp(pArg, "join")) {

                pTaskInfo->dwTaskType = TASK_TYPE_JOIN;
                pState = PARSE_STATE_JOIN;
                joinPState = PARSE_STATE_JOIN_OPEN;

            } else if (!strcmp(pArg, "joinnew")) {

                pTaskInfo->dwTaskType = TASK_TYPE_JOIN_NEW;
                pState = PARSE_STATE_JOIN;
                joinPState = PARSE_STATE_JOIN_OPEN;

            } else if (!strcmp(pArg, "leave")) {

                pTaskInfo->dwTaskType = TASK_TYPE_LEAVE;

            } else if (!strcmp(pArg, "--advanced")) {

                pTaskInfo->bAdvancedMode = TRUE;

            } else if (!strcmp(pArg, "--help")) {

                pTaskInfo->dwTaskType = TASK_TYPE_USAGE;
                pState = PARSE_STATE_DONE;

            } else if (!strcmp(pArg, "--help-internal")) {

                pTaskInfo->dwTaskType = TASK_TYPE_USAGE_INTERNAL;
                pState = PARSE_STATE_DONE;

            } else if (!strcmp(pArg, "--log")) {

                pState = PARSE_STATE_LOG;

            } else if (!strcmp(pArg, "--loglevel")) {

                pState = PARSE_STATE_LOG_LEVEL;

            } else if (!strcmp(pArg, "--nolog")) {

                pTaskInfo->bNoLog = TRUE;

                CT_SAFE_FREE_STRING(pTaskInfo->pszLogFilePath);

            } else if (!strcmp(pArg, "query")) {

                pTaskInfo->dwTaskType = TASK_TYPE_QUERY;

            } else if (!strcmp(pArg, "fixfqdn")) {

                pTaskInfo->dwTaskType = TASK_TYPE_FIXFQDN;

            } else if (!strcmp(pArg, "configure")) {

                pState = PARSE_STATE_CONFIGURE;

            } else if (!strcmp(pArg, "get_os_type")) {

                pTaskInfo->dwTaskType = TASK_TYPE_GETOSTYPE;

            } else if (!strcmp(pArg, "get_arch")) {

                pTaskInfo->dwTaskType = TASK_TYPE_GETARCH;

            } else if (!strcmp(pArg, "get_distro")) {

                pTaskInfo->dwTaskType = TASK_TYPE_GETDISTRO;

            } else if (!strcmp(pArg, "get_distro_version")) {

                pTaskInfo->dwTaskType = TASK_TYPE_GETDISTROVERSION;

            } else if (!strcmp(pArg, "ps")) {

                pTaskInfo->dwTaskType = TASK_TYPE_PS;

            } else if (!strcmp(pArg, "raise_error")) {

		pTaskInfo->dwTaskType = TASK_TYPE_RAISEERROR;
		pState = PARSE_STATE_ERROR;

	    } else if (!strcmp(pArg, "--testprefix")) {

                pState = PARSE_STATE_TEST_PREFIX;

            } else if (!strcmp(pArg, "--long")) {

                pState = PARSE_STATE_LONG_DOMAIN;

            } else if (!strcmp(pArg, "--short")) {

                pState = PARSE_STATE_SHORT_DOMAIN;

            } else if (!strcmp(pArg, "--autoenable")) {

                pTaskInfo->dwEnable = ENABLE_TYPE_AUTO;

            } else if (!strcmp(pArg, "--enable")) {

                pTaskInfo->dwEnable = ENABLE_TYPE_ENABLE;

            } else if (!strcmp(pArg, "--disable")) {

                pTaskInfo->dwEnable = ENABLE_TYPE_DISABLE;

            } else if (!strcmp(pArg, "--program")) {

                pState = PARSE_STATE_PROGRAM;

            } else if (!strcmp(pArg, "--cmd")) {

                pState = PARSE_STATE_CMD;

            } else if (!strcmp(pArg, "--owner")) {

                pState = PARSE_STATE_OWNER;

            } else {

                pTaskInfo->dwTaskType = TASK_TYPE_USAGE;
                pState = PARSE_STATE_DONE;

            }
	}
	break;
    case PARSE_STATE_SYNC_TIME:
	{
        switch(timePState)
        {
            case PARSE_STATE_TIME_SERVER:
            {
                ceError = CTAllocateString(pArg,
                                    &pTaskInfo->pszTimeServer);
                BAIL_ON_CENTERIS_ERROR(ceError);
                timePState = PARSE_STATE_TIME_DRIFT;
            }
            break;
            case PARSE_STATE_TIME_DRIFT:
            {
                PSTR endPtr;
                pTaskInfo->allowedDrift = strtoul(pArg, &endPtr, 10);
                if(*endPtr != '\0')
                {
                    ceError = CENTERROR_INVALID_OPTION_VALUE;
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
                pState = PARSE_STATE_OPEN;
            }
            break;
        }
	}
	break;
        case PARSE_STATE_JOIN:
	{
           /*
            * join [--ou <organizationalUnit>] <domain name> <username> [<password>]
            */

           switch(joinPState)
           {
              case PARSE_STATE_JOIN_OPEN:
              {
                 if (!strcmp(pArg, "--ou"))
                 {
                    joinPState = PARSE_STATE_JOIN_OU;
                 }
                 else if (!strcmp(pArg, "--nohosts"))
                 {
                    pTaskInfo->bDoNotChangeHosts = TRUE;
                 }
                 else if (!strcmp(pArg, "--disable"))
                 {
                    joinPState = PARSE_STATE_JOIN_DISABLE_MODULE;
                 }
                 else if (!strcmp(pArg, "--enable"))
                 {
                    joinPState = PARSE_STATE_JOIN_ENABLE_MODULE;
                 }
                 else
                 {
                    ceError = CTAllocateString(pArg,
                                               &pTaskInfo->pszDomainName);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                    joinPState = PARSE_STATE_JOIN_USERNAME;
                 }
              }
              break;
              case PARSE_STATE_JOIN_DISABLE_MODULE:
              case PARSE_STATE_JOIN_ENABLE_MODULE:
              {
                  DynamicArray *addArray;
                  if(CTArrayFindString(&pTaskInfo->enableModules, pArg) != -1 ||
                      CTArrayFindString(&pTaskInfo->disableModules, pArg) != -1)
                  {
                      fprintf(stderr, "Module %s was already specified\n", pArg);
                      BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_INVALID_OPTION_VALUE);
                  }
                  if(joinPState == PARSE_STATE_JOIN_ENABLE_MODULE)
                      addArray = &pTaskInfo->enableModules;
                  else
                      addArray = &pTaskInfo->disableModules;
                  ceError = CTArrayAppend(addArray, sizeof(PCSTR *), &pArg, 1);
                  BAIL_ON_CENTERIS_ERROR(ceError);
                  joinPState = PARSE_STATE_JOIN_OPEN;
              }
              break;
              case PARSE_STATE_JOIN_OU:
              {
                 ceError = CTAllocateString(pArg,
                                            &pTaskInfo->pszOU);
                 BAIL_ON_CENTERIS_ERROR(ceError);
                 joinPState = PARSE_STATE_JOIN_DOMAINNAME;
              }
              break;
              case PARSE_STATE_JOIN_DOMAINNAME:
              {
                 ceError = CTAllocateString(pArg,
                                            &pTaskInfo->pszDomainName);
                 BAIL_ON_CENTERIS_ERROR(ceError);
                 joinPState = PARSE_STATE_JOIN_USERNAME;
              }
              break;
              case PARSE_STATE_JOIN_USERNAME:
              if (!strcmp(pArg, "--preview"))
              {
                pTaskInfo->dwTaskType = TASK_TYPE_JOIN_LIST_MODULES;
                pState = PARSE_STATE_OPEN;
              }
              else if(!strcmp(pArg, "--details"))
              {
                pTaskInfo->dwTaskType = TASK_TYPE_JOIN_DETAILS;
                joinPState = PARSE_STATE_JOIN_DETAILS;
              }
              else
              {
		ceError = CTAllocateString(pArg, &pTaskInfo->pszUserName);
		BAIL_ON_CENTERIS_ERROR(ceError);
                joinPState = PARSE_STATE_JOIN_PASSWORD;
              }
              break;
              case PARSE_STATE_JOIN_DETAILS:
              {
                 pTaskInfo->detailsModule = pArg;
                 pState = PARSE_STATE_OPEN;
              }
              break;
              case PARSE_STATE_JOIN_PASSWORD:
              {
                 ceError = CTAllocateString(pArg, &pTaskInfo->pszPassword);
                 BAIL_ON_CENTERIS_ERROR(ceError);

                 pState = PARSE_STATE_OPEN;
              }
              break;
           }
	}
	break;
        case PARSE_STATE_SETNAME:
	{
            ceError = CTAllocateString(pArg, &pTaskInfo->pszComputerName);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pState = PARSE_STATE_OPEN;
	}
	break;

	case PARSE_STATE_ERROR:
	{
	    if (pArg[0] && pArg[0] == '0' &&
		pArg[1] && pArg[1] == 'x')
		pTaskInfo->ceError = (CENTERROR) strtoul(pArg+2, NULL, 16);
	    else if (isdigit((int) *pArg))
		pTaskInfo->ceError = (CENTERROR) strtoul(pArg, NULL, 10);
	    else
		pTaskInfo->ceError = (CENTERROR) CTErrorFromName(pArg);
            pState = PARSE_STATE_OPEN;
	}
	break;

        case PARSE_STATE_LOG:
	{
            CT_SAFE_FREE_STRING(pTaskInfo->pszLogFilePath);
            ceError = CTAllocateString(pArg, &pTaskInfo->pszLogFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pTaskInfo->bNoLog = FALSE;

            pState = PARSE_STATE_OPEN;
	}
	break;

        case PARSE_STATE_TEST_PREFIX:
	{
            ceError = CTAllocateString(pArg, &pTaskInfo->pszTestPrefix);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pState = PARSE_STATE_OPEN;
	}
	break;
	
        case PARSE_STATE_LONG_DOMAIN:
	{
            ceError = CTAllocateString(pArg, &pTaskInfo->pszDomainName);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pState = PARSE_STATE_OPEN;
	}
	break;

        case PARSE_STATE_PROGRAM:
	{
            ceError = CTAllocateString(pArg, &pTaskInfo->pszProgramName);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pState = PARSE_STATE_OPEN;
	}
	break;

        case PARSE_STATE_CMD:
	{
            ceError = CTAllocateString(pArg, &pTaskInfo->pszCmdName);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pState = PARSE_STATE_OPEN;
	}
	break;

        case PARSE_STATE_OWNER:
        {
            PSTR endPtr;
            pTaskInfo->owner = (uid_t)strtol(pArg, &endPtr, 10);
            if(*endPtr != '\0')
            {
                ceError = CENTERROR_INVALID_OPTION_VALUE;
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
            pState = PARSE_STATE_OPEN;
        }
        break;
	
        case PARSE_STATE_SHORT_DOMAIN:
	{
            ceError = CTAllocateString(pArg, &pTaskInfo->pszShortName);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pState = PARSE_STATE_OPEN;
	}
	break;
	
        case PARSE_STATE_CONFIGURE:
	{
            if (!strcasecmp(pArg, "pam"))
                pTaskInfo->dwTaskType = TASK_TYPE_CONFIGURE_PAM;
            else if (!strcasecmp(pArg, "nsswitch"))
                pTaskInfo->dwTaskType = TASK_TYPE_CONFIGURE_NSSWITCH;
            else if (!strcasecmp(pArg, "ssh"))
                pTaskInfo->dwTaskType = TASK_TYPE_CONFIGURE_SSH;
            else if (!strcasecmp(pArg, "krb5"))
                pTaskInfo->dwTaskType = TASK_TYPE_CONFIGURE_KRB5;
            else if (!strcasecmp(pArg, "firewall"))
                pTaskInfo->dwTaskType = TASK_TYPE_CONFIGURE_FIREWALL;
            else {
                ceError = CENTERROR_INVALID_OPTION_VALUE;
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            pState = PARSE_STATE_OPEN;
	}
	break;

        case PARSE_STATE_LOG_LEVEL:
	{
            if (!strcasecmp(pArg, "error"))
                pTaskInfo->dwLogLevel = LOG_LEVEL_ERROR;
            else if (!strcasecmp(pArg, "warning"))
                pTaskInfo->dwLogLevel = LOG_LEVEL_WARNING;
            else if (!strcasecmp(pArg, "info"))
                pTaskInfo->dwLogLevel = LOG_LEVEL_INFO;
            else if (!strcasecmp(pArg, "verbose"))
                pTaskInfo->dwLogLevel = LOG_LEVEL_VERBOSE;
            else {
                ceError = CENTERROR_DOMAINJOIN_INVALID_LOG_LEVEL;
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            pState = PARSE_STATE_OPEN;
	}
	break;
    case PARSE_STATE_DONE:
    default:
        // These should never happen
        break;
        }
    }

    if (!pTaskInfo->bNoLog &&
        IsNullOrEmptyString(pTaskInfo->pszLogFilePath)) {

        ceError = CTAllocateString("/tmp/lwidentity.join.log",
                                   &pTaskInfo->pszLogFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);

    }

    *ppTaskInfo = pTaskInfo;

    return ceError;

error:

    if (pTaskInfo) {
        FreeTaskInfo(pTaskInfo);
    }

    return ceError;
}

static
CENTERROR
GetDaysFromEpoch(
    PDWORD pdwDaysFromEpoch
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct timeval tv;

    if (gettimeofday(&tv, NULL) < 0) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* days since the start of the epoch (Jan 1, 1970) */
    *pdwDaysFromEpoch = (int) (tv.tv_sec / (60*60*24));

error:

    return ceError;
}

CENTERROR
DoFixFqdn(
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PDOMAINJOININFO pDomainJoinInfo = NULL;

    ceError = QueryInformation(&pDomainJoinInfo);

    if (CENTERROR_IS_OK(ceError) &&
        pDomainJoinInfo &&
        !IsNullOrEmptyString(pDomainJoinInfo->pszName) &&
        !IsNullOrEmptyString(pDomainJoinInfo->pszDomainName))
    {
       ceError = DJSetComputerName(pDomainJoinInfo->pszName,
                                   pDomainJoinInfo->pszDomainName);
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (pDomainJoinInfo)
        FreeDomainJoinInfo(pDomainJoinInfo);

    return CENTERROR_SUCCESS;
}

CENTERROR
DoQuery(
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PDOMAINJOININFO pDomainJoinInfo = NULL;
    PLICENSEINFO pLicenseInfo = NULL;
    PSTR dn = NULL;

    ceError = QueryInformation(&pDomainJoinInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!IsNullOrEmptyString(pDomainJoinInfo->pszName)) {
        fprintf(stdout, "Name = %s\n", pDomainJoinInfo->pszName);
    } else {
        fprintf(stdout, "Name =\n");
    }

    if (!IsNullOrEmptyString(pDomainJoinInfo->pszDomainName)) {
        fprintf(stdout, "Domain = %s\n", pDomainJoinInfo->pszDomainName);

        ceError = DJGetComputerDN(&dn);
        BAIL_ON_CENTERIS_ERROR(ceError);

        fprintf(stdout, "Distinguished Name = %s\n", dn);
    } else {
        fprintf(stdout, "Domain =\n");
    }

error:

    if (pDomainJoinInfo)
        FreeDomainJoinInfo(pDomainJoinInfo);

    if (pLicenseInfo)
        CTFreeMemory(pLicenseInfo);

    CT_SAFE_FREE_STRING(dn);

    return ceError;
}

CENTERROR
DoSetName(
    PTASKINFO pTaskInfo
    )
{
    return DJSetComputerName(pTaskInfo->pszComputerName, NULL);
}

CENTERROR
DoLeave(
    PTASKINFO pTaskInfo
    )
{
    return JoinWorkgroup("WORKGROUP", "empty", "");
}

CENTERROR
GetPassword(
    PSTR* ppszPassword
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[129];
    DWORD idx = 0;
    struct termios old, new;
    CHAR ch;

    memset(szBuf, 0, sizeof(szBuf));

    tcgetattr(0, &old);
    memcpy(&new, &old, sizeof(struct termios));
    new.c_lflag &= ~(ECHO);
    tcsetattr(0, TCSANOW, &new);

    while ( (idx < 128) ) {

        if (read(0, &ch, 1)) {

            if (ch != '\n') {

                szBuf[idx++] = ch;

            } else {

                break;

            }

        } else {

            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);

        }
    }

    if (idx == 128) {
        ceError = CTMapSystemError(ENOBUFS);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (idx > 0) {

        ceError = CTAllocateString(szBuf, ppszPassword);
        BAIL_ON_CENTERIS_ERROR(ceError);

    } else {

        *ppszPassword = NULL;

    }

error:

    tcsetattr(0, TCSANOW, &old);

    return ceError;
}
