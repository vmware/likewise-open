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


#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif


NTSTATUS
RtlSidInitialize(
    SID *pSid,
    SID_IDENTIFIER_AUTHORITY *pAuthority,
    UINT8 SubAuthCount
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_NULL_PTR_PARAM(pSid);
    BAIL_ON_NULL_PTR_PARAM(pAuthority);

    if (SubAuthCount <= 0)
    {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    /* Revision number */
    pSid->revision = 1;

    /* Authority id */
    memcpy(pSid->authid, pAuthority->Octet, sizeof(pSid->authid));

    /* Subauth count */
    pSid->subauth_count = SubAuthCount;

cleanup:
    return status;
}


BOOL
InitializeSid(
    SID *pSid,
    SID_IDENTIFIER_AUTHORITY *pAuthority,
    UINT8 SubAuthCount
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = RtlSidInitialize(pSid, pAuthority, SubAuthCount);
    return (status == STATUS_SUCCESS);
}


NTSTATUS
RtlSidCopyPartial(
    SID *pSid,
    DWORD Size,
    const SID *pSourceSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t sourceSize = 0;

    BAIL_ON_NULL_PTR_PARAM(pSid);
    BAIL_ON_NULL_PTR_PARAM(pSourceSid);

    sourceSize = SidGetSize(pSourceSid);
    memcpy(pSid, pSourceSid, MIN(Size, sourceSize));

cleanup:
    return status;
}


NTSTATUS
RtlSidCopyAlloc(
    PSID *ppDstSid,
    const SID *pSrcSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwSrcSize = 0;
    SID *pDstSid = NULL;

    BAIL_ON_NULL_PTR_PARAM(ppDstSid);
    BAIL_ON_NULL_PTR_PARAM(pSrcSid);

    dwSrcSize = SidGetSize(pSrcSid);
    pDstSid = RtlMemoryAllocate((size_t)dwSrcSize);
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
RtlSidAllocate(
    PSID* ppSid,
    UINT8 SubAuthorityCount
    )
{
    return RtlSidAllocateResizedCopy(ppSid, SubAuthorityCount, NULL);
}


NTSTATUS
RtlSidAllocateResizedCopy(
    PSID* ppSid,
    UINT8 SubAuthorityCount,
    const SID *pSourceSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwSize = 0;
    PSID pSid = NULL;

    dwSize = SidGetRequiredSize(SubAuthorityCount);
    pSid = RtlMemoryAllocate((size_t)dwSize);
    BAIL_ON_NULL_PTR(pSid);

    if (pSourceSid) {
        RtlSidCopyPartial(pSid, dwSize, pSourceSid);
    }
    pSid->subauth_count = SubAuthorityCount;

    *ppSid = pSid;

cleanup:
    return status;

error:
    *ppSid = NULL;
    goto cleanup;
}


NTSTATUS
RtlSidAllocateAndInitialize(
    PSID* ppSid,
    SID_IDENTIFIER_AUTHORITY Authority,
    UINT8 SubAuthorityCount,
    ...
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SID *pSid = NULL;
    DWORD dwSubAuthId, i;
    va_list ap;

    status = RtlSidAllocateResizedCopy(&pSid, SubAuthorityCount, NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = RtlSidInitialize(pSid, &Authority, SubAuthorityCount);
    BAIL_ON_NTSTATUS_ERROR(status);

    va_start(ap, SubAuthorityCount);

    for (i = 0; i < SubAuthorityCount; i++) {
        dwSubAuthId = va_arg(ap, DWORD);
        pSid->subauth[i] = dwSubAuthId;
    }

    va_end(ap);

    *ppSid = pSid;

cleanup:
    return status;

error:
    SidFree(pSid);
    ppSid = NULL;

    goto cleanup;
}


NTSTATUS
RtlSidAppendRid(
    PSID *ppDstSid,
    DWORD dwRid,
    const SID *pSrcSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SID *pSid = NULL;
    UINT8 SubAuthCount = 0;

    BAIL_ON_NULL_PTR_PARAM(ppDstSid);
    BAIL_ON_NULL_PTR_PARAM(pSrcSid);

    SubAuthCount = SidGetSubAuthorityCount(pSrcSid) + 1;
    if (SubAuthCount > MAXIMUM_SUBAUTHORITY_COUNT) {
        status = STATUS_INVALID_SID;
        goto error;
    }

    status = RtlSidAllocateResizedCopy(&pSid, SubAuthCount, pSrcSid);
    BAIL_ON_NTSTATUS_ERROR(status);

    pSid->subauth[SubAuthCount - 1] = dwRid;

    *ppDstSid = pSid;

cleanup:
    return status;

error:
    if (pSid) {
        SidFree(pSid);
    }

    *ppDstSid = NULL;

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
