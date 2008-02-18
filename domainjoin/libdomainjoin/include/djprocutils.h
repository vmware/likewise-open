/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __DJPROCUTILS_H__
#define __DJPROCUTILS_H__

CENTERROR
DJKillProcess(
    PPROCINFO pProcInfo
    );

void
FreeProcInfo(
    PPROCINFO pProcInfo
    );

CENTERROR
DJSpawnProcess(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    PPROCINFO* ppProcInfo
    );

CENTERROR
DJSpawnProcessWithFds(
    PCSTR pszCommand,
    PSTR* ppszArgs,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PPROCINFO* ppProcInfo
    );

CENTERROR
DJSpawnProcessSilent(
    PCSTR pszCommand,
    PSTR* ppArgs,
    PPROCINFO* ppProcInfo
    );

CENTERROR
DJSpawnProcessOutputToFile(
    PCSTR pszCommand,
    PSTR* ppArgs,
    PCSTR file,
    PPROCINFO* ppProcInfo
    );

CENTERROR
DJSpawnProcessWithEnvironment(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    const PSTR* ppszEnv,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PPROCINFO* ppProcInfo
    );

CENTERROR
DJReadData(
    PPROCINFO pProcInfo,
    PPROCBUFFER pProcBuffer
    );

CENTERROR
DJTimedReadData(
    PPROCINFO pProcInfo,
    PPROCBUFFER pProcBuffer,
    DWORD dwTimeoutSecs,
    PBOOLEAN pbTimedout
    );

CENTERROR
DJWriteData(
    DWORD dwFd,
    PSTR pszBuf,
    DWORD dwLen
    );

CENTERROR
DJGetProcessStatus(
    PPROCINFO pProcInfo,
    PLONG plstatus
    );

CENTERROR
DJCheckProcessRunning(
    PCSTR pszCmdName,
    PBOOLEAN pbRunning
    );

#endif /* __DJPROCUTILS_H__ */
