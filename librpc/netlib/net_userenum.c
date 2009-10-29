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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        net_userenum.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetUserEnum function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
NetUserEnum(
    PCWSTR  pwszHostname,
    DWORD   dwLevel,
    DWORD   dwFilter,
    PVOID  *ppBuffer,
    DWORD   dwMaxLen,
    PDWORD  pdwNumEntries,
    PDWORD  pdwTotalEntries,
    PDWORD  pdwResume
    )
{
    const DWORD dwDomainFlags = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                DOMAIN_ACCESS_OPEN_ACCOUNT;
    const WORD wDomainInfoLevel = 2;
    const WORD wInfoLevel = 21;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    NetConn *pConn = NULL;
    handle_t hSamrBinding = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    DomainInfo *pDomainInfo = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwMaxSize = 0;
    DWORD i = 0;
    PWSTR *ppwszUsernames = NULL;
    PDWORD pdwUserRids = NULL;
    DWORD dwAcctFlags = 0;
    DWORD dwUserFlags = 0;
    PVOID pNetUserInfo = NULL;
    UserInfo21 *pSamrUserInfo21 = NULL;
    UserInfo *pSamrUserInfo = NULL;
    DWORD dwTotal = 0;
    DWORD dwResume = 0;
    PIO_CREDS pCreds = NULL;

    BAIL_ON_INVALID_PTR(ppBuffer);
    BAIL_ON_INVALID_PTR(pdwNumEntries);
    BAIL_ON_INVALID_PTR(pdwTotalEntries);
    BAIL_ON_INVALID_PTR(pdwResume);

    switch (dwFilter)
    {
    case FILTER_NORMAL_ACCOUNT:
        dwAcctFlags = ACB_NORMAL;
        break;

    case FILTER_WORKSTATION_TRUST_ACCOUNT:
        dwAcctFlags = ACB_WSTRUST;
        break;

    case FILTER_SERVER_TRUST_ACCOUNT:
        dwAcctFlags = ACB_SVRTRUST;
        break;

    case FILTER_INTERDOMAIN_TRUST_ACCOUNT:
        dwAcctFlags = ACB_DOMTRUST;
        break;

    default:
        err = NtStatusToWin32Error(STATUS_INVALID_PARAMETER);
        goto error;
    }

    if (!(dwLevel == 0 ||
          dwLevel == 1 ||
          dwLevel == 2 ||
          dwLevel == 20))
    {
        err = ERROR_INVALID_LEVEL;
        goto error;
    }

    status = LwIoGetThreadCreds(&pCreds);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetConnectSamr(&pConn,
                            pwszHostname,
                            dwDomainFlags,
                            0,
                            pCreds);
    BAIL_ON_NTSTATUS_ERROR(status);

    hSamrBinding = pConn->samr.bind;
    hDomain      = pConn->samr.hDomain;

    status = SamrQueryDomainInfo(hSamrBinding,
                                 hDomain,
                                 wDomainInfoLevel,
                                 &pDomainInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    dwTotal    = pDomainInfo->info2.num_users;
    dwResume   = *pdwResume;
    dwMaxSize  = dwMaxLen;

    status = SamrEnumDomainUsers(hSamrBinding,
                                 hDomain,
                                 &dwResume,
                                 dwAcctFlags,
                                 dwMaxSize,
                                 &ppwszUsernames,
                                 &pdwUserRids,
                                 &dwNumEntries);
    if (status != 0 &&
        status != STATUS_MORE_ENTRIES)
    {
        err = NtStatusToWin32Error(status);
        goto error;
    }

    status = NetAllocateMemory((void**)&pSamrUserInfo21,
                               sizeof(UserInfo) * dwNumEntries,
                               NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < dwNumEntries; i++)
    {
        if (dwLevel != 0)
        {
            /*
             * Full query user info of user accounts (one by one)
             * is necessary
             */
            dwUserFlags = USER_ACCESS_GET_NAME_ETC |
                          USER_ACCESS_GET_ATTRIBUTES |
                          USER_ACCESS_GET_LOCALE |
                          USER_ACCESS_GET_LOGONINFO |
                          USER_ACCESS_GET_GROUPS;

            status = SamrOpenUser(hSamrBinding,
                                  hDomain,
                                  dwUserFlags,
                                  pdwUserRids[i],
                                  &hUser);
            BAIL_ON_NTSTATUS_ERROR(status);

            status = SamrQueryUserInfo(hSamrBinding,
                                       hUser,
                                       wInfoLevel,
                                       &pSamrUserInfo);
            BAIL_ON_NTSTATUS_ERROR(status);

            if (pSamrUserInfo)
            {
                memcpy(&(pSamrUserInfo21[i]),
                       &pSamrUserInfo->info21,
                       sizeof(UserInfo21));
                NetFreeMemory((void*)pSamrUserInfo);
            }

            status = SamrClose(hSamrBinding, hUser);
            BAIL_ON_NTSTATUS_ERROR(status);
        }
    }

    if (ppwszUsernames && dwNumEntries)
    {
        switch (dwLevel)
        {
        case 0: status = PullUserInfo0(&pNetUserInfo,
                                       ppwszUsernames,
                                       dwNumEntries);
            break;

        case 1: status = PullUserInfo1(&pNetUserInfo,
                                       pSamrUserInfo21,
                                       dwNumEntries);
            break;

        case 2: status = PullUserInfo2(&pNetUserInfo,
                                       pSamrUserInfo21,
                                       dwNumEntries);
            break;

        case 20: status = PullUserInfo20(&pNetUserInfo,
                                         pSamrUserInfo21,
                                         dwNumEntries);
            break;
        }
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    *ppBuffer        = pNetUserInfo;
    *pdwResume       = dwResume;
    *pdwNumEntries   = dwNumEntries;
    *pdwTotalEntries = dwTotal;

cleanup:
    if (pSamrUserInfo21)
    {
        NetFreeMemory((void*)pSamrUserInfo21);
    }

    if (pDomainInfo)
    {
        SamrFreeMemory((void*)pDomainInfo);
    }

    if (ppwszUsernames)
    {
        SamrFreeMemory((void*)ppwszUsernames);
    }

    if (pdwUserRids)
    {
        SamrFreeMemory((void*)pdwUserRids);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = NtStatusToWin32Error(status);
    }

    return err;

error:
    if (pNetUserInfo)
    {
        NetFreeMemory((void*)pNetUserInfo);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    *ppBuffer        = NULL;
    *pdwResume       = 0;
    *pdwNumEntries   = 0;
    *pdwTotalEntries = 0;

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
