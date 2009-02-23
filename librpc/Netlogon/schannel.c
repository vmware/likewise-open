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
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *          Gerald Carter (gcarter@likewise.com)
 */

#include "includes.h"


static handle_t
CreateNetlogonBinding(handle_t *binding, const wchar16_t *host)
{
    RPCSTATUS status = RPC_S_OK;
    size_t hostname_size = 0;
    char *hostname = NULL;
    PIO_ACCESS_TOKEN access_token = NULL;

    if (binding == NULL || host == NULL) return NULL;

    if (LwIoGetThreadAccessToken(&access_token) != STATUS_SUCCESS) return NULL;

    hostname_size = wc16slen(host) + 1;
    hostname = (char*) malloc(hostname_size * sizeof(char));
    if (hostname == NULL) return NULL;

    wc16stombs(hostname, host, hostname_size);

    status = InitNetlogonBindingDefault(binding, hostname, access_token, TRUE);
    if (status != RPC_S_OK) return NULL;

    SAFE_FREE(hostname);

    if (access_token)
    {
        LwIoDeleteAccessToken(access_token);
    }

    return *binding;
}


handle_t
OpenSchannel(
    handle_t netr_b,
    const wchar16_t * pwszMachineAccount,
    const wchar16_t * pwszHostname,
    const wchar16_t * pwszServer,
    const wchar16_t * pwszDomain,
    const wchar16_t * pwszComputer,
    const wchar16_t * pwszMachinePassword,
    NetrCredentials *Creds,
    NETRESOURCE *SchanRes
    )
{
    RPCSTATUS rpcstatus = RPC_S_OK;
    NTSTATUS status = STATUS_SUCCESS;
    uint8 pass_hash[16] = {0};
    uint8 cli_chal[8] = {0};
    uint8 srv_chal[8] = {0};
    uint8 srv_cred[8] = {0};
    rpc_schannel_auth_info_t schnauth_info = {0};
    handle_t schn_b = NULL;

    md4hash(pass_hash, pwszMachinePassword);

    get_random_buffer((uint8*)cli_chal, sizeof(cli_chal));
    status = NetrServerReqChallenge(netr_b,
                                    pwszServer,
                                    pwszComputer,
                                    cli_chal,
                                    srv_chal);
    goto_if_ntstatus_not_success(status, error);

    NetrCredentialsInit(Creds,
                        cli_chal,
                        srv_chal,
                        pass_hash,
                        NETLOGON_NET_ADS_FLAGS);

    status = NetrServerAuthenticate2(netr_b,
                                     pwszServer,
                                     pwszMachineAccount,
                                     Creds->channel_type,
                                     pwszComputer,
                                     Creds->cli_chal.data,
                                     srv_cred,
                                     &Creds->negotiate_flags);
    goto_if_ntstatus_not_success(status, error);

    if (!NetrCredentialsCorrect(Creds, srv_cred)) {
        status = STATUS_ACCESS_DENIED;
        goto error;
    }

    memcpy(schnauth_info.session_key, Creds->session_key, 16);
    schnauth_info.domain_name  = (unsigned char*) awc16stombs(pwszDomain);
    schnauth_info.machine_name = (unsigned char*) awc16stombs(pwszComputer);
    schnauth_info.sender_flags = rpc_schn_initiator_flags;

    goto_if_no_memory_ntstatus(schnauth_info.domain_name, error);
    goto_if_no_memory_ntstatus(schnauth_info.machine_name, error);

    schn_b = CreateNetlogonBinding(&schn_b, pwszHostname);
    if (schn_b == NULL)
    {
        goto error;
    }

    rpc_binding_set_auth_info(schn_b,
                              NULL,
#if 0
                              /* Helps to debug network traces */
                              rpc_c_authn_level_pkt_integrity,
#else
                              /* Secure */
                              rpc_c_authn_level_pkt_privacy,
#endif
                              rpc_c_authn_schannel,
                              (rpc_auth_identity_handle_t)&schnauth_info,
                              rpc_c_authz_name, /* authz_protocol */
                              &rpcstatus);
    if (rpcstatus != RPC_S_OK) {
        status = STATUS_UNSUCCESSFUL;
        goto error;
    }

cleanup:
    SAFE_FREE(schnauth_info.domain_name);
    SAFE_FREE(schnauth_info.machine_name);

    return (rpcstatus == rpc_s_ok &&
            status == STATUS_SUCCESS) ? schn_b : NULL;

error:
    if (schn_b)
    {
        FreeNetlogonBinding(&schn_b);
    }

    goto cleanup;
}


void
CloseSchannel(
    handle_t schn_b,
    NETRESOURCE *schnr
    )
{
    if (schn_b)
    {
        FreeNetlogonBinding(&schn_b);
    }

    SAFE_FREE(schnr->RemoteName);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

