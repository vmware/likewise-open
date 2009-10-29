/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

#include "includes.h"


NET_API_STATUS
NetUserSetInfo(
    PCWSTR  pwszHostname,
    PCWSTR  pwszUsername,
    DWORD   dwLevel,
    PVOID   pBuffer,
    PDWORD  pdwParmErr
    )
{
    /* This is necessary to be able to set account password.
       Otherwise we get access denied. Don't ask... */
    const DWORD dwDomainAccess = DOMAIN_ACCESS_LOOKUP_INFO_1;

    const uint32 dwUserAccess = USER_ACCESS_GET_NAME_ETC |
                                USER_ACCESS_GET_LOCALE |
                                USER_ACCESS_GET_LOGONINFO |
                                USER_ACCESS_GET_ATTRIBUTES |
                                USER_ACCESS_GET_GROUPS |
                                USER_ACCESS_GET_GROUP_MEMBERSHIP |
                                USER_ACCESS_SET_LOC_COM |
                                USER_ACCESS_SET_ATTRIBUTES |
                                USER_ACCESS_CHANGE_PASSWORD |
                                USER_ACCESS_SET_PASSWORD;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    NetConn *pConn = NULL;
    handle_t hSamrBinding = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    DWORD dwUserRid = 0;
    UserInfo *pSamrUserInfo = NULL;
    USER_INFO_0 *pNetUserInfo0 = NULL;
    USER_INFO_1003 *pNetUserInfo1003 = NULL;
    USER_INFO_1007 *pNetUserInfo1007 = NULL;
    USER_INFO_1008 *pNetUserInfo1008 = NULL;
    USER_INFO_1011 *pNetUserInfo1011 = NULL;
    DWORD dwSamrInfoLevel = 0;
    PIO_CREDS pCreds = NULL;

    BAIL_ON_INVALID_PTR(pwszUsername);
    BAIL_ON_INVALID_PTR(pBuffer);

    status = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetConnectSamr(&pConn,
                            pwszHostname,
                            dwDomainAccess,
                            0,
                            pCreds);
    BAIL_ON_NTSTATUS_ERROR(status);

    hSamrBinding = pConn->samr.bind;

    status = NetOpenUser(pConn,
                         pwszUsername,
                         dwUserAccess,
                         &hUser,
                         &dwUserRid);
    BAIL_ON_NTSTATUS_ERROR(status);

    switch (dwLevel)
    {
    case 0:
        pNetUserInfo0 = (USER_INFO_0*)pBuffer;
        status = PushUserInfo0(&pSamrUserInfo,
                               &dwSamrInfoLevel,
                               pNetUserInfo0);
        break;

    case 1003:
        pNetUserInfo1003 = (USER_INFO_1003*) pBuffer;
        status = PushUserInfo1003(&pSamrUserInfo,
                                  &dwSamrInfoLevel,
                                  pNetUserInfo1003,
                                  pConn);
        break;

    case 1007:
        pNetUserInfo1007 = (USER_INFO_1007*) pBuffer;
        status = PushUserInfo1007(&pSamrUserInfo,
                                  &dwSamrInfoLevel,
                                  pNetUserInfo1007);
        break;

    case 1008:
        pNetUserInfo1008 = (USER_INFO_1008*) pBuffer;
        status = PushUserInfo1008(&pSamrUserInfo,
                                  &dwSamrInfoLevel,
                                  pNetUserInfo1008);
        break;

    case 1011:
        pNetUserInfo1011 = (USER_INFO_1011*) pBuffer;
        status = PushUserInfo1011(&pSamrUserInfo,
                                  &dwSamrInfoLevel,
                                  pNetUserInfo1011);
        break;

    case 1:
    case 2:
    case 3:
    case 4:
    case 21:
    case 22:
    case 1005:
    case 1006:
    case 1009:
    case 1010:
    case 1012:
    case 1014:
    case 1017:
    case 1020:
    case 1024:
    case 1051:
    case 1052:
    case 1053:
        status = STATUS_NOT_IMPLEMENTED;
        break;

    default:
        status = STATUS_INVALID_LEVEL;
        break;
    }
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSetUserInfo(hSamrBinding,
                             hUser,
                             dwSamrInfoLevel,
                             pSamrUserInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrClose(hSamrBinding, hUser);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
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
