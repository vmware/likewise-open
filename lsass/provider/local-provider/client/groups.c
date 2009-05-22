/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        groups.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Group Lookup and Management API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */
#include "localclient.h"


LSASS_API
DWORD
LsaLocalGetGroupMembership(
    HANDLE hLsaConnection,
    PSTR   pszSID,
    PSTR   pszDN,
    DWORD  dwGroupInfoLevel,
    PDWORD pdwGroupsCount,
    PVOID  *ppGroupInfo
    )
{
    DWORD dwError = 0;
    LWMsgContext *context = NULL;
    LSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REQ Request;
    PLSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REP pReply = NULL;
    size_t reqBufferSize = 0;
    PVOID pReqBuffer = NULL;
    DWORD dwRepBufferSize = 0;
    PVOID pRepBuffer = NULL;
    PLSA_GROUP_INFO_1 *pGroupInfo = NULL;
    DWORD dwNumGroups = 0;
    DWORD i = 0;

    Request.pszSID           = pszSID;
    Request.pszDN            = pszDN;
    Request.dwGroupInfoLevel = dwGroupInfoLevel;

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(&context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_marshal_alloc(
                              context,
                              LsaLocalIPCGetGroupMembershipReqSpec(),
                              &Request,
                              &pReqBuffer,
                              &reqBufferSize));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderIoControl(
                  hLsaConnection,
                  LSA_LOCAL_TAG_PROVIDER,
                  LSA_LOCAL_IO_GETGROUPMEMBERSHIP,
                  reqBufferSize,
                  pReqBuffer,
                  &dwRepBufferSize,
                  &pRepBuffer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_unmarshal_simple(
                              context,
                              LsaLocalIPCGetGroupMembershipRepSpec(),
                              pRepBuffer,
                              dwRepBufferSize,
                              (PVOID*)&pReply));
    BAIL_ON_LSA_ERROR(dwError);

    dwNumGroups = pReply->dwNumGroups;

    if (dwNumGroups) {
        dwError = LsaAllocateMemory(sizeof(pGroupInfo[0]) * dwNumGroups,
                                    (PVOID*)&pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);

        for (i = 0; i < dwNumGroups; i++) {
            PLSA_GROUP_INFO_1 pInfo = NULL;
            PLSA_GROUP_INFO_1 pSrcInfo = pReply->pGroups[i];

            dwError = LsaAllocateMemory(sizeof(*pInfo),
                                        (PVOID*)(&pInfo));
            BAIL_ON_LSA_ERROR(dwError);

            pInfo->gid = pSrcInfo->gid;

            dwError = LsaAllocateString(pSrcInfo->pszName,
                                        &pInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAllocateString(pSrcInfo->pszSid,
                                        &pInfo->pszSid);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAllocateString(pSrcInfo->pszDN,
                                        &pInfo->pszDN);
            BAIL_ON_LSA_ERROR(dwError);

            pGroupInfo[i] = pInfo;
        }
    }

    *pdwGroupsCount = dwNumGroups;
    *ppGroupInfo    = pGroupInfo;

cleanup:
    if (pReply) {
        lwmsg_context_free_graph(
            context,
            LsaLocalIPCGetGroupMembershipRepSpec(),
            pReply);
    }

    if (context) {
        lwmsg_context_delete(context);
    }

    if (pReqBuffer) {
        LsaFreeMemory(pReqBuffer);
    }

    if (pRepBuffer) {
        LsaFreeMemory(pRepBuffer);
    }

    return dwError;

error:
    if (pGroupInfo) {
        for (i = 0; i < dwNumGroups; i++) {
            LsaFreeGroupInfo(1, pGroupInfo[i]);
        }

        LsaFreeMemory(pGroupInfo);
    }

    *pdwGroupsCount = 0;
    *ppGroupInfo    = NULL;

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
