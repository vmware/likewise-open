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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


NTSTATUS NetrSamLogonInteractive(
    handle_t b,
    NetrCredentials *creds,
    const wchar16_t *server,
    const wchar16_t *domain,
    const wchar16_t *computer,
    const wchar16_t *username,
    const wchar16_t *password,
    uint16 logon_level, uint16 validation_level,
    NetrValidationInfo **out_info,
    uint8 *out_authoritative
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t *srv = NULL;
    wchar16_t *comp = NULL;
    NetrAuth *auth = NULL;
    NetrAuth *ret_auth = NULL;
    NetrLogonInfo *logon_info = NULL;
    NetrValidationInfo validation = {0};
    NetrValidationInfo *validation_info = NULL;
    uint8 authoritative = 0;

    goto_if_invalid_param_ntstatus(b, cleanup);
    goto_if_invalid_param_ntstatus(creds, cleanup);
    goto_if_invalid_param_ntstatus(server, cleanup);
    goto_if_invalid_param_ntstatus(domain, cleanup);
    goto_if_invalid_param_ntstatus(computer, cleanup);
    goto_if_invalid_param_ntstatus(username, cleanup);
    goto_if_invalid_param_ntstatus(password, cleanup);
    goto_if_invalid_param_ntstatus(out_info, cleanup);
    goto_if_invalid_param_ntstatus(out_authoritative, cleanup);

    if (!(logon_level == 1 || logon_level == 3 || logon_level == 5)) {
        status = STATUS_INVALID_INFO_CLASS;
        goto cleanup;
    }

    status = NetrAllocateUniString(&srv, server, NULL);
    goto_if_ntstatus_not_success(status, error);

    status = NetrAllocateUniString(&comp, computer, NULL);
    goto_if_ntstatus_not_success(status, error);

    /* Create authenticator info with credentials chain */
    status = NetrAllocateMemory((void**)&auth, sizeof(NetrAuth), NULL);
    goto_if_ntstatus_not_success(status, error);

    creds->sequence += 2;
    NetrCredentialsCliStep(creds);

    auth->timestamp = creds->sequence;
    memcpy(auth->cred.data, creds->cli_chal.data, sizeof(auth->cred.data));

    /* Allocate returned authenticator */
    status = NetrAllocateMemory((void**)&ret_auth, sizeof(NetrAuth), NULL);
    goto_if_ntstatus_not_success(status, error);

    status = NetrAllocateLogonInfoHash(&logon_info, logon_level, domain, computer,
                                       username, password);
    goto_if_ntstatus_not_success(status, error);

    DCERPC_CALL(status, _NetrLogonSamLogon(b, srv, comp, auth, ret_auth,
                                           logon_level, logon_info,
                                           validation_level, &validation,
                                           &authoritative));
    goto_if_ntstatus_not_success(status, error);

    status = NetrAllocateValidationInfo(&validation_info, &validation,
                                        validation_level);
    goto_if_ntstatus_not_success(status, error);

    *out_info          = validation_info;
    *out_authoritative = authoritative;

cleanup:
    NetrCleanStubValidationInfo(&validation, validation_level);

    if (srv) {
        NetrFreeMemory((void*)srv);
    }

    if (comp) {
        NetrFreeMemory((void*)comp);
    }

    if (auth) {
        NetrFreeMemory((void*)auth);
    }

    if (logon_info) {
        NetrFreeMemory((void*)logon_info);
    }

    return status;

error:
    if (validation_info) {
        NetrFreeMemory((void*)validation_info);
    }

    *out_info          = NULL;
    *out_authoritative = 0;

    goto cleanup;
}


NTSTATUS NetrSamLogonNetwork(
    handle_t b,
    NetrCredentials *creds,
    const wchar16_t *server,
    const wchar16_t *domain,
    const wchar16_t *computer,
    const wchar16_t *username,
    uint8 *challenge,
    uint8 *lm_resp,
    uint8 *nt_resp,
    uint16 logon_level,
    uint16 validation_level,
    NetrValidationInfo **out_info,
    uint8 *out_authoritative
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t *srv = NULL;
    wchar16_t *comp = NULL;
    NetrAuth *auth = NULL;
    NetrAuth *ret_auth = NULL;
    NetrLogonInfo *logon_info = NULL;
    NetrValidationInfo validation = {0};
    NetrValidationInfo *validation_info = NULL;
    uint8 authoritative = 0;

    goto_if_invalid_param_ntstatus(b, cleanup);
    goto_if_invalid_param_ntstatus(creds, cleanup);
    goto_if_invalid_param_ntstatus(server, cleanup);
    goto_if_invalid_param_ntstatus(domain, cleanup);
    goto_if_invalid_param_ntstatus(computer, cleanup);
    goto_if_invalid_param_ntstatus(username, cleanup);
    goto_if_invalid_param_ntstatus(challenge, cleanup);
    /* LanMan Response could be NULL */
    goto_if_invalid_param_ntstatus(nt_resp, cleanup);
    goto_if_invalid_param_ntstatus(out_info, cleanup);
    goto_if_invalid_param_ntstatus(out_authoritative, cleanup);

    if (!(logon_level == 2 || logon_level == 6)) {
        status = STATUS_INVALID_INFO_CLASS;
        goto cleanup;
    }

    status = NetrAllocateUniString(&srv, server, NULL);
    goto_if_ntstatus_not_success(status, error);

    status = NetrAllocateUniString(&comp, computer, NULL);
    goto_if_ntstatus_not_success(status, error);

    /* Create authenticator info with credentials chain */
    status = NetrAllocateMemory((void**)&auth, sizeof(NetrAuth), NULL);
    goto_if_ntstatus_not_success(status, error);

    creds->sequence += 2;
    NetrCredentialsCliStep(creds);

    auth->timestamp = creds->sequence;
    memcpy(auth->cred.data, creds->cli_chal.data, sizeof(auth->cred.data));

    /* Allocate returned authenticator */
    status = NetrAllocateMemory((void**)&ret_auth, sizeof(NetrAuth), NULL);
    goto_if_ntstatus_not_success(status, error);

    status = NetrAllocateLogonInfoNet(&logon_info, logon_level,
                                      domain, computer,
                                      username,
                                      challenge, lm_resp, nt_resp);
    goto_if_ntstatus_not_success(status, error);

    DCERPC_CALL(status, _NetrLogonSamLogon(b, srv, comp, auth, ret_auth,
                                           logon_level, logon_info,
                                           validation_level, &validation,
                                           &authoritative));
    goto_if_ntstatus_not_success(status, error);

    status = NetrAllocateValidationInfo(&validation_info, &validation,
                                        validation_level);
    goto_if_ntstatus_not_success(status, error);

    *out_info          = validation_info;
    *out_authoritative = authoritative;

cleanup:
    NetrCleanStubValidationInfo(&validation, validation_level);

    if (srv) {
        NetrFreeMemory((void*)srv);
    }

    if (comp) {
        NetrFreeMemory((void*)comp);
    }

    if (auth) {
        NetrFreeMemory((void*)auth);
    }

    if (logon_info) {
        NetrFreeMemory((void*)logon_info);
    }

    return status;

error:
    if (validation_info) {
        NetrFreeMemory((void*)validation_info);
    }

    *out_info          = NULL;
    *out_authoritative = 0;

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
