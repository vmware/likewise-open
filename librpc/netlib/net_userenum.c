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
 *        net_userenum.c
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
    DWORD   dwMaxBufferSize,
    PDWORD  pdwNumEntries,
    PDWORD  pdwTotalEntries,
    PDWORD  pdwResume
    )
{
    const DWORD dwUserAccessFlags = USER_ACCESS_GET_NAME_ETC |
                                    USER_ACCESS_GET_ATTRIBUTES |
                                    USER_ACCESS_GET_LOCALE |
                                    USER_ACCESS_GET_LOGONINFO |
                                    USER_ACCESS_GET_GROUPS;

    const DWORD dwDomainAccessFlags = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                      DOMAIN_ACCESS_OPEN_ACCOUNT;
    const WORD wInfoLevel = 21;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    NetConn *pConn = NULL;
    handle_t hSamrBinding = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwSamrMaxSize = SAMR_MAX_PREFERRED_SIZE;
    DWORD dwSamrResume = 0;
    DWORD i = 0;
    PWSTR *ppwszUsernames = NULL;
    PDWORD pdwUserRids = NULL;
    DWORD dwAcctFlags = 0;
    UserInfo21 **ppSamrUserInfo21 = NULL;
    UserInfo *pSamrUserInfo = NULL;
    PVOID pSourceBuffer = NULL;
    DWORD dwInfoLevelSize = 0;
    DWORD dwTotalNumEntries = 0;
    DWORD dwResume = 0;
    PIO_CREDS pCreds = NULL;
    PVOID pBuffer = NULL;
    PVOID pBufferCursor = NULL;
    DWORD dwTotalSize = 0;
    DWORD dwSize = 0;
    DWORD dwSpaceAvailable = 0;

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
        BAIL_ON_WINERR_ERROR(err);
    }

    switch (dwLevel)
    {
    case 0: dwInfoLevelSize = sizeof(USER_INFO_0);
        break;

    case 1: dwInfoLevelSize = sizeof(USER_INFO_1);
        break;

    case 2: dwInfoLevelSize = sizeof(USER_INFO_2);
        break;

    case 3: dwInfoLevelSize = sizeof(USER_INFO_3);
        break;

    case 4: dwInfoLevelSize = sizeof(USER_INFO_4);
        break;

    case 20: dwInfoLevelSize = sizeof(USER_INFO_20);
        break;

    case 23: dwInfoLevelSize = sizeof(USER_INFO_23);

    default:
        err = ERROR_INVALID_LEVEL;
        BAIL_ON_WINERR_ERROR(err);
    }

    dwResume = *pdwResume;

    status = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetConnectSamr(&pConn,
                            pwszHostname,
                            dwDomainAccessFlags,
                            0,
                            pCreds);
    BAIL_ON_NTSTATUS_ERROR(status);

    hSamrBinding = pConn->samr.bind;
    hDomain      = pConn->samr.hDomain;

    status = SamrEnumDomainUsers(hSamrBinding,
                                 hDomain,
                                 &dwSamrResume,
                                 dwAcctFlags,
                                 dwSamrMaxSize,
                                 &ppwszUsernames,
                                 &pdwUserRids,
                                 &dwTotalNumEntries);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetAllocateMemory((void**)&ppSamrUserInfo21,
                               sizeof(UserInfo*) * dwTotalNumEntries,
                               NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i + dwResume < dwTotalNumEntries; i++)
    {
        if (dwLevel == 0)
        {
            pSourceBuffer = ppwszUsernames[i + dwResume];
        }
        else
        {
            status = SamrOpenUser(hSamrBinding,
                                  hDomain,
                                  dwUserAccessFlags,
                                  pdwUserRids[i + dwResume],
                                  &hUser);
            BAIL_ON_NTSTATUS_ERROR(status);

            status = SamrQueryUserInfo(hSamrBinding,
                                       hUser,
                                       wInfoLevel,
                                       &pSamrUserInfo);
            BAIL_ON_NTSTATUS_ERROR(status);

            ppSamrUserInfo21[i] = &pSamrUserInfo->info21;
            pSourceBuffer       = &pSamrUserInfo->info21;

            status = SamrClose(hSamrBinding, hUser);
            BAIL_ON_NTSTATUS_ERROR(status);
        }

        dwSize = 0;
        err = NetAllocateUserInfo(NULL,
                                  NULL,
                                  dwLevel,
                                  pSourceBuffer,
                                  &dwSize);
        BAIL_ON_WINERR_ERROR(err);

        dwTotalSize += dwSize;
        dwNumEntries++;

        if (dwTotalSize > dwMaxBufferSize)
        {
            dwTotalSize -= dwSize;
            dwNumEntries--;
            break;
        }
    }

    if (dwTotalNumEntries > 0 && dwNumEntries == 0)
    {
        err = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_WINERR_ERROR(err);
    }

    if (dwTotalSize)
    {
        status = NetAllocateMemory((void**)&pBuffer,
                                   dwTotalSize,
                                   NULL);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    dwSize           = 0;
    pBufferCursor    = pBuffer;
    dwSpaceAvailable = dwTotalSize;

    for (i = 0; i < dwNumEntries; i++)
    {
        if (dwLevel == 0)
        {
            pSourceBuffer = ppwszUsernames[i + dwResume];
        }
        else
        {
            pSourceBuffer = ppSamrUserInfo21[i];
        }

        pBufferCursor = pBuffer + (i * dwInfoLevelSize);

        err = NetAllocateUserInfo(pBufferCursor,
                                  &dwSpaceAvailable,
                                  dwLevel,
                                  pSourceBuffer,
                                  &dwSize);
        BAIL_ON_WINERR_ERROR(err);

        /*
         * Special case - level 4 and 23 include a user SID which can't
         * be copied from samr user info level only. A domain SID from
         * samr connection is required too.
         */
        if (dwLevel == 4 ||
            dwLevel == 23)
        {
            PUSER_INFO_4 pBufferInfo4 = NULL;
            PUSER_INFO_23 pBufferInfo23 = NULL;
            UserInfo21 *pSamrUserInfo21 = ppSamrUserInfo21[i];
            DWORD dwUserSidLength = 0;
            PSID pUserSid = NULL;

            switch (dwLevel)
            {
            case 4:
                pBufferInfo4 = pBufferCursor;
                pUserSid     = pBufferInfo4->usri4_user_sid;
                break;

            case 23:
                pBufferInfo23 = pBufferCursor;
                pUserSid      = pBufferInfo23->usri23_user_sid;
                break;
            }

            dwUserSidLength = RtlLengthRequiredSid(
                                   pConn->samr.dom_sid->SubAuthorityCount + 1);

            status = RtlCopySid(dwUserSidLength,
                                pUserSid,
                                pConn->samr.dom_sid);
            BAIL_ON_NTSTATUS_ERROR(status);

            status = RtlAppendRidSid(dwUserSidLength,
                                     pUserSid,
                                     pSamrUserInfo21->rid);
            BAIL_ON_NTSTATUS_ERROR(status);
        }
    }

    if (dwResume + dwNumEntries < dwTotalNumEntries)
    {
        err = ERROR_MORE_DATA;
    }

    *ppBuffer        = pBuffer;
    *pdwResume       = dwResume + dwNumEntries;
    *pdwNumEntries   = dwNumEntries;
    *pdwTotalEntries = dwTotalNumEntries;

cleanup:
    for (i = 0; i < dwNumEntries; i++)
    {
        if (ppSamrUserInfo21[i])
        {
            SamrFreeMemory((void*)ppSamrUserInfo21[i]);
        }
    }

    if (ppSamrUserInfo21)
    {
        NetFreeMemory((void*)ppSamrUserInfo21);
    }

    if (ppwszUsernames)
    {
        SamrFreeMemory((void*)ppwszUsernames);
    }

    if (pdwUserRids)
    {
        SamrFreeMemory((void*)pdwUserRids);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = NtStatusToWin32Error(status);
    }

    return err;

error:
    if (pBuffer)
    {
        NetFreeMemory((void*)pBuffer);
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
