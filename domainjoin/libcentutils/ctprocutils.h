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

#ifndef __CTPROCUTILS_H__
#define __CTPROCUTILS_H__

#define PID_DIR "/var/run"

#define GPAGENT_DAEMON_NAME "centeris-gpagentd"
#define GPAGENT_PID_FILE PID_DIR "/" GPAGENT_DAEMON_NAME ".pid"

#define LWIAUTH_DAEMON_NAME "lwiauthd"
#define LWIAUTH_PID_FILE PID_DIR "/" LWIAUTH_DAEMON_NAME ".pid"

CENTERROR CTMatchProgramToPID(PCSTR pszProgramName, pid_t pid);

CENTERROR
CTIsProgramRunning(PCSTR pszPidFile,
		   PCSTR pszProgramName, pid_t * pPid, PBOOLEAN pbRunning);

CENTERROR CTSendSignal(pid_t pid, int sig);

#endif				/* __CTPROCUTILS_H__ */
