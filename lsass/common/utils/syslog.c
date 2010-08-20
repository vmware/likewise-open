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
 *        logger.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Logging API
 *
 *        Implemenation of logging to syslog
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "includes.h"

#define LSA_SYSLOG_MAX_LEVEL LOG_INFO

DWORD
LsaOpenSyslog(
    PCSTR       pszIdentifier,
    LsaLogLevel maxAllowedLogLevel,
    DWORD       dwOptions,
    DWORD       dwFacility,
    PHANDLE     phLog
    )
{
    DWORD dwError = 0;
    PLSA_SYS_LOG pSyslog = NULL;

    dwError = LwAllocateMemory(
                sizeof(LSA_SYS_LOG),
                (PVOID*)&pSyslog);
    if (dwError)
    {
        goto error;
    }

    dwError = LwAllocateString(
                  (LW_IS_NULL_OR_EMPTY_STR(pszIdentifier) ? "lsass" : pszIdentifier),
                  &pSyslog->pszIdentifier);
    if (dwError)
    {
        goto error;
    }

    pSyslog->dwOptions = dwOptions;
    pSyslog->dwFacility = dwFacility;

    openlog(
        pSyslog->pszIdentifier,
        pSyslog->dwOptions,
        pSyslog->dwFacility);

    pSyslog->bOpened = TRUE;

    // This always need to be the max log level
    // with which this module can call syslog.
    setlogmask(LOG_UPTO(LSA_SYSLOG_MAX_LEVEL));

    dwError = LsaSetupLogging(
                    (HANDLE)pSyslog,
                    maxAllowedLogLevel,
                    &LsaLogToSyslog);
    if (dwError)
    {
        goto error;
    }

    *phLog = (HANDLE)pSyslog;

cleanup:

    return dwError;

error:

    *phLog = (HANDLE)NULL;

    if (pSyslog)
    {
        LsaFreeSysLogInfo(pSyslog);
    }

    goto cleanup;
}

VOID
LsaLogToSyslog(
    HANDLE      hLog,
    LsaLogLevel logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    )
{
    int priority = LOG_INFO;

    switch (logLevel)
    {
        case LSA_LOG_LEVEL_ERROR:
            priority = LOG_ERR;
            break;
        case LSA_LOG_LEVEL_WARNING:
            priority = LOG_WARNING;
            break;
        default:
            priority = LOG_INFO;
    }

    // Sanity check
    priority = LW_MIN(priority, LSA_SYSLOG_MAX_LEVEL);

    lsass_vsyslog(priority, pszFormat, msgList);
}

DWORD
LsaCloseSyslog(
    HANDLE hLog
    )
{
    PLSA_SYS_LOG pSysLog = (PLSA_SYS_LOG)hLog;

    LsaResetLogging();

    if (pSysLog)
    {
        LsaFreeSysLogInfo(pSysLog);
    }
    return 0;
}

VOID
LsaFreeSysLogInfo(
    PLSA_SYS_LOG pSysLog
    )
{
    if (pSysLog->bOpened)
    {
        /* close connection to syslog */
        closelog();
    }

    LW_SAFE_FREE_STRING(pSysLog->pszIdentifier);

    LwFreeMemory(pSysLog);
}

