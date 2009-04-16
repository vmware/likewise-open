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
 *        lsa_memeory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Lsa memory allocation manager
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvInitMemory(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (!bLsaSrvInitialised && !pLsaSrvMemRoot) {
        pLsaSrvMemRoot = talloc(NULL, 0, NULL);
        BAIL_ON_NO_MEMORY(pLsaSrvMemRoot);
    }

error:
    GLOBAL_DATA_UNLOCK(locked);

    return status;
}


NTSTATUS
LsaSrvDestroyMemory(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (bLsaSrvInitialised && pLsaSrvMemRoot) {
        tfree(pLsaSrvMemRoot);
    }

error:
    GLOBAL_DATA_UNLOCK(locked);

    return status;
}


NTSTATUS
LsaSrvAllocateMemory(
    void **ppOut,
    DWORD dwSize,
    void *pDep
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    void *pOut = NULL;
    void *pParent = NULL;
    int locked = 0;

    pParent = (pDep) ? pDep : pLsaSrvMemRoot;

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
LsaSrvReallocMemory(
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
LsaSrvAddDepMemory(
    void *pIn,
    void *pDep
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    void *pParent = NULL;
    int locked = 0;

    pParent = (pDep) ? pDep : pLsaSrvMemRoot;

    GLOBAL_DATA_LOCK(locked);

    tlink(pParent, pIn);

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    goto cleanup;
}


void
LsaSrvFreeMemory(
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
LsaSrvAllocateSidFromWC16String(
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
    status = LsaSrvAllocateMemory((void**)&pSidCopy,
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
LsaSrvDuplicateSid(
    PSID *ppSidOut,
    PSID pSidIn,
    void *pParent
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID pSid = NULL;
    ULONG ulSidSize = 0;

    ulSidSize = RtlLengthSid(pSidIn);
    status = LsaSrvAllocateMemory((void**)&pSid,
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
LsaSrvGetFromUnicodeString(
    PWSTR *ppwszOut,
    UnicodeString *pIn,
    void *pParent
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszStr = NULL;

    status = LsaSrvAllocateMemory((void**)&pwszStr,
                                  (pIn->size + 1) * sizeof(WCHAR),
                                  pParent);
    BAIL_ON_NTSTATUS_ERROR(status);

    wc16sncpy(pwszStr, pIn->string, (pIn->len / sizeof(WCHAR)));
    *ppwszOut = pwszStr;

cleanup:
    return status;

error:
    if (pwszStr) {
        LsaSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
    goto cleanup;
}


NTSTATUS
LsaSrvGetFromUnicodeStringEx(
    PWSTR *ppwszOut,
    UnicodeStringEx *pIn,
    void *pParent
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszStr = NULL;

    status = LsaSrvAllocateMemory((void**)&pwszStr,
                                   (pIn->size) * sizeof(WCHAR),
                                   pParent);
    BAIL_ON_NTSTATUS_ERROR(status);

    wc16sncpy(pwszStr, pIn->string, (pIn->len / sizeof(WCHAR)));
    *ppwszOut = pwszStr;

cleanup:
    return status;

error:
    if (pwszStr) {
        LsaSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
    goto cleanup;
}


NTSTATUS
LsaSrvInitUnicodeString(
    UnicodeString *pOut,
    PCWSTR pwszIn,
    void *pParent
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;

    dwLen  = (pwszIn) ? wc16slen(pwszIn) : 0;
    dwSize = dwLen * sizeof(WCHAR);

    status = LsaSrvAllocateMemory((void**)&(pOut->string),
                                  dwSize,
                                  pParent);
    BAIL_ON_NTSTATUS_ERROR(status);

    memcpy(pOut->string, pwszIn, dwSize);
    pOut->size = dwSize;
    pOut->len  = dwSize;

cleanup:
    return status;

error:
    if (pOut->string) {
        LsaSrvFreeMemory(pOut->string);
    }

    pOut->size = 0;
    pOut->len  = 0;
    goto cleanup;
}


NTSTATUS
LsaSrvInitUnicodeStringEx(
    UnicodeStringEx *pOut,
    PCWSTR pwszIn,
    void *pParent
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;

    dwLen  = (pwszIn) ? wc16slen(pwszIn) : 0;
    dwSize = (dwLen + 1) * sizeof(WCHAR);

    status = LsaSrvAllocateMemory((void**)&(pOut->string),
                                  dwSize,
                                  pParent);
    BAIL_ON_NTSTATUS_ERROR(status);

    memcpy(pOut->string, pwszIn, dwSize - 1);
    pOut->size = dwSize;
    pOut->len  = dwSize - 1;

cleanup:
    return status;

error:
    if (pOut->string) {
        LsaSrvFreeMemory(pOut->string);
    }

    pOut->size = 0;
    pOut->len  = 0;
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
