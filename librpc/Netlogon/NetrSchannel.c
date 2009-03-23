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


NTSTATUS
NetrOpenSchannel(
    handle_t netr_b,
    const wchar16_t * pwszMachineAccount,
    const wchar16_t * pwszHostname,
    const wchar16_t * pwszServer,
    const wchar16_t * pwszDomain,
    const wchar16_t * pwszComputer,
    const wchar16_t * pwszMachinePassword,
    NetrCredentials *pCreds,
    handle_t *schannel_b
    )
{
    RPCSTATUS rpcstatus = RPC_S_OK;
    NTSTATUS status = STATUS_SUCCESS;
    uint8 pass_hash[16] = {0};
    uint8 cli_chal[8] = {0};
    uint8 srv_chal[8] = {0};
    uint8 srv_cred[8] = {0};
    rpc_schannel_auth_info_t schnauth_info = {0};
    PIO_ACCESS_TOKEN access_token = NULL;
    size_t hostname_size = 0;
    char *pszHostname = NULL;
    handle_t schn_b = NULL;

    md4hash(pass_hash, pwszMachinePassword);

    get_random_buffer((uint8*)cli_chal, sizeof(cli_chal));
    status = NetrServerReqChallenge(netr_b,
                                    pwszServer,
                                    pwszComputer,
                                    cli_chal,
                                    srv_chal);
    goto_if_ntstatus_not_success(status, error);

    NetrCredentialsInit(pCreds,
                        cli_chal,
                        srv_chal,
                        pass_hash,
                        NETLOGON_NET_ADS_FLAGS);

    status = NetrServerAuthenticate2(netr_b,
                                     pwszServer,
                                     pwszMachineAccount,
                                     pCreds->channel_type,
                                     pwszComputer,
                                     pCreds->cli_chal.data,
                                     srv_cred,
                                     &pCreds->negotiate_flags);
    goto_if_ntstatus_not_success(status, error);

    if (!NetrCredentialsCorrect(pCreds, srv_cred)) {
        status = STATUS_ACCESS_DENIED;
        goto error;
    }

    memcpy(schnauth_info.session_key, pCreds->session_key, 16);
    schnauth_info.domain_name  = (unsigned char*) awc16stombs(pwszDomain);
    schnauth_info.machine_name = (unsigned char*) awc16stombs(pwszComputer);
    schnauth_info.sender_flags = rpc_schn_initiator_flags;

    goto_if_no_memory_ntstatus(schnauth_info.domain_name, error);
    goto_if_no_memory_ntstatus(schnauth_info.machine_name, error);

    status = LwIoGetThreadAccessToken(&access_token);
    goto_if_ntstatus_not_success(status, error);

    hostname_size = wc16slen(pwszHostname) + 1;
    status = NetrAllocateMemory((void**)&pszHostname,
                                hostname_size * sizeof(char),
                                NULL);
    goto_if_ntstatus_not_success(status, error);

    wc16stombs(pszHostname, pwszHostname, hostname_size);

    rpcstatus = InitNetlogonBindingDefault(&schn_b, pszHostname, access_token,
                                           TRUE);
    goto_if_rpcstatus_not_success(rpcstatus, error);

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
    goto_if_rpcstatus_not_success(rpcstatus, error);

    *schannel_b = schn_b;

cleanup:
    SAFE_FREE(schnauth_info.domain_name);
    SAFE_FREE(schnauth_info.machine_name);

    if (pszHostname) {
        NetrFreeMemory(pszHostname);
    }

    if (access_token)
    {
        LwIoDeleteAccessToken(access_token);
    }

    return status;

error:
    if (schn_b)
    {
        FreeNetlogonBinding(&schn_b);
    }

    goto cleanup;
}


void
NetrCloseSchannel(
    handle_t schn_b
    )
{
    if (schn_b)
    {
        FreeNetlogonBinding(&schn_b);
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
