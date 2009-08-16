/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Likewise Server Service
 *
 */
#ifndef __MAIN_P_H__
#define __MAIN_P_H__

/* This structure captures the arguments that must be
 * sent to the Group Policy Service
 */
typedef struct {
    /* MT safety */
    pthread_mutex_t lock;
    /* Should start as daemon */
    DWORD dwStartAsDaemon;
    /* How much logging do you want? */
    DWORD dwLogLevel;
    /* log file path */
    char szLogFilePath[PATH_MAX + 1];
    /* config file path */
    char szConfigFilePath[PATH_MAX + 1];
    /* Cache path */
    char szCachePath[PATH_MAX+1];
    /* Prefix path */
    char szPrefixPath[PATH_MAX+1];
    /* Process termination flag */
    BOOLEAN  bProcessShouldExit;
    /* Process Exit Code */
    DWORD dwExitCode;

} SRVSVCSERVERINFO, *PSRVSVCSERVERINFO;

extern SRVSVCSERVERINFO gServerInfo;


typedef struct {
    /* MT safety */
    pthread_mutex_t pLock;
    /* path to lsarpc server socket for local procedure calls */
    CHAR pszLsaLpcSocketPath[PATH_MAX + 1];

} SRVSVC_CONFIG, *PSRVSVC_CONFIG;

extern SRVSVC_CONFIG gServerConfig;


DWORD
SrvSvcGetConfigPath(
    PSTR* ppszPath
    );

DWORD
SrvSvcGetCachePath(
    PSTR* ppszPath
    );

DWORD
SrvSvcGetPrefixPath(
    PSTR* ppszPath
    );

BOOLEAN
SrvSvcProcessShouldExit(
    VOID
    );

void
SRVSVCSetProcessShouldExit(
	BOOLEAN val
	);

DWORD
SrvSvcServerMain(
    int argc,
    char* argv[]
    );

void
SrvSvcServerExit(
    int retCode
    );

#endif /* __MAIN_P_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
