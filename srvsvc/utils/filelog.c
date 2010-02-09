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
 *        Likewise Server Service (SRVSVC)
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
SrvSvcOpenFileLog(
    PCSTR            pszFilePath,
    SRVSVC_LOG_LEVEL maxAllowedLogLevel,
    PHANDLE          phLog
    )
{
    DWORD dwError = 0;
    PSRVSVC_FILE_LOG pFileLog = NULL;

    if (IsNullOrEmptyString(pszFilePath))
    {
        dwError = SRVSVC_ERROR_INVALID_PARAMETER;
        goto error;
    }

    dwError = LwAllocateMemory(
                    sizeof(SRVSVC_FILE_LOG),
                    (PVOID*)&pFileLog);
    if (dwError)
    {
        goto error;
    }

    dwError = SrvSvcAllocateString(
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

    dwError = SrvSvcSetupLogging(
                    (HANDLE)pFileLog,
                    maxAllowedLogLevel,
                    &SrvSvcLogToFile
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
        SrvSvcFreeFileLogInfo(pFileLog);
    }

    goto cleanup;
}

DWORD
SrvSvcGetFileLogInfo(
    HANDLE hLog,
    PSRVSVC_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PSRVSVC_LOG_INFO pLogInfo = NULL;
    PSRVSVC_FILE_LOG pFileLog = (PSRVSVC_FILE_LOG)hLog;

    if (!hLog)
    {
        dwError = SRVSVC_ERROR_INVALID_PARAMETER;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    if ((gSRVSVC_LOG_TARGET != SRVSVC_LOG_TARGET_FILE) ||
        IsNullOrEmptyString(pFileLog->pszFilePath))
    {
        dwError = SRVSVC_ERROR_INTERNAL;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                    sizeof(SRVSVC_LOG_INFO),
                    (PVOID*)&pLogInfo);
    BAIL_ON_SRVSVC_ERROR(dwError);

    pLogInfo->logTarget = SRVSVC_LOG_TARGET_FILE;
    pLogInfo->maxAllowedLogLevel = gSrvSvcMaxLogLevel;

    dwError = SrvSvcAllocateString(
                    pFileLog->pszFilePath,
                    &pLogInfo->pszPath);
    BAIL_ON_SRVSVC_ERROR(dwError);

    *ppLogInfo = pLogInfo;

cleanup:

    return dwError;

error:

    if (pLogInfo)
    {
        SrvSvcFreeLogInfo(pLogInfo);
    }

    *ppLogInfo = NULL;

    goto cleanup;
}

DWORD
SrvSvcCloseFileLog(
    HANDLE hLog
    )
{
    PSRVSVC_FILE_LOG pFileLog = (PSRVSVC_FILE_LOG)hLog;

    SrvSvcResetLogging();

    if (pFileLog)
    {
        SrvSvcFreeFileLogInfo(pFileLog);
    }
    return 0;
}

VOID
SrvSvcLogToFile(
    HANDLE      hLog,
    SRVSVC_LOG_LEVEL logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    )
{
    PSRVSVC_FILE_LOG pFileLog = (PSRVSVC_FILE_LOG)hLog;
    PSTR pszEntryType = NULL;
    time_t currentTime;
    struct tm tmp = {0};
    char timeBuf[128];

    switch (logLevel)
    {
        case SRVSVC_LOG_LEVEL_ALWAYS:
        {
            pszEntryType = SRVSVC_INFO_TAG;
            break;
        }
        case SRVSVC_LOG_LEVEL_ERROR:
        {
            pszEntryType = SRVSVC_ERROR_TAG;
            break;
        }

        case SRVSVC_LOG_LEVEL_WARNING:
        {
            pszEntryType = SRVSVC_WARN_TAG;
            break;
        }

        case SRVSVC_LOG_LEVEL_INFO:
        {
            pszEntryType = SRVSVC_INFO_TAG;
            break;
        }

        case SRVSVC_LOG_LEVEL_VERBOSE:
        {
            pszEntryType = SRVSVC_VERBOSE_TAG;
            break;
        }

        case SRVSVC_LOG_LEVEL_DEBUG:
        {
            pszEntryType = SRVSVC_DEBUG_TAG;
            break;
        }

        default:
        {
            pszEntryType = SRVSVC_VERBOSE_TAG;
            break;
        }
    }

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), SRVSVC_LOG_TIME_FORMAT, &tmp);

    fprintf(pFileLog->fp, "%s:%s:", timeBuf, pszEntryType);
    vfprintf(pFileLog->fp, pszFormat, msgList);
    fprintf(pFileLog->fp, "\n");
    fflush(pFileLog->fp);
}

VOID
SrvSvcFreeFileLogInfo(
    PSRVSVC_FILE_LOG pFileLog
    )
{
    if (pFileLog->fp)
    {
        fclose(pFileLog->fp);
    }

    SRVSVC_SAFE_FREE_STRING(pFileLog->pszFilePath);

    LwFreeMemory(pFileLog);
}
