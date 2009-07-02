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

    BAIL_ON_INVALID_PTR(b);
    BAIL_ON_INVALID_PTR(creds);
    BAIL_ON_INVALID_PTR(server);
    BAIL_ON_INVALID_PTR(domain);
    BAIL_ON_INVALID_PTR(computer);
    BAIL_ON_INVALID_PTR(username);
    BAIL_ON_INVALID_PTR(password);
    BAIL_ON_INVALID_PTR(out_info);
    BAIL_ON_INVALID_PTR(out_authoritative);

    if (!(logon_level == 1 || logon_level == 3 || logon_level == 5)) {
        status = STATUS_INVALID_INFO_CLASS;
        goto cleanup;
    }

    status = NetrAllocateUniString(&srv, server, NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetrAllocateUniString(&comp, computer, NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* Create authenticator info with credentials chain */
    status = NetrAllocateMemory((void**)&auth, sizeof(NetrAuth), NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    creds->sequence += 2;
    NetrCredentialsCliStep(creds);

    auth->timestamp = creds->sequence;
    memcpy(auth->cred.data, creds->cli_chal.data, sizeof(auth->cred.data));

    /* Allocate returned authenticator */
    status = NetrAllocateMemory((void**)&ret_auth, sizeof(NetrAuth), NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetrAllocateLogonInfoHash(&logon_info, logon_level, domain, computer,
                                       username, password);
    BAIL_ON_NTSTATUS_ERROR(status);

    DCERPC_CALL(status, _NetrLogonSamLogon(b, srv, comp, auth, ret_auth,
                                           logon_level, logon_info,
                                           validation_level, &validation,
                                           &authoritative));
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetrAllocateValidationInfo(&validation_info, &validation,
                                        validation_level);
    BAIL_ON_NTSTATUS_ERROR(status);

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
    uint32_t lm_resp_len,
    uint8 *nt_resp,
    uint32 nt_resp_len,
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

    BAIL_ON_INVALID_PTR(b);
    BAIL_ON_INVALID_PTR(creds);
    BAIL_ON_INVALID_PTR(server);
    BAIL_ON_INVALID_PTR(domain);
    BAIL_ON_INVALID_PTR(computer);
    BAIL_ON_INVALID_PTR(username);
    BAIL_ON_INVALID_PTR(challenge);
    /* LanMan Response could be NULL */
    BAIL_ON_INVALID_PTR(nt_resp);
    BAIL_ON_INVALID_PTR(out_info);
    BAIL_ON_INVALID_PTR(out_authoritative);

    if (!(logon_level == 2 || logon_level == 6)) {
        status = STATUS_INVALID_INFO_CLASS;
        goto cleanup;
    }

    status = RtlWC16StringDuplicate(&srv, server);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = RtlWC16StringDuplicate(&comp, computer);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* Create authenticator info with credentials chain */
    status = RTL_ALLOCATE((void**)&auth, NetrAuth, sizeof(NetrAuth));
    BAIL_ON_NTSTATUS_ERROR(status);

    creds->sequence += 2;
    NetrCredentialsCliStep(creds);

    auth->timestamp = creds->sequence;
    memcpy(auth->cred.data, creds->cli_chal.data, sizeof(auth->cred.data));

    /* Allocate returned authenticator */
    status = RTL_ALLOCATE((void**)&ret_auth, NetrAuth, sizeof(NetrAuth));
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetrAllocateMemory((void**)&ret_auth, sizeof(NetrAuth), NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetrAllocateLogonInfoNet(&logon_info, logon_level,
                                      domain, computer,
                                      username,
                                      challenge,
                                      lm_resp, lm_resp_len,
                                      nt_resp, nt_resp_len);
    BAIL_ON_NTSTATUS_ERROR(status);

    DCERPC_CALL(status, _NetrLogonSamLogon(b, srv, comp, auth, ret_auth,
                                           logon_level, logon_info,
                                           validation_level, &validation,
                                           &authoritative));
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetrAllocateValidationInfo(&validation_info, &validation,
                                        validation_level);
    BAIL_ON_NTSTATUS_ERROR(status);

    *out_info          = validation_info;
    *out_authoritative = authoritative;

cleanup:
    NetrCleanStubValidationInfo(&validation, validation_level);

    RtlWC16StringFree(&srv);
    RtlWC16StringFree(&comp);
    RTL_FREE(&auth);
    RTL_FREE(&ret_auth);

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
