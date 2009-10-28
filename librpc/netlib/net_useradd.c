/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
*/

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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        net_useradd.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetUserAdd function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


NET_API_STATUS
NetUserAdd(
    PCWSTR  pwszHostname,
    DWORD   dwLevel,
    PVOID   pBuffer,
    PDWORD  pdwParmErr
    )
{
    const DWORD dwUserAccess = USER_ACCESS_GET_NAME_ETC |
                               USER_ACCESS_SET_LOC_COM |
                               USER_ACCESS_GET_LOCALE |
                               USER_ACCESS_GET_LOGONINFO |
                               USER_ACCESS_GET_ATTRIBUTES |
                               USER_ACCESS_GET_GROUPS |
                               USER_ACCESS_GET_GROUP_MEMBERSHIP |
                               USER_ACCESS_CHANGE_GROUP_MEMBERSHIP |
                               USER_ACCESS_SET_ATTRIBUTES |
                               USER_ACCESS_SET_PASSWORD;

    const DWORD dwDomainAccess = DOMAIN_ACCESS_CREATE_USER |
                                 DOMAIN_ACCESS_LOOKUP_INFO_1;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    NetConn *pConn = NULL;
    handle_t hSamrBinding = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    PWSTR pwszUsername = NULL;
    DWORD dwRid = 0;
    DWORD dwSamrInfoLevel = 0;
    DWORD dwParmErr = 0;
    PUSER_INFO_X pNetUserInfo = NULL;
    UserInfo *pSamrUserInfo = NULL;
    PIO_CREDS pCreds = NULL;
    UserInfo PwInfo;
    UserInfo26 *pUserInfo26 = NULL;

    memset((void*)&PwInfo, 0, sizeof(PwInfo));

    BAIL_ON_INVALID_PTR(pBuffer);

    status = PushUserInfoAdd(&pSamrUserInfo,
                             &dwSamrInfoLevel,
                             pBuffer,
                             dwLevel,
                             &dwParmErr);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = LwIoGetThreadCreds(&pCreds);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetConnectSamr(&pConn,
                            pwszHostname,
                            dwDomainAccess,
                            0,
                            pCreds);
    BAIL_ON_NTSTATUS_ERROR(status);

    hSamrBinding  = pConn->samr.bind;
    hDomain       = pConn->samr.hDomain;

    pNetUserInfo  = (USER_INFO_X*)pBuffer;
    pwszUsername  = pNetUserInfo->name;

    status = SamrCreateUser(hSamrBinding,
                            hDomain,
                            pwszUsername,
                            dwUserAccess,
                            &hUser,
                            &dwRid);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* If there was password specified do an extra samr call to set it */
    if (pNetUserInfo->password)
    {
        memset((void*)&PwInfo, 0, sizeof(PwInfo));
        pUserInfo26 = &PwInfo.info26;

        pUserInfo26->password_len = wc16slen(pNetUserInfo->password);

        status = EncPasswordEx(pUserInfo26->password.data,
                               pNetUserInfo->password,
                               pUserInfo26->password_len,
                               pConn);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = SamrSetUserInfo(hSamrBinding,
                                 hUser,
                                 26,
                                 &PwInfo);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    status = SamrSetUserInfo(hSamrBinding,
                             hUser,
                             dwSamrInfoLevel,
                             pSamrUserInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrClose(hSamrBinding, hUser);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    if (pdwParmErr)
    {
        *pdwParmErr = dwParmErr;
    }

    if (pSamrUserInfo)
    {
        NetFreeMemory((void*)pSamrUserInfo);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = NtStatusToWin32Error(status);
    }

    return err;

error:
    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
