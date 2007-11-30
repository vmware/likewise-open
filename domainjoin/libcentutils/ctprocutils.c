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

#include "ctbase.h"

CENTERROR CTMatchProgramToPID(PCSTR pszProgramName, pid_t pid)
{
	CENTERROR ceError = CENTERROR_NO_SUCH_PROCESS;
	CHAR szBuf[PATH_MAX + 1];
	FILE *pFile = NULL;

#if defined(__MACH__) && defined(__APPLE__)
	sprintf(szBuf, "ps -p %d -o command= | grep %s", pid, pszProgramName);
#else
	sprintf(szBuf, "UNIX95=1 ps -p %ld -o comm= | grep %s", (long)pid,
		pszProgramName);
#endif

	pFile = popen(szBuf, "r");
	if (pFile == NULL) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	while (TRUE) {

		if (NULL == fgets(szBuf, PATH_MAX, pFile)) {
			if (feof(pFile))
				break;
			else {
				ceError = CTMapSystemError(errno);
				BAIL_ON_CENTERIS_ERROR(ceError);
			}
		}

		CTStripWhitespace(szBuf);
		if (!IsNullOrEmptyString(szBuf)) {
			ceError = CENTERROR_SUCCESS;
			break;
		}

	}

      error:

	if (pFile)
		fclose(pFile);

	return ceError;
}

CENTERROR
CTIsProgramRunning(PCSTR pszPidFile,
		   PCSTR pszProgramName, pid_t * pPid, PBOOLEAN pbRunning)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	pid_t pid = 0;
	int fd = -1;
	int result;
	char contents[20];
	BOOLEAN bFileExists = FALSE;

	*pbRunning = FALSE;

	ceError = CTCheckFileExists(pszPidFile, &bFileExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (bFileExists) {

		fd = open(pszPidFile, O_RDONLY, 0644);
		if (fd < 0) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		result = read(fd, contents, sizeof(contents) - 1);
		if (result < 0) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		} else if (result > 0) {
			contents[result - 1] = 0;
			result = atoi(contents);
		}

		if (result <= 0) {
			ceError = CENTERROR_NO_SUCH_PROCESS;
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		pid = (pid_t) result;
		result = kill(pid, 0);
		if (!result) {
			// Verify that the peer process is the auth daemon
			ceError = CTMatchProgramToPID(pszProgramName, pid);
			BAIL_ON_CENTERIS_ERROR(ceError);

			*pbRunning = TRUE;
			if (pPid) {
				*pPid = pid;
			}
		} else if (errno == ESRCH) {

			ceError = CENTERROR_NO_SUCH_PROCESS;
			BAIL_ON_CENTERIS_ERROR(ceError);

		} else {

			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

	}

      error:

	if (fd != -1) {
		close(fd);
	}

	return (ceError ==
		CENTERROR_NO_SUCH_PROCESS ? CENTERROR_SUCCESS : ceError);
}

CENTERROR CTSendSignal(pid_t pid, int sig)
{
	int ret = 0;
	ret = kill(pid, sig);

	if (ret < 0) {
		return CTMapSystemError(errno);
	}

	return CENTERROR_SUCCESS;
}
