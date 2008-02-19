/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

typedef enum
{
    TASK_TYPE_USAGE = 0,
    TASK_TYPE_USAGE_INTERNAL,
    TASK_TYPE_QUERY,
    TASK_TYPE_FIXFQDN,
    TASK_TYPE_SETNAME,
    TASK_TYPE_JOIN,
    TASK_TYPE_JOIN_NEW,
    TASK_TYPE_JOIN_LIST_MODULES,
    TASK_TYPE_JOIN_DETAILS,
    TASK_TYPE_SYNC_TIME,
    TASK_TYPE_LEAVE,
    TASK_TYPE_CONFIGURE_PAM,
    TASK_TYPE_CONFIGURE_NSSWITCH,
    TASK_TYPE_CONFIGURE_SSH,
    TASK_TYPE_CONFIGURE_KRB5,
    TASK_TYPE_CONFIGURE_FIREWALL,
    TASK_TYPE_GETOSTYPE,
    TASK_TYPE_GETARCH,
    TASK_TYPE_GETDISTRO,
    TASK_TYPE_GETDISTROVERSION,
    TASK_TYPE_RAISEERROR,
    TASK_TYPE_PS,
} DomainJoinTaskType;

typedef enum
{
    ENABLE_TYPE_ENABLE = 0,
    ENABLE_TYPE_DISABLE,
    /* Enable if joined to the domain, otherwise disable */
    ENABLE_TYPE_AUTO,
} EnableType;

typedef struct __TASKINFO
{
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
    BOOLEAN bAdvancedMode;
    /* This is used by "ps --program" */
    PSTR pszProgramName;
    /* This is used by "ps --cmd" */
    PSTR pszCmdName;
    /* This is used by "ps --owner" */
    uid_t owner;
    /* This is used by "configure pam"
     */
    EnableType dwEnable;
    PSTR pszTimeServer;
    int allowedDrift;
    PSTR pszShortName;
    DWORD dwLogLevel;
    /* This is used by "raise_error" */
    CENTERROR ceError;

    // An array of the short names of the modules the user has explicitly
    // enabled on the command line
    DynamicArray enableModules;
    // An array of the short names of the modules the user has explicitly
    // disabled on the command line
    DynamicArray disableModules;

    PCSTR detailsModule;
} TASKINFO, *PTASKINFO;

CENTERROR
GetTaskInfo(
    int argc,
    char* argv[],
    PTASKINFO* ppTaskInfo
    );

void
FreeTaskInfo(
    PTASKINFO pTaskInfo
    );

CENTERROR
GetPassword(
    PSTR* ppszPassword
    );

CENTERROR
DoFixFqdn(
    );

CENTERROR
DoQuery(
    );

CENTERROR
DoSetName(
    PTASKINFO pTaskInfo
    );

CENTERROR
DoLeave(
    PTASKINFO pTaskInfo
    );

#endif /* __DJCLI_H__ */
