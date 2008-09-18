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

#ifndef __CT_DAEMON_H__
#define __CT_DAEMON_H__

#include <ctstatus.h>
#include <ctlogger.h>
#include <cttypes.h>

typedef void* (*CT_DAEMON_FUNCTION)(void * Context);

const char*
CtGetProgramName(
    const char* Path
    );

CT_STATUS
CtDaemonStartLogger(
    IN CT_LOG_LEVEL LogLevel,
    IN bool IsDaemon,
    IN const char* SyslogIdentifier,
    IN const char* Path
    );

void
CtDaemonStopLogger(
    );

CT_STATUS
CtDaemonRun(
    IN const char* Program,
    IN const char* PidFile,
    IN bool IsDaemon,
    IN CT_DAEMON_FUNCTION ThreadMain,
    IN CT_DAEMON_FUNCTION ThreadStop,
    IN void* Context,
    OUT int* ExitCode
    );

bool
CtDaemonIsExit(
    );

CT_STATUS
CtDaemonExit(
    IN int ExitCode
    );

#endif /* __CT_DAEMON_H__ */
