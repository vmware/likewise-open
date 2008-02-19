/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#include "config/config.h"
#include "ctbase.h"
#include "ctprocutils.h"
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if HAVE_SYS_PSTAT_H
#include <sys/pstat.h>
#endif

CENTERROR
CTMatchProgramToPID(
    PCSTR pszProgramName,
    pid_t    pid
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];
    FILE* pFile = NULL;

#if defined(__MACH__) && defined(__APPLE__)
    sprintf(szBuf, "ps -p %d -o command= | grep %s", pid, pszProgramName);
#else
    sprintf(szBuf, "UNIX95=1 ps -p %ld -o comm= | grep %s", (long)pid, pszProgramName);
#endif

    pFile = popen(szBuf, "r");
    if (pFile == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* Assume that we won't find the process we are looking for */
    ceError = CENTERROR_NO_SUCH_PROCESS;

    while (TRUE) {

        if (NULL == fgets(szBuf, PATH_MAX, pFile)) {
            if (feof(pFile)) {
                break;
            } else {
                ceError = CTMapSystemError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        CTStripWhitespace(szBuf);
        if (!IsNullOrEmptyString(szBuf)) {
            /* Well what do you know, it was found! */
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
CTIsProgramRunning(
	PCSTR pszPidFile,
	PCSTR pszProgramName,
	pid_t *pPid,
    PBOOLEAN pbRunning
    )
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

       result = read(fd, contents, sizeof(contents)-1);
       if (result < 0) {
          ceError = CTMapSystemError(errno);
          BAIL_ON_CENTERIS_ERROR(ceError);
       }
       else if (result > 0) {
          contents[result-1] = 0;
          result = atoi(contents);
       }

       if (result <= 0) {
          ceError = CENTERROR_NO_SUCH_PROCESS;
          BAIL_ON_CENTERIS_ERROR(ceError);
       }

       pid = (pid_t) result;
       result = kill(pid, 0);
       if (!result)
       {
          // Verify that the peer process is the auth daemon
          ceError = CTMatchProgramToPID(pszProgramName, pid);
          BAIL_ON_CENTERIS_ERROR(ceError);

          *pbRunning = TRUE;
		  if ( pPid ) {
		  	*pPid = pid;
		  }
       }
       else if (errno == ESRCH) {

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

    return (ceError == CENTERROR_NO_SUCH_PROCESS ? CENTERROR_SUCCESS : ceError);
}

CENTERROR
CTSendSignal(
	pid_t pid,
	int sig 
	)
{
	int ret = 0;
	ret = kill(pid, sig);
	
	if (ret < 0) {
		return CTMapSystemError(errno);
	}
	
	return CENTERROR_SUCCESS;
}

CENTERROR
CTGetPidOf(
    PCSTR programName,
    uid_t owner,
    pid_t *pid,
    size_t *count
    )
{
    return CTGetPidOfCmdLine(programName, NULL, owner, pid, count);
}

CENTERROR
CTGetPidOfCmdLine(
    PCSTR programName,
    PCSTR cmdLine,
    uid_t owner,
    pid_t *pid,
    size_t *count
    )
{
    CENTERROR ceError = CENTERROR_NOT_IMPLEMENTED;
    size_t fillCount = 0;
    size_t foundCount = 0;
#if HAVE_DECL_PSTAT_GETPROC
    //HPUX should have this
    struct pst_status mystatus;
    struct pst_status status[10];
    int inBuffer;
    int i;
#endif

    if(count)
    {
        fillCount = *count;
        *count = 0;
    }
    else if(pid != NULL)
        fillCount = 1;
    
#if HAVE_DECL_PSTAT_GETPROC
    //First get the process info for this process
    inBuffer = pstat_getproc(&mystatus, sizeof(mystatus), 0,
            getpid());
    if(inBuffer != 1)
		return CTMapSystemError(errno);

    //Now look at all processes
    inBuffer = pstat_getproc(status, sizeof(status[0]),
            sizeof(status)/sizeof(status[0]), 0);
    if(inBuffer < 0)
		return CTMapSystemError(errno);
    while(inBuffer > 0)
    {
        for(i = 0; i < inBuffer; i++)
        {
            if(memcmp(&mystatus.pst_rdir, &status[i].pst_rdir,
                        sizeof(mystatus.pst_rdir)))
            {
                /* This process has a different root directory (it is being run
                   via a chroot). Let's not count this process as a match. */
                continue;
            }
            if (owner != (uid_t)-1 && owner != status[i].pst_euid)
            {
                continue;
            }
            if (programName != NULL && strcmp(status[i].pst_ucomm, programName))
            {
                continue;
            }
            if (cmdLine != NULL && strcmp(status[i].pst_cmd, cmdLine))
            {
                continue;
            }
 
            //This is a match
            if(foundCount < fillCount)
                pid[foundCount] = status[i].pst_pid;
            foundCount++;
        }
        //Continue looking at the process list where we left off
        inBuffer = pstat_getproc(status, sizeof(status[0]),
                sizeof(status)/sizeof(status[0]),
                status[inBuffer - 1].pst_idx + 1);
        if(inBuffer < 0)
            return CTMapSystemError(errno);
    }
    ceError = CENTERROR_SUCCESS;
    if(count == NULL && foundCount == 0)
        ceError = CENTERROR_NO_SUCH_PROCESS;
#endif
    if(count)
        *count = foundCount;
    return ceError;
}
