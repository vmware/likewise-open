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
#if HAVE_PROCFS_H
#include <procfs.h>
#elif HAVE_SYS_PROCFS_H
#include <sys/procfs.h>
#endif

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

CENTERROR
CTMatchProgramToPID(
    PCSTR pszProgramName,
    pid_t    pid
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];
    FILE* pFile = NULL;

#if defined(__LWI_MACINTOSH__)
    sprintf(szBuf, "ps -p %d -o command= | grep %s", pid, pszProgramName);
#elif defined(__LWI_SOLARIS__) || defined(__LWI_HP_UX__) || defined(__LWI_AIX__)
    sprintf(szBuf, "UNIX95=1 ps -p %ld -o comm= | grep %s", (long)pid, pszProgramName);
#else
    sprintf(szBuf, "UNIX95=1 ps -p %ld -o cmd= | grep %s", (long)pid, pszProgramName);
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
        pclose(pFile);

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

    if(pbRunning != NULL)
        *pbRunning = FALSE;
    if(pPid != NULL)
        *pPid = -1;

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

          if(pbRunning != NULL)
          {
              *pbRunning = TRUE;
          }
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
    return CTGetPidOfCmdLine(programName, NULL, NULL, owner, pid, count);
}

CENTERROR
CTGetPidOfCmdLine(
    PCSTR programName,
    PCSTR programFilename,
    PCSTR cmdLine,
    uid_t owner,
    pid_t *pid,
    size_t *count
    )
{
    CENTERROR ceError = CENTERROR_NOT_IMPLEMENTED;
    size_t fillCount = 0;
    size_t foundCount = 0;
    struct stat findStat;
#if HAVE_DECL_PSTAT_GETPROC
    //HPUX should have this
    struct pst_status mystatus;
    struct pst_status status[10];
    int inBuffer;
    int i;
#endif
#ifdef HAVE_STRUCT_PSINFO
    //Solaris and AIX should have this
    DIR *dir = NULL;
    struct dirent *dirEntry = NULL;
    PSTR filePath = NULL;
    struct psinfo infoStruct;
    FILE *infoFile = NULL;
    struct stat compareStat;
#endif

    if(count)
    {
        fillCount = *count;
        *count = 0;
    }
    else if(pid != NULL)
        fillCount = 1;

    if(programFilename != NULL)
    {
        while(stat(programFilename, &findStat) < 0)
        {
            if(errno == EINTR)
                continue;
            GCE(ceError = CTMapSystemError(errno));
        }
    }
    
#if HAVE_DECL_PSTAT_GETPROC
    //First get the process info for this process
    inBuffer = pstat_getproc(&mystatus, sizeof(mystatus), 0,
            getpid());
    if(inBuffer != 1)
        GCE(ceError = CTMapSystemError(errno));

    //Now look at all processes
    inBuffer = pstat_getproc(status, sizeof(status[0]),
            sizeof(status)/sizeof(status[0]), 0);
    if(inBuffer < 0)
        GCE(ceError = CTMapSystemError(errno));
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
            if(programFilename != NULL && (
                    status[i].pst_text.psf_fileid != findStat.st_ino ||
                    status[i].pst_text.psf_fsid.psfs_id != findStat.st_dev ||
                    status[i].pst_text.psf_fsid.psfs_type != findStat.st_fstype
                    ))
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
            GCE(ceError = CTMapSystemError(errno));
    }
    ceError = CENTERROR_SUCCESS;
#endif

#ifdef HAVE_STRUCT_PSINFO
    if ((dir = opendir("/proc")) == NULL) {
        GCE(ceError = CTMapSystemError(errno));
    }

    while(1)
    {
        errno = 0;
        dirEntry = readdir(dir);
        if(dirEntry == NULL)
        {
            if(errno != 0)
                GCE(ceError = CTMapSystemError(errno));
            else
            {
                //No error here. We simply read the last entry
                break;
            }
        }
        if(dirEntry->d_name[0] == '.')
            continue;
        // On AIX, there is a /proc/sys which does not contain a psinfo
        if(!isdigit(dirEntry->d_name[0]))
            continue;
        CT_SAFE_FREE_STRING(filePath);
        GCE(ceError = CTAllocateStringPrintf(&filePath, "/proc/%s/psinfo",
                    dirEntry->d_name));
        GCE(ceError = CTSafeCloseFile(&infoFile));
        GCE(ceError = CTOpenFile(filePath, "r", &infoFile));
        if(fread(&infoStruct, sizeof(infoStruct), 1, infoFile) != 1)
        {
            GCE(ceError = CTMapSystemError(errno));
        }

        if (owner != (uid_t)-1 && owner != infoStruct.pr_euid)
        {
            continue;
        }
        if (programName != NULL && strcmp(infoStruct.pr_fname, programName))
        {
            continue;
        }
        if (cmdLine != NULL && strcmp(infoStruct.pr_psargs, cmdLine))
        {
            continue;
        }
        if(programFilename != NULL)
        {
            CT_SAFE_FREE_STRING(filePath);
            GCE(ceError = CTAllocateStringPrintf(&filePath,
                        "/proc/%s/object/a.out",
                        dirEntry->d_name));

            while(stat(filePath, &compareStat) < 0)
            {
                if(errno == EINTR)
                    continue;
                if(errno == ENOENT || errno == ENOTDIR)
                {
                    //This process wasn't executed from a file?
                    goto not_match;
                }
                GCE(ceError = CTMapSystemError(errno));
            }
            if(findStat.st_ino != compareStat.st_ino)
                continue;
            if(findStat.st_dev != compareStat.st_dev)
                continue;
            if(findStat.st_rdev != compareStat.st_rdev)
                continue;
        }
 
        //This is a match
        if(foundCount < fillCount)
            pid[foundCount] = infoStruct.pr_pid;
        foundCount++;
not_match:
        ;
    }
#endif

    if(count)
        *count = foundCount;
    else if(CENTERROR_IS_OK(ceError) && foundCount == 0)
        ceError = CENTERROR_NO_SUCH_PROCESS;

cleanup:
#ifdef HAVE_STRUCT_PSINFO
    if(dir != NULL)
        closedir(dir);
    CT_SAFE_FREE_STRING(filePath);
    CTSafeCloseFile(&infoFile);
#endif

    return ceError;
}
