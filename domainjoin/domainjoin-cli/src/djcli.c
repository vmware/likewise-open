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
#include "djcli.h"

typedef enum {
	PARSE_STATE_OPEN = 0,
	PARSE_STATE_SETNAME,
	PARSE_STATE_JOIN,
	PARSE_STATE_LOG,
	PARSE_STATE_LOG_LEVEL,
	PARSE_STATE_CONFIGURE,
	PARSE_STATE_TEST_PREFIX,
	PARSE_STATE_DONE
} ParseState;

typedef enum {
	PARSE_STATE_JOIN_OPEN = 0,
	PARSE_STATE_JOIN_OU,
	PARSE_STATE_JOIN_DOMAINNAME,
	PARSE_STATE_JOIN_USERNAME,
	PARSE_STATE_JOIN_PASSWORD
} JoinParseState;

void FreeTaskInfo(PTASKINFO pTaskInfo)
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

		CTFreeMemory(pTaskInfo);
	}
}

CENTERROR GetTaskInfo(int argc, char *argv[], PTASKINFO * ppTaskInfo)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PTASKINFO pTaskInfo = NULL;
	ParseState pState = PARSE_STATE_OPEN;
	JoinParseState joinPState = PARSE_STATE_JOIN_OPEN;
	DWORD iArg = 1;
	PSTR pArg = NULL;

	ceError = CTAllocateMemory(sizeof(TASKINFO), (PVOID *) & pTaskInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	pTaskInfo->bNoLog = FALSE;
	pTaskInfo->dwLogLevel = LOG_LEVEL_ERROR;
	pTaskInfo->dwEnable = ENABLE_TYPE_AUTO;

	while (iArg < argc && pState != PARSE_STATE_DONE) {

		pArg = argv[iArg++];

		switch (pState) {
		case PARSE_STATE_OPEN:
			{
				if (!strcmp(pArg, "setname")) {

					pTaskInfo->dwTaskType =
					    TASK_TYPE_SETNAME;
					pState = PARSE_STATE_SETNAME;

				} else if (!strcmp(pArg, "join")) {

					pTaskInfo->dwTaskType = TASK_TYPE_JOIN;
					pState = PARSE_STATE_JOIN;
					joinPState = PARSE_STATE_JOIN_OPEN;

				} else if (!strcmp(pArg, "leave")) {

					pTaskInfo->dwTaskType = TASK_TYPE_LEAVE;

				} else if (!strcmp(pArg, "--help")) {

					pTaskInfo->dwTaskType = TASK_TYPE_USAGE;
					pState = PARSE_STATE_DONE;

				} else if (!strcmp(pArg, "--log")) {

					pState = PARSE_STATE_LOG;

				} else if (!strcmp(pArg, "--loglevel")) {

					pState = PARSE_STATE_LOG_LEVEL;

				} else if (!strcmp(pArg, "--nolog")) {

					pTaskInfo->bNoLog = TRUE;

					if (pTaskInfo->pszLogFilePath) {
						CTFreeString(pTaskInfo->
							     pszLogFilePath);
						pTaskInfo->pszLogFilePath =
						    NULL;
					}

				} else if (!strcmp(pArg, "query")) {

					pTaskInfo->dwTaskType = TASK_TYPE_QUERY;

				} else if (!strcmp(pArg, "fixfqdn")) {

					pTaskInfo->dwTaskType =
					    TASK_TYPE_FIXFQDN;

				} else if (!strcmp(pArg, "configure")) {

					pState = PARSE_STATE_CONFIGURE;

				} else if (!strcmp(pArg, "--testprefix")) {

					pState = PARSE_STATE_TEST_PREFIX;

				} else if (!strcmp(pArg, "--autoenable")) {

					pTaskInfo->dwEnable = ENABLE_TYPE_AUTO;

				} else if (!strcmp(pArg, "--enable")) {

					pTaskInfo->dwEnable =
					    ENABLE_TYPE_ENABLE;

				} else if (!strcmp(pArg, "--disable")) {

					pTaskInfo->dwEnable =
					    ENABLE_TYPE_DISABLE;

				} else {

					pTaskInfo->dwTaskType = TASK_TYPE_USAGE;
					pState = PARSE_STATE_DONE;

				}
			}
			break;
		case PARSE_STATE_JOIN:
			{
				/*
				 * join [--ou <organizationalUnit>] <domain name> <username> [<password>]
				 */

				switch (joinPState) {
				case PARSE_STATE_JOIN_OPEN:
					{
						if (!strcmp(pArg, "--ou")) {
							joinPState =
							    PARSE_STATE_JOIN_OU;
						} else
						    if (!strcmp
							(pArg, "--nohosts")) {
							pTaskInfo->
							    bDoNotChangeHosts =
							    TRUE;
						} else {
							ceError =
							    CTAllocateString
							    (pArg,
							     &pTaskInfo->
							     pszDomainName);
							BAIL_ON_CENTERIS_ERROR
							    (ceError);
							joinPState =
							    PARSE_STATE_JOIN_USERNAME;
						}
					}
					break;
				case PARSE_STATE_JOIN_OU:
					{
						ceError = CTAllocateString(pArg,
									   &pTaskInfo->
									   pszOU);
						BAIL_ON_CENTERIS_ERROR(ceError);
						joinPState =
						    PARSE_STATE_JOIN_DOMAINNAME;
					}
					break;
				case PARSE_STATE_JOIN_DOMAINNAME:
					{
						ceError = CTAllocateString(pArg,
									   &pTaskInfo->
									   pszDomainName);
						BAIL_ON_CENTERIS_ERROR(ceError);
						joinPState =
						    PARSE_STATE_JOIN_USERNAME;
					}
					break;
				case PARSE_STATE_JOIN_USERNAME:
					{
						ceError =
						    CTAllocateString(pArg,
								     &pTaskInfo->
								     pszUserName);
						BAIL_ON_CENTERIS_ERROR(ceError);
						joinPState =
						    PARSE_STATE_JOIN_PASSWORD;
					}
					break;
				case PARSE_STATE_JOIN_PASSWORD:
					{
						ceError =
						    CTAllocateString(pArg,
								     &pTaskInfo->
								     pszPassword);
						BAIL_ON_CENTERIS_ERROR(ceError);

						pState = PARSE_STATE_OPEN;
					}
					break;
				}
			}
			break;
		case PARSE_STATE_SETNAME:
			{
				ceError =
				    CTAllocateString(pArg,
						     &pTaskInfo->
						     pszComputerName);
				BAIL_ON_CENTERIS_ERROR(ceError);

				pState = PARSE_STATE_OPEN;
			}
			break;

		case PARSE_STATE_LOG:
			{
				ceError =
				    CTAllocateString(pArg,
						     &pTaskInfo->
						     pszLogFilePath);
				BAIL_ON_CENTERIS_ERROR(ceError);

				pTaskInfo->bNoLog = FALSE;

				pState = PARSE_STATE_OPEN;
			}
			break;

		case PARSE_STATE_TEST_PREFIX:
			{
				ceError =
				    CTAllocateString(pArg,
						     &pTaskInfo->pszTestPrefix);
				BAIL_ON_CENTERIS_ERROR(ceError);

				pState = PARSE_STATE_OPEN;
			}
			break;

		case PARSE_STATE_CONFIGURE:
			{
				if (!strcasecmp(pArg, "pam"))
					pTaskInfo->dwTaskType =
					    TASK_TYPE_CONFIGURE_PAM;
				else if (!strcasecmp(pArg, "nsswitch"))
					pTaskInfo->dwTaskType =
					    TASK_TYPE_CONFIGURE_NSSWITCH;
				else if (!strcasecmp(pArg, "ssh"))
					pTaskInfo->dwTaskType =
					    TASK_TYPE_CONFIGURE_SSH;
				else {
					ceError =
					    CENTERROR_INVALID_OPTION_VALUE;
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
					pTaskInfo->dwLogLevel =
					    LOG_LEVEL_WARNING;
				else if (!strcasecmp(pArg, "info"))
					pTaskInfo->dwLogLevel = LOG_LEVEL_INFO;
				else if (!strcasecmp(pArg, "verbose"))
					pTaskInfo->dwLogLevel =
					    LOG_LEVEL_VERBOSE;
				else {
					ceError =
					    CENTERROR_DOMAINJOIN_INVALID_LOG_LEVEL;
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

static CENTERROR GetDaysFromEpoch(PDWORD pdwDaysFromEpoch)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	struct timeval tv;

	if (gettimeofday(&tv, NULL) < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	/* days since the start of the epoch (Jan 1, 1970) */
	*pdwDaysFromEpoch = (int)(tv.tv_sec / (60 * 60 * 24));

      error:

	return ceError;
}

CENTERROR DoFixFqdn(PTASKINFO pTaskInfo)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PDOMAINJOININFO pDomainJoinInfo = NULL;

	ceError = QueryInformation(&pDomainJoinInfo);

	if (CENTERROR_IS_OK(ceError) &&
	    pDomainJoinInfo &&
	    !IsNullOrEmptyString(pDomainJoinInfo->pszName) &&
	    !IsNullOrEmptyString(pDomainJoinInfo->pszDomainName)) {
		ceError = DJSetComputerName(pDomainJoinInfo->pszName,
					    pDomainJoinInfo->pszDomainName);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      error:

	if (pDomainJoinInfo)
		FreeDomainJoinInfo(pDomainJoinInfo);

	return CENTERROR_SUCCESS;
}

CENTERROR DoQuery(PTASKINFO pTaskInfo)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PDOMAINJOININFO pDomainJoinInfo = NULL;
	PLICENSEINFO pLicenseInfo = NULL;
	PSTR pszLicenseKey = NULL;
	DWORD nDaysFromEpoch = 0;

	ceError = QueryInformation(&pDomainJoinInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (!IsNullOrEmptyString(pDomainJoinInfo->pszName)) {
		fprintf(stdout, "Name = %s\n", pDomainJoinInfo->pszName);
	} else {
		fprintf(stdout, "Name =\n");
	}

	if (!IsNullOrEmptyString(pDomainJoinInfo->pszDomainName)) {
		fprintf(stdout, "Domain = %s\n",
			pDomainJoinInfo->pszDomainName);
	} else {
		fprintf(stdout, "Domain =\n");
	}

      error:

	if (pDomainJoinInfo)
		FreeDomainJoinInfo(pDomainJoinInfo);

	if (pszLicenseKey)
		CTFreeString(pszLicenseKey);

	if (pLicenseInfo)
		CTFreeMemory(pLicenseInfo);

	return ceError;
}

CENTERROR DoSetName(PTASKINFO pTaskInfo)
{
	return DJSetComputerName(pTaskInfo->pszComputerName, NULL);
}

CENTERROR DoJoin(PTASKINFO pTaskInfo)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	if (IsNullOrEmptyString(pTaskInfo->pszUserName)) {
		ceError = CENTERROR_DOMAINJOIN_INVALID_USERID;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (IsNullOrEmptyString(pTaskInfo->pszDomainName)) {
		ceError = CENTERROR_DOMAINJOIN_INVALID_DOMAIN_NAME;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (IsNullOrEmptyString(pTaskInfo->pszPassword)) {
		ceError = CENTERROR_DOMAINJOIN_INVALID_PASSWORD;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	DJ_LOG_INFO("Joining AD domain: %s", pTaskInfo->pszDomainName);
	if (!IsNullOrEmptyString(pTaskInfo->pszOU)) {
		DJ_LOG_INFO("Joining OU: %s", pTaskInfo->pszOU);
	}
	DJ_LOG_INFO("Join Admin Userid: %s", pTaskInfo->pszUserName);

	fprintf(stdout, "\nJoining AD domain:   %s...\n",
		pTaskInfo->pszDomainName);
	if (!IsNullOrEmptyString(pTaskInfo->pszOU))
		fprintf(stdout, "Organizational Unit: %s\n", pTaskInfo->pszOU);

	ceError = JoinDomain(pTaskInfo->pszDomainName,
			     pTaskInfo->pszUserName,
			     pTaskInfo->pszPassword,
			     pTaskInfo->pszOU, pTaskInfo->bDoNotChangeHosts);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	return ceError;
}

CENTERROR DoLeave(PTASKINFO pTaskInfo)
{
	return JoinWorkgroup("WORKGROUP", "empty", "");
}

CENTERROR GetPassword(PSTR * ppszPassword)
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

	while ((idx < 128)) {

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
