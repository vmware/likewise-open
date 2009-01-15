/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "includes.h"

NTSTATUS
LwIoCreatePlainAccessTokenA(
    PCSTR pszUsername,
    PCSTR pszPassword,
    PIO_ACCESS_TOKEN* ppAccessToken
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR pwszUsername = NULL;
    PWSTR pwszPassword = NULL;
    
    Status = LwRtlWC16StringAllocateFromCString(&pwszUsername, pszUsername);
    BAIL_ON_NT_STATUS(Status);

    Status = LwRtlWC16StringAllocateFromCString(&pwszPassword, pszPassword);
    BAIL_ON_NT_STATUS(Status);

    Status = LwIoCreatePlainAccessTokenW(pwszUsername, pwszPassword, ppAccessToken);
    BAIL_ON_NT_STATUS(Status);

error:
    
    IO_SAFE_FREE_MEMORY(pwszUsername);
    IO_SAFE_FREE_MEMORY(pwszPassword);

    return Status;
}

NTSTATUS
LwIoCreatePlainAccessTokenW(
    PCWSTR pwszUsername,
    PCWSTR pwszPassword,
    PIO_ACCESS_TOKEN* ppAccessToken
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_ACCESS_TOKEN pAccessToken = NULL;

    Status = LwIoAllocateMemory(sizeof(*pAccessToken), OUT_PPVOID(&pAccessToken));
    BAIL_ON_NT_STATUS(Status);

    pAccessToken->type = IO_ACCESS_TOKEN_TYPE_PLAIN;
    
    Status = RtlWC16StringDuplicate(
        &pAccessToken->payload.plain.pwszUsername,
        pwszUsername);
    BAIL_ON_NT_STATUS(Status);

    Status = RtlWC16StringDuplicate(
        &pAccessToken->payload.plain.pwszPassword,
        pwszPassword);
    BAIL_ON_NT_STATUS(Status);
    
    *ppAccessToken = pAccessToken;
    
cleanup:

    return Status;

error:

    if (pAccessToken)
    {
        LwIoDeleteAccessToken(pAccessToken);
    }

    goto cleanup;
}

NTSTATUS
LwIoCreateKrb5AccessTokenA(
    PCSTR pszPrincipal,
    PCSTR pszCachePath,
    PIO_ACCESS_TOKEN* ppAccessToken
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR pwszPrincipal = NULL;
    PWSTR pwszCachePath = NULL;
    
    Status = LwRtlWC16StringAllocateFromCString(&pwszPrincipal, pszPrincipal);
    BAIL_ON_NT_STATUS(Status);

    Status = LwRtlWC16StringAllocateFromCString(&pwszCachePath, pszCachePath);
    BAIL_ON_NT_STATUS(Status);

    Status = LwIoCreatePlainAccessTokenW(pwszPrincipal, pwszCachePath, ppAccessToken);
    BAIL_ON_NT_STATUS(Status);

error:
    
    IO_SAFE_FREE_MEMORY(pwszPrincipal);
    IO_SAFE_FREE_MEMORY(pwszCachePath);

    return Status;
}

NTSTATUS
LwIoCreateKrb5AccessTokenW(
    PCWSTR pwszPrincipal,
    PCWSTR pwszCachePath,
    PIO_ACCESS_TOKEN* ppAccessToken
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_ACCESS_TOKEN pAccessToken = NULL;

    Status = LwIoAllocateMemory(sizeof(*pAccessToken), OUT_PPVOID(&pAccessToken));
    BAIL_ON_NT_STATUS(Status);

    pAccessToken->type = IO_ACCESS_TOKEN_TYPE_PLAIN;

    Status = RtlWC16StringDuplicate(
        &pAccessToken->payload.krb5.pwszPrincipal,
        pwszPrincipal);
    BAIL_ON_NT_STATUS(Status);

    Status = RtlWC16StringDuplicate(
        &pAccessToken->payload.krb5.pwszCachePath,
        pwszCachePath);
    BAIL_ON_NT_STATUS(Status);
    
    *ppAccessToken = pAccessToken;
    
cleanup:

    return Status;

error:

    if (pAccessToken)
    {
        LwIoDeleteAccessToken(pAccessToken);
    }

    goto cleanup;
}


NTSTATUS
LwIoCopyAccessToken(
    PIO_ACCESS_TOKEN pAccessToken,
    PIO_ACCESS_TOKEN* ppAccessTokenCopy
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_ACCESS_TOKEN pAccessTokenCopy = NULL;

    if (pAccessToken)
    {
        Status = LwIoAllocateMemory(sizeof(*pAccessTokenCopy), OUT_PPVOID(&pAccessTokenCopy));
        BAIL_ON_NT_STATUS(Status);

        pAccessTokenCopy->type = pAccessToken->type;

        switch (pAccessToken->type)
        {
        case IO_ACCESS_TOKEN_TYPE_PLAIN:
            Status = RtlWC16StringDuplicate(
                &pAccessTokenCopy->payload.plain.pwszUsername,
                pAccessToken->payload.plain.pwszUsername);
            BAIL_ON_NT_STATUS(Status);
            Status = RtlWC16StringDuplicate(
                &pAccessTokenCopy->payload.plain.pwszPassword,
                pAccessToken->payload.plain.pwszPassword);
            BAIL_ON_NT_STATUS(Status);
            break;
        case IO_ACCESS_TOKEN_TYPE_KRB5:
            Status = RtlWC16StringDuplicate(
                &pAccessTokenCopy->payload.krb5.pwszPrincipal,
                pAccessToken->payload.krb5.pwszPrincipal);
            BAIL_ON_NT_STATUS(Status);
            Status = RtlWC16StringDuplicate(
                &pAccessTokenCopy->payload.krb5.pwszCachePath,
                pAccessToken->payload.krb5.pwszCachePath);
            BAIL_ON_NT_STATUS(Status);
            break;
        }

        *ppAccessTokenCopy = pAccessTokenCopy;
    }
    else
    {
        *ppAccessTokenCopy = NULL;
    }
    
cleanup:

    return Status;

error:

    if (pAccessTokenCopy)
    {
        LwIoDeleteAccessToken(pAccessTokenCopy);
    }

    goto cleanup;
}

VOID
LwIoDeleteAccessToken(
    PIO_ACCESS_TOKEN pAccessToken
    )
{
    if (pAccessToken)
    {
        switch (pAccessToken->type)
        {
        case IO_ACCESS_TOKEN_TYPE_PLAIN:
            IO_SAFE_FREE_MEMORY(pAccessToken->payload.plain.pwszUsername);
            IO_SAFE_FREE_MEMORY(pAccessToken->payload.plain.pwszPassword);
            break;
        case IO_ACCESS_TOKEN_TYPE_KRB5:
            IO_SAFE_FREE_MEMORY(pAccessToken->payload.krb5.pwszPrincipal);
            IO_SAFE_FREE_MEMORY(pAccessToken->payload.krb5.pwszCachePath);
            break;
        }

        LwIoFreeMemory(pAccessToken);
    }
}

BOOLEAN
LwIoCompareAccessTokens(
    PIO_ACCESS_TOKEN pAccessTokenOne,
    PIO_ACCESS_TOKEN pAccessTokenTwo
    )
{
    if (pAccessTokenOne == NULL && pAccessTokenTwo == NULL)
    {
        return TRUE;
    }
    else if (pAccessTokenOne != NULL && pAccessTokenTwo != NULL &&
             pAccessTokenOne->type == pAccessTokenTwo->type)
    {
        switch (pAccessTokenOne->type)
        {
        case IO_ACCESS_TOKEN_TYPE_PLAIN:
            return (!SMBWc16sCmp(pAccessTokenOne->payload.plain.pwszUsername,
                                 pAccessTokenTwo->payload.plain.pwszUsername) &&
                    !SMBWc16sCmp(pAccessTokenOne->payload.plain.pwszPassword,
                                 pAccessTokenTwo->payload.plain.pwszPassword));
        case IO_ACCESS_TOKEN_TYPE_KRB5:
            return (!SMBWc16sCmp(pAccessTokenOne->payload.krb5.pwszPrincipal,
                                 pAccessTokenTwo->payload.krb5.pwszPrincipal) &&
                    !SMBWc16sCmp(pAccessTokenOne->payload.krb5.pwszCachePath,
                                 pAccessTokenTwo->payload.krb5.pwszCachePath));
        }
    }

    return FALSE;
}
