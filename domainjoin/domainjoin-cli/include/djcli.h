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

#ifndef __DJCLI_H__
#define __DJCLI_H__

typedef enum {
	TASK_TYPE_USAGE = 0,
	TASK_TYPE_QUERY,
	TASK_TYPE_FIXFQDN,
	TASK_TYPE_SETNAME,
	TASK_TYPE_JOIN,
	TASK_TYPE_LEAVE,
	TASK_TYPE_CONFIGURE_PAM,
	TASK_TYPE_CONFIGURE_NSSWITCH,
	TASK_TYPE_CONFIGURE_SSH,
} DomainJoinTaskType;

typedef enum {
	ENABLE_TYPE_ENABLE = 0,
	ENABLE_TYPE_DISABLE,
	/* Enable if joined to the domain, otherwise disable */
	ENABLE_TYPE_AUTO,
} EnableType;

typedef struct __TASKINFO {
	DomainJoinTaskType dwTaskType;
	PSTR pszComputerName;
	PSTR pszUserName;
	PSTR pszPassword;
	PSTR pszDomainName;
	PSTR pszLogFilePath;
	PSTR pszOU;
	/* This is used by the configure task to do a trial run in
	 * another directory */
	PSTR pszTestPrefix;
	BOOLEAN bDoNotChangeHosts;
	BOOLEAN bNoLog;
	/* This is used by "configure pam"
	 */
	EnableType dwEnable;
	DWORD dwLogLevel;
} TASKINFO, *PTASKINFO;

CENTERROR GetTaskInfo(int argc, char *argv[], PTASKINFO * ppTaskInfo);

void FreeTaskInfo(PTASKINFO pTaskInfo);

CENTERROR GetPassword(PSTR * ppszPassword);

CENTERROR DoFixFqdn(PTASKINFO pTaskInfo);

CENTERROR DoQuery(PTASKINFO pTaskInfo);

CENTERROR DoSetName(PTASKINFO pTaskInfo);

CENTERROR DoJoin(PTASKINFO pTaskInfo);

CENTERROR DoLeave(PTASKINFO pTaskInfo);

#endif				/* __DJCLI_H__ */
