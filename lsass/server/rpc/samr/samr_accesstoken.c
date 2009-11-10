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
 *        samr_creds.c
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


NTSTATUS
SamrSrvCreateAccessToken(
    IN  handle_t       hBinding,
    OUT PACCESS_TOKEN *ppToken
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    RPCSTATUS rpcStatus = 0;
    rpc_transport_info_handle_t hTransportInfo = NULL;
    PSTR pszPrincipalName = NULL;
    PBYTE pbSessionKey = NULL;
    USHORT usSessionKeyLen = 0;
    PLW_MAP_SECURITY_CONTEXT *pSecCtx = NULL;
    PACCESS_TOKEN pToken = NULL;

    rpc_binding_inq_transport_info(hBinding,
                                   &hTransportInfo,
                                   &rpcStatus);
    if (rpcStatus)
    {
        ntStatus = LwRpcStatusToNtStatus(rpcStatus);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    rpc_smb_transport_info_inq_peer_principal_name(
                                   hTransportInfo,
                                   (unsigned char**)&pszPrincipalName);

    rpc_smb_transport_info_inq_session_key(hTransportInfo,
                                   (unsigned char**)&pbSessionKey,
                                   (unsigned16*)&usSessionKeyLen);

    ntStatus = LwMapSecurityCreateContext(&pSecCtx);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LwMapSecurityCreateAccessTokenFromCStringUsername(
                                   pSecCtx,
                                   &pToken,
                                   pszPrincipalName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppToken = pToken;

cleanup:
    if (pSecCtx)
    {
        LwMapSecurityFreeContext(&pSecCtx);
    }

    return ntStatus;

error:
    if (pToken)
    {
        RtlReleaseAccessToken(pToken);
    }

    *ppToken = NULL;

    goto cleanup;
}


NTSTATUS
SamrSrvGetSystemCreds(
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
