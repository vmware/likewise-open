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

#include <krb5.h>
#include <ctgoto.h>
#include <ctstring.h>
#include <ctlogger.h>
#include <cttypes.h>
#include <string.h>
#include <pthread.h>
#include <ctfileutils.h>

#include "proxycreds.h"

#define CT_STATUS_KRB5_UNEXPECTED CT_STATUS_ERRNO_UNMAPPED

/* TODO -- Add CtKrb5ToStatus()... */
#define CtKrb5ToStatus(x) ((x) ? CT_STATUS_KRB5_UNEXPECTED : CT_STATUS_SUCCESS)

#define CT_KRB5_TO_STATUS(Error) \
    ((Error) ? CtKrb5ToStatus(Error) : CT_STATUS_KRB5_UNEXPECTED)


typedef struct _PROXY_KRB5_CRED_DATA {
    char* CredCacheName;
    krb5_context Context;
    krb5_ccache CredCache;
} PROXY_KRB5_CRED_DATA;

const char*
ProxypGetKrb5CredHandleCacheName(
    IN PROXY_KRB5_CRED_HANDLE CredHandle
    )
{
    const char* result = NULL;
    if (CredHandle)
    {
        result = CredHandle->CredCacheName;
    }
    return result;
}

void
ProxypDestroyKrb5CredHandle(
    IN PROXY_KRB5_CRED_HANDLE CredHandle
    )
{
    if (CredHandle)
    {
        CT_SAFE_FREE(CredHandle->CredCacheName);
        if (CredHandle->CredCache)
        {
            krb5_cc_destroy(CredHandle->Context, CredHandle->CredCache);
        }
        if (CredHandle->Context)
        {
            krb5_free_context(CredHandle->Context);
        }
        CtFreeMemory(CredHandle);
    }
}

CT_STATUS
ProxypCreateKrb5CredHandle(
    OUT PROXY_KRB5_CRED_HANDLE* CredHandle,
    IN uid_t Uid,
    IN const char* Server,
    IN const char* CredCacheName
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    krb5_context ctx = NULL;
    krb5_error_code error = 0;
    krb5_ccache credCache = NULL;
    krb5_ccache newCredCache = NULL;
    krb5_principal principal = NULL;
    char* newCredCacheName = NULL;
    const char* type = NULL;
    PROXY_KRB5_CRED_HANDLE result = NULL;
    const char* path;
    uid_t uid;
    bool cleanupContext = true;

    error = krb5_init_context(&ctx);
    if (error)
    {
        status = CT_KRB5_TO_STATUS(error);
        GOTO_CLEANUP();
    }

    error = krb5_cc_resolve(ctx, CredCacheName, &credCache);
    if (error)
    {
        status = CT_KRB5_TO_STATUS(error);
        GOTO_CLEANUP();
    }

    type = krb5_cc_get_type(ctx, credCache);
    if (!type)
    {
        status = CT_STATUS_OUT_OF_MEMORY;
        GOTO_CLEANUP();
    }

    if (strcmp(type, "FILE"))
    {
        /* return nothing */
        CT_LOG_DEBUG("Unsupported cache type '%s'", type);
        status = CT_STATUS_SUCCESS;
        GOTO_CLEANUP();
    }

    if (strncmp("FILE:/", CredCacheName, sizeof("FILE:/")-1))
    {
        /* return nothing */
        CT_LOG_DEBUG("Disallowed cache name '%s'", CredCacheName);
        status = CT_STATUS_SUCCESS;
        GOTO_CLEANUP();
    }

    path = &CredCacheName[sizeof("FILE:")-1];

    status = CtFileGetOwnerMode(path, &uid, NULL, NULL);
    if (CT_STATUS_NOT_FOUND == status)
    {
        /* No cred cache, so ok */
        CT_LOG_DEBUG("Missing credentials cache file '%s'", path);
        status = CT_STATUS_SUCCESS;
        GOTO_CLEANUP();
    }
    GOTO_CLEANUP_ON_STATUS(status);

    if (uid != Uid)
    {
        /* Pointing to someone else's cred cache */
        CT_LOG_DEBUG("Incorrect permissions on credentials cache file '%s'", path);
        /* TODO -- perhaps allow w/o reading the creds */
        status = CT_STATUS_ACCESS_DENIED;
        GOTO_CLEANUP();
    }

    error = krb5_cc_get_principal(ctx, credCache, &principal);
    if (error)
    {
        status = CT_KRB5_TO_STATUS(error);
        GOTO_CLEANUP();
    }

    status = CtAllocateStringPrintf(&newCredCacheName, "MEMORY:%ld_%ld_%s",
                                    pthread_self(), Uid, Server);
    GOTO_CLEANUP_ON_STATUS(status);

    error = krb5_cc_resolve(ctx, newCredCacheName, &newCredCache);
    if (error)
    {
        status = CT_KRB5_TO_STATUS(error);
        GOTO_CLEANUP();
    }

    error = krb5_cc_initialize(ctx, newCredCache, principal);
    if (error)
    {
        status = CT_KRB5_TO_STATUS(error);
        GOTO_CLEANUP();
    }

    error = krb5_cc_copy_creds(ctx, credCache, newCredCache);
    if (error)
    {
        status = CT_KRB5_TO_STATUS(error);
        GOTO_CLEANUP();
    }

    status = CtAllocateMemory((void**)&result, sizeof(*result));
    GOTO_CLEANUP_ON_STATUS(status);

    result->CredCacheName = newCredCacheName;
    newCredCacheName = NULL;
    result->CredCache = newCredCache;
    newCredCache = NULL;
    result->Context = ctx;
    cleanupContext = false;

    status = CT_STATUS_SUCCESS;;

cleanup:
    if (principal)
    {
        krb5_free_principal(ctx, principal);
    }
    if (newCredCache)
    {
        krb5_cc_close(ctx, newCredCache);
    }
    if (credCache)
    {
        krb5_cc_close(ctx, credCache);
    }
    if (ctx && cleanupContext)
    {
        krb5_free_context(ctx);
    }

    if (status)
    {
        ProxypDestroyKrb5CredHandle(result);
        result = NULL;
    }

    *CredHandle = result;

    return status;
}
