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

static
NTSTATUS
LwIoCredentialCacheToTgt(
    PIO_ACCESS_TOKEN pCacheToken,
    PIO_ACCESS_TOKEN* ppAccessToken
    );

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

    Status = LwIoCreateKrb5AccessTokenW(pwszPrincipal, pwszCachePath, ppAccessToken);
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

    pAccessToken->type = IO_ACCESS_TOKEN_TYPE_KRB5_CCACHE;
    
    Status = RtlWC16StringDuplicate(
        &pAccessToken->payload.krb5Ccache.pwszPrincipal,
        pwszPrincipal);
    BAIL_ON_NT_STATUS(Status);

    Status = RtlWC16StringDuplicate(
        &pAccessToken->payload.krb5Ccache.pwszCachePath,
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
        case IO_ACCESS_TOKEN_TYPE_KRB5_CCACHE:
            Status = RtlWC16StringDuplicate(
                &pAccessTokenCopy->payload.krb5Ccache.pwszPrincipal,
                pAccessToken->payload.krb5Ccache.pwszPrincipal);
            BAIL_ON_NT_STATUS(Status);
            Status = RtlWC16StringDuplicate(
                &pAccessTokenCopy->payload.krb5Ccache.pwszCachePath,
                pAccessToken->payload.krb5Ccache.pwszCachePath);
            BAIL_ON_NT_STATUS(Status);
            break;
        case IO_ACCESS_TOKEN_TYPE_KRB5_TGT:
            Status = RtlWC16StringDuplicate(
                &pAccessTokenCopy->payload.krb5Tgt.pwszClientPrincipal,
                pAccessToken->payload.krb5Tgt.pwszClientPrincipal);
            BAIL_ON_NT_STATUS(Status);
            Status = RtlWC16StringDuplicate(
                &pAccessTokenCopy->payload.krb5Tgt.pwszServerPrincipal,
                pAccessToken->payload.krb5Tgt.pwszServerPrincipal);
            BAIL_ON_NT_STATUS(Status);
            pAccessTokenCopy->payload.krb5Tgt.authTime = pAccessToken->payload.krb5Tgt.authTime;
            pAccessTokenCopy->payload.krb5Tgt.startTime = pAccessToken->payload.krb5Tgt.startTime;
            pAccessTokenCopy->payload.krb5Tgt.endTime = pAccessToken->payload.krb5Tgt.endTime;
            pAccessTokenCopy->payload.krb5Tgt.renewTillTime = pAccessToken->payload.krb5Tgt.renewTillTime;
            pAccessTokenCopy->payload.krb5Tgt.ulKeySize = pAccessToken->payload.krb5Tgt.ulKeySize;
            Status = LwIoAllocateMemory(
                pAccessToken->payload.krb5Tgt.ulKeySize,
                OUT_PPVOID(&pAccessTokenCopy->payload.krb5Tgt.pKeyData));
            BAIL_ON_NT_STATUS(Status);
            memcpy(
                pAccessTokenCopy->payload.krb5Tgt.pKeyData,
                pAccessToken->payload.krb5Tgt.pKeyData,
                pAccessToken->payload.krb5Tgt.ulKeySize);
            pAccessTokenCopy->payload.krb5Tgt.tgtFlags = pAccessToken->payload.krb5Tgt.tgtFlags;
            pAccessTokenCopy->payload.krb5Tgt.ulTgtSize = pAccessToken->payload.krb5Tgt.ulTgtSize;
            Status = LwIoAllocateMemory(
                pAccessToken->payload.krb5Tgt.ulTgtSize,
                OUT_PPVOID(&pAccessTokenCopy->payload.krb5Tgt.pTgtData));
            BAIL_ON_NT_STATUS(Status);
            memcpy(
                pAccessTokenCopy->payload.krb5Tgt.pTgtData,
                pAccessToken->payload.krb5Tgt.pTgtData,
                pAccessToken->payload.krb5Tgt.ulTgtSize);
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
        case IO_ACCESS_TOKEN_TYPE_KRB5_CCACHE:
            IO_SAFE_FREE_MEMORY(pAccessToken->payload.krb5Ccache.pwszPrincipal);
            IO_SAFE_FREE_MEMORY(pAccessToken->payload.krb5Ccache.pwszCachePath);
            break;
        case IO_ACCESS_TOKEN_TYPE_KRB5_TGT:
            IO_SAFE_FREE_MEMORY(pAccessToken->payload.krb5Tgt.pwszClientPrincipal);
            IO_SAFE_FREE_MEMORY(pAccessToken->payload.krb5Tgt.pwszServerPrincipal);
            IO_SAFE_FREE_MEMORY(pAccessToken->payload.krb5Tgt.pKeyData);
            IO_SAFE_FREE_MEMORY(pAccessToken->payload.krb5Tgt.pTgtData);
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
        case IO_ACCESS_TOKEN_TYPE_KRB5_CCACHE:
            return (!SMBWc16sCmp(pAccessTokenOne->payload.krb5Ccache.pwszPrincipal,
                                 pAccessTokenTwo->payload.krb5Ccache.pwszPrincipal) &&
                    !SMBWc16sCmp(pAccessTokenOne->payload.krb5Ccache.pwszCachePath,
                                 pAccessTokenTwo->payload.krb5Ccache.pwszCachePath));
        case IO_ACCESS_TOKEN_TYPE_KRB5_TGT:
            return (!SMBWc16sCmp(pAccessTokenOne->payload.krb5Tgt.pwszClientPrincipal,
                                 pAccessTokenTwo->payload.krb5Tgt.pwszClientPrincipal) &&
                    !SMBWc16sCmp(pAccessTokenOne->payload.krb5Tgt.pwszServerPrincipal,
                                 pAccessTokenTwo->payload.krb5Tgt.pwszServerPrincipal) &&
                    (pAccessTokenOne->payload.krb5Tgt.ulTgtSize ==
                     pAccessTokenTwo->payload.krb5Tgt.ulTgtSize) &&
                    memcpy(pAccessTokenOne->payload.krb5Tgt.pTgtData,
                           pAccessTokenTwo->payload.krb5Tgt.pTgtData,
                           pAccessTokenOne->payload.krb5Tgt.ulTgtSize) == 0);
        }
    }

    return FALSE;
}

NTSTATUS
LwIoResolveAccessToken(
    PIO_ACCESS_TOKEN pBaseToken,
    PIO_ACCESS_TOKEN* ppResolvedToken
    )
{
    if (pBaseToken)
    {
        switch (pBaseToken->type)
        {
        case IO_ACCESS_TOKEN_TYPE_KRB5_TGT:
        case IO_ACCESS_TOKEN_TYPE_PLAIN:
            return LwIoCopyAccessToken(pBaseToken, ppResolvedToken);
        case IO_ACCESS_TOKEN_TYPE_KRB5_CCACHE:
            return LwIoCredentialCacheToTgt(pBaseToken, ppResolvedToken);
        }
    }

    *ppResolvedToken = NULL;
    return STATUS_SUCCESS;
}

static
NTSTATUS
LwIoCredentialCacheToTgt(
    PIO_ACCESS_TOKEN pCacheToken,
    PIO_ACCESS_TOKEN* ppAccessToken
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    krb5_context pContext = NULL;
    krb5_error_code krb5Error = 0;
    krb5_ccache pCache = NULL;
    PSTR pszClientPrincipalName = NULL;
    PSTR pszServerPrincipalName = NULL;
    PSTR pszDesiredPrincipal = NULL;
    PSTR pszCredCachePath = NULL;
    PIO_ACCESS_TOKEN pAccessToken = NULL;
    BOOLEAN bFoundTgt = FALSE;
    BOOLEAN bStartSeq = FALSE;
    krb5_creds creds;
    krb5_cc_cursor cursor;

    Status = LwRtlCStringAllocateFromWC16String(
        &pszDesiredPrincipal,
        pCacheToken->payload.krb5Ccache.pwszPrincipal);
    BAIL_ON_NT_STATUS(Status);

    Status = LwRtlCStringAllocateFromWC16String(
        &pszCredCachePath,
        pCacheToken->payload.krb5Ccache.pwszCachePath);
    BAIL_ON_NT_STATUS(Status);

    /* Open credentials cache */
    krb5Error = krb5_init_context(&pContext);
    if (krb5Error)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        BAIL_ON_NT_STATUS(Status);
    }

    krb5Error = krb5_cc_resolve(pContext, pszCredCachePath, &pCache);
    if (krb5Error)
    {
        Status = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(Status);
    }

    /* Look for a TGT */
    krb5Error = krb5_cc_start_seq_get(pContext, pCache, &cursor);
    if (krb5Error)
    {
        Status = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(Status);
    }

    bStartSeq = TRUE;

    while ((krb5Error = krb5_cc_next_cred(pContext, pCache, &cursor, &creds)) == 0)
    {
        /* Look tickets with the intial flag set */
        if (creds.ticket_flags & TKT_FLG_INITIAL)
        {
            /* Extract and compare client principal with desired principal */
            krb5Error = krb5_unparse_name(pContext, creds.client, &pszClientPrincipalName);
            if (krb5Error)
            {
                Status = STATUS_UNSUCCESSFUL;
                BAIL_ON_NT_STATUS(Status);
            }
            if (!strcmp(pszClientPrincipalName, pszDesiredPrincipal))
            {
                bFoundTgt = TRUE;
                break;
            }

            krb5_free_unparsed_name(pContext, pszClientPrincipalName);
            pszClientPrincipalName = NULL;
        }

        krb5_free_cred_contents(pContext, &creds);
    }

    if (!bFoundTgt)
    {
        Status = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(Status);
    }

    /* Extract server principal name */
    krb5Error = krb5_unparse_name(pContext, creds.server, &pszServerPrincipalName);
    if (krb5Error)
    {
        Status = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(Status);
    }

    /* Construct token from krb5 credential data */
    Status = LwIoAllocateMemory(sizeof(*pAccessToken), OUT_PPVOID(&pAccessToken));
    BAIL_ON_NT_STATUS(Status);

    pAccessToken->type = IO_ACCESS_TOKEN_TYPE_KRB5_TGT;

    /* Copy principal names */
    Status = LwRtlWC16StringAllocateFromCString(
        &pAccessToken->payload.krb5Tgt.pwszClientPrincipal,
        pszClientPrincipalName);
    BAIL_ON_NT_STATUS(Status);

    Status = LwRtlWC16StringAllocateFromCString(
        &pAccessToken->payload.krb5Tgt.pwszServerPrincipal,
        pszServerPrincipalName);
    BAIL_ON_NT_STATUS(Status);

    /* Set time fields */
    pAccessToken->payload.krb5Tgt.authTime = creds.times.authtime;
    pAccessToken->payload.krb5Tgt.startTime = creds.times.starttime;
    pAccessToken->payload.krb5Tgt.endTime = creds.times.endtime;
    pAccessToken->payload.krb5Tgt.renewTillTime = creds.times.renew_till;

    /* Copy encryption key */
    pAccessToken->payload.krb5Tgt.keyType = creds.keyblock.enctype;
    pAccessToken->payload.krb5Tgt.ulKeySize = creds.keyblock.length;
    Status = LwIoAllocateMemory(
        creds.keyblock.length,
        OUT_PPVOID(&pAccessToken->payload.krb5Tgt.pKeyData));
    BAIL_ON_NT_STATUS(Status);
    memcpy(
        pAccessToken->payload.krb5Tgt.pKeyData,
        creds.keyblock.contents,
        creds.keyblock.length);

    /* Copy tgt */
    pAccessToken->payload.krb5Tgt.tgtFlags = creds.ticket_flags;
    pAccessToken->payload.krb5Tgt.ulTgtSize = creds.ticket.length;
    Status = LwIoAllocateMemory(
        creds.ticket.length,
        OUT_PPVOID(&pAccessToken->payload.krb5Tgt.pTgtData));
    BAIL_ON_NT_STATUS(Status);
    memcpy(
        pAccessToken->payload.krb5Tgt.pTgtData,
        creds.ticket.data,
        creds.ticket.length);

    *ppAccessToken = pAccessToken;

cleanup:

    LWIO_SAFE_FREE_MEMORY(pszDesiredPrincipal);
    LWIO_SAFE_FREE_MEMORY(pszCredCachePath);

    if (pszClientPrincipalName)
    {
        krb5_free_unparsed_name(pContext, pszClientPrincipalName);
    }

    if (pszServerPrincipalName)
    {
        krb5_free_unparsed_name(pContext, pszServerPrincipalName);
    }

    if (bFoundTgt)
    {
        krb5_free_cred_contents(pContext, &creds);
    }

    if (bStartSeq)
    {
        krb5_cc_end_seq_get(pContext, pCache, &cursor);
    }

    if (pCache)
    {
        krb5_cc_close(pContext, pCache);
    }

    if (pContext)
    {
        krb5_free_context(pContext);
    }

    return Status;

error:

    *ppAccessToken = NULL;

    if (pAccessToken)
    {
        LwIoDeleteAccessToken(pAccessToken);
    }

    goto cleanup;
}
