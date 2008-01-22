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

CENTERROR DJModifyKrb5Conf(PSTR pszDomainName, PSTR pszShortDomainName)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	PSTR *ppszArgs = NULL;
	DWORD nArgs = 0;
	long status = 0;
	PPROCINFO pProcInfo = NULL;

	char szBuff[PATH_MAX + 1];

	sprintf(szBuff, "%s/ConfigureKrb5.pl", SCRIPTDIR);

	nArgs = 8;
	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("perl", ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(szBuff, ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (IsNullOrEmptyString(pszDomainName)) {
		ceError = CTAllocateString("--leave", ppszArgs + 2);
		BAIL_ON_CENTERIS_ERROR(ceError);
	} else {
		ceError = CTAllocateString("--join", ppszArgs + 2);
		BAIL_ON_CENTERIS_ERROR(ceError);

		ceError = CTAllocateString(pszDomainName, ppszArgs + 3);
		BAIL_ON_CENTERIS_ERROR(ceError);

		if (!IsNullOrEmptyString(pszShortDomainName)) {
			ceError = CTAllocateString("--short", ppszArgs + 4);
			BAIL_ON_CENTERIS_ERROR(ceError);

			ceError =
			    CTAllocateString(pszShortDomainName, ppszArgs + 5);
			BAIL_ON_CENTERIS_ERROR(ceError);

			ceError = CTAllocateString("--trusts", ppszArgs + 6);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}
	}

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {
		ceError = CENTERROR_DOMAINJOIN_KRB5_EDIT_FAIL;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      error:

	if (ppszArgs)
		CTFreeStringArray(ppszArgs, nArgs);

	if (pProcInfo)
		FreeProcInfo(pProcInfo);

	return ceError;
}
