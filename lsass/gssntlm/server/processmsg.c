/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        processmsg.c
 *
 * Abstract:
 *
 *        Code for processing NTLM authenticate messages
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "server.h"


void
NTLMComputeV1SessionKey(
    ULONG negFlags,
    NTLM_OWF ntOWF,
    PSEC_BUFFER_S serverChallenge,
    PSEC_BUFFER_S clientChallenge,
    PSEC_BUFFER_S baseSessionKey
)
{
    HMAC_CTX        hmacCtxt;
    DWORD           mdLen;
    UCHAR           key[16];
    SEC_BUFFER_S    test;


    MD4(ntOWF, 16, key);

    INIT_SEC_BUFFER_S(&test, 16);
    memcpy(test.buffer, key, 16);

    DBGDumpSecBufferS(D_ERROR, "owf sessionkey", &test); 
    if (negFlags & NEGOTIATE_NTLM2)
    {
        HMAC_CTX_init(&hmacCtxt);
        HMAC_Init_ex(
            &hmacCtxt, 
            key,
            16,
            EVP_md5(),
            NULL
        );

        HMAC_Update(&hmacCtxt, serverChallenge->buffer, 8);
        HMAC_Update(&hmacCtxt, clientChallenge->buffer, 8);
        HMAC_Final(&hmacCtxt, key, &mdLen);
        HMAC_CTX_cleanup(&hmacCtxt);
    }

    INIT_SEC_BUFFER_S(baseSessionKey, 16);
    memcpy(baseSessionKey->buffer, key, 16);
    DBGDumpSecBufferS(D_ERROR, "sessionkey", baseSessionKey); 
}

/*
 * NTLMV1ComputeNTLMResponse()
 *
 * @brief Crypt handler for computing NTLMv1 response.
 *
 * @param in pctxt - context - used for flags
 * @param in pChallenge - challenge to process
 * @param in pResponse - 
 *                          of the challenge messge, with buffer
 *                          relative to the base of the containing message.
 */
DWORD
NTLMComputeNTLMv1Response(
    PAUTH_USER user,
    ULONG negFlags,
    BOOLEAN client,
    PSEC_BUFFER_S serverChallenge,
    PSEC_BUFFER_S baseSessionKey,
    PSEC_BUFFER ntResponse,
    PSEC_BUFFER_S lmResponse 
    )
{
    DWORD dwError = 0;
    NTLM_OWF ntOWF;
    BYTE deskeys[21];
    MD5_CTX md5ctx;
    UCHAR challenge[MD5_DIGEST_LENGTH];

    /* 
     * if we're doing NTLM2 keys, with ntlmv1 protocol, 
     * we generate a client challenge, and store it in
     * the lm response.
     */
    if (negFlags & NEGOTIATE_NTLM2) 
    {
        /* client side - otherwise supplied as fn arg */
        if (client)
        {
            INIT_SEC_BUFFER_S(lmResponse, 24);
            dwError = NTLMCryptGenRandomBytes(lmResponse->buffer, 8);
            BAIL_ON_NTLM_ERROR(dwError);
        }

        MD5_Init(&md5ctx);
        MD5_Update(&md5ctx, serverChallenge->buffer, 8);
        MD5_Update(&md5ctx, lmResponse->buffer, 8);
        MD5_Final(challenge, &md5ctx);
    }
    else
        memcpy(challenge, serverChallenge->buffer, 8);

    ntResponse->length = ntResponse->maxLength = NTLM_V1_RESPONSE_LENGTH;
    ntResponse->buffer = (PBYTE) NTLMAllocateMemory(NTLM_V1_RESPONSE_LENGTH);
    if (!ntResponse->buffer)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    /* get the user's owf */
    dwError = user->provider->getNTOwf(
                    user,
                    &ntOWF
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    NTLMComputeV1SessionKey(
        negFlags,
        ntOWF, 
        serverChallenge,
        lmResponse, /* aka client challenge */
        baseSessionKey
        );

    /* 
     * recipe - null pad 16 byte value to 21 bytes, 
     * split into 3 7 byte keys, DES encrypt + concat 3 results into
     * 24 byte value.
     */ 
    memset(deskeys, 0, sizeof(deskeys));
    memcpy(deskeys, ntOWF, 16);
    NTLMCryptHashChallenge(
        &deskeys[0], 
        challenge,
        &ntResponse->buffer[0]
        );

    NTLMCryptHashChallenge(
        &deskeys[7], 
        challenge,
        &ntResponse->buffer[8]
        );

    NTLMCryptHashChallenge(
        &deskeys[14], 
        challenge,
        &ntResponse->buffer[16]
        );

    if ((negFlags & NEGOTIATE_NTLM2) == 0)
    {
        /* 
         * @todo - verify LM response being copy of NT response is right 
         * thing to do.
         */
        INIT_SEC_BUFFER_S_VAL(
            lmResponse, 
            ntResponse->length,
            ntResponse->buffer 
            );
    }

error:

    return dwError;
}

DWORD
NTLMComputeV2Hash(
    PAUTH_USER authUser,
    NTLM_OWF ntv2OWF
)
{
    DWORD           dwError = 0;
    unsigned int    mdLen = sizeof(NTLM_OWF);
    NTLM_OWF        ntv1OWF;
    HMAC_CTX        hmacCtxt;
    LSA_STRING      upperUser;
    LSA_STRING      upperDomain;

    ZERO_STRUCT(upperUser);
    ZERO_STRUCT(upperDomain);

    /* build v2 OWF */
    /* get the user's owf */
    dwError = authUser->provider->getNTOwf(
                    authUser,
                    &ntv1OWF
                    );

    BAIL_ON_NTLM_ERROR(dwError);
    
    dwError = LsaCopyLsaString(
                    &upperUser,
                    &authUser->user
                    );

    BAIL_ON_NTLM_ERROR(dwError);
                    
    dwError = LsaCopyLsaString(
                    &upperDomain,
                    &authUser->domain
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    LsaUpperCaseLsaString(&upperUser);
    LsaUpperCaseLsaString(&upperDomain);

   /* do the hashing */
    HMAC_CTX_init(&hmacCtxt);
    HMAC_Init_ex(
        &hmacCtxt, 
        ntv1OWF,
        16,
        EVP_md5(),
        NULL
        );

    HMAC_Update(
        &hmacCtxt, 
        (UCHAR*) upperUser.buffer,
        upperUser.length
        );

    HMAC_Update(
        &hmacCtxt, 
        (UCHAR*) upperDomain.buffer,
        upperDomain.length
        );

    HMAC_Final(&hmacCtxt, ntv2OWF, &mdLen);
    HMAC_CTX_cleanup(&hmacCtxt);

error:

    LsaFreeLsaString(&upperUser);
    LsaFreeLsaString(&upperDomain);

    return dwError;
}

DWORD
NTLMHashNTLMv2ResponseBlob(
    PAUTH_USER authUser,
    PSEC_BUFFER_S serverChallenge,
    PSEC_BUFFER_S clientChallenge,
    PSEC_BUFFER_S baseSessionKey,
    PSEC_BUFFER ntResponseBlob,
    PSEC_BUFFER_S lmResponseBlob
)
{
    DWORD           dwError = 0;
    DWORD           mdLen = MD5_DIGEST_LENGTH;
    NTLM_OWF        ntv2OWF;
    HMAC_CTX        hmacCtxt;

    /* create v2 hash */
    dwError = NTLMComputeV2Hash(
                authUser,
                ntv2OWF
                );

    BAIL_ON_NTLM_ERROR(dwError);

   /* do the hashing */
    HMAC_CTX_init(&hmacCtxt);
    HMAC_Init_ex(
        &hmacCtxt, 
        ntv2OWF,
        16,
        EVP_md5(),
        NULL
        );

    HMAC_Update(
        &hmacCtxt, 
        (UCHAR*) serverChallenge->buffer, 
        serverChallenge->length
        );

    HMAC_Update(
        &hmacCtxt, 
        &ntResponseBlob->buffer[MD5_DIGEST_LENGTH],
        ntResponseBlob->length - MD5_DIGEST_LENGTH
        );

    HMAC_Final(&hmacCtxt,ntResponseBlob->buffer, &mdLen);
    HMAC_CTX_cleanup(&hmacCtxt);


    /* session key = HMAC of HMAC */
    HMAC(
        EVP_md5(), 
        ntv2OWF, 
        16, 
        (const UCHAR*) ntResponseBlob->buffer, 
        MD5_DIGEST_LENGTH, 
        baseSessionKey->buffer,
        &mdLen); 

    baseSessionKey->length = baseSessionKey->maxLength = mdLen;

    /* LM response is also filled in, as follows */
    INIT_SEC_BUFFER_S(lmResponseBlob, 24);

    HMAC_CTX_init(&hmacCtxt);
    HMAC_Init_ex(
        &hmacCtxt, 
        ntv2OWF,
        16,
        EVP_md5(),
        NULL
        );

    HMAC_Update(
        &hmacCtxt, 
        (UCHAR*) serverChallenge->buffer, 
        serverChallenge->length
        );

    HMAC_Update(
        &hmacCtxt, 
        (UCHAR*) clientChallenge->buffer,
        clientChallenge->length
        );

    HMAC_Final(&hmacCtxt,lmResponseBlob->buffer, &mdLen);
    HMAC_CTX_cleanup(&hmacCtxt);

    memcpy(&lmResponseBlob->buffer[16], clientChallenge->buffer, 8);

error:

    return dwError;
}

DWORD
NTLMCreateNTLMv2ResponseBlob(
    PSEC_BUFFER_S clientChallenge,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER responseBlob
)
{
    DWORD dwError = 0;
    UINT64 ntTime;
    ULONG ofs = MD5_DIGEST_LENGTH;
    DWORD dwLen = targetInfo->length + sizeof(NTLMV2_RESPONSE_BLOB)
        + sizeof(ULONG);

    /*
     * Format of response blob
     *
     * HMAC
     * NTLMV2_RESPONSE_BLOB
     * Target Info
     * 4 bytes of zero's
     */
    responseBlob->buffer = NTLMAllocateMemory(dwLen);
    if (!responseBlob->buffer)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    responseBlob->length = responseBlob->maxLength = dwLen;

    NTLMGetNTTime(&ntTime);

    /* header */
    PUT_ULONG(responseBlob->buffer, ofs, NTLMV2_RESPONSE_BLOB_VERSION);
    ofs += sizeof(ULONG);

    /* zeros */
    PUT_ULONG(responseBlob->buffer, ofs, 0);
    ofs += sizeof(ULONG);

    /* time */
    PUT_ULONG(responseBlob->buffer, ofs, (ntTime & 0xFFFFFFFF));
    ofs += sizeof(ULONG);
    PUT_ULONG(responseBlob->buffer, ofs, (ntTime >> 32));
    ofs += sizeof(ULONG);
    
    /* client challenge */
    PUT_BYTES(
        responseBlob->buffer, 
        ofs, 
        clientChallenge->buffer, 
        clientChallenge->length);
    
    ofs += clientChallenge->length;

    /* zeros */
    PUT_ULONG(responseBlob->buffer, ofs, 0);
    ofs += sizeof(ULONG);

    /* target info */
    memcpy(&responseBlob->buffer[ofs], targetInfo->buffer, targetInfo->length);
    ofs += targetInfo->length;

    /* more zeros */
    PUT_ULONG(responseBlob->buffer, ofs, 0);
    ofs += sizeof(ULONG);

error:

    return dwError;
}


DWORD
NTLMComputeNTLMv2Response(
    PAUTH_USER user,
    PSEC_BUFFER_S serverChallenge,
    PSEC_BUFFER_S baseSessionKey,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER ntResponse,
    PSEC_BUFFER_S lmResponse
)
{
    DWORD           dwError ;
    SEC_BUFFER      ntResponseBlob;
    SEC_BUFFER_S    clientChallenge;

    ZERO_STRUCT(ntResponseBlob);
    INIT_SEC_BUFFER_S(&clientChallenge, 8);

    dwError = NTLMCryptGenRandomBytes(clientChallenge.buffer, 8);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMCreateNTLMv2ResponseBlob(
                    &clientChallenge,
                    targetInfo,
                    &ntResponseBlob
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMHashNTLMv2ResponseBlob(
                    user,
                    serverChallenge,
                    &clientChallenge,
                    baseSessionKey,
                    &ntResponseBlob,
                    lmResponse
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    memcpy(ntResponse, &ntResponseBlob, sizeof(SEC_BUFFER));
    ntResponseBlob.buffer = NULL;

error:

    NTLM_SAFE_FREE(ntResponseBlob.buffer);
    return dwError;
}

BOOLEAN
NTLMUseNTLMV2( void )
{
    /* @todo - this should be tied into group policy */
    /* @todo - alternately, invoke through sub-oid */
    return FALSE;
}


DWORD
NTLMBuildResponse(
    PAUTH_USER authUser,
    ULONG negFlags,
    PSEC_BUFFER_S serverChallenge,
    PSEC_BUFFER_S baseSessionKey,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER ntResponse,
    PSEC_BUFFER_S lmResponse
)
{
    DWORD dwError = 0;

    /* @todo - real policy here */
    if (NTLMUseNTLMV2())
    {
        dwError = NTLMComputeNTLMv2Response(
                        authUser,
                        serverChallenge,
                        baseSessionKey,
                        targetInfo,
                        ntResponse,
                        lmResponse
                        );
    } 
    else 
    {

        dwError = NTLMComputeNTLMv1Response(
                        authUser,
                        negFlags,
                        true, /*client*/
                        serverChallenge,
                        baseSessionKey,
                        ntResponse,
                        lmResponse
                        );
    }
    
    BAIL_ON_NTLM_ERROR(dwError);

error:

    return dwError;
}

/*
 * NTLMComputeSubSessionKey
 *
 * @brief Wrapper for sub session key
 *
 */
DWORD
NTLMComputeSubSessionKey(
    ULONG negFlags,
    SEC_BUFFER_S *subSessionKey,
    SEC_BUFFER_S *baseSessionKey,
    BOOLEAN client
)
{
    DWORD dwError = 0;
    SEC_BUFFER_S randomSessionKey;

    INIT_SEC_BUFFER_S(&randomSessionKey, 16);

    if (negFlags & NEGOTIATE_KEY_EXCH) 
    { 
        if (client)
        {
            dwError = NTLMCryptGenRandomBytes(
                            randomSessionKey.buffer, 
                            randomSessionKey.length
                            );

            BAIL_ON_NTLM_ERROR(dwError); 

            memcpy(subSessionKey, &randomSessionKey, sizeof(SEC_BUFFER_S));
        }

        /* for server, we  use session key from authenticate msg */
        NTLMCryptRC4Bytes(
            baseSessionKey->buffer, 
            subSessionKey->buffer, 
            subSessionKey->length
            ); 

        if (client)
            memcpy(baseSessionKey, &randomSessionKey, sizeof(SEC_BUFFER_S));
        else
            memcpy(baseSessionKey, subSessionKey, sizeof(SEC_BUFFER_S));
    } 

error:

    return dwError;
}
        

void
NTLMFreeAuthenticateMessage(PAUTHENTICATE_MESSAGE msg)
{
    NTLM_SAFE_FREE(msg->sessionKey.buffer);
    NTLM_SAFE_FREE(msg->workstation.buffer);
    NTLM_SAFE_FREE(msg->userName.buffer);
    NTLM_SAFE_FREE(msg->domainName.buffer);
    NTLM_SAFE_FREE(msg->ntResponse.buffer);
    NTLM_SAFE_FREE(msg->lmResponse.buffer);
}

DWORD
NTLMParseAuthenticateMessage(
    PSEC_BUFFER pBuf,
    PAUTHENTICATE_MESSAGE pAuthMsg
)
{
    DWORD dwError = 0;
    ULONG ofs = 0;

    ZERO_STRUCTP(pAuthMsg);

    if (pBuf->length < sizeof(AUTHENTICATE_MESSAGE))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INSUFFICIENT_BUFFER);

    dwError = NTLMParseMessageHeader(
                    pBuf, 
                    &ofs,
                    AUTHENTICATE_MSG
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    /* LM Response */
    dwError = NTLMGetSecBuffer(&pAuthMsg->lmResponse, pBuf, &ofs); 
    BAIL_ON_NTLM_ERROR(dwError);

    /* NT Response */
    dwError = NTLMGetSecBuffer(&pAuthMsg->ntResponse, pBuf, &ofs); 
    BAIL_ON_NTLM_ERROR(dwError);

    /* domain */
    dwError = NTLMGetLsaString(&pAuthMsg->domainName, pBuf, &ofs);
    BAIL_ON_NTLM_ERROR(dwError);

    /* user */
    dwError = NTLMGetLsaString(&pAuthMsg->userName, pBuf, &ofs);
    BAIL_ON_NTLM_ERROR(dwError);

    /* wks */
    dwError = NTLMGetLsaString(&pAuthMsg->workstation, pBuf, &ofs);
    BAIL_ON_NTLM_ERROR(dwError);

    /* enc key */
    dwError = NTLMGetSecBuffer(&pAuthMsg->sessionKey, pBuf, &ofs); 
    BAIL_ON_NTLM_ERROR(dwError);

    if (ofs + sizeof(ULONG) > pBuf->length)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INSUFFICIENT_BUFFER);

    pAuthMsg->negotiateFlags = GET_ULONG(pBuf->buffer, ofs);

error:

    return dwError;
}



VOID
NTLMGssFreeSecBuffer(PSEC_BUFFER buf)
{
    if (!buf)
        return;

    NTLM_SAFE_FREE(buf->buffer);
    memset(buf, 0, sizeof(SEC_BUFFER));
}


DWORD
NTLMGssBuildAuthenticateMessage(
    ULONG negFlags,
    uid_t uid,
    PSEC_BUFFER marshaledCredential,
    PSEC_BUFFER_S serverChallenge,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER pOutputToken,
    PSEC_BUFFER_S finalSessionKey
    )
{
    DWORD dwError = 0;
    DWORD dwMessageSize;
    AUTH_USER authUser;
    ULONG ofs = 0;
    ULONG bufofs = sizeof(AUTHENTICATE_MESSAGE);
    SEC_BUFFER ntResponse;
    SEC_BUFFER_S baseSessionKey;
    SEC_BUFFER_S subSessionKey;
    SEC_BUFFER_S lmResponse;
    LSA_STRING workstationName;
    PBYTE authenticateMessage = NULL;
    PAUTH_PROVIDER provider;

    ZERO_STRUCT(workstationName);
    ZERO_STRUCT(ntResponse);
    ZERO_STRUCT(authUser);

    INIT_SEC_BUFFER_S(&lmResponse, 0);
    INIT_SEC_BUFFER_S(&baseSessionKey, 16);
    INIT_SEC_BUFFER_S(&subSessionKey, 16);

    /* get the user information, either from marshaled cred, or db */
    if (marshaledCredential->length)
    {
        dwError = NTLMUnMarshalAuthUser(
                    marshaledCredential,
                    &authUser
                    );

        BAIL_ON_NTLM_ERROR(dwError);

        provider = NTLMSelectAuthProvider(&authUser);
        if (!provider)
            BAIL_WITH_NTLM_ERROR(LSA_ERROR_NO_SUCH_USER);
    }
    else
    {
        dwError = NTLMGetAuthUserFromUid(
                    uid,
                    &authUser
                    );

        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* first build NT response and session key, so we can allocate enuf buf */
    dwError = NTLMBuildResponse(
                    &authUser,
                    negFlags,
                    serverChallenge,
                    &baseSessionKey,
                    targetInfo,
                    &ntResponse,
                    &lmResponse
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    if (negFlags & NEGOTIATE_KEY_EXCH) 
    { 
        dwError = NTLMComputeSubSessionKey(
                    negFlags,
                    &subSessionKey,
                    &baseSessionKey,
                    true
                    );

        BAIL_ON_NTLM_ERROR(dwError);

    }

    dwError = NTLMGetWorkstationName(&workstationName);

    BAIL_ON_NTLM_ERROR(dwError);

    dwMessageSize = sizeof(AUTHENTICATE_MESSAGE) +
        authUser.user.length + 
        authUser.domain.length + 
        ntResponse.length +
        lmResponse.length +
        subSessionKey.length + 
        workstationName.length;  

    /* leave room for version struct */
    if (negFlags & NEGOTIATE_VERSION_DEBUG) 
    {
        bufofs += NTLM_VERSION_SIZE;
        dwMessageSize += NTLM_VERSION_SIZE;
    }

    authenticateMessage = NTLMAllocateMemory(dwMessageSize);

    if (!authenticateMessage)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    PUT_BYTES(authenticateMessage, 0, signature, NTLM_SIGNATURE_SIZE);
    ofs += NTLM_SIGNATURE_SIZE;

    PUT_ULONG(authenticateMessage, ofs, AUTHENTICATE_MSG);
    ofs += sizeof(ULONG);

    /* 
     * LM response 
     *
     * Don't do LM - this is either a copy of the ntResponse, an NTLMv2 
     * supplmental LMv2 response, or if we are
     * using a NTLM2 key with NTLMv1, the client challenge (0 padded).
     */
    NTLMPutSecBufferS(&lmResponse, authenticateMessage, &bufofs, &ofs);

    /* NT response */
    NTLMPutSecBuffer(&ntResponse, authenticateMessage, &bufofs, &ofs);

    /* domain name */
    NTLMPutLsaString(&authUser.domain, authenticateMessage, &bufofs, &ofs);

    /* user name */
    NTLMPutLsaString(&authUser.user, authenticateMessage, &bufofs, &ofs);

    /* workstation */
    NTLMPutLsaString(&workstationName, authenticateMessage, &bufofs, &ofs);

    /* session key */
    NTLMPutSecBufferS(&subSessionKey, authenticateMessage, &bufofs, &ofs);

    /* neg flags */
    PUT_ULONG(authenticateMessage, ofs, negFlags);
    ofs += sizeof(ULONG);

    /* debug version info - send 2k3 version */
    if (negFlags & NEGOTIATE_VERSION_DEBUG) 
    {
        PUT_BYTES(authenticateMessage, ofs, version, NTLM_VERSION_SIZE);
        ofs += NTLM_VERSION_SIZE;
    } 

    pOutputToken->buffer = authenticateMessage;
    pOutputToken->length = pOutputToken->maxLength = dwMessageSize;
    authenticateMessage = NULL;

    DBGDumpSecBuffer(D_ERROR, "authenticate msg", pOutputToken); 

    memcpy(finalSessionKey, &baseSessionKey, sizeof(SEC_BUFFER_S));

error:
    
    NTLMFreeAuthUser(&authUser);
    NTLM_SAFE_FREE(authenticateMessage);
    LsaFreeLsaString(&workstationName);

    return dwError;
}

DWORD
NTLMLocalResponseMessageHandler(
    PAUTH_USER      user,
    ULONG           negFlags,
    PSEC_BUFFER_S   serverChallenge,
    PSEC_BUFFER     ntResponse,
    PSEC_BUFFER_S   lmResponse,
    PSEC_BUFFER_S   msgSessionKey,
    PSEC_BUFFER_S   finalSessionKey
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    SEC_BUFFER expected;
    SEC_BUFFER_S baseSessionKey;
    SEC_BUFFER_S clientChallenge;
    NTLMV2_RESPONSE_BLOB v2Response;

    ZERO_STRUCT(expected);
    ZERO_STRUCT(v2Response);
    INIT_SEC_BUFFER_S(&baseSessionKey, 16);

    if (ntResponse->length > NTLM_V1_RESPONSE_LENGTH)
    {
        //
        // NTLMv2 Response
        //

        if (ntResponse->length < sizeof(NTLMV2_RESPONSE_BLOB))
            BAIL_WITH_NTLM_ERROR(LSA_ERROR_LOGON_FAILURE);

        memcpy(&v2Response, ntResponse->buffer, sizeof(NTLMV2_RESPONSE_BLOB));
        INIT_SEC_BUFFER_S_VAL(&clientChallenge, 8, v2Response.clientChallenge); 
        dwError = NTLMHashNTLMv2ResponseBlob(
                        user,
                        serverChallenge,
                        &clientChallenge,
                        &baseSessionKey,
                        ntResponse,
                        lmResponse
                        );

        BAIL_ON_NTLM_ERROR(dwError);

        if (memcmp(ntResponse->buffer, v2Response.hmac, MD5_DIGEST_LENGTH))
        {
            /*@todo - better logging */
            BAIL_WITH_NTLM_ERROR(LSA_ERROR_LOGON_FAILURE);
        }
        
        /*@todo -verify lmResponse */

    }
    else
    {
        //
        // NTLMv1 Response
        //

        /* compute, and validate NTLMv1 challenge */
        dwError = NTLMComputeNTLMv1Response(
                        user,
                        negFlags,
                        false, /*server - client challenge supplied*/
                        serverChallenge,
                        &baseSessionKey,
                        &expected,
                        lmResponse
                        );

        BAIL_ON_NTLM_ERROR(dwError);

        if (memcmp(expected.buffer, ntResponse->buffer, expected.length)) 
        {
            /* @todo debug */
            BAIL_WITH_NTLM_ERROR(LSA_ERROR_LOGON_FAILURE);
        }
    }

    if (negFlags & NEGOTIATE_KEY_EXCH) 
    { 
        dwError = NTLMComputeSubSessionKey(
                    negFlags,
                    msgSessionKey,
                    &baseSessionKey,
                    true
                    );

        BAIL_ON_NTLM_ERROR(dwError);
    }

    memcpy(finalSessionKey, &baseSessionKey, sizeof(SEC_BUFFER_S));

error:

    NTLM_SAFE_FREE(expected.buffer);
    return dwError;

}


DWORD
NTLMGssCheckAuthenticateMessage(
    ULONG negFlags,
    PSEC_BUFFER_S serverChallenge,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER authenticateMessageToken,
    PSEC_BUFFER_S baseSessionKey 
    )

{
    DWORD dwError = 0;
    AUTHENTICATE_MESSAGE authenticateMessage;
    SEC_BUFFER_S msgSessionKey;
    SEC_BUFFER_S lmResponse;
    AUTH_USER authUser;
    PAUTH_PROVIDER provider;

    ZERO_STRUCT(authenticateMessage);
    ZERO_STRUCT(authUser);

    dwError = NTLMParseAuthenticateMessage(
                authenticateMessageToken,
                &authenticateMessage
                );

    BAIL_ON_NTLM_ERROR(dwError);

    if ((negFlags & authenticateMessage.negotiateFlags) != negFlags)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_MESSAGE);

    /* 
     * store away user name / domain name 
     * 
     * this allows us to pass message over secure
     * channel, or do it locally
     */
    dwError = NTLMInitializeAuthUser(
                    &authenticateMessage.userName,
                    &authenticateMessage.domainName,
                    NULL,
                    0,
                    &authUser
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    provider = NTLMSelectAuthProvider(&authUser);
    if (!provider)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_NO_SUCH_USER);

    INIT_SEC_BUFFER_S_VAL(
        &msgSessionKey, 
        authenticateMessage.sessionKey.length,
        authenticateMessage.sessionKey.buffer
        );

    INIT_SEC_BUFFER_S_VAL(
        &lmResponse,
        authenticateMessage.lmResponse.length,
        authenticateMessage.lmResponse.buffer
        );

    /* validate response, get session key */
    dwError = authUser.provider->responseMessageHandler(
                &authUser,
                negFlags,
                serverChallenge,
                &authenticateMessage.ntResponse,
                &lmResponse,
                &msgSessionKey,
                baseSessionKey
                );

    BAIL_ON_NTLM_ERROR(dwError);
    
error:

    NTLMFreeAuthenticateMessage(&authenticateMessage);
    NTLMFreeAuthUser(&authUser);

    return dwError;

}
