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
 *        lwps-logger.h
 *
 * Abstract:
 *
 *        Likewise Password Storage (LWPS)
 *
 *        Log API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LWPS_LOGGER_H_
#define __LWPS_LOGGER_H_

#include "lwps-utils.h"

/*
 * Log levels
 */
#define LOG_LEVEL_ALWAYS  0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_VERBOSE 4

/*
 * Logging targets
 */
#define LOG_TO_SYSLOG 0
#define LOG_TO_FILE   1
#define LOG_DISABLED  3

typedef struct _LOGFILEINFO {
    CHAR szLogPath[PATH_MAX+1];
    FILE* logHandle;
} LOGFILEINFO, *PLOGFILEINFO;

typedef struct _SYSLOGINFO {
    CHAR szIdentifier[PATH_MAX+1];
    DWORD dwOption;
    DWORD dwFacility;
} SYSLOGINFO, *PSYSLOGINFO;

typedef struct _LOGINFO {
    pthread_mutex_t lock;
    DWORD dwLogLevel;
    DWORD logTarget;
    LOGFILEINFO logfile;
    SYSLOGINFO syslog;
    BOOLEAN  bDebug;
    BOOLEAN  bLogToConsole;
    BOOLEAN  bLoggingInitiated;
} LOGINFO, *PLOGINFO;

DWORD
lwps_init_logging_to_syslog(
    DWORD   dwLogLevel,
    BOOLEAN bEnableDebug,
    PCSTR   pszIdentifier,
    DWORD   dwOption,
    DWORD   dwFacility
    );

DWORD
lwps_set_log_level(
	DWORD dwLogLevel
	);

DWORD
lwps_init_logging_to_file(
    DWORD   dwLogLevel,
    BOOLEAN bEnableDebug,
    PSTR    pszLogFilePath
    );

void
lwps_log_message(
    DWORD dwLogLevel,
    PSTR pszFormat, ...
    );

void
lwps_close_log();

extern LOGINFO gLwpsLogInfo;

#define LWPS_LOG_ALWAYS(szFmt...)                     \
    lwps_log_message(LOG_LEVEL_ALWAYS, ## szFmt);

#define LWPS_LOG_ERROR(szFmt...)                      \
    if (gLwpsLogInfo.dwLogLevel >= LOG_LEVEL_ERROR) {     \
        lwps_log_message(LOG_LEVEL_ERROR, ## szFmt);  \
    }

#define LWPS_LOG_WARNING(szFmt...)                    \
    if (gLwpsLogInfo.dwLogLevel >= LOG_LEVEL_WARNING) {   \
        lwps_log_message(LOG_LEVEL_WARNING, ## szFmt);\
    }

#define LWPS_LOG_INFO(szFmt...)                       \
    if (gLwpsLogInfo.dwLogLevel >= LOG_LEVEL_INFO)    {   \
        lwps_log_message(LOG_LEVEL_INFO, ## szFmt);   \
    }

#define LWPS_LOG_VERBOSE(szFmt...)                    \
    if (gLwpsLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {   \
        lwps_log_message(LOG_LEVEL_VERBOSE, ## szFmt);\
    }

#define LWPS_LOG_DEBUG(szFmt...)                      \
    if (gLwpsLogInfo.bDebug) {                            \
        lwps_log_message(LOG_LEVEL_ALWAYS, ## szFmt); \
    }

#endif /*__LWPS_LOGGER_H__*/
