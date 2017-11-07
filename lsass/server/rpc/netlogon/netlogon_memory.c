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
 *        netlogon_memory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Netlogon memory allocation manager
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *          Adam Bernstein (abernstein@vmware.com)
 */

#include "includes.h"


NTSTATUS
NetlogonSrvAllocateMemory(
    void **ppOut,
    DWORD dwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    void *pOut = NULL;

    pOut = rpc_ss_allocate(dwSize);
    BAIL_ON_NO_MEMORY(pOut);

    memset(pOut, 0, dwSize);

    *ppOut = pOut;

cleanup:
    return ntStatus;

error:
    *ppOut = NULL;
    goto cleanup;
}


void
NetlogonSrvFreeMemory(
    void *pPtr
    )
{
    rpc_ss_free(pPtr);
}


NTSTATUS
NetlogonSrvAllocateSidFromWC16String(
    PSID *ppSid,
    PCWSTR pwszSidStr
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pSid = NULL;
    ULONG ulSidSize = 0;
    PSID pSidCopy = NULL;

    ntStatus = RtlAllocateSidFromWC16String(&pSid,
                                          pwszSidStr);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ulSidSize = RtlLengthSid(pSid);
    ntStatus = NetlogonSrvAllocateMemory((void**)&pSidCopy,
                                   ulSidSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlCopySid(ulSidSize, pSidCopy, pSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppSid = pSidCopy;

cleanup:
    if (pSid) {
        RTL_FREE(&pSid);
    }

    return ntStatus;

error:
    if (pSidCopy) {
        NetlogonSrvFreeMemory(pSidCopy);
    }

    *ppSid = NULL;
    goto cleanup;
}


NTSTATUS
NetlogonSrvDuplicateSid(
    PSID *ppSidOut,
    PSID pSidIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pSid = NULL;
    ULONG ulSidSize = 0;

    ulSidSize = RtlLengthSid(pSidIn);
    ntStatus = NetlogonSrvAllocateMemory((void**)&pSid,
                                   ulSidSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlCopySid(ulSidSize, pSid, pSidIn);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppSidOut = pSid;

cleanup:
    return ntStatus;

error:
    if (pSid) {
        NetlogonSrvFreeMemory(pSid);
    }

    *ppSidOut = NULL;
    goto cleanup;
}


NTSTATUS
NetlogonSrvGetFromUnicodeString(
    PWSTR *ppwszOut,
    UNICODE_STRING *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszStr = NULL;

    ntStatus = NetlogonSrvAllocateMemory((void**)&pwszStr,
                                   (pIn->MaximumLength + 1) * sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    wc16sncpy(pwszStr, pIn->Buffer, (pIn->Length / sizeof(WCHAR)));
    *ppwszOut = pwszStr;

cleanup:
    return ntStatus;

error:
    if (pwszStr) {
        NetlogonSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
    goto cleanup;
}


NTSTATUS
NetlogonSrvGetFromUnicodeStringEx(
    PWSTR *ppwszOut,
    UNICODE_STRING *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszStr = NULL;

    ntStatus = NetlogonSrvAllocateMemory((void**)&pwszStr,
                                   (pIn->MaximumLength) * sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    wc16sncpy(pwszStr, pIn->Buffer, (pIn->Length / sizeof(WCHAR)));
    *ppwszOut = pwszStr;

cleanup:
    return ntStatus;

error:
    if (pwszStr) {
        NetlogonSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
    goto cleanup;
}


NTSTATUS
NetlogonSrvInitUnicodeString(
    UNICODE_STRING *pOut,
    PCWSTR pwszIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;

    dwLen  = (pwszIn) ? wc16slen(pwszIn) : 0;
    dwSize = dwLen * sizeof(WCHAR);

    ntStatus = NetlogonSrvAllocateMemory((void**)&(pOut->Buffer),
                                   dwSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memcpy(pOut->Buffer, pwszIn, dwSize);
    pOut->MaximumLength = dwSize;
    pOut->Length        = dwSize;

cleanup:
    return ntStatus;

error:
    if (pOut->Buffer) {
        NetlogonSrvFreeMemory(pOut->Buffer);
    }

    pOut->MaximumLength = 0;
    pOut->Length        = 0;
    goto cleanup;
}


NTSTATUS
NetlogonSrvInitUnicodeStringEx(
    UNICODE_STRING *pOut,
    PCWSTR pwszIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;

    dwLen  = (pwszIn) ? wc16slen(pwszIn) : 0;
    dwSize = (dwLen + 1) * sizeof(WCHAR);

    ntStatus = NetlogonSrvAllocateMemory((void**)&(pOut->Buffer),
                                   dwSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memcpy(pOut->Buffer, pwszIn, dwSize - 1);
    pOut->MaximumLength = dwSize;
    pOut->Length        = dwSize - 1;

cleanup:
    return ntStatus;

error:
    if (pOut->Buffer) {
        NetlogonSrvFreeMemory(pOut->Buffer);
    }

    pOut->MaximumLength = 0;
    pOut->Length        = 0;
    goto cleanup;
}


void
NetlogonSrvFreeUnicodeString(
    UNICODE_STRING *pStr
    )
{
    if (pStr->Buffer)
    {
        NetlogonSrvFreeMemory(pStr->Buffer);
    }
    pStr->Length        = 0;
    pStr->MaximumLength = 0;
}


void
NetlogonSrvFreeUnicodeStringEx(
    UNICODE_STRING *pStr
    )
{
    if (pStr->Buffer)
    {
        NetlogonSrvFreeMemory(pStr->Buffer);
    }
    pStr->Length        = 0;
    pStr->MaximumLength = 0;
}


#if 0
NTSTATUS
NetlogonSrvAllocateSecDescBuffer(
    PNETLOGON_SECURITY_DESCRIPTOR_BUFFER *ppBuffer,
    SECURITY_INFORMATION              SecInfo,
    POCTET_STRING                     pBlob
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNETLOGON_SECURITY_DESCRIPTOR_BUFFER pBuffer = NULL;

    ntStatus = NetlogonSrvAllocateMemory((void**)&pBuffer,
                                     sizeof(*pBuffer));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (pBlob && pBlob->ulNumBytes)
    {
        pBuffer->ulBufferLen = pBlob->ulNumBytes;

        ntStatus = NetlogonSrvAllocateMemory((void**)&pBuffer->pBuffer,
                                         pBlob->ulNumBytes);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = RtlQuerySecurityDescriptorInfo(
                            SecInfo,
                            (PSECURITY_DESCRIPTOR_RELATIVE)pBuffer->pBuffer,
                            &pBuffer->ulBufferLen,
                            (PSECURITY_DESCRIPTOR_RELATIVE)pBlob->pBytes);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    *ppBuffer = pBuffer;

cleanup:
    return ntStatus;

error:
    if (pBuffer)
    {
        if (pBuffer->pBuffer)
        {
            NetlogonSrvFreeMemory(pBuffer->pBuffer);
        }

        NetlogonSrvFreeMemory(pBuffer);
    }

    *ppBuffer = NULL;
    goto cleanup;
}
#endif


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
