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

int main(int argc, char *argv[])
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PPROCINFO pProcInfo = NULL;
	PROCBUFFER procBuf;
	PSTR ppArgs[] = { "/bin/date", NULL };
	LONG lExitCode = 0;

	ceError = DJSpawnProcess("/bin/date", ppArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	printf("Child process id: %d\n", pProcInfo->pid);
	printf("<Begin output>\n");
	do {
		procBuf.bEndOfFile = FALSE;
		ceError = DJReadData(pProcInfo, &procBuf);
		BAIL_ON_CENTERIS_ERROR(ceError);
		if (procBuf.dwOutBytesRead) {
			if (write
			    (STDOUT_FILENO, procBuf.szOutBuf,
			     procBuf.dwOutBytesRead) < 0) {
				ceError = CTMapSystemError(errno);
				BAIL_ON_CENTERIS_ERROR(ceError);
			}
		}
		if (procBuf.dwErrBytesRead) {
			if (write
			    (STDERR_FILENO, procBuf.szErrBuf,
			     procBuf.dwErrBytesRead) < 0) {
				ceError = CTMapSystemError(errno);
				BAIL_ON_CENTERIS_ERROR(ceError);
			}
		}
	} while (!procBuf.bEndOfFile);
	printf("<End output>\n");

	ceError = DJGetProcessStatus(pProcInfo, &lExitCode);
	BAIL_ON_CENTERIS_ERROR(ceError);

	printf("Exit status: %ld\n", lExitCode);

      error:

	if (pProcInfo) {
		FreeProcInfo(pProcInfo);
	}

	return (ceError);
}
