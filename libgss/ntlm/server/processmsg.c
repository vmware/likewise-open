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
 *        Code for processing NTLM messages - core of the protocol
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "server.h"

static const UCHAR signature[NTLM_SIGNATURE_SIZE] = NTLM_SIGNATURE;
    
DWORD
NTLMParseMessageHeader(
    PSEC_BUFFER pbuf,
    ULONG *ofs,
    DWORD expectedMsg
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    UCHAR cmp[NTLM_SIGNATURE_SIZE] = NTLM_SIGNATURE;
    UCHAR tmp[NTLM_SIGNATURE_SIZE];

    if ((*ofs) + sizeof(ULONG) + NTLM_SIGNATURE_SIZE >= pbuf->length)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INSUFFICIENT_BUFFER);

    GET_BYTES(tmp, (*ofs), pbuf->buffer, NTLM_SIGNATURE_SIZE);
    *ofs += NTLM_SIGNATURE_SIZE;

    if (memcmp(tmp, cmp, NTLM_SIGNATURE_SIZE))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_MESSAGE);

    if (expectedMsg != GET_ULONG(pbuf->buffer, (*ofs)))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_MESSAGE);

    (*ofs) += sizeof(ULONG);

error:

    /* @todo debug */
    return dwError;
}

void
NTLMDumpNegotiateMessage(
    DWORD lvl,
    PNTLM_CONTEXT pCtxt,
    PNEGOTIATE_MESSAGE negMsg
    )
{
    if ((lvl & db_level) == 0)
        return;

    DBG(lvl,("Negotiate Message - ctxt(0x%p)\n", pCtxt));
    DBG(lvl,("Neg Flags (0x%lx)\n", negMsg->negotiateFlags));

    /* @todo DumpNegotiateFlags(); */
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
    PNTLM_CONTEXT pCtxt,
    PUCHAR challenge,
    PSEC_BUFFER pNTResponse
    )
{
    DWORD dwError;
    PAUTH_USER pUser = &pCtxt->cred->authUser;
    NTLM_OWF ntOWF;
    BYTE deskeys[21];

    pNTResponse->length = pNTResponse->maxLength = NTLM_V1_RESPONSE_LENGTH;
    pNTResponse->buffer = (PBYTE) NTLMAllocateMemory(NTLM_V1_RESPONSE_LENGTH);
    if (!pNTResponse->buffer)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    /* get the user's owf */
    dwError = pUser->provider->getNTOwf(
                    pUser,
                    &ntOWF
                    );

    BAIL_ON_NTLM_ERROR(dwError);

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
        &pNTResponse->buffer[0]
        );

    NTLMCryptHashChallenge(
        &deskeys[7], 
        challenge,
        &pNTResponse->buffer[8]
        );

    NTLMCryptHashChallenge(
        &deskeys[14], 
        challenge,
        &pNTResponse->buffer[16]
        );

error:

    return dwError;
}

DWORD
NTLMComputeNTLMv2Response(
    PNTLM_CONTEXT pCtxt,
    PUCHAR challenge,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER response
)
{
    /* @todo */
    return (LSA_ERROR_NOT_IMPLEMENTED);
}

BOOLEAN
NTLMUseNTLMV2(
    PNTLM_CONTEXT pCtxt
)
{
    return FALSE;
}


DWORD
NTLMBuildResponse(
    PNTLM_CONTEXT pCtxt,
    PUCHAR challenge,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER ntResponse
)
{
    DWORD dwError;

    /* @todo - real policy here */
    if (NTLMUseNTLMV2(pCtxt))
    {
        dwError = NTLMComputeNTLMv2Response(
                        pCtxt,
                        challenge,
                        targetInfo,
                        ntResponse
                        );
    }
    else
    {
        dwError = NTLMComputeNTLMv1Response(
                        pCtxt,
                        challenge,
                        ntResponse
                        );
    }
    
    BAIL_ON_NTLM_ERROR(dwError);

    /* dump response */
error:

    return dwError;

}

DWORD
NTLMComputeV2Hash(
    NTLM_OWF ntv1OWF,
    NTLM_OWF ntv2OWF,
    PAUTH_USER authUser
)
{

    DWORD dwError;
    unsigned int mdlen;
    LSA_STRING s;

    s.length = s.max =  authUser->user.length + 
        authUser->domain.length;

    s.buffer = NTLMAllocateMemory(s.length);
    if (!s.buffer)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    /* step 1 - concat user-domain, upper case */
    memcpy(s.buffer, authUser->user.buffer, authUser->user.length); 
    memcpy(&s.buffer[authUser->user.length], authUser->domain.buffer,
        authUser->domain.length); 
    
    NTLMUpperCase(&s);

    /* step 2 - hmac-md5 using v1 hash as key */
    HMAC(EVP_md5(), ntv1OWF, 16, (const UCHAR*) s.buffer, s.length, ntv2OWF, &mdlen); 

error:

    NTLM_SAFE_FREE(s.buffer);
    return dwError;
}

void
NTLMComputeV1SessionKey(
    NTLM_OWF ntOWF,
    PUCHAR key
)
{
    MD4(ntOWF, 16, key);
}
    

/*
 * NTLMComputeSessionKey
 *
 * @brief Wrapper for all key derivation schemes.
 *
 * @param in pctxt - context 
 * @param in sk - encrypted session key from authenticate message,
 *                
 */
DWORD
NTLMComputeSessionKey(
    PNTLM_CONTEXT pCtxt,
    PUCHAR serverChallenge,
    SEC_BUFFER_S *clientChallenge,
    SEC_BUFFER_S *sessionKey,
    BOOLEAN client
)
{
    DWORD dwError;
    unsigned int md5len;
    ULONG negFlags;
    NTLM_OWF ntOWF;
    UCHAR entropy[16];
    UCHAR mk[16];
    PAUTH_USER pUser = &pCtxt->cred->authUser;

    NTLMSelectAuthProvider(pUser);

    dwError = pUser->provider->getNTOwf(
                    pUser,
                    &ntOWF
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    negFlags = NTLMContextGetNegotiateFlags(pCtxt);
    
    /* we don't support cheap keys */
    if ((negFlags & NEGOTIATE_56) || 
        (negFlags & NEGOTIATE_LM_KEY))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_NOT_SUPPORTED);

    /* ntlmv1 key */
    NTLMComputeV1SessionKey(ntOWF, mk);

    /* I don't think this is applicable for NTLMv2, just the NTLM2 key */
    /* @todo - figure this out for NTLMv2 */
    if (negFlags & NEGOTIATE_NTLM2)
    {
        memcpy(entropy, serverChallenge, 8);
        memcpy(&entropy[8], clientChallenge->buffer, 8);
        HMAC(EVP_md5(), mk, 16, entropy, 16, mk, &md5len); 
    }


    /* 
     * if key_exch is set, our "real" key is a random key, encrypted
     * with the OWF derivable user key.
     */
    if (negFlags & NEGOTIATE_KEY_EXCH)
    { 
        if (client) 
        {
            dwError = NTLMCryptGenRandomBytes(
                            sessionKey->buffer, 
                            sessionKey->length
                            );

            BAIL_ON_NTLM_ERROR(dwError); 
        }

        /* for server, we must already have session key */
        /*assert(sessionKey && sessionKey->length);*/
        NTLMCryptRC4Bytes(mk, sessionKey->buffer, sessionKey->length); 
    }
    else
        memcpy(sessionKey->buffer, mk, 16);

    NTLM_LOCK_CONTEXTS(pCtxt);
    memcpy(pCtxt->baseKey.buffer, sessionKey->buffer, 16);
    /* dump keys */
    NTLM_UNLOCK_CONTEXTS(pCtxt);

error:

    return dwError;
}
        


/*
 * NTLMBuildTargetInfo()
 *
 * @brief If the client is requesting target info, build 
 * it here.  For standalone / non-joined machines, we can only
 * offer local names.  For domain joined, we should also
 * include netbios and dns names (if requested).
 *
 * @todo - this should be global, and tied into change notification 
 * scheme domain, server name changes.  Right now, we compute every time.
 *
 * @param out targetInfo -  secbuffer (absolute), to be tacked on the end
 *                          of the challenge messge, with buffer
 *                          relative to the base of the containing message.
 */

static DWORD
NTLMBuildTargetInfo(
    PSEC_BUFFER targetInfoOut
    )
{
    DWORD dwError;
    DWORD dwSize;
    PBYTE copyTo;
    DWORD ofs = 0;

    SEC_BUFFER targetInfo;
    LSA_STRING server;
    LSA_STRING serverDNSName;
    LSA_STRING dnsDomain;
    LSA_STRING nbDomain;

    ZERO_STRUCT(targetInfo);
    ZERO_STRUCT(server);
    ZERO_STRUCT(serverDNSName);
    ZERO_STRUCT(dnsDomain);
    ZERO_STRUCT(nbDomain);

    dwError = NTLMGetWorkstationName(&server);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMGetDNSWorkstationName(&serverDNSName);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMGetDNSDomainName(&dnsDomain);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMGetNBDomainName(&nbDomain);
    BAIL_ON_NTLM_ERROR(dwError);

    dwSize = server.length + serverDNSName.length + 
        dnsDomain.length + nbDomain.length + (5 * sizeof(USHORT));

    targetInfo.buffer = NTLMAllocateMemory(dwSize);
    if (!targetInfo.buffer)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    targetInfo.length = targetInfo.maxLength = dwSize;
    copyTo = targetInfo.buffer;

    PUT_USHORT(copyTo,ofs,NAME_TYPE_DOMAIN);
    ofs += sizeof(USHORT);
    PUT_BYTES(copyTo, ofs, nbDomain.buffer, nbDomain.length);
    ofs += nbDomain.length;
    
    PUT_USHORT(copyTo,ofs,NAME_TYPE_SERVER);
    ofs += sizeof(USHORT);
    PUT_BYTES(copyTo, ofs, server.buffer, server.length);
    ofs += server.length;

    PUT_USHORT(copyTo,ofs,NAME_TYPE_DOMAIN_DNS);
    ofs += sizeof(USHORT);
    PUT_BYTES(copyTo, ofs, dnsDomain.buffer, dnsDomain.length);
    ofs += dnsDomain.length;

    PUT_USHORT(copyTo,ofs,NAME_TYPE_SERVER_DNS);
    ofs += sizeof(USHORT);
    PUT_BYTES(copyTo, ofs, serverDNSName.buffer, serverDNSName.length);
    ofs += serverDNSName.length;

    PUT_USHORT(copyTo,ofs,NAME_TYPE_END);

    memcpy(targetInfoOut, &targetInfo, sizeof(SEC_BUFFER));
    targetInfo.buffer = NULL;

error:

    NTLM_SAFE_FREE(targetInfo.buffer);
    NTLM_SAFE_FREE(server.buffer);
    NTLM_SAFE_FREE(nbDomain.buffer);
    NTLM_SAFE_FREE(serverDNSName.buffer);
    NTLM_SAFE_FREE(dnsDomain.buffer);

    return dwError;
}

void
NTLMFreeTargetInfo(PSEC_BUFFER targetInfo)
{
    if (!targetInfo)
        return;

    NTLM_SAFE_FREE(targetInfo->buffer);

}

DWORD
NTLMServerValidateNegotiateFlags(
    PNTLM_CONTEXT pCtxt,
    ULONG negotiateFlags,
    ULONG *newFlags
    )
{
    /* eventually the default should be altered via policy / oids */
    ULONG flags = ( NEGOTIATE_SRV_DEFAULT & negotiateFlags );
    ULONG dwError = LSA_ERROR_SUCCESS;

    /* remove flags which we support, but client doesn't want */
    /* right now, only do NTLM, no LM */
    if ((flags & NEGOTIATE_NTLM) == 0)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_UNSUPPORTED_SUBPROTO);

    /* @todo - we need to eventually support ANSI, or fail here
     * if we aren't provided a UNICODE flags */
    if (flags & NEGOTIATE_UNICODE)
        flags &= ~NEGOTIATE_OEM;
    else
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_UNSUPPORTED_SUBPROTO);

    /* store these flags in context */
    NTLM_LOCK_CONTEXT(pCtxt);
    pCtxt->negotiateFlags = flags;
    NTLM_UNLOCK_CONTEXT(pCtxt);

    /* add new flags, specifically for challenge msg */
    if (flags & NEGOTIATE_REQUEST_TARGET)
        flags |= (NEGOTIATE_REQUEST_TARGET | CHALLENGE_TARGET_INFO);

    *newFlags = flags;

    /* @todo debug ? */

error:

    return dwError;
}

DWORD
NTLMFinalizedMessage(
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    )
{

    /* @todo debug */
    /* this message SHOULD NEVER be called */
    return LSA_ERROR_INVALID_CONTEXT;
}


/*
 * NEGOTIATE
 */
DWORD
NTLMBuildNegotiateMessage( 
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    )
{
    DWORD dwError;
    DWORD ofs = 0;
    PNEGOTIATE_MESSAGE negotiateMsg = NULL;

    /* @todo what should the input token have (mechs?)? */

    negotiateMsg = (PNEGOTIATE_MESSAGE)
        NTLMAllocateMemory(sizeof(NEGOTIATE_MESSAGE));

    if (!negotiateMsg)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    PUT_BYTES(negotiateMsg, 0, signature, NTLM_SIGNATURE_SIZE);
    ofs += NTLM_SIGNATURE_SIZE;

    PUT_ULONG(negotiateMsg, ofs, NEGOTIATE_MSG);
    ofs += sizeof(ULONG);
    
    /* @todo - this should be alterable via policy, and gss api */
    /* OID set? */

    /* 
     * set flags on context 
     * no lock required - only we have ctxt handle
     */
    pCtxt->negotiateFlags = NEGOTIATE_CLI_DEFAULT;
    pCtxt->processNextMessage = NTLMProcessAuthenticateMessage; 

    PUT_ULONG(negotiateMsg, ofs, NEGOTIATE_CLI_DEFAULT);
    ofs += sizeof(ULONG);

    pOutputToken->length = pOutputToken->maxLength =
        sizeof(NEGOTIATE_MESSAGE);
    pOutputToken->buffer = (PBYTE) negotiateMsg;

    negotiateMsg = NULL;

    DBGDumpSecBuffer(D_ERROR, "negotiate msg", pOutputToken);

error:

    /* if we err'd, this context is doa */
    if (dwError)
        pCtxt->processNextMessage = NTLMFinalizedMessage;
    else {
        dwError = LSA_WARNING_CONTINUE_NEEDED;
        pCtxt->processNextMessage = NTLMProcessChallengeMessage; 
    }

    NTLM_SAFE_FREE(negotiateMsg);
    return dwError;
}


DWORD
NTLMParseNegotiateMessage(
    PSEC_BUFFER pBuf,
    ULONG *negotiateFlags
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    ULONG ofs = 0;

    if (pBuf->length < sizeof(NEGOTIATE_MESSAGE))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INSUFFICIENT_BUFFER);

    dwError = NTLMParseMessageHeader(
                    pBuf, 
                    &ofs,
                    NEGOTIATE_MSG
                    );

    BAIL_ON_NTLM_ERROR(dwError);
    
    /* only interesting part is negotiate flag */
    (*negotiateFlags) = GET_ULONG(pBuf->buffer, ofs);

    /* @todo NTLMDumpNegotiateMessage */

error:

    return dwError;
}



DWORD
NTLMProcessNegotiateMessage( 
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    )
{
    DWORD dwError;
    ULONG newFlags = 0;
    ULONG negotiateFlags; 

    /* parse the message */
    dwError = NTLMParseNegotiateMessage(
                pInputToken,
                &negotiateFlags
                );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMServerValidateNegotiateFlags(
                    pCtxt,
                    negotiateFlags,
                    &newFlags
                    );
    
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMBuildChallengeMessage(
                    pCtxt,
                    newFlags,
                    pOutputToken
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LSA_WARNING_CONTINUE_NEEDED;

error:

    /* @todo debug */

    return dwError;
}


/*
 *  CHALLENGE MESSAGE HANDLERS
 */
DWORD
NTLMBuildChallengeMessage(
    PNTLM_CONTEXT pCtxt,
    DWORD challengeFlags,
    PSEC_BUFFER challengeMsg 
    )
{
    DWORD dwError;
    DWORD dwSize = sizeof(CHALLENGE_MESSAGE);
    ULONG ofs = 0;
    ULONG bufofs = sizeof(CHALLENGE_MESSAGE);
    LSA_STRING workstation;
    SEC_BUFFER targetInfo;
    UCHAR challenge[NTLM_CHALLENGE_LENGTH];

    ZERO_STRUCT(workstation);
    ZERO_STRUCT(targetInfo);
    ZERO_STRUCTP(challengeMsg);

    /* build target name, and target info */
    /* @todo - verify this is correct behavior for REQUEST_TARGET */
    if (challengeFlags & NEGOTIATE_REQUEST_TARGET) {

        dwError = NTLMGetWorkstationName(&workstation);
        BAIL_ON_NTLM_ERROR(dwError); 
        dwSize += workstation.max;

        /* @todo we don't support domain currently */
        challengeFlags |= CHALLENGE_TARGET_SERVER;

        dwError = NTLMBuildTargetInfo(&targetInfo);
        BAIL_ON_NTLM_ERROR(dwError);
        dwSize += targetInfo.maxLength;
        challengeFlags |= CHALLENGE_TARGET_INFO;
    }

    challengeMsg->buffer =  NTLMAllocateMemory(dwSize);
    if (!challengeMsg->buffer)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    PUT_BYTES(challengeMsg->buffer, ofs, signature, NTLM_SIGNATURE_SIZE);
    ofs += NTLM_SIGNATURE_SIZE;

    PUT_ULONG(challengeMsg->buffer, ofs, CHALLENGE_MSG);
    ofs += sizeof(ULONG);

    if (challengeFlags & NEGOTIATE_REQUEST_TARGET) {
        NTLMPutLsaString(&workstation, challengeMsg->buffer, &bufofs, &ofs);
    }

    /* these flags aren't persisted */
    PUT_ULONG(challengeMsg->buffer, ofs, challengeFlags);
    ofs += sizeof(ULONG);

    /* compute challenge */
    dwError = NTLMCryptGenRandomBytes(
                challenge,
                NTLM_CHALLENGE_LENGTH
                );

    BAIL_ON_NTLM_ERROR(dwError);

    PUT_BYTES(challengeMsg->buffer, ofs, challenge, NTLM_CHALLENGE_LENGTH);
    ofs += NTLM_CHALLENGE_LENGTH;
    
    /* huh - what are these here for? */
    PUT_ULONG(challengeMsg->buffer, ofs, 0);
    ofs += sizeof(ULONG);

    if (challengeFlags & CHALLENGE_TARGET_INFO) 
        NTLMPutSecBuffer(&targetInfo, challengeMsg->buffer, &bufofs, &ofs);

    /* update context */
    NTLM_LOCK_CONTEXT(pCtxt);
    pCtxt->processNextMessage = NTLMProcessAuthenticateMessage;
    COPY_CHALLENGE(pCtxt->challenge, challenge);
    /* @todo - store off, or version, targetInfo */
    NTLM_UNLOCK_CONTEXT(pCtxt);
    challengeMsg->length = challengeMsg->maxLength = dwSize;

    DBGDumpSecBuffer(D_ERROR, "challenge msg", challengeMsg);

error:

    NTLM_SAFE_FREE(workstation.buffer);
    NTLM_SAFE_FREE(targetInfo.buffer);

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
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pBuf,
    PAUTHENTICATE_MESSAGE pAuthMsg
)
{
    DWORD dwError;
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





DWORD
NTLMBuildAuthenticateMessage(
    PNTLM_CONTEXT pCtxt,
    CHALLENGE_MESSAGE *challengeMsg,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER pOutputToken
    )
{

    DWORD dwError;
    DWORD dwMessageSize;
    ULONG negFlags;
    ULONG ofs = 0;
    ULONG bufofs = sizeof(AUTHENTICATE_MESSAGE);
    SEC_BUFFER ntResponse;
    MD5_CTX md5ctx;
    UCHAR digest[MD5_DIGEST_LENGTH];
    SEC_BUFFER_S clientChallenge;
    SEC_BUFFER_S sessionKey;
    LSA_STRING workstationName;
    PBYTE authenticateMessage = NULL;

    ZERO_STRUCT(workstationName);
    ZERO_STRUCT(ntResponse);
    INIT_SEC_BUFFER_S(&clientChallenge, 24);
    INIT_SEC_BUFFER_S(&sessionKey, 16);

    negFlags = NTLMContextGetNegotiateFlags(pCtxt);


    /* 
     * if we're doing NTLM2 keys, with ntlmv1 protocol, 
     * we generate a client challenge, and store it in
     * the lm response.
     */
    if (negFlags & NEGOTIATE_NTLM2)
    {
        dwError = NTLMCryptGenRandomBytes(clientChallenge.buffer, 8);
        BAIL_ON_NTLM_ERROR(dwError);

        MD5_Init(&md5ctx);
        MD5_Update(&md5ctx, challengeMsg->challenge, 8);
        MD5_Update(&md5ctx, clientChallenge.buffer, 8);
        MD5_Final(digest, &md5ctx);
    }
    else
    {
        memcpy(digest, challengeMsg->challenge, 8);
    }

    /* first build NT response and session key, so we can allocate enuf buf */
    dwError = NTLMBuildResponse(
                    pCtxt,
                    digest,
                    targetInfo,
                    &ntResponse
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMComputeSessionKey(
                    pCtxt,
                    challengeMsg->challenge,
                    &clientChallenge,
                    &sessionKey,
                    true
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMGetWorkstationName(&workstationName);

    BAIL_ON_NTLM_ERROR(dwError);

    dwMessageSize = sizeof(AUTHENTICATE_MESSAGE) +
        pCtxt->cred->authUser.user.length + 
        pCtxt->cred->authUser.domain.length + 
        ntResponse.length +
        sessionKey.length + 
        clientChallenge.length +
        workstationName.length;

    authenticateMessage = NTLMAllocateMemory(dwMessageSize);

    if (!authenticateMessage)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    PUT_BYTES(authenticateMessage, 0, signature, NTLM_SIGNATURE_SIZE);
    ofs += NTLM_SIGNATURE_SIZE;

    PUT_ULONG(authenticateMessage, ofs, AUTHENTICATE_MSG);
    ofs += sizeof(ULONG);

    /* LM response 
     * We don't do LM - this is either a copy of the ntResponse, or if we are
     * using a NTLM2 key, the client challenge (0 padded).
     */
    if (negFlags & NEGOTIATE_NTLM2)
        NTLMPutSecBuffer((PSEC_BUFFER) &clientChallenge, authenticateMessage,
            &bufofs, &ofs);
    else
        NTLMPutSecBuffer(&ntResponse, authenticateMessage, &bufofs, &ofs);

    /* NT response */
    NTLMPutSecBuffer(&ntResponse, authenticateMessage, &bufofs, &ofs);

    /* domain name */
    NTLMPutLsaString(&pCtxt->cred->authUser.domain, authenticateMessage,
        &bufofs, &ofs);

    /* user name */
    NTLMPutLsaString(&pCtxt->cred->authUser.user, authenticateMessage, &bufofs,
        &ofs);

    /* workstation */
    NTLMPutLsaString(&workstationName, authenticateMessage, &bufofs, &ofs);

    /* session key */
    NTLMPutSecBufferS(&sessionKey, authenticateMessage, &bufofs, &ofs);

    /* neg flags */
    PUT_ULONG(authenticateMessage, ofs, negFlags);

    /* update context */
    NTLM_LOCK_CONTEXTS();
    pCtxt->processNextMessage = NTLMFinalizedMessage;
    NTLM_UNLOCK_CONTEXTS();

    pOutputToken->buffer = authenticateMessage;
    pOutputToken->length = pOutputToken->maxLength = dwMessageSize;
    authenticateMessage = NULL;

    DBGDumpSecBuffer(D_ERROR, "authenticate msg", pOutputToken); 

error:
    
    NTLM_SAFE_FREE(authenticateMessage);
    LsaFreeLsaString(&workstationName);

    return dwError;
}

DWORD
NTLMCheckChallengeNegotiateFlags(
    PNTLM_CONTEXT pCtxt,
    ULONG negotiateFlags 
    )
{
    /*@todo - flesh this out - e.g. check for target info, etc */

    return LSA_ERROR_SUCCESS;
}

void
NTLMFreeChallengeMessage(
    PCHALLENGE_MESSAGE msg
)
{
    NTLM_SAFE_FREE(msg->target.buffer);
}

DWORD
NTLMParseChallengeMessage(
    PSEC_BUFFER pBuf,
    PCHALLENGE_MESSAGE pChallengeMsg,
    PSEC_BUFFER pTargetInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    ULONG ofs = 0;

    ZERO_STRUCTP(pChallengeMsg);
    ZERO_STRUCTP(pTargetInfo);

    if (pBuf->length < sizeof(CHALLENGE_MESSAGE))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INSUFFICIENT_BUFFER);

    dwError = NTLMParseMessageHeader(
                    pBuf, 
                    &ofs,
                    CHALLENGE_MSG
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMGetSecBuffer(&pChallengeMsg->target, pBuf, &ofs);
    BAIL_ON_NTLM_ERROR(dwError);

    pChallengeMsg->negotiateFlags = GET_ULONG(pBuf->buffer, ofs);
    ofs += sizeof(ULONG);

    GET_BYTES(pChallengeMsg->challenge, ofs, pBuf->buffer,
        NTLM_CHALLENGE_LENGTH);
    ofs += NTLM_CHALLENGE_LENGTH;

    /* handle reserved 0 padded bytes */
    ofs += sizeof(ULONG);

    if (pChallengeMsg->negotiateFlags & CHALLENGE_TARGET_INFO) {
        dwError = NTLMGetSecBuffer(pTargetInfo, pBuf, &ofs);
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* NTLMDumpChallengeMessage */

error:

    return dwError;
}


DWORD
NTLMProcessChallengeMessage(
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    )
{
    DWORD dwError;
    CHALLENGE_MESSAGE challengeMessage;
    SEC_BUFFER targetInfo;

    ZERO_STRUCT(challengeMessage);
    ZERO_STRUCT(targetInfo);

    dwError = NTLMParseChallengeMessage(
                pInputToken,
                &challengeMessage,
                &targetInfo
                );

    BAIL_ON_NTLM_ERROR(dwError);

    /* check final negotiated flags */
    dwError = NTLMCheckChallengeNegotiateFlags(
                pCtxt,
                challengeMessage.negotiateFlags
                );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMBuildAuthenticateMessage(
                pCtxt,
                &challengeMessage,
                &targetInfo,
                pOutputToken
                );

    BAIL_ON_NTLM_ERROR(dwError);

    /* ? dwError = LSA_WARNING_CONTINUE_NEEDED; */
    /* should be OK - clients need to continue sending */

error:

    return dwError;
}

DWORD
NTLMProcessAuthenticateMessage(
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    )

{
    DWORD dwError;
    AUTHENTICATE_MESSAGE authenticateMessage;
    AUTH_USER authUser;

    ZERO_STRUCT(authenticateMessage);
    ZERO_STRUCT(authUser);

    dwError = NTLMParseAuthenticateMessage(
                pCtxt,
                pInputToken,
                &authenticateMessage
                );

    BAIL_ON_LSA_ERROR(dwError);

    /* 
     * store away user name / domain name 
     * 
     * this allows us to pass message over secure
     * channel, or do it locally
     */
    if (!NTLMInitializeAuthUser(
            &authenticateMessage.userName,
            &authenticateMessage.domainName,
            NULL,
            0,
            &authUser
            )) 
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_NO_SUCH_USER);

    /* validate response, get session key */
    dwError = authUser.provider->responseMessageHandler(
                pCtxt,
                &authenticateMessage.ntResponse,
                &authenticateMessage.lmResponse,
                &authenticateMessage.sessionKey,
                &authUser
                );

    BAIL_ON_NTLM_ERROR(dwError);
    
error:

    NTLMFreeAuthenticateMessage(&authenticateMessage);
    NTLMFreeAuthUser(&authUser);

    return dwError;

}
