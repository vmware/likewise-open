/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        filelog.c
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 * 
 *        Logging API
 * 
 *        Implemenation of logging to file
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */

#include "includes.h"

DWORD
SMBOpenFileLog(
    PCSTR       pszFilePath,
    SMBLogLevel maxAllowedLogLevel,
    PHANDLE     phLog
    )
{
    DWORD dwError = 0;
    PSMB_FILE_LOG pFileLog = NULL;
    
    if (IsNullOrEmptyString(pszFilePath))
    {
        dwError = SMB_ERROR_INVALID_PARAMETER;
        goto error;
    }
    
    dwError = SMBAllocateMemory(
                    sizeof(SMB_FILE_LOG),
                    (PVOID*)&pFileLog);
    if (dwError)
    {
        goto error;
    }
    
    dwError = SMBAllocateString(
                    pszFilePath,
                    &pFileLog->pszFilePath);
    if (dwError)
    {
        goto error;
    }
    
    pFileLog->fp = fopen(pFileLog->pszFilePath, "w");
    if (pFileLog->fp == NULL) {
        dwError = errno;
        goto error;
    }
    
    dwError = SMBSetupLogging(
                    (HANDLE)pFileLog,
                    maxAllowedLogLevel,
                    &SMBLogToFile
                    );
    if (dwError)
    {
        goto error;
    }
    
    *phLog = (HANDLE)pFileLog;

cleanup:

    return dwError;

error:

    *phLog = (HANDLE)NULL;
    
    if (pFileLog)
    {
        SMBFreeFileLogInfo(pFileLog);
    }

    goto cleanup;
}

DWORD
SMBGetFileLogInfo(
    HANDLE hLog,
    PSMB_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PSMB_LOG_INFO pLogInfo = NULL;
    PSMB_FILE_LOG pFileLog = (PSMB_FILE_LOG)hLog;
    
    BAIL_ON_INVALID_HANDLE(hLog);
    
    if ((gSMBLogTarget != SMB_LOG_TARGET_FILE) ||
        IsNullOrEmptyString(pFileLog->pszFilePath))
    {
        dwError = SMB_ERROR_INTERNAL;
        BAIL_ON_SMB_ERROR(dwError);
    }
    
    dwError = SMBAllocateMemory(
                    sizeof(SMB_LOG_INFO),
                    (PVOID*)&pLogInfo);
    BAIL_ON_SMB_ERROR(dwError);
    
    pLogInfo->logTarget = SMB_LOG_TARGET_FILE;
    pLogInfo->maxAllowedLogLevel = gSMBMaxLogLevel;
    
    dwError = SMBAllocateString(
                    pFileLog->pszFilePath,
                    &pLogInfo->pszPath);
    BAIL_ON_SMB_ERROR(dwError);
    
    *ppLogInfo = pLogInfo;
    
cleanup:

    return dwError;
    
error:

    if (pLogInfo)
    {
        SMBFreeLogInfo(pLogInfo);
    }
    
    *ppLogInfo = NULL;

    goto cleanup;
}

DWORD
SMBCloseFileLog(
    HANDLE hLog
    )
{
    PSMB_FILE_LOG pFileLog = (PSMB_FILE_LOG)hLog;
    
    SMBResetLogging();
    
    if (pFileLog)
    {
        SMBFreeFileLogInfo(pFileLog);
    }
    return 0;
}

VOID
SMBLogToFile(
    HANDLE      hLog,
    SMBLogLevel logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    )
{
    PSMB_FILE_LOG pFileLog = (PSMB_FILE_LOG)hLog;
    PSTR pszEntryType = NULL;
    time_t currentTime;
    struct tm tmp = {0};
    char timeBuf[128];
    
    switch (logLevel)
    {
        case SMB_LOG_LEVEL_ALWAYS:
        {
            pszEntryType = SMB_INFO_TAG;
            break;
        }
        case SMB_LOG_LEVEL_ERROR:
        {
            pszEntryType = SMB_ERROR_TAG;
            break;
        }

        case SMB_LOG_LEVEL_WARNING:
        {
            pszEntryType = SMB_WARN_TAG;
            break;
        }

        case SMB_LOG_LEVEL_INFO:
        {
            pszEntryType = SMB_INFO_TAG;
            break;
        }

        case SMB_LOG_LEVEL_VERBOSE:
        {
            pszEntryType = SMB_VERBOSE_TAG;
            break;
        }

        case SMB_LOG_LEVEL_DEBUG:
        {
            pszEntryType = SMB_DEBUG_TAG;
            break;
        }

        default:
        {
            pszEntryType = SMB_VERBOSE_TAG;
            break;
        }
    }

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), SMB_LOG_TIME_FORMAT, &tmp);

    fprintf(pFileLog->fp, "%s:%s:", timeBuf, pszEntryType);
    vfprintf(pFileLog->fp, pszFormat, msgList);
    fprintf(pFileLog->fp, "\n");
    fflush(pFileLog->fp);
}

VOID
SMBFreeFileLogInfo(
    PSMB_FILE_LOG pFileLog
    )
{
    if (pFileLog->fp)
    {
        fclose(pFileLog->fp);
    }

    SMB_SAFE_FREE_STRING(pFileLog->pszFilePath);
    
    SMBFreeMemory(pFileLog);
}
