/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
*/

/*
 * Copyright Likewise Software
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
RtlPolHndCreate(
    OUT PolicyHandle* pH,
    IN ULONG HandleType
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uuid_t uu = {0};

    BAIL_ON_NULL_PTR_PARAM(pH);

    /* Generate time-based uuid (rfc4122) */
    uuid_generate_time(uu);

    memcpy(&pH->guid, &uu, sizeof(pH->guid));
    pH->handle_type = HandleType;

cleanup:
    uuid_clear(uu);

    return status;
}


NTSTATUS
RtlPolHndCopy(
    OUT PolicyHandle* pHout,
    IN PolicyHandle* pHin
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_NULL_PTR_PARAM(pHout);
    BAIL_ON_NULL_PTR_PARAM(pHin);

    memcpy(&pHout->guid, &pHin->guid, sizeof(pHout->guid));
    pHout->handle_type = pHin->handle_type;

cleanup:
    return status;
}


NTSTATUS
RtlPolHndAllocate(
    OUT PolicyHandle** ppH,
    IN ULONG HandleType
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uuid_t uu = {0};
    PolicyHandle *ptr = NULL;

    BAIL_ON_NULL_PTR_PARAM(ppH);

    ptr = RtlMemoryAllocate(sizeof(PolicyHandle));
    BAIL_ON_NULL_PTR(ptr);

    status = RtlPolHndCreate(ptr, HandleType);
    BAIL_ON_NTSTATUS_ERROR(status);

    *ppH = ptr;

cleanup:
    uuid_clear(uu);

    return status;

error:
    if (ptr) {
        RtlMemoryFree((void*)ptr);
    }
    ppH = NULL;

    goto cleanup;
}


void
RtlPolHndFree(
    IN OUT PolicyHandle* pH
    )
{
    if (pH == NULL) return;

    ZERO_STRUCT(*pH);
    RtlMemoryFree(pH);
}


BOOLEAN
RtlPolHndIsEqual(
    IN PolicyHandle* pH1,
    IN PolicyHandle* pH2
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
        memcpy(&uu1, &pH1->guid, sizeof(uu1));
        memcpy(&uu2, &pH2->guid, sizeof(uu2));

        if (uuid_compare(uu1, uu2) == 0) {
            ret = TRUE;
        }
    }

cleanup:
    uuid_clear(uu1);
    uuid_clear(uu2);

    return ret;
}


BOOLEAN
RtlPolHndIsEmpty(
    IN PolicyHandle* pH
    )
{
    BOOL ret = FALSE;
    uuid_t uu = {0};

    if (pH == NULL) goto cleanup;

    if (pH->handle_type == 0) {
        memcpy(&uu, &pH->guid, sizeof(uu));

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
