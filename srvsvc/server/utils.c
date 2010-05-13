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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 * Server Service Utilities
 *
 */

#include "includes.h"


DWORD
SrvSvcSrvAllocateWC16StringFromUnicodeString(
    OUT PWSTR          *ppwszOut,
    IN  PUNICODE_STRING pIn
    )
{
    WINERROR dwError = 0;
    PWSTR pwszStr = NULL;

    BAIL_ON_INVALID_PTR(ppwszOut, dwError);
    BAIL_ON_INVALID_PTR(pIn, dwError);

    dwError = SrvSvcSrvAllocateMemory(pIn->MaximumLength + sizeof(WCHAR),
                                      OUT_PPVOID(&pwszStr));
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwWc16snCpy(pwszStr,
                          pIn->Buffer,
                          pIn->Length / sizeof(WCHAR));
    BAIL_ON_SRVSVC_ERROR(dwError);

    *ppwszOut = pwszStr;

cleanup:
    return dwError;

error:
    if (pwszStr)
    {
        SrvSvcSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
    goto cleanup;
}


DWORD
SrvSvcSrvAllocateWC16String(
    OUT PWSTR  *ppwszOut,
    IN  PCWSTR  pwszIn
    )
{
    DWORD dwError = ERROR_SUCCESS;
    size_t sStrLen = 0;
    PWSTR pwszStr = NULL;

    BAIL_ON_INVALID_PTR(ppwszOut, dwError);
    BAIL_ON_INVALID_PTR(pwszIn, dwError);

    dwError = LwWc16sLen(pwszIn, &sStrLen);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcSrvAllocateMemory(sizeof(WCHAR) * (sStrLen + 1),
                                      OUT_PPVOID(&pwszStr));
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwWc16snCpy(pwszStr, pwszIn, sStrLen);
    BAIL_ON_SRVSVC_ERROR(dwError);

    *ppwszOut = pwszStr;

cleanup:
    return dwError;

error:
    if (pwszStr)
    {
        SrvSvcSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;

    goto cleanup;
}


DWORD
SrvSvcSrvAllocateWC16StringFromCString(
    OUT PWSTR  *ppwszOut,
    IN  PCSTR   pszIn
    )
{
    DWORD dwError = ERROR_SUCCESS;
    size_t sStrLen = 0;
    PWSTR pwszIn = NULL;
    PWSTR pwszStr = NULL;

    BAIL_ON_INVALID_PTR(ppwszOut, dwError);
    BAIL_ON_INVALID_PTR(pszIn, dwError);

    dwError = LwMbsToWc16s(pszIn, &pwszIn);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwWc16sLen(pwszIn, &sStrLen);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcSrvAllocateMemory(sizeof(WCHAR) * (sStrLen + 1),
                                      OUT_PPVOID(&pwszStr));
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwWc16snCpy(pwszStr, pwszIn, sStrLen);
    BAIL_ON_SRVSVC_ERROR(dwError);

    *ppwszOut = pwszStr;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszIn);

    return dwError;

error:
    if (pwszStr)
    {
        SrvSvcSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;

    goto cleanup;
}


DWORD
SrvSvcSrvCopyShareInfo0(
    IN OUT PSHARE_INFO_0 pOutShareInfo,
    IN     PSHARE_INFO_0 pInShareInfo
    )
{
    DWORD dwError = ERROR_SUCCESS;

    dwError = SrvSvcSrvAllocateWC16String(
                  &pOutShareInfo->shi0_netname,
                  pInShareInfo->shi0_netname);
    BAIL_ON_SRVSVC_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
SrvSvcSrvCopyShareInfo1(
    IN OUT PSHARE_INFO_1 pOutShareInfo,
    IN     PSHARE_INFO_1 pInShareInfo
    )
{
    DWORD dwError = ERROR_SUCCESS;

    dwError = SrvSvcSrvAllocateWC16String(
                  &pOutShareInfo->shi1_netname,
                  pInShareInfo->shi1_netname);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcSrvAllocateWC16String(
                  &pOutShareInfo->shi1_remark,
                  pInShareInfo->shi1_remark);
    BAIL_ON_SRVSVC_ERROR(dwError);

    pOutShareInfo->shi1_type = pInShareInfo->shi1_type;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
SrvSvcSrvCopyShareInfo2(
    IN OUT PSHARE_INFO_2 pOutShareInfo,
    IN     PSHARE_INFO_2 pInShareInfo
    )
{
    DWORD dwError = ERROR_SUCCESS;

    dwError = SrvSvcSrvAllocateWC16String(
                  &pOutShareInfo->shi2_netname,
                  pInShareInfo->shi2_netname);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcSrvAllocateWC16String(
                  &pOutShareInfo->shi2_remark,
                  pInShareInfo->shi2_remark);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcSrvAllocateWC16String(
                  &pOutShareInfo->shi2_path,
                  pInShareInfo->shi2_path);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcSrvAllocateWC16String(
                  &pOutShareInfo->shi2_password,
                  pInShareInfo->shi2_password);
    BAIL_ON_SRVSVC_ERROR(dwError);

    pOutShareInfo->shi2_type         = pInShareInfo->shi2_type;
    pOutShareInfo->shi2_permissions  = pInShareInfo->shi2_permissions;
    pOutShareInfo->shi2_max_uses     = pInShareInfo->shi2_max_uses;
    pOutShareInfo->shi2_current_uses = pInShareInfo->shi2_current_uses;

cleanup:

    return dwError;

error:

    goto cleanup;
}


DWORD
SrvSvcSrvCopyShareInfo501(
    IN OUT PSHARE_INFO_501 pOutShareInfo,
    IN     PSHARE_INFO_501 pInShareInfo
    )
{
    DWORD dwError = ERROR_SUCCESS;

    dwError = SrvSvcSrvAllocateWC16String(
                  &pOutShareInfo->shi501_netname,
                  pInShareInfo->shi501_netname);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcSrvAllocateWC16String(
                  &pOutShareInfo->shi501_remark,
                  pInShareInfo->shi501_remark);
    BAIL_ON_SRVSVC_ERROR(dwError);

    pOutShareInfo->shi501_type  = pInShareInfo->shi501_type;
    pOutShareInfo->shi501_flags = pInShareInfo->shi501_flags;

cleanup:

    return dwError;

error:

    goto cleanup;
}


DWORD
SrvSvcSrvCopyShareInfo502(
    IN OUT PSHARE_INFO_502 pOutShareInfo,
    IN     PSHARE_INFO_502 pInShareInfo
    )
{
    DWORD dwError = ERROR_SUCCESS;

    dwError = SrvSvcSrvAllocateWC16String(
                  &pOutShareInfo->shi502_netname,
                  pInShareInfo->shi502_netname);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcSrvAllocateWC16String(
                  &pOutShareInfo->shi502_remark,
                  pInShareInfo->shi502_remark);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcSrvAllocateWC16String(
                  &pOutShareInfo->shi502_path,
                  pInShareInfo->shi502_path);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcSrvAllocateWC16String(
                  &pOutShareInfo->shi502_password,
                  pInShareInfo->shi502_password);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcSrvAllocateMemory(
                  pInShareInfo->shi502_reserved,
                  (PVOID*)&pOutShareInfo->shi502_security_descriptor);
    BAIL_ON_SRVSVC_ERROR(dwError);

    memcpy(pOutShareInfo->shi502_security_descriptor,
           pInShareInfo->shi502_security_descriptor,
           pInShareInfo->shi502_reserved);

    pOutShareInfo->shi502_type         = pInShareInfo->shi502_type;
    pOutShareInfo->shi502_permissions  = pInShareInfo->shi502_permissions;
    pOutShareInfo->shi502_max_uses     = pInShareInfo->shi502_max_uses;
    pOutShareInfo->shi502_current_uses = pInShareInfo->shi502_current_uses;

cleanup:

    return dwError;

error:

    goto cleanup;
}


DWORD
SrvSvcSrvCopyShareInfo1005(
    IN OUT PSHARE_INFO_1005 pOutShareInfo,
    IN     PSHARE_INFO_1005 pInShareInfo
    )
{
    DWORD dwError = ERROR_SUCCESS;

    pOutShareInfo->shi1005_flags = pInShareInfo->shi1005_flags;

cleanup:

    return dwError;

error:

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
