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
 *        pam-logging.h
 *
 * Abstract:
 * 
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Pluggable Authentication Module
 * 
 *        Logging API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __PAM_LOGGING_H__
#define __PAM_LOGGING_H__

extern DWORD gdwLogLevel;

/*
 * Log levels
 */
#define PAM_LOG_LEVEL_ALWAYS  0
#define PAM_LOG_LEVEL_ERROR   1
#define PAM_LOG_LEVEL_WARNING 2
#define PAM_LOG_LEVEL_INFO    3
#define PAM_LOG_LEVEL_VERBOSE 4

#define LSA_LOG_PAM_ALWAYS(szFmt...)                     \
    LsaPamLogMessage(PAM_LOG_LEVEL_ALWAYS, ## szFmt);

#define LSA_LOG_PAM_ERROR(szFmt...)                       \
    if (gdwLogLevel >= PAM_LOG_LEVEL_ERROR) {             \
        LsaPamLogMessage(PAM_LOG_LEVEL_ERROR, ## szFmt);  \
    }

#define LSA_LOG_PAM_WARNING(szFmt...)                     \
    if (gdwLogLevel >= PAM_LOG_LEVEL_WARNING) {           \
        LsaPamLogMessage(PAM_LOG_LEVEL_WARNING, ## szFmt);\
    }

#define LSA_LOG_PAM_INFO(szFmt...)                        \
    if (gdwLogLevel >= PAM_LOG_LEVEL_INFO)    {           \
        LsaPamLogMessage(PAM_LOG_LEVEL_INFO, ## szFmt);   \
    }

#define LSA_LOG_PAM_VERBOSE(szFmt...)                     \
    if (gdwLogLevel >= PAM_LOG_LEVEL_VERBOSE) {           \
        LsaPamLogMessage(PAM_LOG_LEVEL_VERBOSE, ## szFmt);\
    }

#define LSA_LOG_PAM_DEBUG(szFmt...)                       \
    if (gdwLogDebug) {                                    \
        LsaPamLogMessage(PAM_LOG_LEVEL_ALWAYS, ## szFmt); \
    }

void
LsaPamInitLog(
    void
    );

void
LsaPamLogMessage(
    DWORD dwLogLevel,
    PSTR pszFormat, ...
    );

#endif /* __PAM_LOGGING_H__ */
