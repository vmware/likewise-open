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
    PVOID  **pppGroupInfoList
    )
{
    DWORD dwError = 0;
    LWMsgContext *context = NULL;
    LWMsgDataHandle* pDataHandle = NULL;
    LSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REQ Request;
    PLSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REP pReply = NULL;
    size_t reqBufferSize = 0;
    PVOID pReqBuffer = NULL;
    DWORD dwRepBufferSize = 0;
    PVOID pRepBuffer = NULL;
    PVOID *ppGroupInfo = NULL;
    DWORD dwNumGroups = 0;
    DWORD i = 0;

    Request.pszSID           = pszSID;
    Request.pszDN            = pszDN;
    Request.dwGroupInfoLevel = dwGroupInfoLevel;

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_handle_new(context, &pDataHandle));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataHandle,
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

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                              pDataHandle,
                              LsaLocalIPCGetGroupMembershipRepSpec(),
                              pRepBuffer,
                              dwRepBufferSize,
                              (PVOID*)&pReply));
    BAIL_ON_LSA_ERROR(dwError);

    dwNumGroups = pReply->dwNumGroups;

    if (dwNumGroups) {
        dwError = LsaAllocateMemory(sizeof(ppGroupInfo[0]) * dwNumGroups,
                                    (PVOID*)&ppGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);

        for (i = 0; i < dwNumGroups; i++) {
            PVOID pInfo = NULL;

            dwError = LsaAllocateGroupInfo(&pInfo,
                                           pReply->dwGroupInfoLevel,
                                           (PVOID)pReply->Groups.ppInfo0[i]);
            ppGroupInfo[i] = pInfo;
        }
    }

    *pdwGroupsCount   = dwNumGroups;
    *pppGroupInfoList = ppGroupInfo;

cleanup:
    if (pReply) {
        lwmsg_data_free_graph(
            pDataHandle,
            LsaLocalIPCGetGroupMembershipRepSpec(),
            pReply);
    }

    if (pDataHandle)
    {
        lwmsg_data_handle_delete(pDataHandle);
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
    if (ppGroupInfo) {
        LsaFreeGroupInfoList(pReply->dwGroupInfoLevel,
                             ppGroupInfo,
                             pReply->dwNumGroups);
    }

    *pdwGroupsCount   = 0;
    *pppGroupInfoList = NULL;

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
