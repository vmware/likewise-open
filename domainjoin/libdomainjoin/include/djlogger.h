/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __DJLOGGER_H_
#define __DJLOGGER_H_

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
#define LOG_TO_FILE   1
#define LOG_DISABLED  3

typedef struct _LOGFILEINFO {
	CHAR szLogPath[PATH_MAX + 1];
	FILE *logHandle;
} LOGFILEINFO, *PLOGFILEINFO;

typedef struct _LOGINFO {
	DWORD dwLogLevel;
	DWORD dwLogTarget;
	LOGFILEINFO logfile;
} LOGINFO, *PLOGINFO;

CENTERROR dj_set_log_level(DWORD dwLogLevel);

CENTERROR dj_init_logging_to_file(DWORD dwLogLevel, PSTR pszLogFilePath);

CENTERROR dj_init_logging_to_file_handle(DWORD dwLogLevel, FILE* handle);

CENTERROR dj_init_logging_to_console(DWORD dwLogLevel);

CENTERROR dj_disable_logging();

void dj_log_message(DWORD dwLogLevel, PSTR pszFormat, ...
    );

void dj_close_log();

extern LOGINFO gdjLogInfo;

#define DJ_LOG_ALWAYS(szFmt...)                 \
    dj_log_message(LOG_LEVEL_ALWAYS, ## szFmt);

#define DJ_LOG_ERROR(szFmt...)                          \
    if (gdjLogInfo.dwLogLevel >= LOG_LEVEL_ERROR) {     \
        dj_log_message(LOG_LEVEL_ERROR, ## szFmt);      \
    }

#define DJ_LOG_WARNING(szFmt...)                        \
    if (gdjLogInfo.dwLogLevel >= LOG_LEVEL_WARNING) {   \
        dj_log_message(LOG_LEVEL_WARNING, ## szFmt);    \
    }

#define DJ_LOG_INFO(szFmt...)                           \
    if (gdjLogInfo.dwLogLevel >= LOG_LEVEL_INFO)    {   \
        dj_log_message(LOG_LEVEL_INFO, ## szFmt);       \
    }

#define DJ_LOG_VERBOSE(szFmt...)                        \
    if (gdjLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {   \
        dj_log_message(LOG_LEVEL_VERBOSE, ## szFmt);    \
    }

#endif /*__DJLOGGER_H__*/
