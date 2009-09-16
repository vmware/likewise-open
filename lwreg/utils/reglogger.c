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
 *        reglogger.c
 *
 * Abstract:
 *
 *        Registry Logger
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include "includes.h"

DWORD
RegInitLogging(
    PCSTR         pszProgramName,
    RegLogTarget  logTarget,
    RegLogLevel   maxAllowedLogLevel,
    PCSTR         pszPath
    )
{
    DWORD dwError = 0;
    HANDLE hLog = (HANDLE)NULL;

    switch(logTarget)
    {
        case REG_LOG_TARGET_DISABLED:

            break;

        case REG_LOG_TARGET_SYSLOG:

            dwError = RegOpenSyslog(
                        pszProgramName,
                        maxAllowedLogLevel,
                        LOG_PID,
                        LOG_DAEMON,
                        &hLog);
            BAIL_ON_REG_ERROR(dwError);

            break;

        case REG_LOG_TARGET_CONSOLE:

            dwError = RegOpenConsoleLog(
                            maxAllowedLogLevel,
                            &hLog);
            BAIL_ON_REG_ERROR(dwError);

            break;

        case REG_LOG_TARGET_FILE:

            if (IsNullOrEmptyString(pszPath))
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_REG_ERROR(dwError);
            }

            dwError = RegOpenFileLog(
                          pszPath,
                          maxAllowedLogLevel,
                          &hLog);
            BAIL_ON_REG_ERROR(dwError);

            break;

        default:

            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
    }

    gLogTarget = logTarget;
    gRegMaxLogLevel = maxAllowedLogLevel;
    ghLog = hLog;

 cleanup:

    return dwError;

 error:

    gLogTarget = REG_LOG_TARGET_DISABLED;
    ghLog = (HANDLE)NULL;

    goto cleanup;
}

DWORD
RegLogGetInfo(
    PREG_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PREG_LOG_INFO pLogInfo = NULL;

    switch(gLogTarget)
    {
        case REG_LOG_TARGET_DISABLED:
        case REG_LOG_TARGET_CONSOLE:
        case REG_LOG_TARGET_SYSLOG:

            dwError = LwAllocateMemory(
                            sizeof(REG_LOG_INFO),
                            (PVOID*)&pLogInfo);
            BAIL_ON_REG_ERROR(dwError);

            pLogInfo->logTarget = gLogTarget;
            pLogInfo->maxAllowedLogLevel = gRegMaxLogLevel;

            break;

        case REG_LOG_TARGET_FILE:

            dwError = RegGetFileLogInfo(
                            ghLog,
                            &pLogInfo);
            BAIL_ON_REG_ERROR(dwError);

            break;

        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
    }

    *ppLogInfo = pLogInfo;

cleanup:

    return dwError;

error:

    *ppLogInfo = NULL;

    if (pLogInfo)
    {
        RegFreeLogInfo(pLogInfo);
    }

    goto cleanup;
}

DWORD
RegLogSetInfo(
    PREG_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pLogInfo);

    // The only information that is allowed
    // to be set after the log is initialized
    // is the log level

    gRegMaxLogLevel = pLogInfo->maxAllowedLogLevel;

    switch (gLogTarget)
    {
        case REG_LOG_TARGET_SYSLOG:

            RegSetSyslogMask(gRegMaxLogLevel);

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
RegShutdownLogging(
    VOID
    )
{
    DWORD dwError = 0;

    if (ghLog != (HANDLE)NULL)
    {
        switch(gLogTarget)
        {
            case REG_LOG_TARGET_DISABLED:
                break;

            case REG_LOG_TARGET_CONSOLE:
                RegCloseConsoleLog(ghLog);
                break;

            case REG_LOG_TARGET_FILE:
                RegCloseFileLog(ghLog);
                break;

            case REG_LOG_TARGET_SYSLOG:
                RegCloseSyslog(ghLog);
            break;
        }
    }

    return dwError;
}

DWORD
RegSetupLogging(
	HANDLE              hLog,
	RegLogLevel         maxAllowedLogLevel,
	PFN_REG_LOG_MESSAGE pfnLogger
	)
{
	DWORD dwError = 0;

	if ((hLog == (HANDLE)NULL) ||
		!pfnLogger)
	{
		dwError = LW_ERROR_INVALID_PARAMETER;
		goto error;
	}

	ghLog = hLog;
	gRegMaxLogLevel = maxAllowedLogLevel;
	gpfnLogger = pfnLogger;

error:

	return dwError;
}

VOID
RegResetLogging(
    VOID
    )
{
	gRegMaxLogLevel = REG_LOG_LEVEL_ERROR;
	gpfnLogger = NULL;
	ghLog = (HANDLE)NULL;
}

VOID
RegLogMessage(
	PFN_REG_LOG_MESSAGE pfnLogger,
	HANDLE hLog,
	RegLogLevel logLevel,
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
RegValidateLogLevel(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    switch(dwLogLevel)
    {
        case REG_LOG_LEVEL_ALWAYS:
        case REG_LOG_LEVEL_ERROR:
        case REG_LOG_LEVEL_WARNING:
        case REG_LOG_LEVEL_INFO:
        case REG_LOG_LEVEL_VERBOSE:
        case REG_LOG_LEVEL_DEBUG:
        case REG_LOG_LEVEL_TRACE:
            dwError = 0;
            break;
        default:
            dwError = LW_ERROR_INVALID_LOG_LEVEL;
            break;
    }

    return dwError;
}

DWORD
RegTraceInitialize(
    VOID
    )
{
    DWORD dwError = 0;
    PREG_BIT_VECTOR pTraceVector = NULL;

    dwError = RegBitVectorCreate(
        REG_TRACE_FLAG_SENTINEL,
        &pTraceVector
        );

    BAIL_ON_REG_ERROR(dwError);

    if (gpTraceFlags)
    {
        RegBitVectorFree(gpTraceFlags);
    }

    gpTraceFlags = pTraceVector;

cleanup:

    return dwError;

error:

    if (pTraceVector)
    {
        RegBitVectorFree(pTraceVector);
    }

    goto cleanup;
}

BOOLEAN
RegTraceIsFlagSet(
    DWORD dwTraceFlag
    )
{
    BOOLEAN bResult = FALSE;

    if (gpTraceFlags &&
        dwTraceFlag &&
        RegBitVectorIsSet(gpTraceFlags, dwTraceFlag))
    {
        bResult = TRUE;
    }

    return bResult;
}

BOOLEAN
RegTraceIsAllowed(
    DWORD dwTraceFlags[],
    DWORD dwNumFlags
    )
{
    BOOLEAN bResult = FALSE;
    DWORD   iFlag = 0;

    if (gpTraceFlags)
    {
        for (; !bResult && (iFlag < dwNumFlags); iFlag++)
        {
            if (RegTraceIsFlagSet(dwTraceFlags[iFlag]))
            {
                bResult = TRUE;
            }
        }
    }

    return bResult;
}

DWORD
RegTraceSetFlag(
    DWORD dwTraceFlag
    )
{
    DWORD dwError = 0;

    if (!gpTraceFlags)
    {
        dwError = LW_ERROR_TRACE_NOT_INITIALIZED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegBitVectorSetBit(
                    gpTraceFlags,
                    dwTraceFlag);

error:

    return dwError;
}

DWORD
RegTraceUnsetFlag(
    DWORD dwTraceFlag
    )
{
    DWORD dwError = 0;

    if (!gpTraceFlags)
    {
        dwError = LW_ERROR_TRACE_NOT_INITIALIZED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegBitVectorUnsetBit(
                    gpTraceFlags,
                    dwTraceFlag);

error:

    return dwError;
}

VOID
RegTraceShutdown(
    VOID
    )
{
    if (gpTraceFlags)
    {
        RegBitVectorFree(gpTraceFlags);
        gpTraceFlags = NULL;
    }
}
