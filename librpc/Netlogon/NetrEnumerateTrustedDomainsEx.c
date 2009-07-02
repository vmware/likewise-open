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

#include "includes.h"


NTSTATUS
NetrEnumerateTrustedDomainsEx(
    handle_t b,
    const wchar16_t *server_name,
    NetrDomainTrust **trusts,
    uint32 *count
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t *name = NULL;
    NetrDomainTrustList tlist = {0};
    NetrDomainTrust *t = NULL;

    BAIL_ON_INVALID_PTR(b);
    BAIL_ON_INVALID_PTR(server_name);
    BAIL_ON_INVALID_PTR(trusts);
    BAIL_ON_INVALID_PTR(count);

    name = wc16sdup(server_name);
    BAIL_ON_NO_MEMORY(name);

    DCERPC_CALL(status, _NetrEnumerateTrustedDomainsEx(b, name, &tlist));
    BAIL_ON_NTSTATUS_ERROR(status);

    *count  = tlist.count;

    status = NetrAllocateDomainTrusts(&t, &tlist);
    BAIL_ON_NTSTATUS_ERROR(status);

    *trusts = t;

cleanup:
    SAFE_FREE(name);
    NetrCleanStubDomainTrustList(&tlist);

    return status;

error:
    if (t) {
        NetrFreeMemory((void*)t);
    }

    *trusts = NULL;
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
