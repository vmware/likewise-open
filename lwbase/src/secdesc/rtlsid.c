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
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 */


#include "includes.h"

static
NTSTATUS
RtlSidCopyPartial(
    OUT PSID pSid,
    IN DWORD Size,
    IN PSID pSourceSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t sourceSize = 0;

    BAIL_ON_NULL_PTR_PARAM(pSid);
    BAIL_ON_NULL_PTR_PARAM(pSourceSid);

    sourceSize = RtlLengthSid(pSourceSid);
    memcpy(pSid, pSourceSid, LW_MIN(Size, sourceSize));

cleanup:
    return status;
}

NTSTATUS
RtlSidCopyAlloc(
    OUT PSID* ppDstSid,
    IN PSID pSrcSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwSrcSize = 0;
    PSID pDstSid = NULL;

    BAIL_ON_NULL_PTR_PARAM(ppDstSid);
    BAIL_ON_NULL_PTR_PARAM(pSrcSid);

    dwSrcSize = RtlLengthSid(pSrcSid);
    pDstSid = RtlMemoryAllocate(dwSrcSize);
    BAIL_ON_NULL_PTR(pDstSid);

    memcpy(pDstSid, pSrcSid, dwSrcSize);

    *ppDstSid = pDstSid;

cleanup:
    return status;

error:
    *ppDstSid = NULL;
    goto cleanup;
}

NTSTATUS
RtlSidAllocateResizedCopy(
    OUT PSID* ppSid,
    IN UINT8 SubAuthorityCount,
    IN PSID pSourceSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwSize = 0;
    PSID pSid = NULL;

    dwSize = RtlLengthRequiredSid(SubAuthorityCount);
    pSid = RtlMemoryAllocate(dwSize);
    BAIL_ON_NULL_PTR(pSid);

    if (pSourceSid) {
        RtlSidCopyPartial(pSid, dwSize, pSourceSid);
    }
    pSid->SubAuthorityCount = SubAuthorityCount;

    *ppSid = pSid;

cleanup:
    return status;

error:
    *ppSid = NULL;
    goto cleanup;
}

void
SidFree(
    IN OUT PSID pSid
    )
{
    RtlMemoryFree(pSid);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
