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

/* ex: set tabstop=4 expandtab shiftwidth=4: */
#include "domainjoin.h"
#include "djcli.h"

static
void ShowUsage()
{
	fprintf(stdout,
		"usage: domainjoin-cli [options] command [args...]\n\n");
	fprintf(stdout, "  where options are:\n\n");
	fprintf(stdout,
		"    --help            Display this help information.\n");
	fprintf(stdout,
		"    --log {.|path}    Log to a file (or \".\" to log to console).\n\n");
	fprintf(stdout, "  and commands are:\n\n");
	fprintf(stdout, "    query\n");
	fprintf(stdout, "    fixfqdn\n");
	fprintf(stdout, "    setname <computer name>\n");
	fprintf(stdout,
		"    join [--ou <organizationalUnit>] <domain name> <user name> [<password>]\n");
	//fprintf(stdout, "    configure pam [--testprefix <dir>] { --enable | --disable }"
	fprintf(stdout, "    leave\n\n");
	fprintf(stdout, "  Example:\n\n");
	fprintf(stdout, "    domainjoin-cli query\n\n");
}

static CENTERROR FillMissingPassword(PSTR * ppszPassword)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszPassword = NULL;

	fprintf(stdout, "Password: ");
	fflush(stdout);
	ceError = GetPassword(&pszPassword);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (!IsNullOrEmptyString(pszPassword)) {
		*ppszPassword = pszPassword;
		pszPassword = NULL;
	}

      error:

	if (pszPassword)
		CTFreeString(pszPassword);

	return ceError;
}

void ShowErrorMessage(CENTERROR errCode)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszErrMessage = NULL;

	ceError = DJGetErrorMessage(errCode, &pszErrMessage);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (!IsNullOrEmptyString(pszErrMessage)) {

		DJ_LOG_ERROR("%s", pszErrMessage);
		fprintf(stderr, "%s\n", pszErrMessage);

	} else {

		DJ_LOG_ERROR("FAILED [Error code: %.8x]", errCode);
		fprintf(stderr, "FAILED [Error code: %.8x]\n", errCode);

	}

      error:

	if (pszErrMessage)
		CTFreeString(pszErrMessage);
}

CENTERROR ValidateJoinParameters(PTASKINFO pTaskInfo)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	if (IsNullOrEmptyString(pTaskInfo->pszDomainName)) {
		ceError = CENTERROR_DOMAINJOIN_INVALID_DOMAIN_NAME;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (IsNullOrEmptyString(pTaskInfo->pszUserName)) {
		ceError = CENTERROR_DOMAINJOIN_INVALID_USERID;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (IsNullOrEmptyString(pTaskInfo->pszPassword)) {
		ceError = CENTERROR_DOMAINJOIN_INVALID_PASSWORD;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      error:

	return ceError;
}

BOOLEAN GetEnableBoolean(PTASKINFO pTaskInfo)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PDOMAINJOININFO pDomainJoinInfo = NULL;
	if (pTaskInfo->dwEnable == ENABLE_TYPE_AUTO) {
		ceError = QueryInformation(&pDomainJoinInfo);
		BAIL_ON_CENTERIS_ERROR(ceError);

		if (IsNullOrEmptyString(pDomainJoinInfo->pszDomainName))
			pTaskInfo->dwEnable = ENABLE_TYPE_DISABLE;
		else
			pTaskInfo->dwEnable = ENABLE_TYPE_ENABLE;

	}

      error:
	if (pDomainJoinInfo != NULL)
		FreeDomainJoinInfo(pDomainJoinInfo);

	return pTaskInfo->dwEnable == ENABLE_TYPE_ENABLE;
}

int main(int argc, char *argv[]
    )
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	PTASKINFO pTaskInfo = NULL;
	PSTR pszPassword = NULL;
	BOOLEAN bIsResolvable = FALSE;

	if (argc <= 1) {
		ShowUsage();
		goto done;
	}

	ceError = GetTaskInfo(argc, argv, &pTaskInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (pTaskInfo->dwTaskType == TASK_TYPE_USAGE) {
		ShowUsage();
		goto done;
	}

	if (!IsRoot() && pTaskInfo->pszTestPrefix == NULL) {
		fprintf(stderr,
			"Error: This program can only be run by the root user\n");
		ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (pTaskInfo->bNoLog) {

		ceError = dj_disable_logging();
		BAIL_ON_CENTERIS_ERROR(ceError);

	} else if (pTaskInfo->pszLogFilePath == NULL ||
		   !strcmp(pTaskInfo->pszLogFilePath, ".")) {

		ceError = dj_init_logging_to_console(pTaskInfo->dwLogLevel);
		BAIL_ON_CENTERIS_ERROR(ceError);

	} else {

		ceError = dj_init_logging_to_file(pTaskInfo->dwLogLevel,
						  pTaskInfo->pszLogFilePath);
		BAIL_ON_CENTERIS_ERROR(ceError);

	}

	switch (pTaskInfo->dwTaskType) {
	case TASK_TYPE_FIXFQDN:
		{
			ceError = DoFixFqdn(pTaskInfo);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}
		break;
	case TASK_TYPE_QUERY:
		{
			ceError = DoQuery(pTaskInfo);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}
		break;
	case TASK_TYPE_SETNAME:
		{
			ceError = DoSetName(pTaskInfo);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}
		break;
	case TASK_TYPE_JOIN:
		{
			while (1) {

				ceError = ValidateJoinParameters(pTaskInfo);

				if (ceError ==
				    CENTERROR_DOMAINJOIN_INVALID_PASSWORD) {

					if (pTaskInfo->pszPassword) {
						CTFreeString(pTaskInfo->
							     pszPassword);
						pTaskInfo->pszPassword = NULL;
					}

					ceError =
					    FillMissingPassword(&pTaskInfo->
								pszPassword);
					BAIL_ON_CENTERIS_ERROR(ceError);

					continue;
				}
				BAIL_ON_CENTERIS_ERROR(ceError);
				break;
			}

			ceError =
			    DJIsDomainNameResolvable(pTaskInfo->pszDomainName,
						     &bIsResolvable);
			BAIL_ON_CENTERIS_ERROR(ceError);

			if (!bIsResolvable) {
				ceError =
				    CENTERROR_DOMAINJOIN_UNRESOLVED_DOMAIN_NAME;
				BAIL_ON_CENTERIS_ERROR(ceError);
			}

			ceError = DoJoin(pTaskInfo);
			BAIL_ON_CENTERIS_ERROR(ceError);

			fprintf(stdout, "SUCCESS\n");
		}
		break;
	case TASK_TYPE_CONFIGURE_PAM:
		{
			ceError =
			    DJNewConfigurePamForADLogin(pTaskInfo->
							pszTestPrefix,
							GetEnableBoolean
							(pTaskInfo));
			BAIL_ON_CENTERIS_ERROR(ceError);
			fprintf(stdout, "SUCCESS\n");
		}
		break;
	case TASK_TYPE_CONFIGURE_NSSWITCH:
		{
			if (GetEnableBoolean(pTaskInfo))
				ceError = ConfigureNameServiceSwitch();
			else
				ceError = UnConfigureNameServiceSwitch();
			BAIL_ON_CENTERIS_ERROR(ceError);
			fprintf(stdout, "SUCCESS\n");
		}
		break;
	case TASK_TYPE_CONFIGURE_SSH:
		{
			ceError =
			    DJConfigureSshForADLogin(pTaskInfo->pszTestPrefix,
						     GetEnableBoolean
						     (pTaskInfo));
			BAIL_ON_CENTERIS_ERROR(ceError);
			fprintf(stdout, "SUCCESS\n");
		}
		break;
	case TASK_TYPE_LEAVE:
		{
			ceError = DoLeave(pTaskInfo);
			BAIL_ON_CENTERIS_ERROR(ceError);
			fprintf(stdout, "SUCCESS\n");
		}
		break;
	default:
		{
			fprintf(stderr, "Unknown command id [%d]\n",
				pTaskInfo->dwTaskType);
			ceError = 1;
			goto error;
		}
	}

      done:

	if (pTaskInfo) {
		FreeTaskInfo(pTaskInfo);
	}

	if (pszPassword) {
		CTFreeString(pszPassword);
	}

	dj_close_log();

	return CENTERROR_SUCCESS;

      error:

	if (pTaskInfo) {
		FreeTaskInfo(pTaskInfo);
	}

	if (pszPassword) {
		CTFreeString(pszPassword);
	}

	dj_close_log();

	if (!CENTERROR_IS_OK(ceError))
		ShowErrorMessage(ceError);

	return ceError;
}
