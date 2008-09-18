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
 *        crypt.c
 *
 * Abstract:
 *
 *        GSS cryptographic primitives 
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "client.h"

DWORD 
NTLMCreateNTLM2Keys(
    PNTLM_CONTEXT context,
    PSEC_BUFFER_S baseKey
)
{
    MD5_CTX md5ctx;
    ULONG len;
    UCHAR digest[MD5_DIGEST_LENGTH];

    /* derive seal keys */
    if (context->negotiateFlags & NEGOTIATE_SEAL)
    {
        context->sealRoutine = NTLMV2Seal;
        context->unsealRoutine = NTLMV2Unseal;

        if (context->negotiateFlags & NEGOTIATE_128)
            len = 16;
        else if (context->negotiateFlags & NEGOTIATE_56)
            len = 7;
        else 
            len = 5;

        /* derive keys */
        MD5_Init(&md5ctx);
        MD5_Update(&md5ctx, baseKey->buffer, len);
        MD5_Update(&md5ctx, CLIENT_TO_SERVER_SEALKEY, strlen(CLIENT_TO_SERVER_SEALKEY) + 1);
        MD5_Final(digest, &md5ctx);

        if (context->flags & CONTEXT_SERVER)
            memcpy(context->sealKeys.incoming.key.buffer, digest, MD5_DIGEST_LENGTH);
        else 
            memcpy(context->sealKeys.outgoing.key.buffer, digest, MD5_DIGEST_LENGTH);
        
        MD5_Init(&md5ctx);
        MD5_Update(&md5ctx, baseKey->buffer, len);
        MD5_Update(&md5ctx, SERVER_TO_CLIENT_SEALKEY, strlen(SERVER_TO_CLIENT_SEALKEY) + 1);
        MD5_Final(digest, &md5ctx);

        if (context->flags & CONTEXT_SERVER)
            memcpy(context->sealKeys.outgoing.key.buffer, digest, MD5_DIGEST_LENGTH);
        else 
            memcpy(context->sealKeys.incoming.key.buffer, digest, MD5_DIGEST_LENGTH);

        context->sealKeys.incoming.key.length = MD5_DIGEST_LENGTH;
        context->sealKeys.outgoing.key.length = MD5_DIGEST_LENGTH;

    }
    else
    {
        context->sealRoutine = NTLMSealUnsupported;
        context->unsealRoutine = NTLMUnsealUnsupported;
        context->sealKeys.incoming.key.length = 0;
        context->sealKeys.outgoing.key.length = 0;
    }

    /* derive sign keys */
    if (context->negotiateFlags & (NEGOTIATE_SEAL | NEGOTIATE_SIGN))
    {
        /* seal uses checksum, but doesn't necessarily support sign only */
        if (context->negotiateFlags & NEGOTIATE_SIGN)
        {
            context->signRoutine = NTLMV2Sign;
            context->verifyRoutine = NTLMV2Verify;
        }
        else
        {
            context->signRoutine = NTLMSignUnsupported;
            context->verifyRoutine = NTLMVerifyUnsupported;
        }

        /* derive keys */
        MD5_Init(&md5ctx);
        MD5_Update(&md5ctx, baseKey->buffer, baseKey->length);
        MD5_Update(&md5ctx, CLIENT_TO_SERVER_SIGNKEY, strlen(CLIENT_TO_SERVER_SIGNKEY) + 1);
        MD5_Final(digest, &md5ctx);

        if (context->flags & CONTEXT_SERVER)
            memcpy(context->signKeys.incoming.key.buffer, digest, MD5_DIGEST_LENGTH);
        else 
            memcpy(context->signKeys.outgoing.key.buffer, digest, MD5_DIGEST_LENGTH);
        
        MD5_Init(&md5ctx);
        MD5_Update(&md5ctx, baseKey->buffer, 16);
        MD5_Update(&md5ctx, SERVER_TO_CLIENT_SIGNKEY, strlen(SERVER_TO_CLIENT_SIGNKEY) + 1);
        MD5_Final(digest, &md5ctx);

        if (context->flags & CONTEXT_SERVER)
            memcpy(context->signKeys.outgoing.key.buffer, digest, MD5_DIGEST_LENGTH);
        else 
            memcpy(context->signKeys.incoming.key.buffer, digest, MD5_DIGEST_LENGTH);

        context->signKeys.incoming.key.length = MD5_DIGEST_LENGTH;
        context->signKeys.outgoing.key.length = MD5_DIGEST_LENGTH;

    }
    else
    {
        context->signKeys.incoming.key.length = 0;
        context->signKeys.outgoing.key.length = 0;
    }

    return LSA_ERROR_SUCCESS;

}


DWORD
NTLMCreateNTLM1Keys(
    PNTLM_CONTEXT context,
    PSEC_BUFFER_S baseKey
)
{
    SEC_BUFFER_S key;
    ULONG len;
    CHAR bit56[1] = {0xA0};
    CHAR bit40[3] = {0xE5, 0x38, 0xB0};

    INIT_SEC_BUFFER_S(&key, 8);

    if (context->negotiateFlags & NEGOTIATE_SEAL)
    {
        if (context->negotiateFlags & NEGOTIATE_56)
            len = 7;
        else 
            len = 5;

        memcpy(key.buffer, baseKey->buffer, len);

        /* @todo - add in support for NTLM 1 session security */
        /* pad to 64 bits */
        if (context->negotiateFlags & NEGOTIATE_56)
            memcpy(&key.buffer[len], bit56, 1);
        else 
            memcpy(&key.buffer[len], bit40, 3);

        context->sealRoutine = NTLMSealUnsupported;
        context->unsealRoutine = NTLMUnsealUnsupported;

        /* @todo - support v1 crypto */
        /* context->sealRoutine = NTLMV1Seal56; */
        /* context->unsealRoutine = NTLMV1Unseal56; */

        context->sealKeys.incoming.key.length = 8;
        context->sealKeys.outgoing.key.length = 8;
        memcpy(context->sealKeys.incoming.key.buffer, key.buffer, 8);
        memcpy(context->sealKeys.outgoing.key.buffer, key.buffer, 8);
    }

    /* NTLM1 keys can't be used for signing */
    context->signKeys.incoming.key.length = 0;
    context->signKeys.outgoing.key.length = 0;
    context->signRoutine = NTLMSignUnsupported;
    context->verifyRoutine = NTLMVerifyUnsupported;

    return LSA_ERROR_SUCCESS;

}

DWORD
NTLMCreateKeys(
    PNTLM_CONTEXT context
)
{

    if (context->negotiateFlags & NEGOTIATE_NTLM2)
        return NTLMCreateNTLM2Keys(context, &context->baseSessionKey);
    else
        return NTLMCreateNTLM1Keys(context, &context->baseSessionKey);

}

DWORD
NTLMSetupKeyState(
    PNTLM_CONTEXT context,
    BOOLEAN reKey,
    BOOLEAN outgoing
)
{
    PNTLM_GSS_KEY gssKey = (outgoing ? &context->sealKeys.outgoing :
        &context->sealKeys.incoming);

    /* datagram needs to rekey from master key */
    if (!reKey && (gssKey->flags & NTLM_GSS_KEY_INITIALIZED))
        return LSA_ERROR_SUCCESS;

    RC4_set_key(&gssKey->keyHandle, gssKey->key.length, gssKey->key.buffer);
    gssKey->seqNum = 0;
    gssKey->flags |= NTLM_GSS_KEY_INITIALIZED;

    return LSA_ERROR_SUCCESS;

}


//
// Get mic / verify mic routines (signature)
//

VOID
NTLMGenerateGSSSignature(
    PNTLM_CONTEXT context,
    BOOLEAN outgoing,
    PNTLM_GSS_SIGNATURE signature,
    gss_buffer_t data
)
{
    HMAC_CTX        hmacCtxt;
    DWORD           mdLen;
    UCHAR           sig[MD5_DIGEST_LENGTH];

    PNTLM_GSS_KEY   gssSignKey = (outgoing ? &context->signKeys.outgoing :
        &context->signKeys.incoming);

    PNTLM_GSS_KEY   gssSealKey = (outgoing ? &context->sealKeys.outgoing :
        &context->sealKeys.incoming);

    HMAC_CTX_init(&hmacCtxt);
    HMAC_Init_ex(
        &hmacCtxt, 
        gssSignKey->key.buffer,
        gssSignKey->key.length,
        EVP_md5(),
        NULL
        );

    HMAC_Update(&hmacCtxt, (UCHAR*) &gssSignKey->seqNum, sizeof(ULONG));
    HMAC_Update(&hmacCtxt, data->value, data->length);
    HMAC_Final(&hmacCtxt,sig,&mdLen);
    HMAC_CTX_cleanup(&hmacCtxt);


    /* NTLM2 encrypts checksum */
    if (context->negotiateFlags & NEGOTIATE_NTLM2)
    {

        RC4(
            (RC4_KEY*) &gssSealKey->keyHandle, 
            8,
            sig,
            sig
            );
    }

    signature->version = 1;
    signature->seqNum = gssSignKey->seqNum;
    memcpy(signature->checksum, sig, 8);
}

DWORD
NTLMV2Sign(
    PNTLM_CONTEXT context,
    ULONG qop,
    gss_buffer_t inputToken,
    gss_buffer_t outputToken
)
{

    DWORD dwError = 0;
    gss_buffer_desc sigToken;
    NTLM_GSS_SIGNATURE signature;

    ZERO_STRUCT(sigToken);

    sigToken.length = sizeof(NTLM_GSS_SIGNATURE);
    sigToken.value = NTLMAllocateMemory(sigToken.length);
    if (!sigToken.value)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    dwError = NTLMSetupKeyState(
                    context,
                    FALSE, /*rekey */
                    TRUE /*outbound*/
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    NTLMGenerateGSSSignature(
        context,
        TRUE, /* outgoing */
        &signature,
        inputToken
        );

    context->signKeys.outgoing.seqNum++;

    memcpy(
        sigToken.value,
        &signature,
        sizeof(NTLM_GSS_SIGNATURE)
        );

    (*outputToken) = sigToken;
    sigToken.value = NULL;

error:

    NTLM_SAFE_FREE(sigToken.value);
    return dwError;
}

DWORD
NTLMV2Verify(
    PNTLM_CONTEXT context,
    ULONG *qop,
    gss_buffer_t inputToken,
    gss_buffer_t sigToken
)
{
    DWORD dwError = 0;

    NTLM_GSS_SIGNATURE signature;
    PNTLM_GSS_SIGNATURE pSig = (PNTLM_GSS_SIGNATURE) sigToken->value;

    dwError = NTLMSetupKeyState(
                    context,
                    FALSE, /*rekey */
                    FALSE /*inbound*/
                    );

    BAIL_ON_NTLM_ERROR(dwError);
    NTLMGenerateGSSSignature(
        context,
        FALSE, /* incoming */ 
        &signature,
        inputToken
        );

    if (!memcmp(pSig, &signature, sizeof(NTLM_GSS_SIGNATURE)))
        return LSA_ERROR_INVALID_MESSAGE;

error:

    return dwError;
}


//
// Encryption routines
// 

DWORD
NTLMV2Seal(
    PNTLM_CONTEXT context,
    ULONG qop,
    gss_buffer_t data,
    gss_buffer_t encData
)
{

    DWORD dwError;
    gss_buffer_desc enc;
    PBYTE tmp;
    NTLM_GSS_SIGNATURE signature;

    ZERO_STRUCT(enc);

    dwError = NTLMSetupKeyState(
                    context,
                    FALSE, /*rekey */
                    TRUE /*outbound*/
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    /* first, encrypt the data */
    enc.length = sizeof(NTLM_GSS_SIGNATURE) + data->length;
    enc.value = NTLMAllocateMemory(enc.length);
    if (!enc.value)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    memcpy(enc.value, data->value, data->length);
    tmp = enc.value;

    RC4(
        (RC4_KEY*) &context->sealKeys.outgoing.keyHandle, 
        data->length, 
        data->value, 
        enc.value
        );

    NTLMGenerateGSSSignature(
        context,
        TRUE, /* outgoing */
        &signature,
        data
        );

    context->sealKeys.outgoing.seqNum++;

    /* add signature at end */
    memcpy(&tmp[data->length], &signature, sizeof(NTLM_GSS_SIGNATURE));
    
    (*encData) = enc;
    enc.value = NULL;

error:
    
    NTLM_SAFE_FREE(enc.value);

    return dwError;
}

DWORD
NTLMV2Unseal(
    PNTLM_CONTEXT context,
    ULONG *qop,
    gss_buffer_t data,
    gss_buffer_t decData
)
{
    DWORD dwError;
    gss_buffer_desc buf = { 0 };
    PBYTE tmp;
    NTLM_GSS_SIGNATURE signature;
    PNTLM_GSS_SIGNATURE msgSignature;

    if (data->length < sizeof(NTLM_GSS_SIGNATURE))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_MESSAGE);

    dwError = NTLMSetupKeyState(
                    context,
                    FALSE, /*rekey */
                    FALSE /*inbound*/
                    );

    BAIL_ON_NTLM_ERROR(dwError);
    
    buf.length = data->length - sizeof(NTLM_GSS_SIGNATURE);
        
    buf.value = NTLMAllocateMemory(buf.length);
    if (!buf.value)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    RC4(
        (RC4_KEY*) &context->sealKeys.incoming.keyHandle, 
        buf.length, 
        data->value, 
        buf.value
        );

    /* check the signature */    
    
    NTLMGenerateGSSSignature(
        context,
        FALSE, /* incoming */
        &signature,
        &buf
        );

    context->sealKeys.incoming.seqNum++;

    tmp = data->value;
    msgSignature = (PNTLM_GSS_SIGNATURE) &tmp[buf.length];
    if (memcmp(msgSignature->checksum, signature.checksum, 8))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_MESSAGE);

    (*decData) = buf;
    buf.value = NULL;

error:

    NTLM_SAFE_FREE(buf.value);

    return dwError;
}



DWORD
NTLMSealUnsupported(
    PNTLM_CONTEXT context,
    ULONG qop,
    gss_buffer_t data,
    gss_buffer_t encData
)
{
    return LSA_ERROR_UNSUPPORTED_CRYPTO_OP;
}

DWORD
NTLMUnsealUnsupported(
    PNTLM_CONTEXT context,
    ULONG *qop,
    gss_buffer_t data,
    gss_buffer_t decData
)
{
    return LSA_ERROR_UNSUPPORTED_CRYPTO_OP;
}

DWORD
NTLMSignUnsupported(
    PNTLM_CONTEXT context,
    ULONG qop,
    gss_buffer_t inputToken,
    gss_buffer_t sigToken
)
{
    return LSA_ERROR_UNSUPPORTED_CRYPTO_OP;
}

DWORD
NTLMVerifyUnsupported(
    PNTLM_CONTEXT context,
    ULONG *qop,
    gss_buffer_t inputToken,
    gss_buffer_t sigToken
)
{
    return LSA_ERROR_UNSUPPORTED_CRYPTO_OP;
}
