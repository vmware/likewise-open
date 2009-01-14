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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Server Service logging utilities
 *
 */
#include "includes.h"

static PCSTR ERROR_TAG   = "ERROR";
static PCSTR WARN_TAG    = "WARNING";
static PCSTR INFO_TAG    = "INFO";
static PCSTR VERBOSE_TAG = "VERBOSE";

static PCSTR LOG_TIME_FORMAT = "%Y%m%d%H%M%S";

DWORD
SRVSVCInitLoggingToSyslog(
    DWORD dwLogLevel,
    PCSTR pszIdentifier,
    DWORD dwOption,
    DWORD dwFacility
    )
{
    DWORD dwError = 0;

    SRVSVC_LOCK_LOGGER;

    dwError = SRVSVCValidateLogLevel(dwLogLevel);
    BAIL_ON_SRVSVC_ERROR(dwError);

    gSrvSvcLogInfo.logTarget = LOG_TO_SYSLOG;

    strncpy(gSrvSvcLogInfo.data.syslog.szIdentifier, pszIdentifier, PATH_MAX);
    *(gSrvSvcLogInfo.data.syslog.szIdentifier+PATH_MAX) = '\0';

    gSrvSvcLogInfo.data.syslog.dwOption = dwOption;
    gSrvSvcLogInfo.data.syslog.dwFacility = dwFacility;

    openlog(pszIdentifier, dwOption, dwFacility);

    SRVSVCSetSyslogMask(dwLogLevel);

    gSrvSvcLogInfo.bLoggingInitiated = 1;

error:

    SRVSVC_UNLOCK_LOGGER;

    return dwError;
}

VOID
SRVSVCSetSyslogMask(
    DWORD dwLogLevel
    )
{
    DWORD dwSysLogLevel;

    switch (dwLogLevel)
    {
        case LOG_LEVEL_ALWAYS:
        {
            dwSysLogLevel = LOG_UPTO(LOG_INFO);
            break;
        }
        case LOG_LEVEL_ERROR:
        {
            dwSysLogLevel = LOG_UPTO(LOG_ERR);
            break;
        }

        case LOG_LEVEL_WARNING:
        {
            dwSysLogLevel = LOG_UPTO(LOG_WARNING);
            break;
        }

        case LOG_LEVEL_INFO:
        {
            dwSysLogLevel = LOG_UPTO(LOG_INFO);
            break;
        }

        default:
        {
            dwSysLogLevel = LOG_UPTO(LOG_INFO);
            break;
        }
    }

    setlogmask(dwSysLogLevel);
}

DWORD
SRVSVCInitLoggingToFile(
     DWORD dwLogLevel,
     PCSTR  pszLogFilePath
     )
{
    DWORD dwError = 0;
    FILE* pLog = NULL;

    SRVSVC_LOCK_LOGGER;

    dwError = SRVSVCValidateLogLevel(dwLogLevel);
    BAIL_ON_SRVSVC_ERROR(dwError);

    if (IsNullOrEmptyString(pszLogFilePath))
    {
        gSrvSvcLogInfo.logTarget = LOG_TO_CONSOLE;
        gSrvSvcLogInfo.data.logfile.szLogPath[0] = '\0';
        gSrvSvcLogInfo.data.logfile.logHandle = stdout;
    }
    else
    {
        gSrvSvcLogInfo.logTarget = LOG_TO_FILE;
        strcpy(gSrvSvcLogInfo.data.logfile.szLogPath, pszLogFilePath);

        gSrvSvcLogInfo.data.logfile.logHandle = NULL;

        if (gSrvSvcLogInfo.data.logfile.szLogPath[0] != '\0')
        {
             pLog = freopen(
                        gSrvSvcLogInfo.data.logfile.szLogPath,
                        "w",
                        stderr);
            if (gSrvSvcLogInfo.data.logfile.logHandle == NULL) {
                dwError = errno;
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
        }

        gSrvSvcLogInfo.data.logfile.logHandle = pLog;
    }

    gSrvSvcLogInfo.dwLogLevel = dwLogLevel;

    gSrvSvcLogInfo.bLoggingInitiated = 1;

cleanup:

    SRVSVC_UNLOCK_LOGGER;

    return (dwError);

 error:

    if (pLog != NULL) {
        fclose(pLog);
    }

    goto cleanup;
}


DWORD
SRVSVCSetLogLevel(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    SRVSVC_LOCK_LOGGER;

    dwError = SRVSVCValidateLogLevel(dwLogLevel);
    BAIL_ON_SRVSVC_ERROR(dwError);

    gSrvSvcLogInfo.dwLogLevel = dwLogLevel;

error:

    SRVSVC_UNLOCK_LOGGER;

    return dwError;
}

DWORD
SRVSVCValidateLogLevel(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    if (dwLogLevel < 1) {
        dwError = EINVAL;
    }

    return dwError;
}

VOID
SRVSVCLogMessage(
    DWORD dwLogLevel,
    PCSTR pszFormat,
    ...
    )
{
    va_list argp;

    SRVSVC_LOCK_LOGGER;

    if ( !gSrvSvcLogInfo.bLoggingInitiated ||
        gSrvSvcLogInfo.logTarget == LOG_DISABLED ) {
        goto cleanup;
    }

    if (gSrvSvcLogInfo.dwLogLevel < dwLogLevel) {
        goto cleanup;
    }

    va_start(argp, pszFormat);

    switch (gSrvSvcLogInfo.logTarget)
    {
        case LOG_TO_SYSLOG:
        {
            SRVSVCLogToSyslog_InLock(
                            dwLogLevel,
                            pszFormat,
                            argp);
            break;
        }
        case LOG_TO_FILE:
        case LOG_TO_CONSOLE:
        {
            SRVSVCLogToFile_InLock(
                            &gSrvSvcLogInfo.data.logfile,
                            dwLogLevel,
                            pszFormat,
                            argp);
            break;
        }
    }

    va_end(argp);

cleanup:

    SRVSVC_UNLOCK_LOGGER;

    return;
}

VOID
SRVSVCLogToFile_InLock(
    PLOGFILEINFO pLogInfo,
    DWORD        dwLogLevel,
    PCSTR        pszFormat,
    va_list      msgList
    )
{
    PCSTR pszEntryType = NULL;
    BOOLEAN bUseErrorStream = FALSE;
    time_t currentTime;
    struct tm tmp;
    char timeBuf[1024];
    FILE* pTarget = NULL;

    switch (dwLogLevel)
    {
        case LOG_LEVEL_ALWAYS:
        {
            pszEntryType = INFO_TAG;
            break;
        }
        case LOG_LEVEL_ERROR:
        {
            pszEntryType = ERROR_TAG;
            bUseErrorStream = TRUE;
            break;
        }

        case LOG_LEVEL_WARNING:
        {
            pszEntryType = WARN_TAG;
            bUseErrorStream = TRUE;
            break;
        }

        case LOG_LEVEL_INFO:
        {
            pszEntryType = INFO_TAG;
            break;
        }

        default:
        {
            pszEntryType = VERBOSE_TAG;
            break;
        }
    }

    //Set pTarget
    if (pLogInfo->logHandle)
        pTarget = pLogInfo->logHandle;
    else
        pTarget = bUseErrorStream ? stderr : stdout;

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), LOG_TIME_FORMAT, &tmp);

    fprintf(pTarget, "%s:0x%lx:%s:", timeBuf, (unsigned long)pthread_self(), pszEntryType);
    vfprintf(pTarget, pszFormat, msgList);
    fprintf(pTarget, "\n");
    fflush(pTarget);
}

VOID
SRVSVCLogToSyslog_InLock(
    DWORD   dwLogLevel,
    PCSTR   pszFormat,
    va_list msgList
    )
{
    switch (dwLogLevel)
    {
        case LOG_LEVEL_ALWAYS:
        {
            evt_sys_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }
        case LOG_LEVEL_ERROR:
        {
            evt_sys_vsyslog(LOG_ERR, pszFormat, msgList);
            break;
        }

        case LOG_LEVEL_WARNING:
        {
            evt_sys_vsyslog(LOG_WARNING, pszFormat, msgList);
            break;
        }

        case LOG_LEVEL_INFO:
        {
            evt_sys_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }

        default:
        {
            evt_sys_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }
    }
}

VOID
SRVSVCCloseLog()
{
    SRVSVC_LOCK_LOGGER;

    if (!gSrvSvcLogInfo.bLoggingInitiated) {
        goto cleanup;
    }

    switch (gSrvSvcLogInfo.logTarget)
    {
        case LOG_TO_SYSLOG:
            {
                /* close connection to syslog */
                closelog();
            }
            break;
        case LOG_TO_FILE:
            {
                if (gSrvSvcLogInfo.data.logfile.logHandle != NULL) {
                    fclose(gSrvSvcLogInfo.data.logfile.logHandle);
                    gSrvSvcLogInfo.data.logfile.logHandle = NULL;
                }
            }
            break;
    }

cleanup:

    SRVSVC_UNLOCK_LOGGER;

    return;
}
