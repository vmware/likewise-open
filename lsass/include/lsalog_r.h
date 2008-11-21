/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsalog.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Logging API
 *
 * 		  Global variables
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __LSALOG_R_H__
#define __LSALOG_R_H__

DWORD
LsaInitLogging_r(
    PCSTR         pszProgramName,
    LsaLogTarget  logTarget,
    LsaLogLevel   maxAllowedLogLevel,
    PCSTR         pszPath
    );

DWORD
LsaLogGetInfo_r(
    PLSA_LOG_INFO* ppLogInfo
    );

DWORD
LsaLogSetInfo_r(
    PLSA_LOG_INFO pLogInfo
    );

DWORD
LsaShutdownLogging_r(
    VOID
    );

DWORD
LsaInitTracing_r(
    VOID
    );

DWORD
LsaTraceSetFlag_r(
    DWORD   dwTraceFlag,
    BOOLEAN bStatus
    );

DWORD
LsaTraceGetInfo_r(
    DWORD    dwTraceFlag,
    PBOOLEAN pbStatus
    );

VOID
LsaShutdownTracing_r(
    VOID
    );

#endif /* __LSALOG_R_H__ */
