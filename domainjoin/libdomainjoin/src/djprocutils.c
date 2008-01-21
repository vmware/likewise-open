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

void WaitTimeout(int val)
{
}

void FreeProcInfo(PPROCINFO pProcInfo)
{
	CTFreeProcInfo(pProcInfo);
}

CENTERROR
DJSpawnProcessWithEnvironment(PCSTR pszCommand,
			      PCSTR * ppszArgs,
			      PCSTR * ppszEnv,
			      int dwFdIn,
			      int dwFdOut, int dwFdErr, PPROCINFO * ppProcInfo)
{
	return CTSpawnProcessWithEnvironment(pszCommand,
					     ppszArgs,
					     ppszEnv,
					     dwFdIn,
					     dwFdOut, dwFdErr, ppProcInfo);
}

CENTERROR
DJSpawnProcess(PCSTR pszCommand, PSTR * ppszArgs, PPROCINFO * ppProcInfo)
{
	return DJSpawnProcessWithEnvironment(pszCommand, ppszArgs, NULL, -1, -1,
					     -1, ppProcInfo);
}

#ifdef NOT_YET
CENTERROR
DJSpawnProcessCaptureOutput(PCSTR pszCommand,
			    PSTR * ppszArgs,
			    PPROCINFO * ppProcInfo,
			    PSTR * ppszStdout, PSTR * ppszStderr)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	BOOLEAN bFirst = FALSE;
	PPROCINFO pProcInfo = NULL;
	PROCBUFFER procBuffer;
	DWORD iOutBufIdx = 0;
	DWORD dwOutBufLen = 0;
	DWORD dwOutBufAvailable = 0;
	PSTR pszOutBuffer = NULL;
	DWORD iErrBufIdx = 0;
	DWORD dwErrBufLen = 0;
	DWORD dwErrBufAvailable = 0;
	PSTR pszErrBuffer = NULL;

	ceError = DJSpawnProcess(pszCommand, ppszArgs, ppProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	pProcInfo = *ppProcInfo;

	do {

		if (!bFirst) {
			ceError = DJReadData(pProcInfo, &procBuffer);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		bFirst = FALSE;

		if (procBuffer.dwOutBytesRead) {

			while (1) {

				if (procBuffer.dwOutBytesRead <
				    dwOutBufAvailable) {

					memcpy(pszOutBuffer + iOutBufIdx,
					       procBuffer.szOutBuf,
					       procBuffer.dwOutBytesRead);

					iOutBufIdx += procBuffer.dwOutBytesRead;
					dwOutBufAvailable -=
					    procBuffer.dwOutBytesRead;

					*(pszOutBuffer + iOutBufIdx + 1) = '\0';

					break;

				} else {

					/*
					 * TODO: Limit the amount of memory acquired
					 */

					ceError = CTReallocMemory(pszOutBuffer,
								  (PVOID *) &
								  pszOutBuffer,
								  dwOutBufLen +
								  1024);
					BAIL_ON_CENTERIS_ERROR(ceError);

					dwOutBufLen += 1024;
					dwOutBufAvailable += 1024;
				}
			}
		}

		if (procBuffer.dwErrBytesRead) {

			while (1) {

				if (procBuffer.dwErrBytesRead <
				    dwErrBufAvailable) {

					memcpy(pszErrBuffer + iErrBufIdx,
					       procBuffer.szErrBuf,
					       procBuffer.dwErrBytesRead);

					iErrBufIdx += procBuffer.dwErrBytesRead;
					dwErrBufAvailable -=
					    procBuffer.dwErrBytesRead;

					*(pszErrBuffer + iErrBufIdx + 1) = '\0';

					break;

				} else {

					/*
					 * TODO: Limit the amount of memory acquired
					 */
					ceError = CTReallocMemory(pszErrBuffer,
								  (PVOID *) &
								  pszErrBuffer,
								  dwErrBufLen +
								  1024);
					BAIL_ON_CENTERIS_ERROR(ceError);

					dwErrBufLen += 1024;
					dwErrBufAvailable += 1024;
				}
			}
		}

	} while (!procBuffer.bEndOfFile);

	ceError = CTAllocateString(pszOutBuffer, ppszStdout);
	BAIL_ON_CENTERIS_ERROR(ceError);

	CT_SAFE_FREE_STRING(pszOutBuffer);

	ceError = CTAllocateString(pszErrBuffer, ppszStderr);
	BAIL_ON_CENTERIS_ERROR(ceError);

	CT_SAFE_FREE_STRING(pszErrBuffer);

      error:

	if (pszOutBuffer) {
		*ppszStdout = pszOutBuffer;
	}
	if (pszErrBuffer) {
		*ppszStderr = pszErrBuffer;
	}

	return ceError;
}
#endif				/* NOT_YET */

CENTERROR
DJSpawnProcessWithFds(PCSTR pszCommand,
		      PCSTR * ppszArgs,
		      int dwFdIn,
		      int dwFdOut, int dwFdErr, PPROCINFO * ppProcInfo)
{
	return CTSpawnProcessWithFds(pszCommand, ppszArgs, dwFdIn, dwFdOut,
				     dwFdErr, ppProcInfo);
}

CENTERROR
DJSpawnProcessSilent(PCSTR pszCommand, PSTR * ppArgs, PPROCINFO * ppProcInfo)
{
	int dwFdIn = -1, dwFdOut = -1, dwFdErr = -1;
	CENTERROR ceError = CENTERROR_SUCCESS;

	dwFdIn = open("/dev/zero", O_RDONLY, S_IRUSR);

	if (dwFdIn < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	dwFdOut = open("/dev/null", O_WRONLY, S_IWUSR);

	if (dwFdOut < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	dwFdErr = open("/dev/null", O_WRONLY, S_IWUSR);

	if (dwFdErr < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError =
	    DJSpawnProcessWithFds(pszCommand, ppArgs, dwFdIn, dwFdOut, dwFdErr,
				  ppProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	if (dwFdIn != -1)
		close(dwFdIn);
	if (dwFdOut != -1)
		close(dwFdOut);
	if (dwFdErr != -1)
		close(dwFdErr);

	return ceError;
}

CENTERROR
DJSpawnProcessOutputToFile(PCSTR pszCommand,
			   PSTR * ppArgs, PCSTR file, PPROCINFO * ppProcInfo)
{
	int dwFdIn = -1, dwFdOut = -1, dwFdErr = -1;
	CENTERROR ceError = CENTERROR_SUCCESS;

	dwFdIn = open("/dev/zero", O_RDONLY);

	if (dwFdIn < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	dwFdOut = open(file, O_CREAT|O_WRONLY, S_IWUSR);

	if (dwFdOut < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	dwFdErr = open("/dev/null", O_WRONLY);

	if (dwFdErr < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError =
	    DJSpawnProcessWithFds(pszCommand, ppArgs, dwFdIn, dwFdOut, dwFdErr,
				  ppProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	if (dwFdIn != -1)
		close(dwFdIn);
	if (dwFdOut != -1)
		close(dwFdOut);
	if (dwFdErr != -1)
		close(dwFdErr);

	return ceError;
}

CENTERROR
DJTimedReadData(PPROCINFO pProcInfo,
		PPROCBUFFER pProcBuffer,
		DWORD dwTimeoutSecs, PBOOLEAN pbTimedout)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszBuf = NULL;
	DWORD dwBytesRead = 0;
	int maxfd;
	fd_set read_fd_set;
	int select_status;
	int fd = 0;
	int iFd = 0;

	struct timeval timeout;
	timeout.tv_sec = dwTimeoutSecs;
	timeout.tv_usec = 0;

	pProcBuffer->dwOutBytesRead = 0;
	pProcBuffer->dwErrBytesRead = 0;
	pProcBuffer->bEndOfFile = FALSE;

	FD_ZERO(&read_fd_set);
	while (!pProcBuffer->dwOutBytesRead &&
	       !pProcBuffer->dwErrBytesRead && !pProcBuffer->bEndOfFile) {

		if (pProcInfo->fdout >= 0) {
			FD_SET(pProcInfo->fdout, &read_fd_set);
		}
		if (pProcInfo->fderr >= 0) {
			FD_SET(pProcInfo->fderr, &read_fd_set);
		}

		maxfd =
		    (pProcInfo->fdout >
		     pProcInfo->fderr ? pProcInfo->fdout +
		     1 : pProcInfo->fderr + 1);

		select_status = select(maxfd,
				       &read_fd_set, NULL /* write_fds */ ,
				       NULL /* except_fds */ ,
				       &timeout);
		if (select_status < 0) {

			if (errno == EINTR)
				continue;

			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);

		} else if (select_status == 0) {

			/* timed out */

		} else {

			for (iFd = 0; iFd < 2; iFd++) {

				fd = (iFd ==
				      0 ? pProcInfo->fdout : pProcInfo->fderr);
				if (fd < 0) {
					continue;
				}
				pszBuf =
				    (iFd ==
				     0 ? pProcBuffer->szOutBuf : pProcBuffer->
				     szErrBuf);

				if (FD_ISSET(fd, &read_fd_set)) {

					dwBytesRead =
					    read(fd, pszBuf, MAX_PROC_BUF_LEN);
					if (dwBytesRead < 0) {

						if (errno != EAGAIN
						    && errno != EINTR) {
							ceError =
							    CTMapSystemError
							    (errno);
							BAIL_ON_CENTERIS_ERROR
							    (ceError);
						}

					} else if (dwBytesRead == 0) {

						pProcBuffer->bEndOfFile = TRUE;

					} else {

						if (iFd == 0)
							pProcBuffer->
							    dwOutBytesRead =
							    dwBytesRead;
						else
							pProcBuffer->
							    dwErrBytesRead =
							    dwBytesRead;

					}

				}
			}
		}
	}

      error:

	return (ceError);
}

CENTERROR DJReadData(PPROCINFO pProcInfo, PPROCBUFFER pProcBuffer)
{
	BOOLEAN bTimedout = FALSE;
	return DJTimedReadData(pProcInfo, pProcBuffer, 5, &bTimedout);
}

CENTERROR DJWriteData(DWORD dwFd, PSTR pszBuf, DWORD dwLen)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	DWORD nWritten = 0;
	DWORD dwRemaining = 0;
	PSTR pStr;

	dwRemaining = dwLen;
	pStr = pszBuf;
	while (dwRemaining > 0) {

		nWritten = write(dwFd, pStr, dwRemaining);
		if (nWritten < 0) {
			if (errno != EAGAIN && errno != EINTR) {
				ceError = CTMapSystemError(errno);
				BAIL_ON_CENTERIS_ERROR(ceError);
			}
		} else {
			dwRemaining -= nWritten;
			pStr += nWritten;
		}
	}

      error:

	return (ceError);
}

CENTERROR DJGetProcessStatus(PPROCINFO pProcInfo, PLONG plstatus)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	int status = 0;

	do {
		if (waitpid(pProcInfo->pid, &status, 0) < 0) {
			if (errno == EINTR)
				continue;
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		if (WIFEXITED(status)) {
			*plstatus = WEXITSTATUS(status);
		} else if (WIFSIGNALED(status)) {
			DJ_LOG_ERROR("Process [%d] killed by signal %d\n",
				     pProcInfo->pid, WTERMSIG(status));
		} else if (WIFSTOPPED(status)) {
			DJ_LOG_ERROR("Process [%d] stopped by signal %d\n",
				     pProcInfo->pid, WSTOPSIG(status));
		} else {
			DJ_LOG_ERROR("Process [%d] unknown status 0x%x\n",
				     pProcInfo->pid, status);
		}
	} while (!WIFEXITED(status) && !WIFSIGNALED(status));

      error:

	return ceError;
}

CENTERROR DJKillProcess(PPROCINFO pProcInfo)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	DWORD dwFlag = 0;
	DWORD dwTimeout = 30;
	int status = 0;

	sigset(SIGALRM, WaitTimeout);

	alarm(dwTimeout);

	while ((waitpid(pProcInfo->pid, &status, 0) < 0) && errno == EINTR) {

		alarm(0);
		if (dwFlag == 0) {

			kill(pProcInfo->pid, SIGTERM);

		} else if (dwFlag == 1) {

			kill(pProcInfo->pid, SIGKILL);
			break;

		}

		dwFlag++;
		alarm(dwTimeout);
	}

	alarm(0);
	sigset(SIGALRM, SIG_DFL);

	return ceError;
}

CENTERROR DJCheckProcessRunning(PCSTR pszCmdName, PBOOLEAN pbRunning)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR *ppszArgs = NULL;
	PPROCINFO pProcInfo = NULL;
	LONG status = 0;
	DWORD nArgs = 4;

	ceError = CTAllocateMemory(sizeof(PSTR) * nArgs, (PVOID *) & ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("ps", ppszArgs);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString("-C", ppszArgs + 1);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(pszCmdName, ppszArgs + 2);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = DJGetProcessStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	*pbRunning = (status ? 0 : 1);

      error:

	if (pProcInfo)
		FreeProcInfo(pProcInfo);

	if (ppszArgs)
		CTFreeStringArray(ppszArgs, nArgs);

	return ceError;
}
