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
 *        lwps-logger.c
 *
 * Abstract:
 *
 *        Likewise Password Storage (LWPS)
 *
 *        Log API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */

#include "lwps-utils.h"
#include "lwps-logger.h"
#include "lwps-sysfuncs.h"

static const PSTR ERROR_TAG   = "ERROR";
static const PSTR WARN_TAG    = "WARNING";
static const PSTR INFO_TAG    = "INFO";
static const PSTR VERBOSE_TAG = "VERBOSE";

static const PSTR LOG_TIME_FORMAT = "%Y%m%d%H%M%S";

LOGINFO gLwpsLogInfo =
{
    PTHREAD_MUTEX_INITIALIZER,
    LOG_LEVEL_ERROR,
    LOG_DISABLED,
    {"", NULL},
    {"lwps", LOG_PID, LOG_DAEMON},
    0,
    0,
    0
};

#define LWPS__LOCK_LOGGER   pthread_mutex_lock(&gLwpsLogInfo.lock)
#define LWPS__UNLOCK_LOGGER pthread_mutex_unlock(&gLwpsLogInfo.lock)

DWORD
lwps_validate_log_level(
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
lwps_set_syslogmask(
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
lwps_init_logging_to_syslog(
    DWORD   dwLogLevel,
    BOOLEAN bEnableDebug,
    PCSTR   pszIdentifier,
    DWORD   dwOption,
    DWORD   dwFacility
    )
{
    DWORD dwError = 0;
    
    LWPS__LOCK_LOGGER;

    dwError = lwps_validate_log_level(dwLogLevel);
    BAIL_ON_LWPS_ERROR(dwError);

    gLwpsLogInfo.logTarget = LOG_TO_SYSLOG;
    
    gLwpsLogInfo.bDebug = bEnableDebug;

    strncpy(gLwpsLogInfo.syslog.szIdentifier, pszIdentifier, PATH_MAX);
    *(gLwpsLogInfo.syslog.szIdentifier+PATH_MAX) = '\0';

    gLwpsLogInfo.syslog.dwOption = dwOption;
    gLwpsLogInfo.syslog.dwFacility = dwFacility;

    openlog(pszIdentifier, dwOption, dwFacility);

    lwps_set_syslogmask(dwLogLevel);
    gLwpsLogInfo.dwLogLevel = dwLogLevel;

    gLwpsLogInfo.bLoggingInitiated = 1;

    LWPS__UNLOCK_LOGGER;

    return (dwError);

  error:

    LWPS__UNLOCK_LOGGER;
    
    return (dwError);
}

DWORD
lwps_init_logging_to_file(
    DWORD   dwLogLevel,
    BOOLEAN bEnableDebug,
    PSTR    pszLogFilePath
    )
{
    DWORD dwError = 0;

    LWPS__LOCK_LOGGER;

    gLwpsLogInfo.logTarget = LOG_TO_FILE;
    gLwpsLogInfo.bDebug = bEnableDebug;
    
    if (IsNullOrEmptyString(pszLogFilePath)) {

       gLwpsLogInfo.bLogToConsole = TRUE;
       gLwpsLogInfo.logfile.logHandle = stdout;

    } else {

       strncpy(gLwpsLogInfo.logfile.szLogPath, pszLogFilePath, PATH_MAX);
       *(gLwpsLogInfo.logfile.szLogPath+PATH_MAX) = '\0';
    
       gLwpsLogInfo.logfile.logHandle = NULL;
       if (gLwpsLogInfo.logfile.szLogPath[0] != '\0') {
          gLwpsLogInfo.logfile.logHandle = freopen(gLwpsLogInfo.logfile.szLogPath, "w", stdout);
          if (gLwpsLogInfo.logfile.logHandle == NULL) {
             dwError = errno;
             fprintf(stderr, "Failed to redirect logging. %s", strerror(errno));
             goto error;
          }
       }
    }
  
    dwError = lwps_validate_log_level(dwLogLevel);
    if (dwError) {
       fprintf(stderr, "An invalid log level [%d] was specified.", dwLogLevel);
       goto error;
    }

    gLwpsLogInfo.dwLogLevel = dwLogLevel;

    gLwpsLogInfo.bLoggingInitiated = 1;

    LWPS__UNLOCK_LOGGER;

    return (dwError);

  error:

    if (!gLwpsLogInfo.bLogToConsole &&
        (gLwpsLogInfo.logfile.logHandle != NULL)) {
        fclose(gLwpsLogInfo.logfile.logHandle);
        gLwpsLogInfo.logfile.logHandle = NULL;
    }

    LWPS__UNLOCK_LOGGER;

    return (dwError);
}

void
lwps_close_log()
{
    LWPS__LOCK_LOGGER;

    if (!gLwpsLogInfo.bLoggingInitiated) {
        LWPS__UNLOCK_LOGGER;
        return;
    }

    switch (gLwpsLogInfo.logTarget)
    {
        case LOG_TO_SYSLOG:
            {
                /* close connection to syslog */
                closelog();
            }
            break;
        case LOG_TO_FILE:
            {
                if (!gLwpsLogInfo.bLogToConsole &&
                    (gLwpsLogInfo.logfile.logHandle != NULL)) {
                    fclose(gLwpsLogInfo.logfile.logHandle);
                    gLwpsLogInfo.logfile.logHandle = NULL;
                }
            }
            break;
    }

    LWPS__UNLOCK_LOGGER;
}

void
lwps_log_to_file_mt_unsafe(
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

        default:
        {
            pszEntryType = VERBOSE_TAG;
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
lwps_log_to_syslog_mt_unsafe(
    DWORD dwLogLevel,
    PSTR pszFormat,
    va_list msgList
    )
{
    switch (dwLogLevel)
    {
        case LOG_LEVEL_ALWAYS:
        {
            lwps_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }
        case LOG_LEVEL_ERROR:
        {
            lwps_vsyslog(LOG_ERR, pszFormat, msgList);
            break;
        }

        case LOG_LEVEL_WARNING:
        {
            lwps_vsyslog(LOG_WARNING, pszFormat, msgList);
            break;
        }

        case LOG_LEVEL_INFO:
        {
            lwps_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }

        default:
        {
            lwps_vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }
    }
}

void
lwps_log_message(
    DWORD dwLogLevel,
    PSTR pszFormat,...
    )
{
    va_list argp;
  
    LWPS__LOCK_LOGGER;

    if ( !gLwpsLogInfo.bLoggingInitiated ||
         gLwpsLogInfo.logTarget == LOG_DISABLED ) {
        LWPS__UNLOCK_LOGGER;
        return;
    }

    if (gLwpsLogInfo.dwLogLevel < dwLogLevel) {
        LWPS__UNLOCK_LOGGER;
        return;
    }

    va_start(argp, pszFormat);

    switch (gLwpsLogInfo.logTarget)
    {
        case LOG_TO_SYSLOG:
        {
            lwps_log_to_syslog_mt_unsafe(dwLogLevel, pszFormat, argp);
            break;
        }
        case LOG_TO_FILE:
        {
            lwps_log_to_file_mt_unsafe(dwLogLevel, pszFormat, argp);
            break;
        }
    }

    va_end(argp);

    LWPS__UNLOCK_LOGGER;
}

DWORD
lwps_set_log_level(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    LWPS__LOCK_LOGGER;

    dwError = lwps_validate_log_level(dwLogLevel);
    BAIL_ON_LWPS_ERROR(dwError);

    gLwpsLogInfo.dwLogLevel = dwLogLevel;

    LWPS__UNLOCK_LOGGER;

    return (dwError);
    
error:

    LWPS__UNLOCK_LOGGER;

    return (dwError);
}

