/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 *        lwkrb5.c
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi)
 *        
 *        Kerberos 5 API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak <rszczesniak@likewisesoftware.com>
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */

#include "includes.h"

DWORD
LwKrb5Init(
    IN OPTIONAL LW_KRB5_REALM_IS_OFFLINE_CALLBACK pfIsOfflineCallback,
    IN OPTIONAL LW_KRB5_REALM_TRANSITION_OFFLINE_CALLBACK pfTransitionOfflineCallback
    )
{
    DWORD dwError = 0;
    
    dwError = pthread_mutex_init(&gLwKrb5State.ExistingClientLock, NULL);
    BAIL_ON_LW_ERROR(dwError);

    dwError = pthread_mutex_init(&gLwKrb5State.UserCacheMutex, NULL);
    BAIL_ON_LW_ERROR(dwError);

    gLwKrb5State.pfIsOfflineCallback = pfIsOfflineCallback;
    gLwKrb5State.pfTransitionOfflineCallback = pfTransitionOfflineCallback;

cleanup:

    return dwError;
    
error:

    LW_LOG_ERROR("Error: Failed to initialize Krb5. [Error code: %d]", dwError);

    goto cleanup;
}

DWORD
LwKrb5GetDefaultRealm(
    PSTR* ppszRealm
    )
{
    DWORD dwError = 0;
    krb5_context ctx = NULL;
    PSTR pszKrb5Realm = NULL;
    PSTR pszRealm = NULL;

    krb5_init_context(&ctx);
    krb5_get_default_realm(ctx, &pszKrb5Realm);

    if (LW_IS_NULL_OR_EMPTY_STR(pszKrb5Realm)) {
        dwError = LW_ERROR_NO_DEFAULT_REALM;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwAllocateString(pszKrb5Realm, &pszRealm);
    BAIL_ON_LW_ERROR(dwError);

    *ppszRealm = pszRealm;
    
cleanup:

    if (pszKrb5Realm)
    {
        krb5_free_default_realm(ctx, pszKrb5Realm);
    }

    krb5_free_context(ctx);

    return(dwError);

error:

    *ppszRealm = NULL;
    
    LW_SAFE_FREE_STRING(pszRealm);

    goto cleanup;
}


DWORD
LwKrb5GetSystemCachePath(
    PSTR*         ppszCachePath
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = NULL;
    krb5_context ctx = NULL;
    const char *pszKrbDefault = NULL;
    krb5_error_code ret = 0;
    
    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    pszKrbDefault = krb5_cc_default_name(ctx);

    dwError = LwAllocateString(
                pszKrbDefault,
                &pszCachePath);
    BAIL_ON_LW_ERROR(dwError);
    
    *ppszCachePath = pszCachePath;
    
cleanup:
    if (ctx)
    {
        krb5_free_context(ctx);
    }

    return dwError;
    
error:

    *ppszCachePath = NULL;
    
    goto cleanup;
}


DWORD
LwKrb5GetUserCachePath(
    uid_t         uid,
    Krb5CacheType cacheType,
    PSTR*         ppszCachePath
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = NULL;
    
    switch (cacheType)
    {
        case KRB5_InMemory_Cache:
            
            dwError = LwAllocateStringPrintf(
                        &pszCachePath,
                        "MEMORY:krb5cc_%ld",
                        (long)uid);
            BAIL_ON_LW_ERROR(dwError);
            
            break;
            
        case KRB5_File_Cache:
            
            dwError = LwAllocateStringPrintf(
                        &pszCachePath,
                        "FILE:/tmp/krb5cc_%ld",
                        (long)uid);
            BAIL_ON_LW_ERROR(dwError);
            
            break;
            
        default:
            
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LW_ERROR(dwError);
            
            break;
    }
    
    *ppszCachePath = pszCachePath;
    
cleanup:

    return dwError;
    
error:

    *ppszCachePath = NULL;
    
    goto cleanup;
}


DWORD
LwKrb5SetDefaultCachePath(
    PCSTR pszCachePath,
    PSTR* ppszOrigCachePath
    )
{
    DWORD dwError       = 0;
    DWORD dwMajorStatus = 0;
    DWORD dwMinorStatus = 0;
    PSTR  pszOrigCachePath = NULL;

    // Set the default for gss
    dwMajorStatus = gss_krb5_ccache_name(
                            (OM_uint32 *)&dwMinorStatus,
                            pszCachePath,
                            (ppszOrigCachePath) ? (const char**)&pszOrigCachePath : NULL);
    BAIL_ON_SEC_ERROR(dwMajorStatus);
    
    if (ppszOrigCachePath) {
        if (!LW_IS_NULL_OR_EMPTY_STR(pszOrigCachePath)) {
            dwError = LwAllocateString(pszOrigCachePath, ppszOrigCachePath);
            BAIL_ON_LW_ERROR(dwError);
        } else {
            *ppszOrigCachePath = NULL;
        }
    }
    
cleanup:

    return dwError;
    
error:

    if (ppszOrigCachePath) {
        *ppszOrigCachePath = NULL;
    }

    goto cleanup;
}


DWORD
LwKrb5GetSystemKeytabPath(
    PSTR* ppszKeytabPath
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    PSTR pszPath = NULL;
    size_t size = 64;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    do {
        LW_SAFE_FREE_STRING(pszPath);

        size *= 2;
        dwError = LwAllocateMemory(size, OUT_PPVOID(&pszPath));
        BAIL_ON_LW_ERROR(dwError);

        ret = krb5_kt_default_name(ctx, pszPath, size);
    } while (ret == KRB5_CONFIG_NOTENUFSPACE);
    
    BAIL_ON_KRB_ERROR(ctx, ret);
    *ppszKeytabPath = pszPath;

cleanup:
    if (ctx) {
        krb5_free_context(ctx);
    }

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszPath);
    *ppszKeytabPath = NULL;
    
    goto cleanup;
}

DWORD
LwKrb5SetProcessDefaultCachePath(
    PCSTR pszCachePath
    )
{
    DWORD dwError = 0;
    PSTR pszEnvironmentEntry = NULL;
    static volatile PSTR pszSavedEnvironmentEntry = NULL;
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    BOOLEAN bLocked = FALSE;

    dwError = pthread_mutex_lock(&lock);
    if (dwError)
    {
        dwError = LwMapErrnoToLwError(dwError);
        BAIL_ON_LW_ERROR(dwError);
    }
    bLocked = TRUE;

    dwError = LwAllocateStringPrintf(&pszEnvironmentEntry,
                                      "KRB5CCNAME=%s",
                                      pszCachePath);
    BAIL_ON_LW_ERROR(dwError);

    /*
     * putenv requires that the buffer not be free'd.
     */
    if (putenv(pszEnvironmentEntry) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    LW_SAFE_FREE_STRING(pszSavedEnvironmentEntry);
    pszSavedEnvironmentEntry = pszEnvironmentEntry;
    pszEnvironmentEntry = NULL;

error:
    LW_SAFE_FREE_STRING(pszEnvironmentEntry);
    
    if (bLocked)
    {
        pthread_mutex_unlock(&lock);
    }

    return dwError;
}

DWORD
LwSetupMachineSession(
    PCSTR  pszSamAccountName,
    PCSTR  pszPassword,
    PCSTR  pszRealm,
    PCSTR  pszDomain,
    PDWORD pdwGoodUntilTime
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR pszHostKeytabFile = NULL;
    PSTR pszKrb5CcPath = NULL;
    PSTR pszDomname = NULL;
    PSTR pszRealmCpy = NULL;
    PSTR pszMachPrincipal = NULL;
    DWORD dwGoodUntilTime = 0;

    dwError = LwKrb5GetSystemKeytabPath(&pszHostKeytabFile);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwKrb5GetSystemCachePath(&pszKrb5CcPath);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwAllocateString(pszRealm, &pszRealmCpy);
    BAIL_ON_LW_ERROR(dwError);
    LwStrToUpper(pszRealmCpy);

    dwError = LwAllocateStringPrintf(&pszMachPrincipal, "%s@%s",
                                      pszSamAccountName, pszRealm);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwAllocateString(pszDomain, &pszDomname);
    BAIL_ON_LW_ERROR(dwError);
    LwStrToLower(pszDomname);

    dwError = LwKrb5GetTgt(
    		      pszMachPrincipal,
                  pszPassword,
                  pszKrb5CcPath,
                  &dwGoodUntilTime);
    BAIL_ON_LW_ERROR(dwError);

    if (pdwGoodUntilTime)
    {
    	*pdwGoodUntilTime = dwGoodUntilTime;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszMachPrincipal);
    LW_SAFE_FREE_STRING(pszDomname);
    LW_SAFE_FREE_STRING(pszRealmCpy);
    LW_SAFE_FREE_STRING(pszKrb5CcPath);
    LW_SAFE_FREE_STRING(pszHostKeytabFile);
    
    return (dwError);
    
error:

    if (pdwGoodUntilTime)
    {
    	*pdwGoodUntilTime = 0;
    }

	goto cleanup;
}

DWORD
LwKrb5CleanupMachineSession(
    VOID
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR pszKrb5CcPath = NULL;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;

    dwError = LwKrb5GetSystemCachePath(&pszKrb5CcPath);
    BAIL_ON_LW_ERROR(dwError);

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_resolve(ctx, pszKrb5CcPath, &cc);
    if (KRB5_FCC_NOFILE == ret)
    {
        goto cleanup;
    }
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_destroy(ctx, cc);
    // This always frees the cc reference, even on error.
    cc = NULL;
    if (KRB5_FCC_NOFILE == ret)
    {
        goto cleanup;
    }
    BAIL_ON_KRB_ERROR(ctx, ret);

cleanup:
    LW_SAFE_FREE_STRING(pszKrb5CcPath);

    if (cc)
    {
        // ctx must be valid.
        krb5_cc_close(ctx, cc);
    }

    if (ctx)
    {
        krb5_free_context(ctx);
    }

    return dwError;

error:
    goto cleanup;
}

#define AD_IF_RELEVANT_TYPE 1
#define AD_WIN2K_PAC        128
#define BAIL_ON_DCE_ERROR(dest, status)                   \
    if ((status) != 0) {                    \
        LW_LOG_ERROR("DCE Error [Code:%d]", (status));   \
        (dest) = LW_ERROR_DCE_CALL_FAILED;               \
        goto error;                                       \
    }

DWORD
LwKrb5CopyFromUserCache(
                krb5_context ctx,
                krb5_ccache destCC,
                uid_t uid
                )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszCachePath = NULL;
    krb5_ccache srcCC = NULL;
    krb5_cc_cursor srcPos = NULL;
    krb5_cc_cursor destPos = NULL;
    // Free with krb5_free_cred_contents
    krb5_creds srcCreds = {0};
    // Free with krb5_free_cred_contents
    krb5_creds destCreds = {0};
    krb5_error_code ret = 0;
    krb5_principal destClient = 0;
    BOOLEAN bIncludeTicket = TRUE;
    DWORD dwTime = 0;

    ret = krb5_cc_get_principal(
            ctx,
            destCC,
            &destClient);
    BAIL_ON_KRB_ERROR(ctx, ret);

    dwError = LwKrb5GetUserCachePath(
                    uid,
                    KRB5_File_Cache,
                    &pszCachePath);
    BAIL_ON_LW_ERROR(dwError);
    
    ret = krb5_cc_resolve(
            ctx,
            pszCachePath,
            &srcCC);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_start_seq_get(
            ctx,
            srcCC,
            &srcPos);
    if (ret == KRB5_FCC_NOFILE)
    {
        // The cache file does not exist
        ret = 0;
        goto cleanup;
    }
    if (ret == KRB5_CC_FORMAT)
    {
        // Some other user put a bad cc in place - don't copy anything
        // from it.
        ret = 0;
        goto cleanup;
    }
    BAIL_ON_KRB_ERROR(ctx, ret);

    dwTime = time(NULL);

    while (1)
    {
        krb5_free_cred_contents(
                ctx,
                &srcCreds);

        ret = krb5_cc_next_cred(
                ctx,
                srcCC,
                &srcPos,
                &srcCreds);
        if (ret == KRB5_CC_FORMAT) {
            break;
        } else if (ret == KRB5_CC_END) {
            break;
        } else {
            BAIL_ON_KRB_ERROR(ctx, ret);
        }

        if (!krb5_principal_compare(ctx, destClient, srcCreds.client))
        {
            /* Can't keep these creds. The client principal doesn't
             * match. */
            continue;
        }

        if ( srcCreds.times.endtime < dwTime )
        {
            /* Credentials are too old. */
            continue;
        }

        if (destPos != NULL)
        {
            krb5_cc_end_seq_get(
                    ctx,
                    destCC,
                    &destPos);
            destPos = NULL;
        }

        ret = krb5_cc_start_seq_get(
                ctx,
                destCC,
                &destPos);
        BAIL_ON_KRB_ERROR(ctx, ret);

        bIncludeTicket = TRUE;

        while(bIncludeTicket)
        {
            krb5_free_cred_contents(
                    ctx,
                    &destCreds);

            ret = krb5_cc_next_cred(
                    ctx,
                    destCC,
                    &destPos,
                    &destCreds);
            if (ret == KRB5_CC_END) {
                break;
            } else {
                BAIL_ON_KRB_ERROR(ctx, ret);
            }

            if (krb5_principal_compare(
                        ctx,
                        destCreds.server,
                        srcCreds.server))
            {
                /* These credentials are already in the dest cache
                 */
                bIncludeTicket = FALSE;
            }
        }

        if (bIncludeTicket)
        {
            // These creds can go in the new cache
            ret = krb5_cc_store_cred(ctx, destCC, &srcCreds);
            BAIL_ON_KRB_ERROR(ctx, ret);
        }
    }

cleanup:

    LW_SAFE_FREE_STRING(pszCachePath);

    if (ctx != NULL)
    {
        if (srcPos != NULL)
        {
            krb5_cc_end_seq_get(
                    ctx,
                    srcCC,
                    &srcPos);
        }
        if (destPos != NULL)
        {
            krb5_cc_end_seq_get(
                    ctx,
                    destCC,
                    &destPos);
        }
        if (srcCC != NULL)
        {
            krb5_cc_close(ctx, srcCC);
        }
        krb5_free_cred_contents(ctx, &srcCreds);
        krb5_free_cred_contents(ctx, &destCreds);
        if (destClient != NULL)
        {
            krb5_free_principal(ctx, destClient);
        }
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
LwKrb5MoveCCacheToUserPath(
    krb5_context ctx,
    PCSTR pszNewCacheName,
    uid_t uid,
    gid_t gid
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszCachePath = NULL;
    PCSTR  pszCachePathReal = NULL;

    dwError = LwKrb5GetUserCachePath(
                    uid,
                    KRB5_File_Cache,
                    &pszCachePath);
    BAIL_ON_LW_ERROR(dwError);
    
    if (strncasecmp(pszCachePath, "FILE:", sizeof("FILE:")-1)) {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LW_ERROR(dwError);
    } else {
        pszCachePathReal = pszCachePath + sizeof("FILE:") - 1;
    }

    dwError = LwMoveFile(pszNewCacheName,
                pszCachePathReal);
    BAIL_ON_LW_ERROR(dwError);

    /* Let the user read and write to their cache file (before this, only
     * root was allowed to read and write the file).
     */
    dwError = LwChangeOwnerAndPermissions(
                pszCachePathReal,
                uid,
                gid,
                S_IRWXU);
    BAIL_ON_LW_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszCachePath);
    return dwError;

error:
    goto cleanup;
}

DWORD
LwKrb5Shutdown(
    VOID
    )
{
    DWORD dwError = 0;

    gLwKrb5State.pfIsOfflineCallback = NULL;
    gLwKrb5State.pfTransitionOfflineCallback = NULL;

    dwError = pthread_mutex_destroy(&gLwKrb5State.ExistingClientLock);
    BAIL_ON_LW_ERROR(dwError);

    dwError = pthread_mutex_destroy(&gLwKrb5State.UserCacheMutex);
    BAIL_ON_LW_ERROR(dwError);
    
cleanup:

    return dwError;
    
error:

    LW_LOG_ERROR("Error: Failed to shutdown Krb5. [Error code: %d]", dwError);

    goto cleanup;
}

DWORD
LwKrb5GetMachineCreds(
    PSTR* ppszUsername,
    PSTR* ppszPassword,
    PSTR* ppszDomainDnsName,
    PSTR* ppszHostDnsDomain
    )
{
    DWORD dwError = 0;
    PSTR  pszUsername = NULL;
    PSTR  pszPassword = NULL;
    PSTR  pszDomainDnsName = NULL;
    PSTR  pszHostDnsDomain = NULL;
    PLWPS_PASSWORD_INFO pMachineAcctInfo = NULL;
    HANDLE hPasswordStore = (HANDLE)NULL;

    dwError = LwpsOpenPasswordStore(
                    LWPS_PASSWORD_STORE_SQLDB,
                    &hPasswordStore);
    BAIL_ON_LW_ERROR(dwError);
    
    dwError = LwpsGetPasswordByCurrentHostName(
                    hPasswordStore,
                    &pMachineAcctInfo);
    if (dwError)
    {
        LW_LOG_ERROR("Unable to read machine password for hostname");
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwWc16sToMbs(
                    pMachineAcctInfo->pwszMachineAccount,
                    &pszUsername);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwWc16sToMbs(
                    pMachineAcctInfo->pwszMachinePassword,
                    &pszPassword);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwWc16sToMbs(
                    pMachineAcctInfo->pwszDnsDomainName,
                    &pszDomainDnsName);
    BAIL_ON_LW_ERROR(dwError);
    
    dwError = LwWc16sToMbs(
                    pMachineAcctInfo->pwszHostDnsDomain,
                    &pszHostDnsDomain);
    BAIL_ON_LW_ERROR(dwError);
    
    if (LW_IS_NULL_OR_EMPTY_STR(pszUsername)) {
        dwError = LW_ERROR_INVALID_ACCOUNT;
        BAIL_ON_LW_ERROR(dwError);
    }
    
    if (LW_IS_NULL_OR_EMPTY_STR(pszPassword)) {
        dwError = LW_ERROR_INVALID_PASSWORD;
        BAIL_ON_LW_ERROR(dwError);
    }
    
    if (LW_IS_NULL_OR_EMPTY_STR(pszUsername)) {
        dwError = LW_ERROR_INVALID_DOMAIN;
        BAIL_ON_LW_ERROR(dwError);
    }
    
    *ppszUsername = pszUsername;
    *ppszPassword = pszPassword;
    *ppszDomainDnsName = pszDomainDnsName;
    *ppszHostDnsDomain = pszHostDnsDomain;
    
cleanup:

    if (pMachineAcctInfo) {
        LwpsFreePasswordInfo(hPasswordStore, pMachineAcctInfo);
    }

    if (hPasswordStore != (HANDLE)NULL) {
       LwpsClosePasswordStore(hPasswordStore);
    }

    return dwError;
    
error:

    *ppszUsername = NULL;
    *ppszPassword = NULL;
    *ppszDomainDnsName = NULL;
    
    LW_SAFE_FREE_STRING(pszUsername);
    LW_SAFE_FREE_STRING(pszPassword);
    LW_SAFE_FREE_STRING(pszDomainDnsName);
    LW_SAFE_FREE_STRING(pszHostDnsDomain);

    goto cleanup;
}

DWORD
LwKrb5RefreshMachineTGT(
    PDWORD pdwGoodUntilTime
    )
{
    DWORD dwError = 0;
    DWORD dwGoodUntilTime = 0;
    PSTR  pszUsername = NULL;
    PSTR  pszPassword = NULL;
    PSTR  pszDomainDnsName = NULL;
    PSTR  pszHostDnsDomain = NULL;

    LW_LOG_VERBOSE("Refreshing machine TGT");

    dwError = LwKrb5GetMachineCreds(
                    &pszUsername,
                    &pszPassword,
                    &pszDomainDnsName,
                    &pszHostDnsDomain);
    BAIL_ON_LW_ERROR(dwError);
	
    dwError = LwSetupMachineSession(
                    pszUsername,
                    pszPassword,
                    pszDomainDnsName,
                    pszHostDnsDomain,
                    &dwGoodUntilTime);
    BAIL_ON_LW_ERROR(dwError);
    
    if (pdwGoodUntilTime != NULL)
    {
        *pdwGoodUntilTime = dwGoodUntilTime;
    }
	
cleanup:

    LW_SAFE_FREE_STRING(pszUsername);
    LW_SAFE_FREE_STRING(pszPassword);
    LW_SAFE_FREE_STRING(pszDomainDnsName);
    LW_SAFE_FREE_STRING(pszHostDnsDomain);

    return dwError;
	
error:

    if (pdwGoodUntilTime != NULL)
    {
        *pdwGoodUntilTime = 0;
    }
    goto cleanup;
}

DWORD
LwTranslateKrb5Error(
    krb5_context ctx,
    krb5_error_code krbError,
    PCSTR pszFile,
    DWORD dwLine
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCSTR pszKrb5Error = NULL;

    if (ctx)
    {
        pszKrb5Error = krb5_get_error_message(ctx, krbError);
    }
    if (pszKrb5Error)
    {
        LW_LOG_ERROR("KRB5 Error at %s:%d: [Code:%d] [Message: %s]",
                pszFile,
                dwLine,
                krbError,
                pszKrb5Error);
    }
    else
    {
        LW_LOG_ERROR("KRB5 Error at %s:%d [Code:%d]",
                pszFile,
                dwLine,
                krbError);
    }

    switch (krbError)
    {
        case KRB5KDC_ERR_KEY_EXP:
            dwError = LW_ERROR_PASSWORD_EXPIRED;
            break;
        case KRB5_LIBOS_BADPWDMATCH:
            dwError = LW_ERROR_PASSWORD_MISMATCH;
            break;
        case KRB5KRB_AP_ERR_SKEW:
            dwError = LW_ERROR_CLOCK_SKEW;
            break;
        case KRB5KDC_ERR_CLIENT_REVOKED:
            dwError = LW_ERROR_ACCOUNT_DISABLED;
            break;
        case ENOENT:
            dwError = LW_ERROR_KRB5_NO_KEYS_FOUND;
            break;
        case KRB5KDC_ERR_PREAUTH_FAILED:
            dwError = LW_ERROR_PASSWORD_MISMATCH;
            break;
        case KRB5KDC_ERR_C_PRINCIPAL_UNKNOWN:
            dwError = LW_ERROR_INVALID_ACCOUNT;            
            break;
        case KRB5KDC_ERR_S_PRINCIPAL_UNKNOWN:
            dwError = LW_ERROR_KRB5_S_PRINCIPAL_UNKNOWN;
            break;            
        default:
            dwError = LW_ERROR_KRB5_CALL_FAILED;
            break;
    }

    if (pszKrb5Error)
    {
        krb5_free_error_message(ctx, pszKrb5Error);
    }
    return dwError;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
