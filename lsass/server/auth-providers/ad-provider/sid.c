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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        sid.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        SID Allocation Wrappers that take care of allocation
 *        and error mapping.
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "adprovider.h"

static
DWORD
LsaSidConversionNtStatusToLsaError(
    IN NTSTATUS Status
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    switch (Status)
    {
        case STATUS_SUCCESS:
            dwError = LSA_ERROR_SUCCESS;
            break;
        case STATUS_INSUFFICIENT_RESOURCES:
        case STATUS_NO_MEMORY:
            dwError = LSA_ERROR_OUT_OF_MEMORY;
            break;
        case STATUS_INVALID_PARAMETER:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            break;
        case STATUS_INVALID_SID:
            dwError = LSA_ERROR_INVALID_SID;
            break;
        default:
            dwError = LSA_ERROR_INTERNAL;
            break;
    }

    return dwError;
}

DWORD
LsaAllocateCStringFromSid(
    OUT PSTR* ppszStringSid,
    IN PSID pSid
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszStringSid = NULL;
    PSTR pszResultStringSid = NULL;

    status = SidToString(pSid, &pwszStringSid);
    dwError = LsaSidConversionNtStatusToLsaError(status);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaWc16sToMbs(pwszStringSid, &pszResultStringSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LSA_ERROR_SUCCESS;

cleanup:
    if (pwszStringSid)
    {
        SidFreeString(pwszStringSid);
    }

    *ppszStringSid = pszResultStringSid;

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszResultStringSid);
    goto cleanup;
}

DWORD
LsaAllocateSidFromCString(
    OUT PSID* ppSid,
    IN PCSTR pszStringSid
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    PSID pSid = NULL;
    PSID pResultSid = NULL;
    ULONG size = 0;

    status = ParseSidString(&pSid, pszStringSid);
    dwError = LsaSidConversionNtStatusToLsaError(status);
    BAIL_ON_LSA_ERROR(dwError);

    size = SidGetSize(pSid);

    dwError = LsaAllocateMemory(size, (PVOID*)&pResultSid);
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pResultSid, pSid, size);

    dwError = LSA_ERROR_SUCCESS;

cleanup:
    if (pSid)
    {
        SidFree(pSid);
    }

    *ppSid = pResultSid;

    return dwError;

error:
    LSA_SAFE_FREE_MEMORY(pResultSid);
    goto cleanup;
}

DWORD
LsaAllocateSidAppendRid(
    OUT PSID* ppSid,
    IN PSID pDomainSid,
    IN ULONG Rid
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSID pResultSid = NULL;
    ULONG size = SidGetRequiredSize(pDomainSid->subauth_count + 1);

    dwError = LsaAllocateMemory(size, (PVOID*)&pResultSid);
    BAIL_ON_LSA_ERROR(dwError);

    SidCopy(pResultSid, pDomainSid);

    pResultSid->subauth[pDomainSid->subauth_count] = Rid;
    pResultSid->subauth_count++;

cleanup:
    *ppSid = pResultSid;

    return dwError;

error:
    LSA_SAFE_FREE_MEMORY(pResultSid);
    goto cleanup;
}

BOOLEAN
LsaIsEqualSid(
    IN PSID Sid1,
    IN PSID Sid2
    )
{
    return ((Sid1->subauth_count == Sid2->subauth_count) &&
            !memcmp(Sid1, Sid2, SidGetSize(Sid1)));
}
