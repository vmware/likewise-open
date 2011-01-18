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
static LWMsgSession* gpSession = NULL;
static PIO_CREDS gpProcessCreds = NULL;
static pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;
static LW_LIST_LINKS gPathCreds = {&gPathCreds, &gPathCreds};
static pthread_key_t gStateKey;
static BOOLEAN gbStateKeyInit = FALSE;

static
NTSTATUS
LwIoNormalizePath(
    PCWSTR pwszPath,
    PWSTR* ppwszNormal
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR pwszIn = NULL;
    PWSTR pwszOut = NULL;
    PWSTR pwszNormal = NULL;

    Status = LwRtlWC16StringDuplicate(&pwszNormal, pwszPath);
    BAIL_ON_NT_STATUS(Status);

    for (pwszIn = pwszOut = pwszNormal; *pwszIn; pwszIn++)
    {
        switch (*pwszIn)
        {
        case '\\':
        case '/':
            *(pwszOut++) = '/';
            while (pwszIn[1] == '\\' ||
                   pwszIn[1] == '/')
            {
                pwszIn++;
            }
            break;
        default:
            *(pwszOut++) = *pwszIn;
            break;
        }
    }

    *pwszOut = '\0';

    *ppwszNormal = pwszNormal;

error:

    return Status;
}


static
NTSTATUS
LwIoFindPathCreds(
    PCWSTR pwszPath,
    BOOL bPrecise,
    PIO_PATH_CREDS* ppCreds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR pwszNormal = NULL;
    size_t pathLength = 0;
    size_t prefixLength = 0;
    PIO_PATH_CREDS pCreds = NULL;
    PLW_LIST_LINKS pLink = NULL;

    Status = LwIoNormalizePath(pwszPath, &pwszNormal);
    BAIL_ON_NT_STATUS(Status);

    pathLength = LwRtlWC16StringNumChars(pwszNormal);

    while ((pLink = LwListTraverse(&gPathCreds, pLink)))
    {
        pCreds = LW_STRUCT_FROM_FIELD(pLink, IO_PATH_CREDS, link);

        prefixLength = LwRtlWC16StringNumChars(pCreds->pwszPathPrefix);

        if ((bPrecise && LwRtlWC16StringIsEqual(pwszNormal, pCreds->pwszPathPrefix, TRUE)) ||
            (!bPrecise && !memcmp(
                pwszNormal,
                pCreds->pwszPathPrefix,
                prefixLength < pathLength ? prefixLength : pathLength)))
        {
            goto cleanup;
        }
    }

    pCreds = NULL;

cleanup:

    *ppCreds = pCreds;

    RTL_FREE(&pwszNormal);

    return Status;

error:

    goto cleanup;
}

static
VOID
LwIoDeletePathCreds(
    PIO_PATH_CREDS pPathCreds
    )
{
    if (pPathCreds)
    {
        RTL_FREE(&pPathCreds->pwszPathPrefix);

        if (pPathCreds->pCreds)
        {
            LwIoDeleteCreds(pPathCreds->pCreds);
        }

        RTL_FREE(&pPathCreds);
    }
}

static void
LwIoThreadStateDestruct(
    void* pData
    )
{
    PIO_THREAD_STATE pState = (PIO_THREAD_STATE) pData;

    if (pState)
    {
        if (pState->pCreds)
        {
            LwIoDeleteCreds(pState->pCreds);
        }

        LwIoFreeMemory(pState);
    }
}

static
NTSTATUS
LwIoCreateDefaultKrb5Creds(
    PIO_CREDS* ppCreds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    krb5_context pKrb5Context = NULL;
    krb5_error_code krb5Error = 0;
    krb5_ccache pKrb5Cache = NULL;
    krb5_principal pKrb5Principal = NULL;
    char* pszPrincipalName = NULL;
    const char* pszCredCachePath = NULL;
    PIO_CREDS pCreds = NULL;

    *ppCreds = NULL;

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
    
    Status = LwIoAllocateMemory(sizeof(*pCreds), OUT_PPVOID(&pCreds));
    BAIL_ON_NT_STATUS(Status);

    pCreds->type = IO_CREDS_TYPE_KRB5_CCACHE;

    Status = LwRtlWC16StringAllocateFromCString(
        &pCreds->payload.krb5Ccache.pwszPrincipal,
        pszPrincipalName
        );
    BAIL_ON_NT_STATUS(Status);
    
    Status = LwRtlWC16StringAllocateFromCString(
        &pCreds->payload.krb5Ccache.pwszCachePath,
        pszCredCachePath
        );
    BAIL_ON_NT_STATUS(Status);

    *ppCreds = pCreds;

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

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    goto cleanup;
}

static
NTSTATUS
LwIoInitProcessCreds(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_CREDS pCreds = NULL;

    Status = LwIoCreateDefaultKrb5Creds(&pCreds);
    BAIL_ON_NT_STATUS(Status);

    if (pCreds)
    {
        gpProcessCreds = pCreds;
    }
    
error:

    return Status;
}

static
NTSTATUS
LwIoThreadInit(
    void
    )
{
    NTSTATUS Status = 0;
    BOOL bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gLock);

    if (!gpLwIoProtocol)
    {
        LwIoInitialize();
    }

    if (!gbStateKeyInit)
    {
        Status = LwErrnoToNtStatus(pthread_key_create(&gStateKey, LwIoThreadStateDestruct));
        BAIL_ON_NT_STATUS(Status);
        gbStateKeyInit = TRUE;
    }

    if (!gpProcessCreds)
    {
        Status = LwIoInitProcessCreds();
        BAIL_ON_NT_STATUS(Status);
    }

    if (!gpClient)
    {
        Status = NtIpcLWMsgStatusToNtStatus(lwmsg_peer_new(NULL, gpLwIoProtocol, &gpClient));
        BAIL_ON_NT_STATUS(Status);

        Status = NtIpcLWMsgStatusToNtStatus(lwmsg_peer_add_connect_endpoint(
                                                gpClient,
                                                LWMSG_ENDPOINT_LOCAL,
                                                LWIO_SERVER_FILENAME));
        BAIL_ON_NT_STATUS(Status);
    }

    if (!gpSession)
    {
        Status = NtIpcLWMsgStatusToNtStatus(lwmsg_peer_connect(
                                                gpClient,
                                                &gpSession));
        BAIL_ON_NT_STATUS(Status);
    }

error:

    LWIO_UNLOCK_MUTEX(bInLock, &gLock);

    return Status;
}

NTSTATUS
LwIoGetThreadState(
    OUT PIO_THREAD_STATE* ppState
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_THREAD_STATE pState = NULL;

    Status = LwIoThreadInit();
    BAIL_ON_NT_STATUS(Status);

    pState = pthread_getspecific(gStateKey);

    if (!pState)
    {
        Status = LwIoAllocateMemory(sizeof(*pState), OUT_PPVOID(&pState));
        BAIL_ON_NT_STATUS(Status);
        
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
LwIoSetThreadCreds(
    PIO_CREDS pCreds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_THREAD_STATE pState = NULL;

    Status = LwIoGetThreadState(&pState);
    BAIL_ON_NT_STATUS(Status);

    if (pState->pCreds)
    {
        LwIoDeleteCreds(pState->pCreds);
    }

    Status = LwIoCopyCreds(
        pCreds ? pCreds : gpProcessCreds,
        &pState->pCreds);
    BAIL_ON_NT_STATUS(Status);

error:

    return Status;
}

NTSTATUS
LwIoGetThreadCreds(
    PIO_CREDS* ppCreds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_THREAD_STATE pState = NULL;

    *ppCreds = NULL;

    Status = LwIoGetThreadState(&pState);
    BAIL_ON_NT_STATUS(Status);

    Status = LwIoCopyCreds(pState->pCreds, ppCreds);
    BAIL_ON_NT_STATUS(Status);

error:

    return Status;
}

LW_NTSTATUS
LwIoSetPathCreds(
    LW_PCWSTR pwszPathPrefix,
    LW_PIO_CREDS pCreds
    )
{
    LW_NTSTATUS Status = STATUS_SUCCESS;
    PIO_PATH_CREDS pPathCreds = NULL;
    PIO_CREDS pCredCopy = NULL;
    BOOL bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gLock);

    Status = LwIoFindPathCreds(pwszPathPrefix, TRUE, &pPathCreds);
    BAIL_ON_NT_STATUS(Status);

    if (pPathCreds)
    {
        Status = LwIoCopyCreds(pCreds, &pCredCopy);
        BAIL_ON_NT_STATUS(Status);

        if (pPathCreds->pCreds)
        {
            LwIoDeleteCreds(pPathCreds->pCreds);
        }

        pPathCreds->pCreds = pCredCopy;
        pCredCopy = NULL;
        pPathCreds = NULL;
    }
    else if (pCreds)
    {
        Status = RTL_ALLOCATE(&pPathCreds, IO_PATH_CREDS, sizeof(IO_PATH_CREDS));
        BAIL_ON_NT_STATUS(Status);

        LwListInit(&pPathCreds->link);

        Status = LwIoNormalizePath(pwszPathPrefix, &pPathCreds->pwszPathPrefix);
        BAIL_ON_NT_STATUS(Status);

        Status = LwIoCopyCreds(pCreds, &pPathCreds->pCreds);
        BAIL_ON_NT_STATUS(Status);

        LwListInsertBefore(&gPathCreds, &pPathCreds->link);
        pPathCreds = NULL;
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &gLock);

    if (pCredCopy)
    {
        LwIoDeleteCreds(pCredCopy);
    }

    if (pPathCreds)
    {
        LwIoDeletePathCreds(pPathCreds);
    }

    return Status;

error:

    goto cleanup;
}

LW_NTSTATUS
LwIoGetActiveCreds(
    LW_PCWSTR pwszPath,
    LW_PIO_CREDS* ppToken
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_PATH_CREDS pPathCreds = NULL;
    PIO_CREDS pCreds = NULL;
    BOOL bInLock = FALSE;

    Status = LwIoGetThreadCreds(&pCreds);
    BAIL_ON_NT_STATUS(Status);

    if (!pCreds && pwszPath)
    {
        LWIO_LOCK_MUTEX(bInLock, &gLock);

        Status = LwIoFindPathCreds(pwszPath, FALSE, &pPathCreds);
        BAIL_ON_NT_STATUS(Status);

        if (pPathCreds)
        {
            Status = LwIoCopyCreds(pPathCreds->pCreds, &pCreds);
            BAIL_ON_NT_STATUS(Status);
        }
    }

    if (!pCreds && gpProcessCreds)
    {
        Status = LwIoCopyCreds(gpProcessCreds, &pCreds);
        BAIL_ON_NT_STATUS(Status);
    }

    *ppToken = pCreds;

error:

    LWIO_UNLOCK_MUTEX(bInLock, &gLock);

    return Status;
}

NTSTATUS
LwIoAcquireContext(
    OUT PIO_CONTEXT pContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = LwIoThreadInit();
    BAIL_ON_NT_STATUS(Status);

    pContext->pClient = gpClient;
    pContext->pSession = gpSession;

error:

    return Status;
}

NTSTATUS
LwIoReleaseContext(
    IN OUT PIO_CONTEXT pContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    memset(pContext, 0, sizeof(*pContext));

    return Status;
}

NTSTATUS
LwIoOpenContextShared(
    PIO_CONTEXT* ppContext
    )
{
    PIO_CONTEXT pContext = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = LwIoThreadInit();
    BAIL_ON_NT_STATUS(Status);

    Status = LwIoAllocateMemory(
        sizeof(*pContext),
        OUT_PPVOID(&pContext));
    BAIL_ON_NT_STATUS(Status);

    pContext->pClient = gpClient;
    pContext->pSession = gpSession;

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
