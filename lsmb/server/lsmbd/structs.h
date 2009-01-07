/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsassd.h
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem
 *
 *        Service Structures
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct {
    /* MT safety */
    pthread_mutex_t lock;
    /* Should start as daemon */
    DWORD dwStartAsDaemon;
    /* where are we logging */
    SMBLogTarget logTarget;
    /* How much logging do you want? */
    SMBLogLevel maxAllowedLogLevel;
    /* Enable debug logs */
    BOOLEAN bEnableDebugLogs;
    /* log file path */
    char szLogFilePath[PATH_MAX + 1];
    /* Cache path */
    char szCachePath[PATH_MAX+1];
    /* Prefix path */
    char szPrefixPath[PATH_MAX+1];
    /* Process termination flag */
    BOOLEAN  bProcessShouldExit;
    /* Process Exit Code */
    DWORD dwExitCode;
} SMBSERVERINFO, *PSMBSERVERINFO;

typedef struct __SMB_CONFIG
{

    DWORD dwPlaceHolder;

} SMB_CONFIG, *PSMB_CONFIG;

#endif /* __STRUCTS_H__ */

