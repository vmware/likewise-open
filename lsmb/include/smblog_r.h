/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        smblog_r.h
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 *        Logging API (Thread Safe)
 * 
 * 	  Global variables
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */

#ifndef __SMBLOG_R_H__
#define __SMBLOG_R_H__

DWORD
SMBInitLogging_r(
    PCSTR         pszProgramName,
    SMBLogTarget  logTarget,
    SMBLogLevel   maxAllowedLogLevel,
    PCSTR         pszPath
    );

DWORD
SMBLogGetInfo_r(
    PSMB_LOG_INFO* ppLogInfo
    );

DWORD
SMBLogSetInfo_r(
    PSMB_LOG_INFO pLogInfo
    );

DWORD
SMBShutdownLogging_r(
    VOID
    );

#endif /* __SMBLOG_R_H__ */
