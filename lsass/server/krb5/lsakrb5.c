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
 *        lsakrb5.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Kerberos 5 API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak <rszczesniak@likewisesoftware.com>
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#include "lsakrb.h"
#include "lsakrb5_p.h"
#include <lber.h>
#include "lwps/lwps.h"

DWORD
LsaKrb5Init(
    IN OPTIONAL LSA_KRB5_REALM_IS_OFFLINE_CALLBACK pfIsOfflineCallback,
    IN OPTIONAL LSA_KRB5_REALM_TRANSITION_OFFLINE_CALLBACK pfTransitionOfflineCallback
    )
{
    DWORD dwError = 0;
    
    dwError = pthread_mutex_init(&gLsaKrb5State.ExistingClientLock, NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pthread_mutex_init(&gLsaKrb5State.UserCacheMutex, NULL);
    BAIL_ON_LSA_ERROR(dwError);

    gLsaKrb5State.pfIsOfflineCallback = pfIsOfflineCallback;
    gLsaKrb5State.pfTransitionOfflineCallback = pfTransitionOfflineCallback;

cleanup:

    return dwError;
    
error:

    LSA_LOG_ERROR("Error: Failed to initialize Krb5. [Error code: %d]", dwError);

    goto cleanup;
}

DWORD
LsaKrb5GetDefaultRealm(
    PSTR* ppszRealm
    )
{
    DWORD dwError = 0;
    krb5_context ctx = NULL;
    PSTR pszKrb5Realm = NULL;
    PSTR pszRealm = NULL;

    krb5_init_context(&ctx);
    krb5_get_default_realm(ctx, &pszKrb5Realm);

    if (IsNullOrEmptyString(pszKrb5Realm)) {
        dwError = LSA_ERROR_NO_DEFAULT_REALM;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateString(pszKrb5Realm, &pszRealm);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszRealm = pszRealm;
    
cleanup:

    if (pszKrb5Realm){
        krb5_free_realm_string(ctx,pszKrb5Realm);
    }

    krb5_free_context(ctx);

    return(dwError);

error:

    *ppszRealm = NULL;
    
    LSA_SAFE_FREE_STRING(pszRealm);

    goto cleanup;
}


DWORD
LsaKrb5GetSystemCachePath(
    Krb5CacheType cacheType,
    PSTR*         ppszCachePath
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = NULL;
    
    switch (cacheType)
    {
        case KRB5_InMemory_Cache:
            
            dwError = LsaAllocateString(
                        "MEMORY:krb5cc_lsass",
                        &pszCachePath);
            BAIL_ON_LSA_ERROR(dwError);
            
            break;
            
        case KRB5_File_Cache:
            
            dwError = LsaAllocateString(
                        "FILE:/var/lib/likewise/krb5cc_lsass",
                        &pszCachePath);
            BAIL_ON_LSA_ERROR(dwError);
            
            break;
            
        default:
            
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
            
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
LsaKrb5GetUserCachePath(
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
            
            dwError = LsaAllocateStringPrintf(
                        &pszCachePath,
                        "MEMORY:krb5cc_%ld",
                        (long)uid);
            BAIL_ON_LSA_ERROR(dwError);
            
            break;
            
        case KRB5_File_Cache:
            
            dwError = LsaAllocateStringPrintf(
                        &pszCachePath,
                        "FILE:/tmp/krb5cc_%ld",
                        (long)uid);
            BAIL_ON_LSA_ERROR(dwError);
            
            break;
            
        default:
            
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
            
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
LsaKrb5SetDefaultCachePath(
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
        if (!IsNullOrEmptyString(pszOrigCachePath)) {
            dwError = LsaAllocateString(pszOrigCachePath, ppszOrigCachePath);
            BAIL_ON_LSA_ERROR(dwError);
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
LsaKrb5GetSystemKeytabPath(
    PSTR* ppszKeytabPath
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    PSTR pszPath = NULL;
    size_t size = 64;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    do {
        LSA_SAFE_FREE_STRING(pszPath);

        size *= 2;
        dwError = LsaAllocateMemory(size, (PVOID*)&pszPath);
        BAIL_ON_LSA_ERROR(dwError);

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
    LSA_SAFE_FREE_STRING(pszPath);
    *ppszKeytabPath = NULL;
    
    goto cleanup;
}

DWORD
LsaKrb5SetProcessDefaultCachePath(
    PCSTR pszCachePath
    )
{
    DWORD dwError = 0;
    PSTR pszEnvironmentEntry = NULL;
    static volatile PSTR pszSavedEnvironmentEntry = NULL;

    // ISSUE-2008/07/18-dalmeida -- Better atomicity.

    if (pszSavedEnvironmentEntry)
    {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateStringPrintf(&pszEnvironmentEntry,
                                      "KRB5CCNAME=%s",
                                      pszCachePath);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * putenv requires that the buffer not be free'd.
     */
    if (putenv(pszEnvironmentEntry) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pszSavedEnvironmentEntry = pszEnvironmentEntry;
    pszEnvironmentEntry = NULL;

error:
    LSA_SAFE_FREE_STRING(pszEnvironmentEntry);
    
    return dwError;
}

DWORD
LsaSetupMachineSession(
    PCSTR  pszMachname,
    PCSTR  pszSamAccountName,
    PCSTR  pszPassword,
    PCSTR  pszRealm,
    PCSTR  pszDomain,
    PDWORD pdwGoodUntilTime
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszHostKeytabFile = NULL;
    PSTR pszKrb5CcPath = NULL;
    PSTR pszDomname = NULL;
    PSTR pszRealmCpy = NULL;
    PSTR pszMachPrincipal = NULL;
    DWORD dwGoodUntilTime = 0;

    dwError = LsaKrb5GetSystemKeytabPath(&pszHostKeytabFile);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaKrb5GetSystemCachePath(KRB5_File_Cache, &pszKrb5CcPath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(pszRealm, &pszRealmCpy);
    BAIL_ON_LSA_ERROR(dwError);
    LsaStrToUpper(pszRealmCpy);

    dwError = LsaAllocateStringPrintf(&pszMachPrincipal, "%s@%s",
                                      pszSamAccountName, pszRealm);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(pszDomain, &pszDomname);
    BAIL_ON_LSA_ERROR(dwError);
    LsaStrToLower(pszDomname);

    dwError = LsaKrb5GetTgt(
    		      pszMachPrincipal,
                  pszPassword,
                  pszKrb5CcPath,
                  &dwGoodUntilTime);
    BAIL_ON_LSA_ERROR(dwError);

    if (pdwGoodUntilTime)
    {
    	*pdwGoodUntilTime = dwGoodUntilTime;
    }

cleanup:

    LSA_SAFE_FREE_STRING(pszMachPrincipal);
    LSA_SAFE_FREE_STRING(pszDomname);
    LSA_SAFE_FREE_STRING(pszRealmCpy);
    LSA_SAFE_FREE_STRING(pszKrb5CcPath);
    LSA_SAFE_FREE_STRING(pszHostKeytabFile);
    
    return (dwError);
    
error:

    if (pdwGoodUntilTime)
    {
    	*pdwGoodUntilTime = 0;
    }

	goto cleanup;
}

DWORD
LsaKrb5CleanupMachineSession(
    VOID
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszKrb5CcPath = NULL;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;

    dwError = LsaKrb5GetSystemCachePath(KRB5_File_Cache, &pszKrb5CcPath);
    BAIL_ON_LSA_ERROR(dwError);

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
    LSA_SAFE_FREE_STRING(pszKrb5CcPath);

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
        LSA_LOG_ERROR("DCE Error [Code:%d]", (status));   \
        (dest) = LSA_ERROR_DCE_CALL_FAILED;               \
        goto error;                                       \
    }

#define PAC_TYPE_LOGON_INFO              1
#define PAC_TYPE_SRV_CHECKSUM            6
#define PAC_TYPE_KDC_CHECKSUM            7
#define PAC_TYPE_LOGON_NAME             10
#define PAC_TYPE_CONSTRAINED_DELEGATION 11

typedef struct _PAC_BUFFER {
    DWORD dwType;
    DWORD dwSize;
    uint64_t qwOffset;
} PAC_BUFFER;

typedef struct _PAC_DATA {
    DWORD dwBufferCount;
    DWORD dwVersion;
    PAC_BUFFER buffers[1];
} PAC_DATA;

typedef struct _PAC_SIGNATURE_DATA {
    DWORD dwType;
    // Goes until the end of the buffer (defined in PAC_DATA)
    char pchSignature[1];
} PAC_SIGNATURE_DATA;

typedef struct _PAC_LOGON_NAME {
    NtTime ticketTime;
    WORD wAccountNameLen;
    wchar16_t pwszName[1];
} PAC_LOGON_NAME;

DWORD
LsaKrb5DecodePac(
    krb5_context ctx,
    const krb5_ticket *pTgsTicket,
    const struct berval *pPacBerVal,
    const krb5_keyblock *serviceKey,
    PAC_LOGON_INFO **ppLogonInfo
    )
{
    krb5_error_code ret = 0;
    PAC_DATA *pPacData = NULL;
    DWORD i;
    char *pchPacCopy = NULL;
    //Do not free
    krb5_data krbPacData = {0};
    //Do not free
    krb5_checksum checksum = {0};
    //Do not free
    PAC_SIGNATURE_DATA *pServerSig = NULL;
    PAC_LOGON_NAME *pLogonName = NULL;
    size_t sServerSig = 0;
    //Do not free
    char *pchLogonInfoStart = NULL;
    size_t sLogonInfoLen = 0;
    krb5_boolean bHasGoodChecksum = FALSE;
    error_status_t dceStatus = 0;
    uint64_t qwNtAuthTime;
    DWORD dwError = LSA_ERROR_SUCCESS;
    //Free with krb5_free_unparsed_name
    PSTR pszClientPrincipal = NULL;
    PSTR pszLogonName = NULL;
    PAC_LOGON_INFO *pLogonInfo = NULL;

    #if defined(WORDS_BIGENDIAN)
    WORD * pwNameLocal = NULL;
    DWORD dwCount = 0;
    #endif

    dwError = LsaAllocateMemory(
                pPacBerVal->bv_len,
                (PVOID*)&pPacData);
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pPacData, pPacBerVal->bv_val, pPacBerVal->bv_len);

    #if defined(WORDS_BIGENDIAN)
        pPacData->dwBufferCount = LW_ENDIAN_SWAP32(pPacData->dwBufferCount);
        pPacData->dwVersion = LW_ENDIAN_SWAP32(pPacData->dwVersion);
    #endif

    // We only know about version 0
    if (pPacData->dwVersion != 0)
    {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    // Make sure that the last buffer in the pac data doesn't go out of bounds
    // of the parent buffer
    if ((void *)&pPacData->buffers[pPacData->dwBufferCount] -
            (void *)pPacData > pPacBerVal->bv_len)
    {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Make sure the data associated with each buffer doesn't go out of
    // bounds
    for (i = 0; i < pPacData->dwBufferCount; i++)
    {
        #if defined(WORDS_BIGENDIAN)
            pPacData->buffers[i].dwType = LW_ENDIAN_SWAP32(pPacData->buffers[i].dwType);
            pPacData->buffers[i].dwSize = LW_ENDIAN_SWAP32(pPacData->buffers[i].dwSize);
            pPacData->buffers[i].qwOffset = LW_ENDIAN_SWAP64(pPacData->buffers[i].qwOffset);
        #endif

        if (pPacData->buffers[i].qwOffset + pPacData->buffers[i].dwSize <
                pPacData->buffers[i].qwOffset)
        {
            dwError = LSA_ERROR_INVALID_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
        if (pPacData->buffers[i].qwOffset + pPacData->buffers[i].dwSize >
                pPacBerVal->bv_len)
        {
            dwError = LSA_ERROR_INVALID_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = LsaAllocateMemory(
                pPacBerVal->bv_len,
                (PVOID*)&pchPacCopy);
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pchPacCopy, pPacBerVal->bv_val, pPacBerVal->bv_len);

    krbPacData.magic = KV5M_DATA;
    krbPacData.length = pPacBerVal->bv_len;
    krbPacData.data = pchPacCopy;

    for (i = 0; i < pPacData->dwBufferCount; i++)
    {
    	switch (pPacData->buffers[i].dwType)
    	{
    	    case PAC_TYPE_LOGON_INFO:
    	        pchLogonInfoStart = (char *)pPacData + pPacData->buffers[i].qwOffset;
                sLogonInfoLen = pPacData->buffers[i].dwSize;
                break;
            case PAC_TYPE_SRV_CHECKSUM:
                pServerSig = (PAC_SIGNATURE_DATA *)((char *)pPacData +
                             pPacData->buffers[i].qwOffset);

                #if defined(WORDS_BIGENDIAN)
                    pServerSig->dwType = LW_ENDIAN_SWAP32(pServerSig->dwType);
                #endif

                sServerSig = pPacData->buffers[i].dwSize -
                        (size_t)&((PAC_SIGNATURE_DATA *)0)->pchSignature;
                /* The checksum is calculated with the signatures zeroed out. */
                memset(pchPacCopy + pPacData->buffers[i].qwOffset +
                       (size_t)&((PAC_SIGNATURE_DATA *)0)->pchSignature,
                       0,
                       pPacData->buffers[i].dwSize -
                           (size_t)&((PAC_SIGNATURE_DATA *)0)->pchSignature);
                break;
            case PAC_TYPE_KDC_CHECKSUM:
                /* The checksum is calculated with the signatures zeroed out. */
    		memset(pchPacCopy + pPacData->buffers[i].qwOffset +
    	               (size_t)&((PAC_SIGNATURE_DATA *)0)->pchSignature,
    		       0,
    		       pPacData->buffers[i].dwSize -
    		           (size_t)&((PAC_SIGNATURE_DATA *)0)->pchSignature);
    		break;
            case PAC_TYPE_LOGON_NAME:
                pLogonName = (PAC_LOGON_NAME *)((char *)pPacData +
                             pPacData->buffers[i].qwOffset);

                #if defined(WORDS_BIGENDIAN)
                    pLogonName->ticketTime = LW_ENDIAN_SWAP64(pLogonName->ticketTime);
                    pLogonName->wAccountNameLen = LW_ENDIAN_SWAP16(pLogonName->wAccountNameLen);
                    pwNameLocal = pLogonName->pwszName;

                    for ( dwCount = 0 ;
                          dwCount < pLogonName->wAccountNameLen / 2 ;
                          dwCount++ )
                    {
                        pwNameLocal[dwCount] = LW_ENDIAN_SWAP16(pwNameLocal[dwCount]);
                    }
                #endif

                if ((char *)&pLogonName->pwszName +
                    pLogonName->wAccountNameLen >
                    (char *)pPacData + pPacData->buffers[i].qwOffset +
                    pPacData->buffers[i].dwSize)
                {
                    // The message is invalid because the terminating null
                    // of the name lands outside of the buffer.
                    dwError = LSA_ERROR_INVALID_MESSAGE;
                    BAIL_ON_LSA_ERROR(dwError);
                }
                break;
            default:
                break;
    	}
    }

    if (pServerSig == NULL)
    {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pLogonName == NULL)
    {
        //We need the logon name to verify the pac is for the right user
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pchLogonInfoStart == NULL)
    {
        /* The buffer we really care about isn't in the pac. */
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    checksum.magic = KV5M_CHECKSUM;
    checksum.checksum_type = pServerSig->dwType;
    checksum.length = sServerSig;
    checksum.contents = (unsigned char *)pServerSig->pchSignature;

    ret = krb5_c_verify_checksum(
                    ctx,
                    serviceKey,
                    KRB5_KEYUSAGE_APP_DATA_CKSUM,
                    &krbPacData,
                    &checksum,
                    &bHasGoodChecksum);
    BAIL_ON_KRB_ERROR(ctx, ret);

    if (!bHasGoodChecksum)
    {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Make sure the pac was issued with this ticket, not an old ticket
    qwNtAuthTime = pTgsTicket->enc_part2->times.authtime;
    qwNtAuthTime += 11644473600LL;
    qwNtAuthTime *= 1000*1000*10;
    if (pLogonName->ticketTime != qwNtAuthTime)
    {
        dwError = LSA_ERROR_CLOCK_SKEW;
        BAIL_ON_LSA_ERROR(dwError);
    }
    ret = krb5_unparse_name(
                    ctx,
                    pTgsTicket->enc_part2->client,
                    &pszClientPrincipal);
    BAIL_ON_KRB_ERROR(ctx, ret);

    // Strip off the domain name
    if (strchr(pszClientPrincipal, '@') != NULL)
    {
        strchr(pszClientPrincipal, '@')[0] = '\0'; 
    }

    dwError = LsaWc16snToMbs(
        pLogonName->pwszName,
        &pszLogonName,
        pLogonName->wAccountNameLen / 2);
    BAIL_ON_LSA_ERROR(dwError);    

    if (strcasecmp(pszClientPrincipal, pszLogonName))
    {
        // The pac belongs to a different user
        dwError = LSA_ERROR_INVALID_LOGIN_ID;
        BAIL_ON_LSA_ERROR(dwError);    
    }

    dceStatus = DecodePacLogonInfo(
        pchLogonInfoStart,
        sLogonInfoLen,
        &pLogonInfo);
    BAIL_ON_DCE_ERROR(dwError, dceStatus);

    *ppLogonInfo = pLogonInfo;

cleanup:
    LSA_SAFE_FREE_STRING(pszLogonName);
    LSA_SAFE_FREE_MEMORY(pPacData);
    LSA_SAFE_FREE_MEMORY(pchPacCopy);
    if (pszClientPrincipal != NULL)
    {
        krb5_free_unparsed_name(ctx, pszClientPrincipal);
    }
    return dwError;

error:
    if (pLogonInfo != NULL)
    {
        FreePacLogonInfo(pLogonInfo);
    }
    *ppLogonInfo = NULL;
    goto cleanup;
}

DWORD
LsaKrb5FindPac(
    krb5_context ctx,
    const krb5_ticket *pTgsTicket,
    const krb5_keyblock *serviceKey,
    PAC_LOGON_INFO **ppLogonInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    //Do not free
    struct berval bv = {0};
    struct berval contents = {0};
    //Do not free
    krb5_authdata **ppCur = NULL;
    //Do not free associated buffer
    BerElement *ber = NULL;
    ber_tag_t tag = 0;
    ber_len_t len = 0;
    // Do not free
    char *cookie = NULL;
    int adType;
    ber_tag_t seqTag, context0Tag, context1Tag;
    PAC_LOGON_INFO *pLogonInfo = NULL;
    
    ber = ber_alloc_t(0);
    
    if (pTgsTicket && pTgsTicket->enc_part2)
    {
        ppCur = pTgsTicket->enc_part2->authorization_data;
    }

    while (ppCur && (*ppCur != NULL))
    {
        if (ppCur[0]->ad_type == AD_IF_RELEVANT_TYPE)
        {
            // This auth data contains a DER encoded sequence of more
            // auth data. One of them could be a pac.
            bv.bv_len = ppCur[0]->length;
            bv.bv_val = (char *)ppCur[0]->contents;
            ber_init2(ber, &bv, 0);

            tag = ber_first_element(ber, &len, &cookie);
            while (tag != LBER_ERROR)
            {
                // Free does nothing if pointer is NULL
                ber_memfree(contents.bv_val);
                contents.bv_val = NULL;

                tag = ber_scanf(ber,
                        "t{t[i]t[",
                        &seqTag,
                        &context0Tag,
                        &adType,
                        &context1Tag);
                if (tag == LBER_ERROR)
                {
                    // This auth data is invalid. Skip it and try
                    // the next one
                    break;
                }
                tag = ber_scanf(ber,
                        "o]}",
                        &contents);
                if (tag == LBER_ERROR)
                {
                    // This auth data is invalid. Skip it and try
                    // the next one
                    break;
                }

                if (adType == AD_WIN2K_PAC)
                {
                    dwError = LsaKrb5DecodePac(
                        ctx,
                        pTgsTicket,
                        &contents,
                        serviceKey,
                        &pLogonInfo);
                    if (dwError == LSA_ERROR_INVALID_MESSAGE)
                    {
                        dwError = LSA_ERROR_SUCCESS;
                        continue;
                    }
                    BAIL_ON_LSA_ERROR(dwError);
                    // Found a good PAC !
                    goto end_search;
                }

                //returns LBER_ERROR when there are no more elements left.
                tag = ber_next_element(ber, &len, cookie);
            }
        }

        ppCur++;
    }
end_search:

    *ppLogonInfo = pLogonInfo;

cleanup:
    if (contents.bv_val != NULL)
    {
        ber_memfree(contents.bv_val);
    }
    if (ber != NULL)
    {
        ber_free(ber, 0);
    }

    return dwError;

error:
    if (pLogonInfo != NULL)
    {
        FreePacLogonInfo(pLogonInfo);
    }
    *ppLogonInfo = NULL;
    goto cleanup;
}

DWORD
LsaKrb5CopyFromUserCache(
                krb5_context ctx,
                krb5_ccache destCC,
                uid_t uid
                )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
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

    dwError = LsaKrb5GetUserCachePath(
                    uid,
                    KRB5_File_Cache,
                    &pszCachePath);
    BAIL_ON_LSA_ERROR(dwError);
    
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

    LSA_SAFE_FREE_STRING(pszCachePath);

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
LsaKrb5MoveCCacheToUserPath(
    krb5_context ctx,
    PCSTR pszNewCacheName,
    uid_t uid,
    gid_t gid
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR  pszCachePath = NULL;
    PCSTR  pszCachePathReal = NULL;

    dwError = LsaKrb5GetUserCachePath(
                    uid,
                    KRB5_File_Cache,
                    &pszCachePath);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (strncasecmp(pszCachePath, "FILE:", sizeof("FILE:")-1)) {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    } else {
        pszCachePathReal = pszCachePath + sizeof("FILE:") - 1;
    }

    dwError = LsaMoveFile(pszNewCacheName,
                pszCachePathReal);
    BAIL_ON_LSA_ERROR(dwError);

    /* Let the user read and write to their cache file (before this, only
     * root was allowed to read and write the file).
     */
    dwError = LsaChangeOwnerAndPermissions(
                pszCachePathReal,
                uid,
                gid,
                S_IRWXU);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_STRING(pszCachePath);
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaSetupUserLoginSession(
    uid_t uid,
    gid_t gid,
    PCSTR pszUsername,
    PCSTR pszPassword,
    BOOLEAN bUpdateUserCache,
    PCSTR pszServicePrincipal,
    PCSTR pszServicePassword,
    PAC_LOGON_INFO **ppLogonInfo,
    PDWORD pdwGoodUntilTime
    )
{
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;
    // Free with krb5_free_cred_contents
    krb5_creds credsRequest = {0};
    krb5_creds *pTgsCreds = NULL;
    krb5_ticket *pTgsTicket = NULL;
    krb5_ticket *pDecryptedTgs = NULL;
    krb5_auth_context authContext = NULL;
    krb5_data apReqPacket = {0};
    krb5_keyblock serviceKey = {0};
    krb5_data salt = {0};
    // Do not free
    krb5_data machinePassword = {0};
    krb5_flags flags = 0;
    krb5_int32 authcon_flags = 0;
    PAC_LOGON_INFO *pLogonInfo = NULL;
    BOOLEAN bInLock = FALSE;
    PCSTR pszTempCacheName = NULL;
    PSTR pszTempCachePath = NULL;
    PSTR pszUnreachableRealm = NULL;

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

    dwError = LsaKrb5GetTgt(
            pszUsername,
            pszPassword,
            krb5_cc_get_name(ctx, cc),
            pdwGoodUntilTime
            );
    BAIL_ON_LSA_ERROR(dwError);

    ret = krb5_parse_name(ctx, pszServicePrincipal, &credsRequest.server);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_get_principal(ctx, cc, &credsRequest.client);
    BAIL_ON_KRB_ERROR(ctx, ret);
 
    /* Get a TGS for our service using the tgt in the cache */
    ret = krb5_get_credentials(
            ctx,
            0, /*no options (not user to user encryption,
                 and not only cached) */
            cc,
            &credsRequest,
            &pTgsCreds);
    if (KRB5_KDC_UNREACH == ret)
    {
        // ISSUE-2008/09/22-dalmeida -- I think that we do not
        // necessarily know the domain here because there
        // may be been some traversal issue in a trust scenario.
        // It may be ok to just transition the user's domain offline since
        // logons from that domain will have problems.
        // Figuring out how to solve this issue so that we
        // can transition the proper domain offline is left as
        // a future enhancement.  One way would be to save off realm from
        // the error message (see krb5_sendto_kdc).  However, that
        // would be dependent of the version of Kerberos being used.
        pszUnreachableRealm = NULL;
    }
    BAIL_ON_KRB_ERROR(ctx, ret);

    //No need to store the tgs in the cc. Kerberos does that automatically

    /* Generate an ap_req message, but don't send it anywhere. Just decode it
     * immediately. This is the only way to get kerberos to decrypt the tgs
     * using public APIs */
    ret = krb5_mk_req_extended(
            ctx,
            &authContext,
            0, /* no options necessary */
            NULL, /* since this isn't a real ap_req, we don't have any
                     supplemental data to send with it. */
            pTgsCreds,
            &apReqPacket);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* Decode (but not decrypt) the tgs ticket so that we can figure out
     * which encryption type was used in it. */
    ret = krb5_decode_ticket(&pTgsCreds->ticket, &pTgsTicket);

    /* The TGS ticket is encrypted with the machine password and salted with
     * the service principal. pszServicePrincipal could probably be used
     * directly, but it's safer to unparse pTgsCreds->server, because the KDC
     * sent that to us.
     */
    salt.magic = KV5M_DATA;
    ret = krb5_unparse_name(
            ctx,
            pTgsCreds->server,
            &salt.data);
    BAIL_ON_KRB_ERROR(ctx, ret);
    salt.length = strlen(salt.data);

    machinePassword.magic = KV5M_DATA;
    machinePassword.data = (PSTR)pszServicePassword,
    machinePassword.length = strlen(pszServicePassword),

    /* Generate a key to decrypt the TGS */
    ret = krb5_c_string_to_key(
            ctx,
            pTgsTicket->enc_part.enctype,
	    &machinePassword,
            &salt,
            &serviceKey);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* Typically krb5_rd_req would decode the AP_REQ using the keytab, but
     * we don't want to depend on the keytab. As a side effect of kerberos'
     * user to user authentication support, if a key is explictly set on the
     * auth context, that key will be used to decrypt the TGS instead of the
     * keytab.
     *
     * By manually generating the key and setting it, we don't require
     * a keytab.
     */
    if (authContext != NULL)
    {
        ret = krb5_auth_con_free(ctx, authContext);
        BAIL_ON_KRB_ERROR(ctx, ret);
    }
    
    ret = krb5_auth_con_init(ctx, &authContext);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_auth_con_setuseruserkey(
            ctx,
            authContext,
            &serviceKey);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* Disable replay detection which is unnecessary and
     * can fail when authenticating large numbers of users.
     */
    krb5_auth_con_getflags(ctx,
                           authContext,
                           &authcon_flags);
    krb5_auth_con_setflags(ctx,
                           authContext,
                           authcon_flags & ~KRB5_AUTH_CONTEXT_DO_TIME);

    /* This decrypts the TGS. As a side effect it ensures that the KDC that
     * the user's TGT came from is in the same realm that the machine was
     * joined to (this prevents users from spoofing the KDC).
     */
    ret = krb5_rd_req(
            ctx,
            &authContext,
            &apReqPacket,
            pTgsCreds->server,
            NULL, /* we're not using the keytab */
            &flags,
	    &pDecryptedTgs);
    BAIL_ON_KRB_ERROR(ctx, ret);

    dwError = LsaKrb5FindPac(
        ctx,
        pDecryptedTgs,
        &serviceKey,
        &pLogonInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (bUpdateUserCache)
    {
        /* 1. Copy old credentials from the existing user creds cache to
         *      the temporary cache.
         * 2. Delete the existing creds cache.
         * 3. Move the temporary cache file into the final path.
         */
        dwError = pthread_mutex_lock(&gLsaKrb5State.UserCacheMutex);
        BAIL_ON_LSA_ERROR(dwError);
        bInLock = TRUE;

        dwError = LsaKrb5CopyFromUserCache(
                    ctx,
                    cc,
                    uid
                    );
        BAIL_ON_LSA_ERROR(dwError);

        pszTempCacheName = krb5_cc_get_name(ctx, cc);
        if (!strncasecmp(pszTempCacheName, "FILE:", sizeof("FILE:")-1)) {
            pszTempCacheName += sizeof("FILE:") - 1;
        }

        dwError = LsaAllocateString(pszTempCacheName, &pszTempCachePath);
        BAIL_ON_LSA_ERROR(dwError);

        krb5_cc_close(ctx, cc);
        // Just to make sure no one accesses this now invalid pointer
        cc = NULL;

        dwError = LsaKrb5MoveCCacheToUserPath(
                    ctx,
                    pszTempCachePath,
                    uid,
                    gid);
        if (dwError != LSA_ERROR_SUCCESS)
        {
            /* Let the user login, even if we couldn't create the ccache for
             * them. Possible causes are:
             * 1. /tmp is readonly
             * 2. Another user maliciously setup a weird file (such as a
             *    directory) where the ccache would go.
             * 3. Someone created a ccache in the small window after we delete
             *    the old one and before we move in the new one.
             */
            LSA_LOG_WARNING("Unable to set up credentials cache with tgt for uid %ld", (long)uid);
            dwError = LsaRemoveFile(pszTempCachePath);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *ppLogonInfo = pLogonInfo;
    
cleanup:
    LSA_SAFE_FREE_STRING(pszUnreachableRealm);
    if (ctx)
    {
        // This function skips fields which are NULL
        krb5_free_cred_contents(ctx, &credsRequest);
    
        if (pTgsCreds != NULL)
        {
            krb5_free_creds(ctx, pTgsCreds);
        }
        
        if (pTgsTicket != NULL)
        {
            krb5_free_ticket(ctx, pTgsTicket);
        }
        
        if (pDecryptedTgs != NULL)
        {
            krb5_free_ticket(ctx, pDecryptedTgs);
        }
        
        if (authContext != NULL)
        {
            krb5_auth_con_free(ctx, authContext);
        }
        
        krb5_free_data_contents(ctx, &apReqPacket);
        krb5_free_data_contents(ctx, &salt);
        krb5_free_keyblock_contents(ctx, &serviceKey);

        if (cc != NULL)
        {
            krb5_cc_destroy(ctx, cc);
        }
        krb5_free_context(ctx);
    }
    if (bInLock)
    {
        pthread_mutex_unlock(&gLsaKrb5State.UserCacheMutex);
    }
    LSA_SAFE_FREE_STRING(pszTempCachePath);

    return dwError;
    
error:
    if ((LSA_ERROR_KRB5_CALL_FAILED == dwError) &&
        (KRB5_KDC_UNREACH == ret))
    {
        if (pszUnreachableRealm)
        {
            LsaKrb5RealmTransitionOffline(pszUnreachableRealm);
        }
        dwError = LSA_ERROR_DOMAIN_IS_OFFLINE;
    }

    if (pLogonInfo != NULL)
    {
        FreePacLogonInfo(pLogonInfo);
    }
    *ppLogonInfo = NULL;
    
    goto cleanup;
}

DWORD
LsaKrb5Shutdown(
    VOID
    )
{
    DWORD dwError = 0;

    gLsaKrb5State.pfIsOfflineCallback = NULL;
    gLsaKrb5State.pfTransitionOfflineCallback = NULL;

    dwError = pthread_mutex_destroy(&gLsaKrb5State.ExistingClientLock);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pthread_mutex_destroy(&gLsaKrb5State.UserCacheMutex);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    return dwError;
    
error:

    LSA_LOG_ERROR("Error: Failed to shutdown Krb5. [Error code: %d]", dwError);

    goto cleanup;
}

DWORD
LsaKrb5GetMachineCreds(
    PCSTR pszHostname,
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
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwpsGetPasswordByHostName(
                    hPasswordStore,
                    pszHostname,
                    &pMachineAcctInfo);
    if (dwError)
    {
        LSA_LOG_ERROR("Unable to read machine password for hostname '%s'",
            LSA_SAFE_LOG_STRING(pszHostname));
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaWc16sToMbs(
                    pMachineAcctInfo->pwszMachineAccount,
                    &pszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaWc16sToMbs(
                    pMachineAcctInfo->pwszMachinePassword,
                    &pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaWc16sToMbs(
                    pMachineAcctInfo->pwszDnsDomainName,
                    &pszDomainDnsName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWc16sToMbs(
                    pMachineAcctInfo->pwszHostDnsDomain,
                    &pszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (IsNullOrEmptyString(pszUsername)) {
        dwError = LSA_ERROR_INVALID_ACCOUNT;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (IsNullOrEmptyString(pszPassword)) {
        dwError = LSA_ERROR_INVALID_PASSWORD;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (IsNullOrEmptyString(pszUsername)) {
        dwError = LSA_ERROR_INVALID_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
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
    
    LSA_SAFE_FREE_STRING(pszUsername);
    LSA_SAFE_FREE_STRING(pszPassword);
    LSA_SAFE_FREE_STRING(pszDomainDnsName);
    LSA_SAFE_FREE_STRING(pszHostDnsDomain);

    goto cleanup;
}

DWORD
LsaKrb5RefreshMachineTGT(
    PDWORD pdwGoodUntilTime
    )
{
    DWORD dwError = 0;
    DWORD dwGoodUntilTime = 0;
    PSTR  pszUsername = NULL;
    PSTR  pszPassword = NULL;
    PSTR  pszDomainDnsName = NULL;
    PSTR  pszHostDnsDomain = NULL;
    PSTR pszHostname = NULL;

    LSA_LOG_VERBOSE("Refreshing machine TGT");

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrToUpper(pszHostname);

    dwError = LsaKrb5GetMachineCreds(
                    pszHostname,
                    &pszUsername,
                    &pszPassword,
                    &pszDomainDnsName,
                    &pszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);
	
    dwError = LsaSetupMachineSession(
                    pszHostname,
                    pszUsername,
                    pszPassword,
                    pszDomainDnsName,
                    pszHostDnsDomain,
                    &dwGoodUntilTime);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (pdwGoodUntilTime != NULL)
    {
        *pdwGoodUntilTime = dwGoodUntilTime;
    }
	
cleanup:

    LSA_SAFE_FREE_STRING(pszHostname);
    LSA_SAFE_FREE_STRING(pszUsername);
    LSA_SAFE_FREE_STRING(pszPassword);
    LSA_SAFE_FREE_STRING(pszDomainDnsName);
    LSA_SAFE_FREE_STRING(pszHostDnsDomain);

    return dwError;
	
error:

    if (pdwGoodUntilTime != NULL)
    {
        *pdwGoodUntilTime = 0;
    }
    goto cleanup;
}

DWORD
LsaTranslateKrb5Error(
    krb5_context ctx,
    krb5_error_code krbError,
    PCSTR pszFile,
    DWORD dwLine
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PCSTR pszKrb5Error = NULL;

    if (ctx)
    {
        pszKrb5Error = krb5_get_error_message(ctx, krbError);
    }
    if (pszKrb5Error)
    {
        LSA_LOG_ERROR("KRB5 Error at %s:%d: [Code:%d] [Message: %s]",
                pszFile,
                dwLine,
                krbError,
                pszKrb5Error);
    }
    else
    {
        LSA_LOG_ERROR("KRB5 Error at %s:%d [Code:%d]",
                pszFile,
                dwLine,
                krbError);
    }

    switch (krbError)
    {
        case KRB5KDC_ERR_KEY_EXP:
            dwError = LSA_ERROR_PASSWORD_EXPIRED;
            break;
        case KRB5_LIBOS_BADPWDMATCH:
            dwError = LSA_ERROR_PASSWORD_MISMATCH;
            break;
        case KRB5KRB_AP_ERR_SKEW:
            dwError = LSA_ERROR_CLOCK_SKEW;
            break;
        case KRB5KDC_ERR_CLIENT_REVOKED:
            dwError = LSA_ERROR_ACCOUNT_DISABLED;
            break;
        case ENOENT:
            dwError = LSA_ERROR_KRB5_NO_KEYS_FOUND;
            break;
        case KRB5KDC_ERR_PREAUTH_FAILED:
            dwError = LSA_ERROR_PASSWORD_MISMATCH;
            break;
        case KRB5KDC_ERR_C_PRINCIPAL_UNKNOWN:
            dwError = LSA_ERROR_INVALID_ACCOUNT;
            break;
        default:
            dwError = LSA_ERROR_KRB5_CALL_FAILED;
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
