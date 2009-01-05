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
 *        Likewise Server Message Block (LSMB)
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
SMBInitLogging(
    PCSTR         pszProgramName,
    SMBLogTarget  logTarget,
    SMBLogLevel   maxAllowedLogLevel,
    PCSTR         pszPath
    )
{
    DWORD dwError = 0;
    HANDLE hLog = (HANDLE)NULL;

    switch(logTarget)
    {
        case SMB_LOG_TARGET_DISABLED:
            
            break;
            
        case SMB_LOG_TARGET_SYSLOG:
        
            dwError = SMBOpenSyslog(
                        pszProgramName,
                        maxAllowedLogLevel,
                        LOG_PID,
                        LOG_DAEMON,
                        &hLog);
            BAIL_ON_SMB_ERROR(dwError);
      
            break;
            
        case SMB_LOG_TARGET_CONSOLE:

            dwError = SMBOpenConsoleLog(
                            maxAllowedLogLevel,
                            &hLog);
            BAIL_ON_SMB_ERROR(dwError);
              
            break;

        case SMB_LOG_TARGET_FILE:
            
            if (IsNullOrEmptyString(pszPath))
            {
                dwError = SMB_ERROR_INVALID_PARAMETER;
                BAIL_ON_SMB_ERROR(dwError);
            }
                        
            dwError = SMBOpenFileLog(
                          pszPath,
                          maxAllowedLogLevel,
                          &hLog);
            BAIL_ON_SMB_ERROR(dwError);
            
            break;
            
        default:
            
            dwError = SMB_ERROR_INVALID_PARAMETER;
            BAIL_ON_SMB_ERROR(dwError);      
    }
    
    gSMBLogTarget = logTarget;
    gSMBMaxLogLevel = maxAllowedLogLevel;
    ghSMBLog = hLog;

 cleanup:
    
    return dwError;

 error:
 
    gSMBLogTarget = SMB_LOG_TARGET_DISABLED;
    ghSMBLog = (HANDLE)NULL;

    goto cleanup;
}

DWORD
SMBLogGetInfo(
    PSMB_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PSMB_LOG_INFO pLogInfo = NULL;
    
    switch(gSMBLogTarget)
    {
        case SMB_LOG_TARGET_DISABLED:
        case SMB_LOG_TARGET_CONSOLE:
        case SMB_LOG_TARGET_SYSLOG:
            
            dwError = SMBAllocateMemory(
                            sizeof(SMB_LOG_INFO),
                            (PVOID*)&pLogInfo);
            BAIL_ON_SMB_ERROR(dwError);
            
            pLogInfo->logTarget = gSMBLogTarget;
            pLogInfo->maxAllowedLogLevel = gSMBMaxLogLevel;
            
            break;
            
        case SMB_LOG_TARGET_FILE:
            
            dwError = SMBGetFileLogInfo(
                            ghSMBLog,
                            &pLogInfo);
            BAIL_ON_SMB_ERROR(dwError);
            
            break;
            
        default:
            dwError = SMB_ERROR_INVALID_PARAMETER;
            BAIL_ON_SMB_ERROR(dwError);
    }
    
    *ppLogInfo = pLogInfo;
    
cleanup:

    return dwError;
    
error:

    *ppLogInfo = NULL;
    
    if (pLogInfo)
    {
        SMBFreeLogInfo(pLogInfo);
    }

    goto cleanup;
}

DWORD
SMBLogSetInfo(
    PSMB_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;
    
    BAIL_ON_INVALID_POINTER(pLogInfo);
    
    // The only information that is allowed
    // to be set after the log is initialized
    // is the log level
    
    gSMBMaxLogLevel = pLogInfo->maxAllowedLogLevel;
    
    switch (gSMBLogTarget)
    {
        case SMB_LOG_TARGET_SYSLOG:
            
            SMBSetSyslogMask(gSMBMaxLogLevel);
            
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
SMBShutdownLogging(
    VOID
    )
{
    DWORD dwError = 0;
    
    if (ghSMBLog != (HANDLE)NULL)
    {
        switch(gSMBLogTarget)
        {
            case SMB_LOG_TARGET_DISABLED:
                break;
                
            case SMB_LOG_TARGET_CONSOLE:
                SMBCloseConsoleLog(ghSMBLog);
                break;
                
            case SMB_LOG_TARGET_FILE:
                SMBCloseFileLog(ghSMBLog);
                break;
                
            case SMB_LOG_TARGET_SYSLOG:
                SMBCloseSyslog(ghSMBLog);
            break;
        }
    }
    
    return dwError;
}

DWORD
SMBSetupLogging(
	HANDLE              hLog,
	SMBLogLevel         maxAllowedLogLevel,
	PFN_SMB_LOG_MESSAGE pfnLogger
	)
{
	DWORD dwError = 0;
	
	if ((hLog == (HANDLE)NULL) ||
		!pfnLogger)
	{
		dwError = SMB_ERROR_INVALID_PARAMETER;
		goto error;
	}
	
	ghSMBLog = hLog;
	gSMBMaxLogLevel = maxAllowedLogLevel;
	gpfnSMBLogger = pfnLogger;
	
error:

	return dwError;
}

VOID
SMBResetLogging(
    VOID
    )
{
	gSMBMaxLogLevel = SMB_LOG_LEVEL_ERROR;
	gpfnSMBLogger = NULL;
	ghSMBLog = (HANDLE)NULL;
}

VOID
SMBLogMessage(
	PFN_SMB_LOG_MESSAGE pfnLogger,
	HANDLE hLog,
	SMBLogLevel logLevel,
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
SMBValidateLogLevel(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    switch(dwLogLevel)
    {
        case SMB_LOG_LEVEL_ALWAYS:
        case SMB_LOG_LEVEL_ERROR:
        case SMB_LOG_LEVEL_WARNING:
        case SMB_LOG_LEVEL_INFO:
        case SMB_LOG_LEVEL_VERBOSE:
        case SMB_LOG_LEVEL_DEBUG:
            dwError = 0;
            break;
        default:
            dwError = SMB_ERROR_INVALID_LOG_LEVEL;
            break;
    }
    
    return dwError;
}
