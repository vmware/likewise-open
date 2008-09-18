/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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


NTSTATUS LsaLookupSids(handle_t b, PolicyHandle *handle, SidArray *sids,
                       RefDomainList **domains, TranslatedName **names,
                       uint16 level, uint32 *count)
{
    NTSTATUS status = STATUS_SUCCESS;
    TranslatedNameArray name_array = {0};
    RefDomainList *ref_domains = NULL;
    TranslatedName *names_out = NULL;

    goto_if_invalid_param_ntstatus(b, cleanup);
    goto_if_invalid_param_ntstatus(handle, cleanup);
    goto_if_invalid_param_ntstatus(sids, cleanup);
    goto_if_invalid_param_ntstatus(domains, cleanup);
    goto_if_invalid_param_ntstatus(names, cleanup);
    goto_if_invalid_param_ntstatus(count, cleanup);

    /* windows allows level to be in range 1-6 */

    *count = 0;

    TRY
    {
        status = _LsaLookupSids(b, handle, sids, &ref_domains,
                                &name_array, level, count);
    }
    CATCH_ALL
    {
        status = STATUS_UNHANDLED_EXCEPTION;
    }
    ENDTRY;

    goto_if_ntstatus_not_success(status, cleanup);

    status = LsaAllocateTranslatedNames(&names_out, &name_array);
    goto_if_ntstatus_not_success(status, cleanup);

    status = LsaAllocateRefDomainList(domains, ref_domains);
    goto_if_ntstatus_not_success(status, cleanup);

    *names = names_out;
    names_out = NULL;

cleanup:
    if (names_out) {
        LsaRpcFreeMemory((void*)names_out);
    }

    /* Free pointers returned from stub */
    if (ref_domains) {
        LsaFreeStubRefDomainList(ref_domains);
    }

    LsaCleanStubTranslatedNameArray(&name_array);


    return status;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
