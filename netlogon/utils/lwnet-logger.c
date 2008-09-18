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
 *        lwnet-logger.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        Logger API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

static const PSTR ERROR_TAG   = "ERROR";
static const PSTR WARN_TAG    = "WARNING";
static const PSTR INFO_TAG    = "INFO";
static const PSTR VERBOSE_TAG = "VERBOSE";
static const PSTR DEBUG_TAG   = "DEBUG";

static const PSTR LOG_TIME_FORMAT = "%Y%m%d%H%M%S";

#define LWNET_LOCK_LOGGER   pthread_mutex_lock(&gLwnetLogInfo.lock)
#define LWNET_UNLOCK_LOGGER pthread_mutex_unlock(&gLwnetLogInfo.lock)

DWORD
lwnet_validate_log_level(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    if (dwLogLevel < 1) {
        dwError = EINVAL;
    }

    return (dwError);
}

void
lwnet_set_syslogmask(
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
        case LOG_LEVEL_VERBOSE:
        {
            dwSysLogLevel = LOG_UPTO(LOG_INFO);
            break;
        }

        default:
        {
            dwSysLogLevel = LOG_UPTO(LOG_DEBUG);
            break;
        }
    }

    setlogmask(dwSysLogLevel);
}

DWORD
lwnet_init_logging_to_syslog(
    DWORD   dwLogLevel,
    BOOLEAN bEnableDebug,
    PCSTR   pszIdentifier,
    DWORD   dwOption,
    DWORD   dwFacility
    )
{
    DWORD dwError = 0;
    
    LWNET_LOCK_LOGGER;

    dwError = lwnet_validate_log_level(dwLogLevel);
    BAIL_ON_LWNET_ERROR(dwError);

    gLwnetLogInfo.logTarget = LOG_TO_SYSLOG;
    
    gLwnetLogInfo.bDebug = bEnableDebug;

    strncpy(gLwnetLogInfo.syslog.szIdentifier, pszIdentifier, PATH_MAX);
    *(gLwnetLogInfo.syslog.szIdentifier+PATH_MAX) = '\0';

    gLwnetLogInfo.syslog.dwOption = dwOption;
    gLwnetLogInfo.syslog.dwFacility = dwFacility;

    openlog(pszIdentifier, dwOption, dwFacility);

    lwnet_set_syslogmask(dwLogLevel);
    gLwnetLogInfo.dwLogLevel = dwLogLevel;

    gLwnetLogInfo.bLoggingInitiated = 1;

    LWNET_UNLOCK_LOGGER;

    return (dwError);

  error:

    LWNET_UNLOCK_LOGGER;
    
    return (dwError);
}

DWORD
lwnet_init_logging_to_file(
    DWORD   dwLogLevel,
    BOOLEAN bEnableDebug,
    PSTR    pszLogFilePath
    )
{
    DWORD dwError = 0;

    LWNET_LOCK_LOGGER;

    gLwnetLogInfo.logTarget = LOG_TO_FILE;
    gLwnetLogInfo.bDebug = bEnableDebug;
    
    if (IsNullOrEmptyString(pszLogFilePath)) {

       gLwnetLogInfo.bLogToConsole = TRUE;
       gLwnetLogInfo.logfile.logHandle = stdout;

    } else {

       strncpy(gLwnetLogInfo.logfile.szLogPath, pszLogFilePath, PATH_MAX);
       *(gLwnetLogInfo.logfile.szLogPath+PATH_MAX) = '\0';
    
       gLwnetLogInfo.logfile.logHandle = NULL;
       if (gLwnetLogInfo.logfile.szLogPath[0] != '\0') {
          gLwnetLogInfo.logfile.logHandle = freopen(gLwnetLogInfo.logfile.szLogPath, "w", stdout);
          if (gLwnetLogInfo.logfile.logHandle == NULL) {
             dwError = errno;
             fprintf(stderr, "Failed to redirect logging. %s", strerror(errno));
             goto error;
          }
       }
    }
  
    dwError = lwnet_validate_log_level(dwLogLevel);
    if (dwError) {
       fprintf(stderr, "An invalid log level [%d] was specified.", dwLogLevel);
       goto error;
    }

    gLwnetLogInfo.dwLogLevel = dwLogLevel;

    gLwnetLogInfo.bLoggingInitiated = 1;

    LWNET_UNLOCK_LOGGER;

    return (dwError);

  error:

    if (!gLwnetLogInfo.bLogToConsole &&
        (gLwnetLogInfo.logfile.logHandle != NULL)) {
        fclose(gLwnetLogInfo.logfile.logHandle);
        gLwnetLogInfo.logfile.logHandle = NULL;
    }

    LWNET_UNLOCK_LOGGER;

    return (dwError);
}

void
lwnet_close_log(
    void
    )
{
    LWNET_LOCK_LOGGER;

    if (!gLwnetLogInfo.bLoggingInitiated) {
        LWNET_UNLOCK_LOGGER;
        return;
    }

    switch (gLwnetLogInfo.logTarget)
    {
        case LOG_TO_SYSLOG:
            {
                /* close connection to syslog */
                closelog();
            }
            break;
        case LOG_TO_FILE:
            {
                if (!gLwnetLogInfo.bLogToConsole &&
                    (gLwnetLogInfo.logfile.logHandle != NULL)) {
                    fclose(gLwnetLogInfo.logfile.logHandle);
                    gLwnetLogInfo.logfile.logHandle = NULL;
                }
            }
            break;
    }

    LWNET_UNLOCK_LOGGER;
}

void
lwnet_log_to_file_mt_unsafe(
    DWORD dwLogLevel,
    PSTR pszFormat,
    va_list msgList
    )
{
    PSTR pszEntryType = NULL;
    time_t currentTime;
    struct tm tmp;
    char timeBuf[1024];
    FILE* pTarget = NULL;

    switch (dwLogLevel)
    {
        case LOG_LEVEL_ALWAYS:
        {
            pszEntryType = INFO_TAG;
            pTarget = stdout;
            break;
        }
        case LOG_LEVEL_ERROR:
        {
            pszEntryType = ERROR_TAG;
            pTarget = stderr;
            break;
        }

        case LOG_LEVEL_WARNING:
        {
            pszEntryType = WARN_TAG;
            pTarget = stderr;
            break;
        }

        case LOG_LEVEL_INFO:
        {
            pszEntryType = INFO_TAG;
            pTarget = stdout;
            break;
        }

        case LOG_LEVEL_VERBOSE:
        {
            pszEntryType = VERBOSE_TAG;
            pTarget = stdout;
            break;
        }

        default:
        {
            pszEntryType = DEBUG_TAG;
            pTarget = stdout;
            break;
        }
    }

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), LOG_TIME_FORMAT, &tmp);

    fprintf(pTarget, "%s:0x%x:%s:", timeBuf, (unsigned int)pthread_self(), pszEntryType);
    vfprintf(pTarget, pszFormat, msgList);
    fprintf(pTarget, "\n");
    fflush(pTarget);
}

void
lwnet_log_to_syslog_mt_unsafe(
    DWORD dwLogLevel,
    PSTR pszFormat,
    va_list msgList
    )
{
    switch (dwLogLevel)
    {
        case LOG_LEVEL_ALWAYS:
        {
            lwnet_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }
        case LOG_LEVEL_ERROR:
        {
            lwnet_vsyslog(LOG_ERR, pszFormat, msgList);
            break;
        }

        case LOG_LEVEL_WARNING:
        {
            lwnet_vsyslog(LOG_WARNING, pszFormat, msgList);
            break;
        }

        case LOG_LEVEL_INFO:
        case LOG_LEVEL_VERBOSE:
        {
            lwnet_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }

        default:
        {
            lwnet_vsyslog(LOG_DEBUG, pszFormat, msgList);
            break;
        }
    }
}

void
lwnet_log_message(
    DWORD dwLogLevel,
    PSTR pszFormat,...
    )
{
    va_list argp;
  
    LWNET_LOCK_LOGGER;

    if ( !gLwnetLogInfo.bLoggingInitiated ||
         gLwnetLogInfo.logTarget == LOG_DISABLED ) {
        LWNET_UNLOCK_LOGGER;
        return;
    }

    if (gLwnetLogInfo.dwLogLevel < dwLogLevel) {
        LWNET_UNLOCK_LOGGER;
        return;
    }

    va_start(argp, pszFormat);

    switch (gLwnetLogInfo.logTarget)
    {
        case LOG_TO_SYSLOG:
        {
            lwnet_log_to_syslog_mt_unsafe(dwLogLevel, pszFormat, argp);
            break;
        }
        case LOG_TO_FILE:
        {
            lwnet_log_to_file_mt_unsafe(dwLogLevel, pszFormat, argp);
            break;
        }
    }

    va_end(argp);

    LWNET_UNLOCK_LOGGER;
}

DWORD
lwnet_set_log_level(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    LWNET_LOCK_LOGGER;

    dwError = lwnet_validate_log_level(dwLogLevel);
    BAIL_ON_LWNET_ERROR(dwError);

    gLwnetLogInfo.dwLogLevel = dwLogLevel;

    LWNET_UNLOCK_LOGGER;

    return (dwError);
    
error:

    LWNET_UNLOCK_LOGGER;

    return (dwError);
}

