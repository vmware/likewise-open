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
 *        logger.c
 *
 * Abstract:
 *
 *        Likewise IO (SRVSVC)
 *
 *        Utilities
 *
 *        Logger
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
SrvSvcInitLogging(
    PCSTR         pszProgramName,
    SRVSVC_LOG_TARGET  logTarget,
    SRVSVC_LOG_LEVEL   maxAllowedLogLevel,
    PCSTR         pszPath
    )
{
    DWORD dwError = 0;
    HANDLE hLog = (HANDLE)NULL;

    switch(logTarget)
    {
        case SRVSVC_LOG_TARGET_DISABLED:

            break;

        case SRVSVC_LOG_TARGET_SYSLOG:

            dwError = SrvSvcOpenSyslog(
                        pszProgramName,
                        maxAllowedLogLevel,
                        LOG_PID,
                        LOG_DAEMON,
                        &hLog);
            BAIL_ON_SRVSVC_ERROR(dwError);

            break;

        case SRVSVC_LOG_TARGET_CONSOLE:

            dwError = SrvSvcOpenConsoleLog(
                            maxAllowedLogLevel,
                            &hLog);
            BAIL_ON_SRVSVC_ERROR(dwError);

            break;

        case SRVSVC_LOG_TARGET_FILE:

            if (IsNullOrEmptyString(pszPath))
            {
                dwError = SRVSVC_ERROR_INVALID_PARAMETER;
                BAIL_ON_SRVSVC_ERROR(dwError);
            }

            dwError = SrvSvcOpenFileLog(
                          pszPath,
                          maxAllowedLogLevel,
                          &hLog);
            BAIL_ON_SRVSVC_ERROR(dwError);

            break;

        default:

            dwError = SRVSVC_ERROR_INVALID_PARAMETER;
            BAIL_ON_SRVSVC_ERROR(dwError);
    }

    gSRVSVC_LOG_TARGET = logTarget;
    gSrvSvcMaxLogLevel = maxAllowedLogLevel;
    ghSrvSvcLog = hLog;

 cleanup:

    return dwError;

 error:

    gSRVSVC_LOG_TARGET = SRVSVC_LOG_TARGET_DISABLED;
    ghSrvSvcLog = (HANDLE)NULL;

    goto cleanup;
}

DWORD
SrvSvcLogGetInfo(
    PSRVSVC_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PSRVSVC_LOG_INFO pLogInfo = NULL;

    switch(gSRVSVC_LOG_TARGET)
    {
        case SRVSVC_LOG_TARGET_DISABLED:
        case SRVSVC_LOG_TARGET_CONSOLE:
        case SRVSVC_LOG_TARGET_SYSLOG:

            dwError = LwAllocateMemory(
                            sizeof(SRVSVC_LOG_INFO),
                            (PVOID*)&pLogInfo);
            BAIL_ON_SRVSVC_ERROR(dwError);

            pLogInfo->logTarget = gSRVSVC_LOG_TARGET;
            pLogInfo->maxAllowedLogLevel = gSrvSvcMaxLogLevel;

            break;

        case SRVSVC_LOG_TARGET_FILE:

            dwError = SrvSvcGetFileLogInfo(
                            ghSrvSvcLog,
                            &pLogInfo);
            BAIL_ON_SRVSVC_ERROR(dwError);

            break;

        default:
            dwError = SRVSVC_ERROR_INVALID_PARAMETER;
            BAIL_ON_SRVSVC_ERROR(dwError);
    }

    *ppLogInfo = pLogInfo;

cleanup:

    return dwError;

error:

    *ppLogInfo = NULL;

    if (pLogInfo)
    {
        SrvSvcFreeLogInfo(pLogInfo);
    }

    goto cleanup;
}

DWORD
SrvSvcLogSetInfo(
    PSRVSVC_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;

    if (!pLogInfo)
    {
        dwError = SRVSVC_ERROR_INVALID_PARAMETER;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    // The only information that is allowed
    // to be set after the log is initialized
    // is the log level

    gSrvSvcMaxLogLevel = pLogInfo->maxAllowedLogLevel;

    switch (gSRVSVC_LOG_TARGET)
    {
        case SRVSVC_LOG_TARGET_SYSLOG:

            SrvSvcSetSyslogMask(gSrvSvcMaxLogLevel);

            break;

        default:

            break;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
SrvSvcShutdownLogging(
    VOID
    )
{
    DWORD dwError = 0;

    if (ghSrvSvcLog != (HANDLE)NULL)
    {
        switch(gSRVSVC_LOG_TARGET)
        {
            case SRVSVC_LOG_TARGET_DISABLED:
                break;

            case SRVSVC_LOG_TARGET_CONSOLE:
                SrvSvcCloseConsoleLog(ghSrvSvcLog);
                break;

            case SRVSVC_LOG_TARGET_FILE:
                SrvSvcCloseFileLog(ghSrvSvcLog);
                break;

            case SRVSVC_LOG_TARGET_SYSLOG:
                SrvSvcCloseSyslog(ghSrvSvcLog);
            break;
        }
    }

    return dwError;
}

DWORD
SrvSvcSetupLogging(
	HANDLE              hLog,
	SRVSVC_LOG_LEVEL         maxAllowedLogLevel,
	PFN_SRVSVC_LOG_MESSAGE pfnLogger
	)
{
	DWORD dwError = 0;

	if ((hLog == (HANDLE)NULL) ||
		!pfnLogger)
	{
		dwError = SRVSVC_ERROR_INVALID_PARAMETER;
		goto error;
	}

	ghSrvSvcLog = hLog;
	gSrvSvcMaxLogLevel = maxAllowedLogLevel;
	gpfnSrvSvcLogger = pfnLogger;

error:

	return dwError;
}

VOID
SrvSvcResetLogging(
    VOID
    )
{
	gSrvSvcMaxLogLevel = SRVSVC_LOG_LEVEL_ERROR;
	gpfnSrvSvcLogger = NULL;
	ghSrvSvcLog = (HANDLE)NULL;
}

VOID
SrvSvcLogMessage(
	PFN_SRVSVC_LOG_MESSAGE pfnLogger,
	HANDLE hLog,
	SRVSVC_LOG_LEVEL logLevel,
	PCSTR  pszFormat,
	...
	)
{
	va_list msgList;
	va_start(msgList, pszFormat);

	pfnLogger(hLog, logLevel, pszFormat, msgList);

	va_end(msgList);
}

DWORD
SrvSvcValidateLogLevel(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    switch(dwLogLevel)
    {
        case SRVSVC_LOG_LEVEL_ALWAYS:
        case SRVSVC_LOG_LEVEL_ERROR:
        case SRVSVC_LOG_LEVEL_WARNING:
        case SRVSVC_LOG_LEVEL_INFO:
        case SRVSVC_LOG_LEVEL_VERBOSE:
        case SRVSVC_LOG_LEVEL_DEBUG:
            dwError = 0;
            break;
        default:
            dwError = SRVSVC_ERROR_INVALID_PARAMETER;
            break;
    }

    return dwError;
}
