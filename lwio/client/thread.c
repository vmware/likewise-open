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

static LWMsgClient* gpClient = NULL;
static PIO_ACCESS_TOKEN gpProcessAccessToken = NULL;

#if defined(__LWI_SOLARIS__) || defined (__LWI_AIX__)
static pthread_once_t gOnceControl = {PTHREAD_ONCE_INIT};
#else
static pthread_once_t gOnceControl = PTHREAD_ONCE_INIT;
#endif

static pthread_key_t gStateKey;

static void
LwIoThreadStateDestruct(
    void* pData
    )
{
    PIO_THREAD_STATE pState = (PIO_THREAD_STATE) pData;

    if (pState->pAccessToken)
    {
        LwIoDeleteAccessToken(pState->pAccessToken);
    }

    LwIoFreeMemory(pState);
}

static
NTSTATUS
LwIoCreateDefaultKrb5AccessToken(
    PIO_ACCESS_TOKEN* ppAccessToken
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    krb5_context pKrb5Context = NULL;
    krb5_error_code krb5Error = 0;
    krb5_ccache pKrb5Cache = NULL;
    krb5_principal pKrb5Principal = NULL;
    char* pszPrincipalName = NULL;
    const char* pszCredCachePath = NULL;
    PIO_ACCESS_TOKEN pAccessToken = NULL;

    *ppAccessToken = NULL;

    krb5Error = krb5_init_context(&pKrb5Context);
    if (krb5Error)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        BAIL_ON_NT_STATUS(Status);
    }

    pszCredCachePath = krb5_cc_default_name(pKrb5Context);
    if (!pszCredCachePath)
    {
        /* If there is no default path, give up */
        goto cleanup;
    }

    krb5Error = krb5_cc_resolve(pKrb5Context, pszCredCachePath, &pKrb5Cache);
    if (krb5Error)
    {
        /* If we can't access the cache, give up */
        goto cleanup;
    }

    krb5Error = krb5_cc_get_principal(pKrb5Context, pKrb5Cache, &pKrb5Principal);
    if (krb5Error)
    {
        /* If there is no principal, give up */
        goto cleanup;
    }

    krb5Error = krb5_unparse_name(pKrb5Context, pKrb5Principal, &pszPrincipalName);
    if (krb5Error)
    {
        Status = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(Status);
    }
    
    Status = LwIoAllocateMemory(sizeof(*pAccessToken), OUT_PPVOID(&pAccessToken));
    BAIL_ON_NT_STATUS(Status);

    pAccessToken->type = IO_ACCESS_TOKEN_TYPE_KRB5_CCACHE;

    Status = LwRtlWC16StringAllocateFromCString(
        &pAccessToken->payload.krb5Ccache.pwszPrincipal,
        pszPrincipalName
        );
    BAIL_ON_NT_STATUS(Status);
    
    Status = LwRtlWC16StringAllocateFromCString(
        &pAccessToken->payload.krb5Ccache.pwszCachePath,
        pszCredCachePath
        );
    BAIL_ON_NT_STATUS(Status);

    *ppAccessToken = pAccessToken;

cleanup:

    if (pszPrincipalName)
    {
        krb5_free_unparsed_name(pKrb5Context, pszPrincipalName);
    }
    if (pKrb5Principal)
    {
        krb5_free_principal(pKrb5Context, pKrb5Principal);
    }
    if (pKrb5Cache)
    {
        krb5_cc_close(pKrb5Context, pKrb5Cache);
    }
    if (pKrb5Context)
    {
        krb5_free_context(pKrb5Context);
    }

    return Status;

error:

    if (pAccessToken)
    {
        LwIoDeleteAccessToken(pAccessToken);
    }

    goto cleanup;
}

static
NTSTATUS
LwIoInitProcessAccessToken(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_ACCESS_TOKEN pAccessToken = NULL;

    Status = LwIoCreateDefaultKrb5AccessToken(&pAccessToken);
    BAIL_ON_NT_STATUS(Status);

    if (pAccessToken)
    {
        gpProcessAccessToken = pAccessToken;
    }
    
error:

    return Status;
}

static void
__LwIoThreadInit(
    void
    )
{
    NTSTATUS Status = 0;

    LwIoInitialize();

    Status = NtIpcLWMsgStatusToNtStatus(lwmsg_client_new(NULL, gpLwIoProtocol, &gpClient));
    BAIL_ON_NT_STATUS(Status);

    Status = NtIpcLWMsgStatusToNtStatus(lwmsg_client_set_endpoint(
                                            gpClient,
                                            LWMSG_CONNECTION_MODE_LOCAL,
                                            LWIO_SERVER_FILENAME));
    BAIL_ON_NT_STATUS(Status);

    if (pthread_key_create(&gStateKey, LwIoThreadStateDestruct))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        BAIL_ON_NT_STATUS(Status);
    }

    Status = LwIoInitProcessAccessToken();
    BAIL_ON_NT_STATUS(Status);

    return;

error:

    abort();
}

static void
LwIoThreadInit()
{
    pthread_once(&gOnceControl, __LwIoThreadInit);
}

NTSTATUS
LwIoGetThreadState(
    OUT PIO_THREAD_STATE* ppState
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_THREAD_STATE pState = NULL;

    LwIoThreadInit();

    pState = pthread_getspecific(gStateKey);

    if (!pState)
    {
        Status = LwIoAllocateMemory(sizeof(*pState), OUT_PPVOID(&pState));
        BAIL_ON_NT_STATUS(Status);
        
        if (gpProcessAccessToken)
        {
            Status = LwIoCopyAccessToken(gpProcessAccessToken, &pState->pAccessToken);
            BAIL_ON_NT_STATUS(Status);
        }

        if (pthread_setspecific(gStateKey, pState))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            BAIL_ON_NT_STATUS(Status);
        }
    }

    *ppState = pState;

error:

    return Status;
}

NTSTATUS
LwIoSetThreadAccessToken(
    PIO_ACCESS_TOKEN pAccessToken
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_THREAD_STATE pState = NULL;

    Status = LwIoGetThreadState(&pState);
    BAIL_ON_NT_STATUS(Status);

    if (pState->pAccessToken)
    {
        LwIoDeleteAccessToken(pState->pAccessToken);
    }

    Status = LwIoCopyAccessToken(
        pAccessToken ? pAccessToken : gpProcessAccessToken,
        &pState->pAccessToken);
    BAIL_ON_NT_STATUS(Status);

error:

    return Status;
}

NTSTATUS
LwIoGetThreadAccessToken(
    PIO_ACCESS_TOKEN* ppAccessToken
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_THREAD_STATE pState = NULL;

    *ppAccessToken = NULL;

    Status = LwIoGetThreadState(&pState);
    BAIL_ON_NT_STATUS(Status);

    Status = LwIoCopyAccessToken(pState->pAccessToken, ppAccessToken);
    BAIL_ON_NT_STATUS(Status);

error:

    return Status;
}

NTSTATUS
LwIoAcquireContext(
    OUT PIO_CONTEXT pContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    LwIoThreadInit();

    pContext->pAssoc = NULL;

    Status = NtIpcLWMsgStatusToNtStatus(
        lwmsg_client_acquire_assoc(
            gpClient,
            &pContext->pAssoc));
    BAIL_ON_NT_STATUS(Status);

error:
    return Status;
}

NTSTATUS
LwIoReleaseContext(
    IN OUT PIO_CONTEXT pContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    LwIoThreadInit();

    if (pContext->pAssoc)
    {
        Status = NtIpcLWMsgStatusToNtStatus(
            lwmsg_client_release_assoc(
                gpClient,
                pContext->pAssoc));
        BAIL_ON_NT_STATUS(Status);
        pContext->pAssoc = NULL;
    }
    
error:
    
    return Status;
}

NTSTATUS
LwIoOpenContextShared(
    PIO_CONTEXT* ppContext
    )
{
    PIO_CONTEXT pContext = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    LwIoThreadInit();

    Status = LwIoAllocateMemory(
        sizeof(*pContext),
        OUT_PPVOID(&pContext));
    BAIL_ON_NT_STATUS(Status);

    Status = NtIpcLWMsgStatusToNtStatus(
        lwmsg_client_create_assoc(
            gpClient,
            &pContext->pAssoc));
    BAIL_ON_NT_STATUS(Status);

    Status = NtIpcLWMsgStatusToNtStatus(
        lwmsg_assoc_establish(pContext->pAssoc));
    BAIL_ON_NT_STATUS(Status);

    *ppContext = pContext;

cleanup:

    return Status;

error:

    if (pContext)
    {
        LwIoCloseContext(pContext);
    }

    *ppContext = NULL;

    goto cleanup;
}
