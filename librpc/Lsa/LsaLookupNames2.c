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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


NTSTATUS
LsaLookupNames2(
    handle_t b,
    PolicyHandle *handle,
    uint32 num_names,
    wchar16_t *names[],
    RefDomainList **domains,
    TranslatedSid2** sids,
    uint16 level,
    uint32 *count
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS ret_status = STATUS_SUCCESS;
    uint32 unknown1 = 0;
    uint32 unknown2 = 0;
    UnicodeStringEx *lsa_names = NULL;
    RefDomainList *ref_domains = NULL;
    RefDomainList *out_domains = NULL;
    TranslatedSidArray2 sid_array = {0};
    TranslatedSid2* out_sids = NULL;
	
    BAIL_ON_INVALID_PTR(b);
    BAIL_ON_INVALID_PTR(handle);
    BAIL_ON_INVALID_PTR(names);
    BAIL_ON_INVALID_PTR(domains);
    BAIL_ON_INVALID_PTR(sids);
    BAIL_ON_INVALID_PTR(count);

    lsa_names = InitUnicodeStringExArray(names, num_names);
    BAIL_ON_NO_MEMORY(lsa_names);

    *count = 0;

    DCERPC_CALL(_LsaLookupNames2(b, handle, num_names, lsa_names, &ref_domains,
                                 &sid_array, level, count, unknown1, unknown2));
    ret_status = status;

    /* Status other than success doesn't have to mean failure here */
    if (ret_status != STATUS_SUCCESS &&
        ret_status != STATUS_SOME_UNMAPPED) goto error;

    status = LsaAllocateTranslatedSids2(&out_sids, &sid_array);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = LsaAllocateRefDomainList(&out_domains, ref_domains);
    BAIL_ON_NTSTATUS_ERROR(status);
    
    *sids    = out_sids;
    *domains = out_domains;

cleanup:
    FreeUnicodeStringExArray(lsa_names, num_names);

    /* Free pointers returned from stub */
    LsaCleanStubTranslatedSidArray2(&sid_array);

    if (ref_domains) {
        LsaFreeStubRefDomainList(ref_domains);
    }

    if (status == STATUS_SUCCESS &&
        (ret_status == STATUS_SUCCESS ||
         ret_status == STATUS_SOME_UNMAPPED)) {
        status = ret_status;
    }

    return status;

error:
    if (out_sids) {
        LsaRpcFreeMemory((void*)out_sids);
    }

    if (out_domains) {
        LsaRpcFreeMemory((void*)out_domains);
    }

    *sids    = NULL;
    *domains = NULL;

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
