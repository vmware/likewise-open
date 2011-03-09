/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        servermain.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */
#ifndef __SERVERMAIN_H__
#define __SERVERMAIN_H__

int
main(
    int argc,
    char* argv[]
    );

DWORD
RegSrvSetDefaults(
    VOID
    );

DWORD
RegSrvParseArgs(
    int argc,
    PSTR argv[],
    PREGSERVERINFO pRegServerInfo
    );

PSTR
RegGetProgramName(
    PSTR pszFullProgramPath
    );

VOID
ShowUsage(
    PCSTR pszProgramName
    );

VOID
RegSrvExitHandler(
    VOID
    );

DWORD
RegSrvInitialize(
    VOID
    );

DWORD
RegInitCacheFolders(
    VOID
    );

BOOLEAN
RegSrvShouldStartAsDaemon(
    VOID
    );

DWORD
RegSrvStartAsDaemon(
    VOID
    );

DWORD
RegSrvGetProcessExitCode(
    PDWORD pdwExitCode
    );

VOID
RegSrvSetProcessExitCode(
    DWORD dwExitCode
    );

DWORD
RegSrvGetCachePath(
    PSTR* ppszPath
    );

DWORD
RegSrvGetPrefixPath(
    PSTR* ppszPath
    );

DWORD
RegSrvInitLogging(
    PCSTR pszProgramName,
    RegLogTarget* pTarget,
    PHANDLE phLog
    );

DWORD
RegBlockSelectedSignals(
    VOID
    );

BOOLEAN
RegSrvShouldProcessExit(
    VOID
    );

VOID
RegSrvSetProcessToExit(
    BOOLEAN bExit
    );

VOID
RegSrvLogProcessStartedEvent(
    VOID
    );

VOID
RegSrvLogProcessStoppedEvent(
    DWORD dwExitCode
    );

VOID
RegSrvLogProcessFailureEvent(
    DWORD dwErrCode
    );

#endif /* __SERVERMAIN_H__ */

