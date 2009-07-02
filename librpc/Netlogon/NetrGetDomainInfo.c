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
 */

#include "includes.h"


NTSTATUS
NetrGetDomainInfo(
    handle_t b,
    NetrCredentials *creds,
    const wchar16_t *server,
    const wchar16_t *computer,
    uint32 level,
    NetrDomainQuery *query,
    NetrDomainInfo **out_info
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t *srv = NULL;
    wchar16_t *comp = NULL;
    NetrAuth *auth = NULL;
    NetrAuth *ret_auth = NULL;
    NetrDomainInfo *domain_info = NULL;
    NetrDomainInfo info;

    memset((void*)&info, 0, sizeof(info));

    BAIL_ON_INVALID_PTR(b);
    BAIL_ON_INVALID_PTR(creds);
    BAIL_ON_INVALID_PTR(server);
    BAIL_ON_INVALID_PTR(computer);
    BAIL_ON_INVALID_PTR(query);
    BAIL_ON_INVALID_PTR(out_info);

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

    DCERPC_CALL(status, _NetrLogonGetDomainInfo(b, srv, comp, auth, ret_auth,
                                                level, query, &info));

    status = NetrAllocateDomainInfo(&domain_info, &info, level);
    BAIL_ON_NTSTATUS_ERROR(status);

    *out_info = domain_info;

cleanup:
    NetrCleanStubDomainInfo(&info, level);

    if (srv) {
        NetrFreeMemory((void*)srv);
    }

    if (comp) {
        NetrFreeMemory((void*)comp);
    }

    if (auth) {
        NetrFreeMemory((void*)auth);
    }

    return status;

error:
    if (domain_info) {
        NetrFreeMemory((void*)domain_info);
    }

    *out_info = NULL;
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
