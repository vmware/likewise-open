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
static HANDLE ghProcessAccessToken = NULL;

#if defined(__LWI_SOLARIS__) || defined (__LWI_AIX__)
static pthread_once_t gOnceControl = {PTHREAD_ONCE_INIT};
#else
static pthread_once_t gOnceControl = PTHREAD_ONCE_INIT;
#endif

static pthread_key_t gContextKey;

static void
SMBFreeClientContext(
    void* pData
    )
{
    PSMB_CLIENT_CONTEXT pContext = (PSMB_CLIENT_CONTEXT) pData;

    if (pContext->hAccessToken)
    {
        SMBCloseHandle(NULL, pContext->hAccessToken);
    }

    SMBFreeMemory(pContext);
}

static DWORD
SMBGetProcessKrb5AccessToken(
    PHANDLE phAccessToken
    )
{
    DWORD dwError = 0;
    krb5_context pKrb5Context = NULL;
    krb5_error_code krb5Error = 0;
    krb5_ccache pKrb5Cache = NULL;
    krb5_principal pKrb5Principal = NULL;
    char* pszPrincipalName = NULL;
    const char* pszCredCachePath = NULL;
    PSMB_API_HANDLE pAPIHandle = NULL;
    PSMB_SECURITY_TOKEN_REP pSecurityToken = NULL;

    *phAccessToken = NULL;

    krb5Error = krb5_init_context(&pKrb5Context);
    if (krb5Error)
    {
        dwError = -1;
        BAIL_ON_SMB_ERROR(dwError);
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
        dwError = -1;
        BAIL_ON_SMB_ERROR(dwError);
    }
    
    dwError = SMBAllocateMemory(sizeof(*pAPIHandle), (void**) (void*) &pAPIHandle);
    BAIL_ON_SMB_ERROR(dwError);

    pAPIHandle->type = SMB_API_HANDLE_ACCESS;
    pSecurityToken = &pAPIHandle->variant.securityToken;

    pSecurityToken->type = SMB_SECURITY_TOKEN_TYPE_KRB5;

    dwError = SMBMbsToWc16s(pszPrincipalName, &pSecurityToken->payload.krb5.pwszPrincipal);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBMbsToWc16s(pszCredCachePath, &pSecurityToken->payload.krb5.pwszCachePath);
    BAIL_ON_SMB_ERROR(dwError);

    *phAccessToken = (HANDLE) pAPIHandle;

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

    return dwError;

error:

    if (pAPIHandle)
    {
        SMBCloseHandle(NULL, pAPIHandle);
    }

    goto cleanup;
}

static DWORD
SMBInitProcessAccessToken(
    void
    )
{
    DWORD dwError = 0;
    HANDLE hAccessToken = NULL;

    dwError = SMBGetProcessKrb5AccessToken(&hAccessToken);
    BAIL_ON_SMB_ERROR(dwError);

    if (hAccessToken)
    {
        ghProcessAccessToken = hAccessToken;
    }
    
error:

    return dwError;
}

static void
__SMBInit(
    void
    )
{
    DWORD dwError = 0;

    SMBInitialize();

    dwError = MAP_LWMSG_STATUS(lwmsg_client_new(gpSMBProtocol, &gpClient));
    BAIL_ON_SMB_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_client_set_endpoint(
                                   gpClient,
                                   LWMSG_CONNECTION_MODE_LOCAL,
                                   SMB_SERVER_FILENAME));
    BAIL_ON_SMB_ERROR(dwError);

    dwError = pthread_key_create(&gContextKey, SMBFreeClientContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBInitProcessAccessToken();
    BAIL_ON_SMB_ERROR(dwError);

    return;

error:

    abort();
}

static void
SMBInit()
{
    pthread_once(&gOnceControl, __SMBInit);
}

DWORD
SMBGetClientContext(
    PSMB_CLIENT_CONTEXT* ppContext
    )
{
    DWORD dwError = 0;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    SMBInit();

    pContext = pthread_getspecific(gContextKey);

    if (!pContext)
    {
        dwError = SMBAllocateMemory(sizeof(*pContext), (void**) (void*) &pContext);
        BAIL_ON_SMB_ERROR(dwError);
        
        if (ghProcessAccessToken)
        {
            dwError = SMBCopyHandle(ghProcessAccessToken, &pContext->hAccessToken);
            BAIL_ON_SMB_ERROR(dwError);
        }

        pContext->bSecurityTokenIsPrivate = FALSE;
        
        dwError = pthread_setspecific(gContextKey, pContext);
        BAIL_ON_SMB_ERROR(dwError);
    }

    *ppContext = pContext;

error:

    return dwError;
}

DWORD
SMBSetThreadToken(
    HANDLE hAccessToken
    )
{
    DWORD dwError = 0;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBGetClientContext(&pContext);
    BAIL_ON_SMB_ERROR(dwError);

    if (pContext->hAccessToken)
    {
        dwError = SMBCloseHandle(NULL, pContext->hAccessToken);
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = SMBCopyHandle(
        hAccessToken ? hAccessToken : ghProcessAccessToken,
        &pContext->hAccessToken);
    BAIL_ON_SMB_ERROR(dwError);

error:

    return dwError;
}

DWORD
SMBGetThreadToken(
    PHANDLE phAccessToken
    )
{
    DWORD dwError = 0;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    *phAccessToken = NULL;

    dwError = SMBGetClientContext(&pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCopyHandle(pContext->hAccessToken, phAccessToken);
    BAIL_ON_SMB_ERROR(dwError);

error:

    return dwError;
}

DWORD
SMBAcquireState(
    PSMB_SERVER_CONNECTION pConnection,
    PSMB_CLIENT_CONTEXT* ppContext
    )
{
    DWORD dwError = 0;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    pConnection->pAssoc = NULL;
    *ppContext = NULL;

    dwError = SMBGetClientContext(&pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_client_acquire_assoc(
                                   gpClient,
                                   &pConnection->pAssoc));
    BAIL_ON_SMB_ERROR(dwError);

    *ppContext = pContext;

error:

    return dwError;
}

VOID
SMBReleaseState(
    PSMB_SERVER_CONNECTION pConnection,
    PSMB_CLIENT_CONTEXT pContext
    )
{
    if (pConnection->pAssoc)
    {
        lwmsg_client_release_assoc(
            gpClient,
            pConnection->pAssoc);
    }
}
