/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        common.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Join to Active Directory
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "lsakrb.h"
#include <lwrdr/lwrdr.h>

typedef struct _LSA_ACCESS_TOKEN_FREE_INFO
{
    krb5_context ctx;
    krb5_ccache cc;
    HANDLE hAccessToken;
} LSA_ACCESS_TOKEN_FREE_INFO;

DWORD
LsaSetSMBAccessToken(
    IN PCSTR pszDomain,
    IN PCSTR pszUsername,
    IN PCSTR pszPassword,
    IN BOOLEAN bSetDefaultCachePath,
    OUT PLSA_ACCESS_TOKEN_FREE_INFO* ppFreeInfo
    )
{
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    PCSTR pszNewCachePath = NULL;
    krb5_context ctx = 0;
    krb5_ccache cc = 0;
    HANDLE hAccessToken = 0;
    PLSA_ACCESS_TOKEN_FREE_INFO pFreeInfo = NULL;

    BAIL_ON_INVALID_POINTER(ppFreeInfo);
    BAIL_ON_INVALID_STRING(pszDomain);
    BAIL_ON_INVALID_STRING(pszUsername);

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* Generates a new filed based credentials cache in /tmp. The file will
     * be owned by root and only accessible by root.
     */
    ret = krb5_cc_new_unique(
            ctx,
            "FILE",
            "hint",
            &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    pszNewCachePath = krb5_cc_get_name(ctx, cc);

    if (bSetDefaultCachePath)
    {
        dwError = LsaKrb5SetDefaultCachePath(
                  pszNewCachePath,
                  NULL);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaKrb5GetTgt(
                pszUsername,
                pszPassword,
                pszNewCachePath,
                NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SMBCreateKrb5AccessTokenA(
        pszUsername,
        pszNewCachePath,
        &hAccessToken);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SMBSetThreadToken(hAccessToken);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(sizeof(*pFreeInfo), (PVOID*)&pFreeInfo);
    BAIL_ON_LSA_ERROR(dwError);

    pFreeInfo->ctx = ctx;
    pFreeInfo->cc = cc;
    pFreeInfo->hAccessToken = hAccessToken;

cleanup:
    *ppFreeInfo = pFreeInfo;

    return dwError;

error:

    if (hAccessToken != NULL)
    {
        SMBCloseHandle(NULL, hAccessToken);
    }

    if (ctx != NULL)
    {
        if (cc != NULL)
        {
            krb5_cc_destroy(ctx, cc);
        }
        krb5_free_context(ctx);
    }

    if (pFreeInfo)
    {
        LsaFreeMemory(pFreeInfo);
        pFreeInfo = NULL;
    }

    goto cleanup;
}

void
LsaFreeSMBAccessToken(
    IN OUT PLSA_ACCESS_TOKEN_FREE_INFO* ppFreeInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    HANDLE hThreadToken = NULL;
    BOOL bClearThreadToken = FALSE;
    PLSA_ACCESS_TOKEN_FREE_INFO pFreeInfo = *ppFreeInfo;

    if (!pFreeInfo)
    {
        goto cleanup;
    }

    if (pFreeInfo->hAccessToken != NULL)
    {
        dwError = SMBGetThreadToken(&hThreadToken);

        if (dwError == LSA_ERROR_SUCCESS)
        {
            SMBCompareHandles(
                hThreadToken,
                &pFreeInfo->hAccessToken,
                &bClearThreadToken);

            SMBCloseHandle(NULL, hThreadToken);
        }

        if (bClearThreadToken)
        {
            SMBSetThreadToken(NULL);
        }

        SMBCloseHandle(NULL, pFreeInfo->hAccessToken);
    }

    if (pFreeInfo->ctx != NULL)
    {
        if (pFreeInfo->cc != NULL)
        {
            krb5_cc_destroy(pFreeInfo->ctx, pFreeInfo->cc);
        }
        krb5_free_context(pFreeInfo->ctx);
    }

    LsaFreeMemory(pFreeInfo);

    *ppFreeInfo = NULL;

cleanup:
    return;
}
