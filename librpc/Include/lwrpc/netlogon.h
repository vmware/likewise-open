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
 * Abstract: Netlogon interface (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _NETLOGON_H_
#define _NETLOGON_H_

#include <lwrpc/netlogonbinding.h>
#include <lwrpc/netrdefs.h>
#include <lwrpc/mpr.h>


NTSTATUS
NetrOpenSchannel(
    handle_t binding_handle,
    const wchar16_t * pwszMachineAccount,
    const wchar16_t * pwszHostname,
    const wchar16_t * pwszServer,
    const wchar16_t * pwszDomain,
    const wchar16_t * pwszComputer,
    const wchar16_t * pwszMachinePassword,
    NetrCredentials *Creds,
    handle_t *schannel_binding
    );


void
NetrCloseSchannel(
    handle_t schannel_binding
    );


int
NetrCredentialsCorrect(
    NetrCredentials *creds,
    uint8 srv_creds[8]
);


NTSTATUS
NetrServerReqChallenge(
    handle_t binding_handle,
    const wchar16_t *server,
    const wchar16_t *computer,
    uint8 cli_challenge[8],
    uint8 srv_challenge[8]
    );


NTSTATUS
NetrServerAuthenticate(
    handle_t binding_handle,
    const wchar16_t *server,
    const wchar16_t *account,
    uint16 sec_chan_type,
    const wchar16_t *computer,
    uint8 cli_creds[8],
    uint8 srv_creds[8]
    );


NTSTATUS
NetrServerAuthenticate2(
    handle_t binding_handle,
    const wchar16_t *server,
    const wchar16_t *account,
    uint16 sec_chan_type,
    const wchar16_t *computer,
    uint8 cli_credentials[8],
    uint8 srv_credentials[8],
    uint32 *neg_flags
    );


NTSTATUS
NetrServerAuthenticate3(
    handle_t binding_handle,
    const wchar16_t *server,
    const wchar16_t *account,
    uint16 sec_chan_type,
    const wchar16_t *computer,
    uint8 cli_creds[8],
    uint8 srv_creds[8],
    uint32 *neg_flags,
    uint32 *rid
    );


NTSTATUS
NetrGetDomainInfo(
    handle_t schannel_binding,
    NetrCredentials *creds,
    const wchar16_t *server,
    const wchar16_t *computer,
    uint32 level,
    NetrDomainQuery *query,
    NetrDomainInfo **out_info
    );


NTSTATUS NetrSamLogonInteractive(
    handle_t schannel_binding,
    NetrCredentials *creds,
    const wchar16_t *server,
    const wchar16_t *domain,
    const wchar16_t *computer,
    const wchar16_t *username,
    const wchar16_t *password,
    uint16 logon_level, uint16 validation_level,
    NetrValidationInfo **out_info,
    uint8 *out_authoritative
    );

NTSTATUS NetrSamLogonNetwork(
    handle_t schannel_binding,
    NetrCredentials *creds,
    const wchar16_t *server,
    const wchar16_t *domain,
    const wchar16_t *computer,
    const wchar16_t *username,
    uint8 * challenge,
    uint8 * lm_resp,
    uint32 lm_resp_len,
    uint8 * nt_resp,
    uint32 nt_resp_len,
    uint16 logon_level,
    uint16 validation_level,
    NetrValidationInfo **out_info,
    uint8 *out_authoritative
    );

NTSTATUS
NetrSamLogoff(
    handle_t schannel_binding,
    NetrCredentials *creds,
    const wchar16_t *server,
    const wchar16_t *domain,
    const wchar16_t *computer,
    const wchar16_t *username,
    const wchar16_t *password,
    uint32 logon_level
    );


NTSTATUS NetrSamLogonEx(
    handle_t schannel_binding,
    const wchar16_t *server,
    const wchar16_t *domain,
    const wchar16_t *computer,
    const wchar16_t *username,
    const wchar16_t *password,
    uint16 logon_level,
    uint16 validation_level,
    NetrValidationInfo **info,
    uint8 *authoritative
    );


NTSTATUS
NetrEnumerateTrustedDomainsEx(
    handle_t binding_handle,
    const wchar16_t *server,
    NetrDomainTrust **trusts,
    uint32 *count
    );


WINERR
DsrEnumerateDomainTrusts(
    handle_t binding_handle,
    const wchar16_t *server,
    uint32 flags,
    NetrDomainTrust **trusts,
    uint32 *count
    );


WINERR
DsrGetDcName(
    handle_t binding_handle,
    const wchar16_t *server,
    const wchar16_t *domain,
    const Guid *domain_guid,
    const Guid *site_guid,
    uint32 get_dc_flags,
    DsrDcNameInfo **out_info
    );


void
NetrCredentialsInit(
    NetrCredentials *creds,
    uint8 cli_chal[8],
    uint8 srv_chal[8],
    uint8 pass_hash[16],
    uint32 neg_flags
    );


NTSTATUS
NetrInitMemory(
    void
    );


NTSTATUS
NetrDestroyMemory(
    void
    );


NTSTATUS
NetrFreeMemory(
    void *ptr
    );


#endif /* _NETLOGON_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
