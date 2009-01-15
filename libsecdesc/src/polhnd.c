/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
*/

/*
    Linux Policy Handle library
    Copyright (C) Rafal Szczesniak  2008


    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/


#include "includes.h"


NTSTATUS
PolHndCreate(
    PolicyHandle *pH,
    DWORD dwHandleType
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uuid_t uu = {0};

    BAIL_ON_NULL_PTR_PARAM(pH);

    /* Generate time-based uuid (rfc4122) */
    uuid_generate_time(uu);

    memcpy((void*)&pH->guid, (void*)&uu, sizeof(pH->guid));
    pH->handle_type = dwHandleType;

cleanup:
    uuid_clear(uu);

    return status;
}


NTSTATUS
PolHndCopy(
    PolicyHandle *pHout,
    const PolicyHandle *pHin
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_NULL_PTR_PARAM(pHout);
    BAIL_ON_NULL_PTR_PARAM(pHin);

    memcpy((void*)&pHout->guid, (void*)&pHin->guid, sizeof(pHout->guid));
    pHout->handle_type = pHin->handle_type;

cleanup:
    return status;
}


NTSTATUS
PolHndAllocate(
    PolicyHandle **ppH,
    DWORD dwHandleType
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uuid_t uu = {0};
    PolicyHandle *ptr = NULL;

    BAIL_ON_NULL_PTR_PARAM(ppH);

    status = SdAllocateMemory((void**)&ptr, sizeof(PolicyHandle));
    BAIL_ON_NTSTATUS_ERROR(status);

    status = PolHndCreate(ptr, dwHandleType);
    BAIL_ON_NTSTATUS_ERROR(status);

    *ppH = ptr;

cleanup:
    uuid_clear(uu);

    return status;

error:
    if (ptr) {
        SdFreeMemory((void*)ptr);
    }
    ppH = NULL;

    goto cleanup;
}


void
PolHndFree(
    PolicyHandle *pH
    )
{
    if (pH == NULL) return;

    ZERO_STRUCT(*pH);
    SdFreeMemory((void*)pH);
}


BOOL
PolHndIsEqual(
    PolicyHandle *pH1,
    PolicyHandle *pH2
    )
{
    BOOL ret = FALSE;
    uuid_t uu1 = {0};
    uuid_t uu2 = {0};

    if (pH1 == NULL || pH2 == NULL) goto cleanup;

    /* This is obvious situation */
    if (pH1 == pH2) {
        ret = TRUE;
        goto cleanup;
    }

    /* Compare handle type and uuid */
    if (pH1->handle_type == pH2->handle_type) {
        memcpy((void*)&uu1, (void*)&pH1->guid, sizeof(uu1));
        memcpy((void*)&uu2, (void*)&pH2->guid, sizeof(uu2));

        if (uuid_compare(uu1, uu2) == 0) {
            ret = TRUE;
        }
    }

cleanup:
    uuid_clear(uu1);
    uuid_clear(uu2);

    return ret;
}


BOOL
PolHndIsEmpty(
    PolicyHandle *pH
    )
{
    BOOL ret = FALSE;
    uuid_t uu = {0};

    if (pH == NULL) goto cleanup;

    if (pH->handle_type == 0) {
        memcpy((void*)&uu, (void*)&pH->guid, sizeof(uu));

        if (uuid_is_null(uu)) {
            ret = TRUE;
        }
    }

cleanup:
    uuid_clear(uu);

    return ret;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
