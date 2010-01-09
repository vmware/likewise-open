/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        lsa_accesstoken.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Access token handling functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
LsaSrvInitNpAuthInfo(
    IN  rpc_transport_info_handle_t hTransportInfo,
    OUT PPOLICY_CONTEXT             pPolCtx
    );


static
NTSTATUS
LsaSrvInitLpcAuthInfo(
    IN  rpc_transport_info_handle_t hTransportInfo,
    OUT PPOLICY_CONTEXT             pPolCtx
    );


NTSTATUS
LsaSrvInitAuthInfo(
    IN  handle_t          hBinding,
    OUT PPOLICY_CONTEXT   pPolCtx
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    RPCSTATUS rpcStatus = 0;
    DWORD dwAuthentication = 0;
    PVOID pAuthCtx = NULL;
    rpc_transport_info_handle_t hTransportInfo = NULL;
    DWORD dwProtSeq = rpc_c_invalid_protseq_id;
    PLW_MAP_SECURITY_CONTEXT pSecCtx = gpLsaSecCtx;

    if (!pSecCtx)
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    rpc_binding_inq_security_context(hBinding,
                                     (unsigned32*)&dwAuthentication,
                                     (PVOID*)&pAuthCtx,
                                     &rpcStatus);
    if (rpcStatus == rpc_s_binding_has_no_auth)
    {
        /*
         * There's no DCE/RPC authentication info so check
         * the transport layer.
         */
        rpcStatus = 0;
        rpc_binding_inq_transport_info(hBinding,
                                       &hTransportInfo,
                                       &rpcStatus);
        if (rpcStatus)
        {
            ntStatus = LwRpcStatusToNtStatus(rpcStatus);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        if (hTransportInfo)
        {
            rpcStatus = 0;
            rpc_binding_inq_prot_seq(hBinding,
                                     (unsigned32*)&dwProtSeq,
                                     &rpcStatus);
            if (rpcStatus)
            {
                ntStatus = LwRpcStatusToNtStatus(rpcStatus);
                BAIL_ON_NTSTATUS_ERROR(ntStatus);
            }

            switch (dwProtSeq)
            {
            case rpc_c_protseq_id_ncacn_np:
                ntStatus = LsaSrvInitNpAuthInfo(hTransportInfo,
                                                pPolCtx);
                break;

            case rpc_c_protseq_id_ncalrpc:
                ntStatus = LsaSrvInitLpcAuthInfo(hTransportInfo,
                                                 pPolCtx);
                break;
            }
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }
    }
    else if (rpcStatus)
    {
        ntStatus = LwRpcStatusToNtStatus(rpcStatus);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

cleanup:
    return ntStatus;

error:
    LsaSrvFreeAuthInfo(pPolCtx);

    goto cleanup;
}


static
NTSTATUS
LsaSrvInitNpAuthInfo(
    IN  rpc_transport_info_handle_t hTransportInfo,
    OUT PPOLICY_CONTEXT             pPolCtx
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PLW_MAP_SECURITY_CONTEXT pSecCtx = gpLsaSecCtx;
    PSTR pszPrincipalName = NULL;
    PUCHAR pucSessionKey = NULL;
    USHORT usSessionKeyLen = 0;
    PACCESS_TOKEN pToken = NULL;
    PBYTE pSessionKey = NULL;
    DWORD dwSessionKeyLen = 0;

    if (!pSecCtx)
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    rpc_smb_transport_info_inq_peer_principal_name(
                                   hTransportInfo,
                                   (unsigned char**)&pszPrincipalName);

    rpc_smb_transport_info_inq_session_key(
                                   hTransportInfo,
                                   (unsigned char**)&pucSessionKey,
                                   (unsigned16*)&usSessionKeyLen);

    ntStatus = LwMapSecurityCreateAccessTokenFromCStringUsername(
                                   pSecCtx,
                                   &pToken,
                                   pszPrincipalName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwSessionKeyLen = usSessionKeyLen;
    if (dwSessionKeyLen)
    {
        dwError = LwAllocateMemory(dwSessionKeyLen,
                                   OUT_PPVOID(&pSessionKey));
        BAIL_ON_LSA_ERROR(dwError);

        memcpy(pSessionKey, pucSessionKey, dwSessionKeyLen);
    }

    pPolCtx->pUserToken      = pToken;
    pPolCtx->pSessionKey     = pSessionKey;
    pPolCtx->dwSessionKeyLen = dwSessionKeyLen;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaSrvInitLpcAuthInfo(
    IN  rpc_transport_info_handle_t hTransportInfo,
    OUT PPOLICY_CONTEXT             pPolCtx
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PLW_MAP_SECURITY_CONTEXT pSecCtx = gpLsaSecCtx;
    uid_t uid = 0;
    gid_t gid = 0;
    PACCESS_TOKEN pToken = NULL;

    if (!pSecCtx)
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    rpc_lrpc_transport_info_inq_peer_eid(
                                   hTransportInfo,
                                   (unsigned32*)&uid,
                                   (unsigned32*)&gid);

    ntStatus = LwMapSecurityCreateAccessTokenFromUidGid(
                                   pSecCtx,
                                   &pToken,
                                   uid,
                                   gid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pPolCtx->pUserToken = pToken;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


VOID
LsaSrvFreeAuthInfo(
    IN  PPOLICY_CONTEXT pPolCtx
    )
{
    if (pPolCtx == NULL) return;

    if (pPolCtx->pUserToken)
    {
        RtlReleaseAccessToken(&pPolCtx->pUserToken);
        pPolCtx->pUserToken = NULL;
    }

    if (pPolCtx->pSessionKey)
    {
        LW_SAFE_FREE_MEMORY(pPolCtx->pSessionKey);
        pPolCtx->pSessionKey     = NULL;
        pPolCtx->dwSessionKeyLen = 0;
    }
}


NTSTATUS
LsaSrvGetSystemCreds(
    LW_PIO_CREDS *ppCreds
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    LW_PIO_CREDS pCreds = NULL;
    PSTR pszUsername = NULL;
    PSTR pszPassword = NULL;
    PSTR pszDomainDnsName = NULL;
    PSTR pszHostDnsDomain = NULL;
    PSTR pszMachinePrincipal = NULL;

    dwError = LwKrb5GetMachineCreds(
                    &pszUsername,
                    &pszPassword,
                    &pszDomainDnsName,
                    &pszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszMachinePrincipal,
                    "%s@%s",
                    pszUsername,
                    pszDomainDnsName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwIoCreateKrb5CredsA(
                    pszMachinePrincipal,
                    LSASS_KRB5_CACHE_PATH,
                    &pCreds);
    BAIL_ON_LSA_ERROR(dwError);

    *ppCreds = pCreds;

cleanup:
    LW_SAFE_FREE_STRING(pszUsername);
    LW_SAFE_FREE_STRING(pszPassword);
    LW_SAFE_FREE_STRING(pszDomainDnsName);
    LW_SAFE_FREE_STRING(pszHostDnsDomain);
    LW_SAFE_FREE_STRING(pszMachinePrincipal);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    *ppCreds = NULL;
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
