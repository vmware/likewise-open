/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        dsr_memeory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        DsSetup memory allocation manager
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
DsrSrvInitMemory(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (!bDsrSrvInitialised && !pDsrSrvMemRoot) {
        pDsrSrvMemRoot = talloc(NULL, 0, NULL);
        BAIL_ON_NO_MEMORY(pDsrSrvMemRoot);
    }

error:
    GLOBAL_DATA_UNLOCK(locked);

    return status;
}


NTSTATUS
DsrSrvDestroyMemory(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (bDsrSrvInitialised && pDsrSrvMemRoot) {
        tfree(pDsrSrvMemRoot);
    }

error:
    GLOBAL_DATA_UNLOCK(locked);

    return status;
}


NTSTATUS
DsrSrvAllocateMemory(
    void **ppOut,
    DWORD dwSize,
    void *pDep
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    void *pOut = NULL;
    void *pParent = NULL;
    int locked = 0;

    pParent = (pDep) ? pDep : pDsrSrvMemRoot;

    GLOBAL_DATA_LOCK(locked);

    pOut = talloc(pParent, dwSize, NULL);
    BAIL_ON_NO_MEMORY(pOut);

    memset(pOut, 0, dwSize);

    *ppOut = pOut;

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    *ppOut = NULL;
    goto cleanup;
}


NTSTATUS
DsrSrvReallocMemory(
    void **ppOut,
    DWORD dwNewSize,
    void *pIn
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    void *pOut = NULL;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    pOut = trealloc(pIn, dwNewSize);
    BAIL_ON_NO_MEMORY(pOut);

    *ppOut = pOut;

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    *ppOut = NULL;
    goto cleanup;
}


NTSTATUS
DsrSrvAddDepMemory(
    void *pIn,
    void *pDep
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    void *pParent = NULL;
    int locked = 0;

    pParent = (pDep) ? pDep : pDsrSrvMemRoot;

    GLOBAL_DATA_LOCK(locked);

    tlink(pParent, pIn);

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    goto cleanup;
}


void
DsrSrvFreeMemory(
    void *pPtr
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    tfree(pPtr);

error:
    GLOBAL_DATA_UNLOCK(locked);
}


NTSTATUS
DsrSrvAllocateSidFromWC16String(
    PSID *ppSid,
    PCWSTR pwszSidStr,
    void *pParent
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID pSid = NULL;
    ULONG ulSidSize = 0;
    PSID pSidCopy = NULL;

    status = RtlAllocateSidFromWC16String(&pSid,
                                          pwszSidStr);
    BAIL_ON_NTSTATUS_ERROR(status);

    ulSidSize = RtlLengthSid(pSid);
    status = DsrSrvAllocateMemory((void**)&pSidCopy,
                                  ulSidSize,
                                  pParent);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = RtlCopySid(ulSidSize, pSidCopy, pSid);
    BAIL_ON_NTSTATUS_ERROR(status);

    *ppSid = pSidCopy;

cleanup:
    if (pSid) {
        RTL_FREE(&pSid);
    }

    return status;

error:
    if (pSidCopy) {
        RTL_FREE(&pSidCopy);
    }

    *ppSid = NULL;
    goto cleanup;
}


NTSTATUS
DsrSrvDuplicateSid(
    PSID *ppSidOut,
    PSID pSidIn,
    void *pParent
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID pSid = NULL;
    ULONG ulSidSize = 0;

    ulSidSize = RtlLengthSid(pSidIn);
    status = DsrSrvAllocateMemory((void**)&pSid,
                                  ulSidSize,
                                  pParent);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = RtlCopySid(ulSidSize, pSid, pSidIn);
    BAIL_ON_NTSTATUS_ERROR(status);

    *ppSidOut = pSid;

cleanup:
    return status;

error:
    if (pSid) {
        RTL_FREE(&pSid);
    }

    *ppSidOut = NULL;
    goto cleanup;
}


NTSTATUS
DsrSrvGetFromUnicodeStringEx(
    PWSTR *ppwszOut,
    UnicodeStringEx *pIn,
    void *pParent
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszStr = NULL;

    status = DsrSrvAllocateMemory((void**)&pwszStr,
                                  (pIn->size) * sizeof(WCHAR),
                                  pParent);
    BAIL_ON_NTSTATUS_ERROR(status);

    wc16sncpy(pwszStr, pIn->string, (pIn->len / sizeof(WCHAR)));
    *ppwszOut = pwszStr;

cleanup:
    return status;

error:
    if (pwszStr) {
        DsrSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
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
