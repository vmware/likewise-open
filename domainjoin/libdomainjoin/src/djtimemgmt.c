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

#define CLOCK_DRIFT_SECONDS "60"

CENTERROR DJSyncTimeToDC(PSTR pszDomainName)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR *ppszArgs = NULL;
	DWORD nArgs = 5;
	LONG status = 0;
	PPROCINFO pProcInfo = NULL;
	CHAR szBuf[256];

	sprintf(szBuf, "%s/bin/gpsynctime.pl", PREFIXDIR);

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("perl", ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(szBuf, ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(pszDomainName, ppszArgs + 2);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(CLOCK_DRIFT_SECONDS, ppszArgs + 3);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {
		DJ_LOG_ERROR("Failed to sync time with DC. Exit code: %d",
			     pszDomainName, status);
	}

      error:

	if (ppszArgs)
		CTFreeStringArray(ppszArgs, nArgs);

	if (pProcInfo)
		FreeProcInfo(pProcInfo);

	return ceError;
}
