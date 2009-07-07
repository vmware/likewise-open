/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:
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
n * This library is distributed in the hope that it will be useful,
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
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *          Gerald Carter (gcarter@likewise.com)
 */

#include "includes.h"


NTSTATUS
NetrOpenSchannel(
    IN  handle_t          hNetrBinding,
    IN  PCWSTR            pwszMachineAccount,
    IN  PCWSTR            pwszHostname,
    IN  PCWSTR            pwszServer,
    IN  PCWSTR            pwszDomain,
    IN  PCWSTR            pwszComputer,
    IN  PCWSTR            pwszMachinePassword,
    IN  NetrCredentials  *pCreds,
    OUT handle_t         *phSchannelBinding
    )
{
    RPCSTATUS rpcStatus = RPC_S_OK;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BYTE PassHash[16] = {0};
    BYTE CliChal[8] = {0};
    BYTE SrvChal[8] = {0};
    BYTE SrvCred[8] = {0};
    rpc_schannel_auth_info_t SchannelAuthInfo = {0};
    PIO_ACCESS_TOKEN pAccessToken = NULL;
    size_t HostnameSize = 0;
    PSTR pszHostname = NULL;
    handle_t hSchannelBinding = NULL;

    md4hash(PassHash, pwszMachinePassword);

    get_random_buffer((uint8*)CliChal, sizeof(CliChal));
    ntStatus = NetrServerReqChallenge(hNetrBinding,
                                      pwszServer,
                                      pwszComputer,
                                      CliChal,
                                      SrvChal);
    BAIL_ON_NT_STATUS(ntStatus);

    NetrCredentialsInit(pCreds,
                        CliChal,
                        SrvChal,
                        PassHash,
                        NETLOGON_NET_ADS_FLAGS);

    ntStatus = NetrServerAuthenticate2(hNetrBinding,
                                       pwszServer,
                                       pwszMachineAccount,
                                       pCreds->channel_type,
                                       pwszComputer,
                                       pCreds->cli_chal.data,
                                       SrvCred,
                                       &pCreds->negotiate_flags);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!NetrCredentialsCorrect(pCreds, SrvCred)) {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(SchannelAuthInfo.session_key,
           pCreds->session_key,
           16);

    SchannelAuthInfo.domain_name  = (unsigned char*) awc16stombs(pwszDomain);
    SchannelAuthInfo.machine_name = (unsigned char*) awc16stombs(pwszComputer);
    SchannelAuthInfo.sender_flags = rpc_schn_initiator_flags;

    BAIL_ON_NULL_PTR(SchannelAuthInfo.domain_name, ntStatus);
    BAIL_ON_NULL_PTR(SchannelAuthInfo.machine_name, ntStatus);

    ntStatus = LwIoGetThreadAccessToken(&pAccessToken);
    BAIL_ON_NT_STATUS(ntStatus);

    HostnameSize = wc16slen(pwszHostname) + 1;
    ntStatus = NetrAllocateMemory((void**)&pszHostname,
                                  HostnameSize * sizeof(CHAR),
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    wc16stombs(pszHostname, pwszHostname, HostnameSize);

    ntStatus = InitNetlogonBindingDefault(&hSchannelBinding,
                                          pszHostname,
                                          pAccessToken,
                                          TRUE);
    BAIL_ON_RPCSTATUS_ERROR(rpcStatus);

    rpc_binding_set_auth_info(hSchannelBinding,
                              NULL,
#if 0
                              /* Helps to debug network traces */
                              rpc_c_authn_level_pkt_integrity,
#else
                              /* Secure */
                              rpc_c_authn_level_pkt_privacy,
#endif
                              rpc_c_authn_schannel,
                              (rpc_auth_identity_handle_t)&SchannelAuthInfo,
                              rpc_c_authz_name, /* authz_protocol */
                              &rpcStatus);
    BAIL_ON_RPCSTATUS_ERROR(rpcStatus);

    *phSchannelBinding = hSchannelBinding;

cleanup:
    SAFE_FREE(SchannelAuthInfo.domain_name);
    SAFE_FREE(SchannelAuthInfo.machine_name);

    if (pszHostname)
    {
        NetrFreeMemory(pszHostname);
    }

    if (pAccessToken)
    {
        LwIoDeleteAccessToken(pAccessToken);
    }

    return ntStatus;

error:
    if (hSchannelBinding)
    {
        FreeNetlogonBinding(&hSchannelBinding);
    }

    *phSchannelBinding = NULL;

    goto cleanup;
}


VOID
NetrCloseSchannel(
    IN  handle_t hSchannelBinding
    )
{
    if (hSchannelBinding)
    {
        FreeNetlogonBinding(&hSchannelBinding);
    }
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
