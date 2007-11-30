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

#ifndef __CT_EXEC_H__
#define __CT_EXEC_H__

#include "ctbase.h"

/**
 * @defgroup CTExec Unix program interaction
 */
/*@{*/

typedef struct __PROCINFO {
	pid_t pid;
	int fdin;
	int fdout;
	int fderr;
} PROCINFO, *PPROCINFO;

/**
 * @brief Capture output of a Unix command
 *
 * Runs the specified Unix command and captures output
 * to a buffer.  The buffer is dynamically allocated and
 * freeing it becomes the responsibility of the caller.
 * The command is passed to the standard Unix shell
 * (/bin/sh), which is reponsible for parsing and executing
 * it; shell features such as pipelines may be used, but
 * care must be taken to properly escape commands.
 * @see CTEscapeString
 * @param command @in the Unix command to execute
 * @param output @out the dynamically-allocated buffer containing
 * the output of the command
 * @errcode
 * @canfail
 */
CENTERROR CTCaptureOutput(PCSTR command, PSTR * output);

/**
 * @brief Run a command
 *
 * Runs the specified Unix command and waits for it to
 * complete.  The command is passed to the standard Unix shell
 * (/bin/sh), which is responsible for parsing and executing
 * it; shell features such as pipelines may be used, but
 * care must be taken to properly escape commands.
 * @see CTEscapeString
 * @param command @in the Unix command to execute
 * @errcode
 * @canfail
 */
CENTERROR CTRunCommand(PCSTR command);

CENTERROR
CTSpawnProcessWithFds(PCSTR pszCommand,
		      PCSTR * ppszArgs,
		      int dwFdIn,
		      int dwFdOut, int dwFdErr, PPROCINFO * ppProcInfo);

CENTERROR
CTSpawnProcessWithEnvironment(PCSTR pszCommand,
			      PCSTR * ppszArgs,
			      PCSTR * ppszEnv,
			      int dwFdIn,
			      int dwFdOut, int dwFdErr, PPROCINFO * ppProcInfo);

void CTFreeProcInfo(PPROCINFO pProcInfo);

CENTERROR CTGetExitStatus(PPROCINFO pProcInfo, PLONG plstatus);

/*@}*/

#endif
