/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        netlogon.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        netlogon rpc server stub functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *          Adam Bernstein (abernstein@vmware.com)
 */

#include "includes.h"

/* Save this stuff (hash table) for access later */
static    BYTE g_bzSaveSessionKey[16]  = {0};
static    BYTE g_bzClientCredential[8] = {0};
static    BYTE g_bzServerCredential[8] = {0};

#define _NETLOGON_TEST_VECTORS
#ifdef _NETLOGON_TEST_VECTORS
static CHAR unicodePwdTest[] = {
 0x73, 0x00, 0x4f, 0x00, 0x62, 0x00, 0x54, 0x00, 0x6c, 0x00, 0x6e, 0x00, 0x2c, 0x00
, 0x6e, 0x00, 0x55, 0x00, 0x38, 0x00, 0x75, 0x00, 0x47, 0x00, 0x49, 0x00, 0x56, 0x00, 0x24, 0x00
, 0x20, 0x00, 0x78, 0x00, 0x69, 0x00, 0x5c, 0x00, 0x6c, 0x00, 0x75, 0x00, 0x25, 0x00, 0x57, 0x00
, 0x23, 0x00, 0x3c, 0x00, 0x27, 0x00, 0x5c, 0x00, 0x65, 0x00, 0x37, 0x00, 0x5c, 0x00, 0x6a, 0x00
, 0x52, 0x00, 0x40, 0x00, 0x45, 0x00, 0x6f, 0x00, 0x37, 0x00, 0x62, 0x00, 0x6d, 0x00, 0x77, 0x00
, 0x3c, 0x00, 0x42, 0x00, 0x4e, 0x00, 0x35, 0x00, 0x55, 0x00, 0x4f, 0x00, 0x6c, 0x00, 0x65, 0x00
, 0x4a, 0x00, 0x31, 0x00, 0x6c, 0x00, 0x6e, 0x00, 0x67, 0x00, 0x20, 0x00, 0x46, 0x00, 0x53, 0x00
, 0x2d, 0x00, 0x27, 0x00, 0x22, 0x00, 0x3d, 0x00, 0x4d, 0x00, 0x4b, 0x00, 0x74, 0x00, 0x28, 0x00
, 0x2e, 0x00, 0x6a, 0x00, 0x54, 0x00, 0x5a, 0x00, 0x64, 0x00, 0x74, 0x00, 0x60, 0x00, 0x76, 0x00
, 0x38, 0x00, 0x68, 0x00, 0x50, 0x00, 0x75, 0x00, 0x2e, 0x00, 0x37, 0x00, 0x52, 0x00, 0x4b, 0x00
, 0x28, 0x00, 0x5d, 0x00, 0x76, 0x00, 0x4d, 0x00, 0x5b, 0x00, 0x33, 0x00, 0x6e, 0x00, 0x6b, 0x00
, 0x49, 0x00, 0x47, 0x00, 0x58, 0x00, 0x60, 0x00, 0x7a, 0x00, 0x77, 0x00, 0x5c, 0x00, 0x26, 0x00
, 0x48, 0x00, 0x6c, 0x00, 0x29, 0x00, 0x64, 0x00, 0x23, 0x00, 0x25, 0x00, 0x4e, 0x00, 0x54, 0x00
, 0x60, 0x00, 0x69, 0x00, 0x73, 0x00, 0x67, 0x00, 0x6c, 0x00, 0x4c, 0x00, 0x29, 0x00, 0x46, 0x00
, 0x75, 0x00, 0x6d, 0x00, 0x72, 0x00, 0x24, 0x00, 0x5d, 0x00, 0x2b, 0x00, 0x70, 0x00, 0x4c, 0x00
, 0x64, 0x00 };
/* Canned client nonce */
static BYTE cliNonceTest[] = { 0x4e, 0x00, 0x47, 0xa6, 0x75, 0x8b, 0xeb, 0x79, };
/* Canned server nonce */
static BYTE srvNonceTest[] = { 0xa7, 0x4c, 0x75, 0x78, 0x56, 0xa3, 0xe0, 0x78, };
#if 0
/* Canned server challenge */
/* Not needed; server can compute these values for the given none/pwd value */
static BYTE cliCredentialTest[] = { 0xb4, 0x41, 0x72, 0x25, 0xbc, 0x7c, 0xbe, 0x59, };
/* Canned client challenge */
static BYTE srvCredentialTest[]  = { 0x5d, 0xe8, 0x64, 0xca, 0x7f, 0x58, 0x47, 0xe8, };
#endif
static    BOOLEAN bUseTestVector = FALSE;
#endif


#ifndef _NETLOGON_UNIT_TEST
static
#endif
int
compute_md4_digest(
   PUCHAR puValue,
   DWORD dwValueLen,
   PUCHAR pMd4Digest)
{
    MD4_CTX ctx = {0};

    /* Returns 1 success, 0 failure */
    if (MD4_Init(&ctx) == 0)
    {
        return 0;
    }
    if (MD4_Update(&ctx, puValue, dwValueLen) == 0)
    {
        return 0;
    }
    if (MD4_Final(pMd4Digest, &ctx) == 0)
    {
        return 0;
    }

    return 1;
}


static
void
InitLMKey(
    unsigned char *DesKeyIn, 
    unsigned char *DesKeyOutRet)
{
    unsigned char DesKeyOut[8] = {0};
    int i = 0;

    DesKeyOut[0] = DesKeyIn[0] >> 0x01; 
    DesKeyOut[1] = ((DesKeyIn[0]&0x01)<<6) | (DesKeyIn[1]>>2); 
    DesKeyOut[2] = ((DesKeyIn[1]&0x03)<<5) | (DesKeyIn[2]>>3); 
    DesKeyOut[3] = ((DesKeyIn[2]&0x07)<<4) | (DesKeyIn[3]>>4); 
    DesKeyOut[4] = ((DesKeyIn[3]&0x0F)<<3) | (DesKeyIn[4]>>5); 
    DesKeyOut[5] = ((DesKeyIn[4]&0x1F)<<2) | (DesKeyIn[5]>>6); 
    DesKeyOut[6] = ((DesKeyIn[5]&0x3F)<<1) | (DesKeyIn[6]>>7); 
    DesKeyOut[7] = DesKeyIn[6] & 0x7F; 
    for (i=0; i<8; i++)
    { 
        DesKeyOut[i] = (DesKeyOut[i] << 1) & 0xfe; 
    }
    memcpy(DesKeyOutRet, DesKeyOut, sizeof(DesKeyOut));
}


static
void
CopyBytesRange(
    int byte_first,
    int byte_last,
    unsigned char *array_in,
    unsigned char *array_out)
{
    int i = 0;
    int x = 0;

    for (i=byte_first, x=0; i<=byte_last; i++, x++)
    {
        array_out[x] = array_in[i];
    }
}


static
NTSTATUS
DesEncryptEcb(
    unsigned char *desKey,
    unsigned char *inPlainText,
    int inPlainTextLen,
    unsigned char *outDesEnc,
    int *outDesEncLen)
{
    NTSTATUS ntStatus = 0;
    EVP_CIPHER_CTX des_ctx = {0};
    int outDesEncLenRet = 0;
    BYTE dummyFinal[8] = {0};
    int dummyFinalLen = 0;

    EVP_CIPHER_CTX_init(&des_ctx);
    EVP_CIPHER_CTX_set_padding(&des_ctx, 0);

    ntStatus = EVP_EncryptInit_ex(&des_ctx, EVP_des_ecb(), NULL, desKey, NULL); 
    if (ntStatus == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    ntStatus = EVP_EncryptUpdate(&des_ctx,
                                 outDesEnc,
                                 &outDesEncLenRet,
                                 inPlainText,
                                 inPlainTextLen); 
    if (ntStatus == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    /* This data is meanless for ECB mode */
    ntStatus = EVP_EncryptFinal_ex(&des_ctx, dummyFinal, &dummyFinalLen);
    if (ntStatus == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    /* Got here, return success */
    ntStatus = 0;
    *outDesEncLen = outDesEncLenRet;

cleanup:
    EVP_CIPHER_CTX_cleanup(&des_ctx);

    return ntStatus;
}


#ifndef _NETLOGON_UNIT_TEST
// zzz static
#endif
NTSTATUS
ComputeSessionKeyDes(
    PBYTE sharedSecret,
    DWORD sharedSecretLen,
    PBYTE cliNonce,
    DWORD cliNonceLen,
    PBYTE srvNonce,
    DWORD srvNonceLen,
    PBYTE sessionKey,
    DWORD sessionKeyLen)
{
    NTSTATUS ntStatus  = 0;
    BYTE key1[8] = {0};
    BYTE key2[8] = {0};
    BYTE output1[sizeof(key1)] = {0};
    int output1Len = 0;
    BYTE output2[sizeof(key1)] = {0};
    int output2Len = 0;
    UCHAR md4DigestBuf[MD4_DIGEST_LENGTH] = {0};
    UINT64 sumNonce = 0;
    UINT64 sumCli = 0;
    UINT64 sumSrv = 0;
    PBYTE psumNonce = (PBYTE) &sumNonce;

    /* 1:Compute M4SS value. Tested good with known test data */
    if (compute_md4_digest(sharedSecret,
                           sharedSecretLen,
                           md4DigestBuf) == 0)
    {
        ntStatus = ERROR_INVALID_PASSWORD;
        goto cleanup;
    }
    memcpy(&sumCli, cliNonce, cliNonceLen);
    memcpy(&sumSrv, srvNonce, srvNonceLen);

    /* Section 3.1.4.3 Session-Key Computation [MS-NRPC] */
    sumNonce = sumCli + sumSrv;

    CopyBytesRange(0, 6, md4DigestBuf, key1);
    CopyBytesRange(9, 15, md4DigestBuf, key2);

    ntStatus = DesEncryptEcb(
                   key1,
                   psumNonce,
                   sizeof(sumNonce),
                   output1,
                   &output1Len);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);
    ntStatus = DesEncryptEcb(
                   key2,
                   output1,
                   sizeof(output1),
                   output2,
                   &output2Len);

    memcpy(sessionKey, output2, output2Len);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}

#ifndef _NETLOGON_UNIT_TEST
// zzz static
#endif
NTSTATUS
ComputeNetlogonCredentialDes(
    PBYTE sessionKey,
    DWORD sessionKeyLen,
    PBYTE inputChallenge, 
    int inputChallengeLen,
    PBYTE retCredential,
    DWORD *retCredentialLen)
{
    NTSTATUS ntStatus  = 0;
    unsigned char key1[8] = {0};
    unsigned char key2[8] = {0};
    unsigned char key3[8] = {0};
    unsigned char key4[8] = {0};
    unsigned char output1[8] = {0};
    int output1Len = sizeof(output1);
    unsigned char output2[8] = {0};
    int output2Len = sizeof(output2);

    CopyBytesRange(0, 6, sessionKey, key1);
    InitLMKey(key1, key3);

    CopyBytesRange(7, 13, sessionKey, key2);
    InitLMKey(key2, key4);

    ntStatus = DesEncryptEcb(
                   key3,
                   inputChallenge,
                   inputChallengeLen,
                   output1,
                   &output1Len);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = DesEncryptEcb(
                   key4,
                   output1,
                   sizeof(output1),
                   output2,
                   &output2Len);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *retCredentialLen = output2Len;
    memcpy(retCredential, output2, *retCredentialLen);

cleanup:
     return ntStatus;

error:
    goto cleanup;
}



#ifndef _NETLOGON_UNIT_TEST
static
#endif
NTSTATUS
ComputeNetlogonCredentialAes(
    PBYTE challengeData,
    DWORD challengeDataLen,
    PBYTE key,
    DWORD keyLen,
    PBYTE credentialData,
    DWORD credentialDataLen)
{
    BYTE IV[16] = {0}; /* Assuming IV is same length as key */
    DWORD IV_len = 0;
    NTSTATUS ntStatus = 0;
    EVP_CIPHER_CTX enc_ctx = {0};
    BYTE encryptedData[16] = {0};
    int encryptedDataLen = sizeof(encryptedData);
    int encryptedDataFinalLen = 0;

    ntStatus = EVP_EncryptInit_ex(
                   &enc_ctx,
                   EVP_aes_128_cfb8(),
                   NULL, /* Engine implementation */
                   key,
                   IV);
    if (ntStatus == 0)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    /* Double check IV length is correct */
    IV_len = EVP_CIPHER_iv_length(EVP_aes_128_cfb8());
    if (IV_len != sizeof(IV))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    ntStatus = EVP_EncryptUpdate(
                   &enc_ctx, 
                   encryptedData,
                   &encryptedDataLen,
                   challengeData,
                   challengeDataLen);
    if (ntStatus == 0)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    ntStatus = EVP_EncryptFinal_ex(&enc_ctx, 
                                   &encryptedData[encryptedDataLen],
                                   &encryptedDataFinalLen);
    if (ntStatus == 0)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    /* encryptedDataFinalLen == credentialDataLen I think */
    memcpy(credentialData, encryptedData, credentialDataLen);

    /* EVP_xxx() return 1 for success. Being here means success */
    ntStatus = 0;

cleanup:
    return ntStatus;
}


NTSTATUS
ComputeHmacMD5(
    unsigned char *key,
    unsigned int keyLen,
    unsigned char *data1,
    unsigned int data1Len,
    unsigned char *data2,
    unsigned int data2Len,
    unsigned char *mac,
    unsigned int macLen)
{
    unsigned int ntStatus = 0;
    HMAC_CTX hctx;
    unsigned char hmac_buf[SHA512_DIGEST_LENGTH] = {0};
    unsigned int hmac_len = 0;


    HMAC_CTX_init(&hctx);

    if (HMAC_Init(&hctx,
        key,
        keyLen,
        EVP_md5()) == 0)
    {
         ntStatus = 5;
         goto cleanup;
    }

    if (data1)
    {
        if (HMAC_Update(&hctx, data1, data1Len) != 1)
        {
            ntStatus = 5;
            goto cleanup;
        }
    }
    if (data2)
    {
        if (HMAC_Update(&hctx, data2, data2Len) != 1)
        {
            ntStatus = 5;
            goto cleanup;
        }
    }
    if (HMAC_Final(&hctx, hmac_buf, &hmac_len) == 0)
    {
        ntStatus = 5;
        goto cleanup;
    }

    ntStatus = 0;
    memcpy(mac, hmac_buf, hmac_len);

cleanup:
    HMAC_cleanup(&hctx);

    return ntStatus;
}

#ifndef _NETLOGON_UNIT_TEST
static
#endif
NTSTATUS
ComputeSessionKeyMD5(
    PBYTE sharedSecret,
    DWORD sharedSecretLen,
    PBYTE cliNonce,
    DWORD cliNonceLen,
    PBYTE srvNonce,
    DWORD srvNonceLen,
    PBYTE sessionKey,
    DWORD sessionKeyLen)
{
    NTSTATUS ntStatus = 0;
    MD5_CTX md5_ctx;
    UCHAR md4DigestBuf[MD4_DIGEST_LENGTH] = {0};
    BYTE zeros4[4] = {0};
    BYTE hmac_md5_buf[16] = {0};
    DWORD hmac_md5_len = sizeof(hmac_md5_buf);
    BYTE md5_buf[MD5_DIGEST_LENGTH] = {0};
    DWORD md5_len = sizeof(md5_buf);
    BOOLEAN bHaveSharedSecret = FALSE;

    if (strncmp((char *) sharedSecret, "owfdigest:",  10) == 0)
    {
        /* Test vector with MD4 digest of unicodePassword already computed */
        bHaveSharedSecret = TRUE;
        sharedSecretLen -= 10;
        sharedSecret += 10;
        memcpy(md4DigestBuf, sharedSecret, sharedSecretLen);
    }

    if (!bHaveSharedSecret &&
        compute_md4_digest((unsigned char *) sharedSecret,
                           sharedSecretLen,
                           md4DigestBuf) == 0)
    {
        ntStatus = ERROR_INVALID_PASSWORD;
        goto cleanup;
    }

    ntStatus = MD5_Init(&md5_ctx);
    if (ntStatus == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    ntStatus = MD5_Update(&md5_ctx, zeros4, sizeof(zeros4));
    if (ntStatus == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    ntStatus = MD5_Update(&md5_ctx, cliNonce, cliNonceLen);
    if (ntStatus == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    ntStatus = MD5_Update(&md5_ctx, srvNonce, srvNonceLen);
    if (ntStatus == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    ntStatus = MD5_Final(md5_buf, &md5_ctx);
    if (ntStatus == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    ntStatus = ComputeHmacMD5(
                   md4DigestBuf, sizeof(md4DigestBuf),
                   md5_buf, md5_len,
                   NULL, 0, 
                   hmac_md5_buf, hmac_md5_len);
    if (ntStatus)
    {
        goto cleanup;
    }

    ntStatus = 0; 
    memcpy(sessionKey, hmac_md5_buf, sessionKeyLen);
    
cleanup:
    return ntStatus;
}


#ifndef _NETLOGON_UNIT_TEST
static
#endif
NTSTATUS
ComputeSessionKeyAes(
    PBYTE sharedSecret,
    DWORD sharedSecretLen,
    PBYTE cliNonce,
    DWORD cliNonceLen,
    PBYTE srvNonce,
    DWORD srvNonceLen,
    PBYTE sessionKey,
    DWORD sessionKeyLen)
{
    NTSTATUS ntStatus = 0;
    UCHAR md4DigestBuf[MD4_DIGEST_LENGTH] = {0};
    HMAC_CTX hctx;
    unsigned char hmac_buf[SHA512_DIGEST_LENGTH] = {0};
    unsigned int hmac_len = 0;
    unsigned int i = 0;
    BOOLEAN bHaveSharedSecret = FALSE;

    if (strncmp((char *) sharedSecret, "owfdigest:",  10) == 0)
    {
        /* Test vector with MD4 digest of unicodePassword already computed */
        bHaveSharedSecret = TRUE;
        sharedSecretLen -= 10; 
        sharedSecret += 10; 
        memcpy(md4DigestBuf, sharedSecret, sharedSecretLen);
    }

    if (!bHaveSharedSecret &&
        compute_md4_digest((unsigned char *) sharedSecret,
                           sharedSecretLen,
                           md4DigestBuf) == 0)
    {
        ntStatus = ERROR_INVALID_PASSWORD;
        goto cleanup;
    }

    HMAC_CTX_init(&hctx);

    if (HMAC_Init(&hctx,
        md4DigestBuf,
        sizeof(md4DigestBuf),
        EVP_sha256()) == 0)
    {
         ntStatus = ERROR_INVALID_HANDLE;
         goto cleanup;
    }

    if (HMAC_Update(&hctx, cliNonce, cliNonceLen) != 1)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }
    if (HMAC_Update(&hctx, srvNonce, srvNonceLen) != 1)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }
    if (HMAC_Final(&hctx, hmac_buf, &hmac_len) == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    for (i=0; i<16 && i<hmac_len && i<sessionKeyLen; i++)
    {
        sessionKey[i] = hmac_buf[i];
    }

cleanup:
    HMAC_cleanup(&hctx);

    return ntStatus;
}

#if 0 /* TBD:Adam Deleteme, just copyied sample code */
static
void 
digest_message(const unsigned char *message, size_t message_len, unsigned char **digest, unsigned int *digest_len)
{
    EVP_MD_CTX *mdctx;

    if((mdctx = EVP_MD_CTX_create()) == NULL)
        handleErrors();

    if(1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL))
        handleErrors();

    if(1 != EVP_DigestUpdate(mdctx, message, message_len))
        handleErrors();

    if((*digest = (unsigned char *)OPENSSL_malloc(EVP_MD_size(EVP_sha256()))) == NULL)
        handleErrors();

    if(1 != EVP_DigestFinal_ex(mdctx, *digest, digest_len))
        handleErrors();

    EVP_MD_CTX_destroy(mdctx);
}
#endif


NTSTATUS srv_netr_Function00(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function01(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


    /* function 0x02 */
NTSTATUS srv_NetrLogonSamLogon(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ NetrAuth *creds,
        /* [in] */ NetrAuth *ret_creds,
        /* [in] */ UINT16 logon_level,
        /* [in] */ NetrLogonInfo *logon,
        /* [in] */ UINT16 validation_level,
        /* [out] */ NetrValidationInfo *validation,
        /* [out] */ UINT8 *authoritative
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}

	/* function 0x03 */
NTSTATUS srv_NetrLogonSamLogoff(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ NetrAuth *creds,
        /* [in] */ NetrAuth *ret_creds,
        /* [in] */ UINT16 logon_level,
        /* [in] */ NetrLogonInfo *logon
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}


	/* function 0x04 */
NTSTATUS srv_NetrServerReqChallenge(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ NetrCred *client_challenge,
        /* [out] */ NetrCred *server_challenge
        )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HANDLE hDirectory = NULL;
    PNETLOGON_AUTH_PROVIDER_CONTEXT pContext = NULL;
    LDAP *pLd = NULL;
    PSTR pszLdapSearchBase = NULL;
    PSTR ppszAttributes[] = { "unicodePwd", NULL }; /* TBD:Adam-Store md4 hash?? */
    PSTR pszDnsDomainName = NULL;
    PSTR pszDomainDn = NULL;
    PSTR pszComputerName = NULL;
    LDAPMessage *pLdapObject = NULL;
    struct berval **bv_objectValue = NULL;
    PWSTR pwszUnicodePwd = NULL;
    PWSTR pwszUnicodePwdAlloc = NULL;
    DWORD dwUnicodePwdLen = 0;
    BOOLEAN bUseAesSessionKey = TRUE; /* TBD:Adam-How to determine what client skey is computed? */

    UCHAR randBytes[8];
    ULONG ulRandBytesLen = sizeof(randBytes);
    PBYTE pClientChallenge = NULL;
    PBYTE pServerChallenge = NULL;
    DWORD dwClientChallengeLen = sizeof(randBytes);
    DWORD dwServerChallengeLen = dwClientChallengeLen;
    WCHAR wcQuote = { '"' };

    dwError = NetlogonLdapOpen(&hDirectory);
    BAIL_ON_NTSTATUS_ERROR(dwError);

    pContext = (PNETLOGON_AUTH_PROVIDER_CONTEXT) hDirectory;
    pLd = pContext->dirContext.pLd;

    /* Obtain the domain name for this DC */
    ntStatus = LwRtlCStringDuplicate(&pszDnsDomainName,
                                     pContext->dirContext.pBindInfo->pszDomainFqdn);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwLdapConvertDomainToDN(pszDnsDomainName,
                                      &pszDomainDn);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    ntStatus = LwRtlCStringAllocateFromWC16String(
                 &pszComputerName,
                 computer_name);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwAllocateStringPrintf(
                  &pszLdapSearchBase,
                  "cn=%s,cn=Computers,%s",
                  pszComputerName,
                  pszDomainDn);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    /* Get the computer account password */
    dwError = NetlogonLdapQueryObjects(
                  pLd,
                  pszLdapSearchBase,
                  LDAP_SCOPE_SUBTREE,
                  NULL,
                  ppszAttributes,
                  -1,
                  &pLdapObject);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    bv_objectValue = ldap_get_values_len(pLd, pLdapObject, ppszAttributes[0]);
    if (bv_objectValue && bv_objectValue[0])
    {
         pwszUnicodePwd = (PWSTR) bv_objectValue[0]->bv_val;
         dwUnicodePwdLen = (DWORD) bv_objectValue[0]->bv_len;
    }
    else
    {
        ntStatus = STATUS_OBJECT_NAME_NOT_FOUND;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }


    /* Populate this value! */
    ntStatus = NetlogonSrvAllocateMemory(
                 (VOID *) &pwszUnicodePwdAlloc,
                 dwUnicodePwdLen + sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memcpy(pwszUnicodePwdAlloc, pwszUnicodePwd, dwUnicodePwdLen);
    pwszUnicodePwdAlloc[dwUnicodePwdLen] = (WCHAR) 0;

#ifdef _NETLOGON_TEST_VECTORS
    /* Respond with using previously captured data */
    if (memcmp(client_challenge->data, cliNonceTest, 8) == 0)
    {
        bUseTestVector = TRUE;
        memcpy(pwszUnicodePwdAlloc, unicodePwdTest, sizeof(unicodePwdTest));
        pwszUnicodePwdAlloc[sizeof(unicodePwdTest) / sizeof(WCHAR)] = (WCHAR) 0;
    }
#endif

    dwUnicodePwdLen = (DWORD) LwRtlWC16StringNumChars(pwszUnicodePwdAlloc);

    /* Must prune off the " " around the password */
    if (pwszUnicodePwdAlloc[0] == wcQuote &&
        pwszUnicodePwdAlloc[dwUnicodePwdLen-1] == wcQuote)
    {
        pwszUnicodePwd = pwszUnicodePwdAlloc + 1;

        /* -2, as the strlen is one shorter after removing the leading " */
        pwszUnicodePwd[dwUnicodePwdLen - 2] = (WCHAR) '\0';

        /* TBD:Adam Debug */
        dwUnicodePwdLen = (DWORD) LwRtlWC16StringNumChars(pwszUnicodePwd);
    }
    else
    {
        pwszUnicodePwd = pwszUnicodePwdAlloc;
    }

    /* Generate server nonce (challenge) */
    RAND_pseudo_bytes(randBytes, ulRandBytesLen);
    memcpy(server_challenge->data, randBytes, ulRandBytesLen);
    pServerChallenge = server_challenge->data;

    
    /* Save off client nonce (challenge) */
    pClientChallenge = client_challenge->data;

#ifdef _NETLOGON_TEST_VECTORS
    if (bUseTestVector)
    {
        /* Use canned server nonce in response to canned client nonce */
        /* The computed credential should match the canned server cred */
        memcpy(server_challenge->data, srvNonceTest, ulRandBytesLen);
    }
#endif

    if (bUseAesSessionKey)
    {
        ntStatus = ComputeSessionKeyAes(
                       (PBYTE) pwszUnicodePwd,
                       dwUnicodePwdLen * sizeof (WCHAR),
                       pClientChallenge,
                       dwClientChallengeLen, /* Same length a server challenge */
                       pServerChallenge,
                       dwServerChallengeLen,
                       g_bzSaveSessionKey,
                       sizeof(g_bzSaveSessionKey));
    }
    else
    {
        ntStatus = ComputeSessionKeyMD5(
                       (PBYTE) pwszUnicodePwd,
                       dwUnicodePwdLen * sizeof (WCHAR),
                       pClientChallenge,
                       dwClientChallengeLen, /* Same length a server challenge */
                       pServerChallenge,
                       dwServerChallengeLen,
                       g_bzSaveSessionKey,
                       sizeof(g_bzSaveSessionKey));
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* Compute client credential */
    ntStatus = ComputeNetlogonCredentialAes(
                   pClientChallenge,
                   dwClientChallengeLen,
                   g_bzSaveSessionKey,
                   sizeof(g_bzSaveSessionKey),
                   g_bzClientCredential,
                   sizeof(g_bzClientCredential));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* Compute server credential */
    ntStatus = ComputeNetlogonCredentialAes(
                   pServerChallenge,
                   dwServerChallengeLen,
                   g_bzSaveSessionKey,
                   sizeof(g_bzSaveSessionKey),
                   g_bzServerCredential,
                   sizeof(g_bzServerCredential));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

#if 1
{
    /* Write client/server credentials to files for debugging */
    FILE *fp = fopen("/tmp/clicred.dat", "w");
    fwrite(g_bzClientCredential, 1,  sizeof(g_bzClientCredential), fp);
    fclose(fp);

    fp = fopen("/tmp/srvcred.dat", "w");
    fwrite(g_bzServerCredential, 1,  sizeof(g_bzServerCredential), fp);
    fclose(fp);
}
#endif

cleanup:
    return ntStatus ? ntStatus : dwError;

error:
    goto cleanup;
}



	/* function 0x05 */
NTSTATUS srv_NetrServerAuthenticate(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t account_name[],
        /* [in] */ UINT16 secure_channel_type,
        /* [in] */ wchar16_t computer_name[],
        /* [in] */ NetrCred *cli_credentials,
        /* [out] */ NetrCred *srv_credentials
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}



NTSTATUS srv_netr_Function06(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function07(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function08(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function09(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0c(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0d(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0e(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


	/* function 0x0f */
NTSTATUS srv_NetrServerAuthenticate2(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t account_name[],
        /* [in] */ UINT16 secure_channel_type,
        /* [in] */ wchar16_t computer_name[],
        /* [in] */ NetrCred *cli_credentials,
        /* [out] */ NetrCred *srv_credentials,
        /* [in] */ UINT32 *negotiate_flags
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}



NTSTATUS srv_netr_Function10(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function11(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function12(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function13(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


    /* function 0x14 */
WINERROR srv_DsrGetDcName(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *domain_name,
        /* [in] */ GUID *domain_guid,
        /* [in] */ GUID *site_guid,
        /* [in] */ UINT32 get_dc_flags,
        /* [out] */ DsrDcNameInfo **info
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}



NTSTATUS srv_netr_Function15(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function16(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function17(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function18(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function19(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}



	/* function 0x1a */
NTSTATUS srv_NetrServerAuthenticate3(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t account_name[],
        /* [in] */ UINT16 secure_channel_type,
        /* [in] */ wchar16_t computer_name[],
        /* [in] */ NetrCred *credentials_in,
        /* [out] */ NetrCred *credentials_out,
        /* [in] */ UINT32 *negotiate_flags,
        /* [out] */ UINT32 *rid
        )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG retRid = 0;
    PNETLOGON_AUTH_PROVIDER_CONTEXT pContext = NULL;
    HANDLE hDirectory = NULL;
    LDAP *pLd = NULL;
    PSTR pszDnsDomainName = NULL;
    PSTR pszDomainDn = NULL;
    PSTR pszAccountName = NULL;
    PSTR pszLdapSearchBase = NULL;
    PSTR ppszAttributes[] = { "objectSid", NULL };
    LDAPMessage *pObjectSid = NULL;
    struct berval **bv_objectValue = NULL;
    PSID pDomainSid = NULL;

    BAIL_ON_INVALID_PTR(credentials_out);


    /* Validate locally computed client credential with client's value */
    if (memcmp(credentials_in->data,
               g_bzClientCredential, 
               sizeof(g_bzClientCredential)) != 0)
    {
        ntStatus = ERROR_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }
                   
    dwError = NetlogonLdapOpen(&hDirectory);
    BAIL_ON_NTSTATUS_ERROR(dwError);

    pContext = (PNETLOGON_AUTH_PROVIDER_CONTEXT) hDirectory;
    pLd = pContext->dirContext.pLd;

    /* Obtain the domain name for this DC */
    ntStatus = LwRtlCStringDuplicate(
                   &pszDnsDomainName,
                   pContext->dirContext.pBindInfo->pszDomainFqdn);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwLdapConvertDomainToDN(pszDnsDomainName,
                                      &pszDomainDn);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    ntStatus = LwRtlCStringAllocateFromWC16String(
                  &pszAccountName,
                 account_name);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwAllocateStringPrintf(
                  &pszLdapSearchBase,
                  "(sAMAccountName=%s)",
                  pszAccountName);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    /* Get the domain objectSid */
    dwError = NetlogonLdapQueryObjects(
                  pLd,
                  pszDomainDn,
                  LDAP_SCOPE_SUBTREE,
                  pszLdapSearchBase,
                  ppszAttributes,
                  -1,
                  &pObjectSid);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    bv_objectValue = ldap_get_values_len(pLd, pObjectSid, ppszAttributes[0]);
    if (bv_objectValue && bv_objectValue[0])
    {
         pDomainSid = (PSID) bv_objectValue[0]->bv_val;
    }

    ntStatus = RtlGetRidSid(
                   &retRid,
                   pDomainSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

#ifdef _NETLOGON_TEST_VECTORS
    if (bUseTestVector)
    {
#if 0
        memcpy(credentials_out->data,
               srvCredentialTest,
               sizeof(g_bzServerCredential));
#endif
        *rid = 1103;
    }
    else
    {
#if 0
        /* Return previously computed server credential */
        memcpy(credentials_out->data,
               g_bzServerCredential,
               sizeof(g_bzServerCredential));
#endif

        *rid = retRid;
    }
#endif

    /* Return previously computed server credential */
    memcpy(credentials_out->data,
           g_bzServerCredential,
           sizeof(g_bzServerCredential));

cleanup:
    LW_SAFE_FREE_MEMORY(pszAccountName);
    NetlogonLdapRelease(hDirectory);
#ifdef _NETLOGON_TEST_VECTORS
    bUseTestVector = FALSE;
#endif
    return ntStatus;

error:
    goto cleanup;
}



NTSTATUS srv_netr_Function1b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function1c(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


    /* function 0x1d */
NTSTATUS srv_NetrLogonGetDomainInfo(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ NetrAuth *creds,
        /* [in] */ NetrAuth *ret_creds,
        /* [in] */ UINT32 level,
        /* [in] */ NetrDomainQuery *query,
        /* [out] */ NetrDomainInfo *info
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}




NTSTATUS srv_netr_Function1e(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function1f(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function20(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function21(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function22(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function23(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


	/* function 0x24 */
NTSTATUS srv_NetrEnumerateTrustedDomainsEx(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [out] */ NetrDomainTrustList *domain_trusts
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}



NTSTATUS srv_netr_Function25(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function26(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


	/* function 0x27 */
NTSTATUS srv_NetrLogonSamLogonEx(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ UINT16 logon_level,
        /* [in] */ NetrLogonInfo *logon,
        /* [in] */ UINT16 validation_level,
        /* [out] */ NetrValidationInfo *validation,
        /* [out] */ UINT8 *authoritative,
        /* [in] */ UINT32 *flags
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}




/* function 0x28 */
WINERROR
srv_DsrEnumerateDomainTrusts(
        /* [in] */  handle_t IDL_handle,
        /* [in] */  wchar16_t *server_name,
        /* [in] */  UINT32 trust_flags,
        /* [out] */ NetrDomainTrustList *trusts
        )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszServerName = NULL;
    DWORD dwDomainTrustCount = 1;
    DWORD i = 0;
    NetrDomainTrust *pDomainTrustArray = {0};
    CHAR szHostFqdn[256] = {0}; /* MAX hostname length */
    CHAR szNetBiosName[32] = {0}; /* NetBIOS formatted name */
    PSTR pszDnsDomainName = NULL;
    PVOID pDomainGuid = NULL;
    PSTR pszDomainDn = NULL;
    DWORD dwDomainGuidLen = 0;
    HANDLE hDirectory = NULL;

#if 1 /* TBD:Adam-Perform ldap queries to get this data; hard code now */
    /* 0x1d */
    DWORD dwTrustFlags = NETR_TRUST_FLAG_NATIVE  | NETR_TRUST_FLAG_PRIMARY |
                         NETR_TRUST_FLAG_TREEROOT | NETR_TRUST_FLAG_IN_FOREST;
    DWORD dwParentIndex = 0x00;
    DWORD dwTrustType = 0x02;
    DWORD dwTrustAttrs = 0x00;
#endif

    PWSTR pwszNetBiosName = NULL;
    PWSTR pwszDnsDomainName = NULL;
    PSID pDomainSid = NULL;
    uuid_t domainGuid;
    PNETLOGON_AUTH_PROVIDER_CONTEXT pContext = NULL;
    LDAP *pLd = NULL;
    PSTR ppszAttributes[] = { "objectSid", "objectGUID", NULL };
    LDAPMessage *pObjects = NULL;
    struct berval **bv_objectValue = NULL;

    dwError = NetlogonLdapOpen(&hDirectory);
    BAIL_ON_NTSTATUS_ERROR(dwError);

    pContext = (PNETLOGON_AUTH_PROVIDER_CONTEXT) hDirectory;
    pLd = pContext->dirContext.pLd;

    gethostname(szHostFqdn, sizeof(szHostFqdn));


    /* NetBios Hostname */
    for (i=0; szHostFqdn[i] && i<15 && szHostFqdn[i] != '.'; i++)
    {
        szNetBiosName[i] = (CHAR) toupper((int) szHostFqdn[i]);
    }
    szNetBiosName[i] = '\0';

    /* Obtain the domain name for this DC */
    status = LwRtlCStringDuplicate(&pszDnsDomainName,
                                    pContext->dirContext.pBindInfo->pszDomainFqdn);
    BAIL_ON_NTSTATUS_ERROR(status);

    dwError = LwLdapConvertDomainToDN(pszDnsDomainName,
                                      &pszDomainDn);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    /* Get the domain objectSid */
    dwError = NetlogonLdapQueryObjects(
                  pLd,
                  pszDomainDn,
                  LDAP_SCOPE_SUBTREE,
                  "(objectClass=*)",
                  ppszAttributes,
                  -1,
                  &pObjects);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    bv_objectValue = ldap_get_values_len(pLd, pObjects, ppszAttributes[0]);
    if (bv_objectValue && bv_objectValue[0])
    {
         pDomainSid = (PSID) bv_objectValue[0]->bv_val;
    }

    /* Get the domain objectGUID */
    bv_objectValue = ldap_get_values_len(pLd, pObjects, ppszAttributes[1]);
    if (bv_objectValue && bv_objectValue[0])
    {
         pDomainGuid = bv_objectValue[0]->bv_val;
         dwDomainGuidLen = bv_objectValue[0]->bv_len;
    }

    /*
     * I am the DC, so return data returned from calling
     *  NetrLogonGetDomainInfo()
     *
     * "If the server is a domain controller (section 3.1.4.8), it MUST perform
     *  behavior equivalent to locally invoking NetrLogonGetDomainInfo with
     *  the previously described parameters."
     */
    status = LwRtlCStringAllocateFromWC16String(
                 &pszServerName,
                 server_name);
    BAIL_ON_NTSTATUS_ERROR(status);

    LSA_LOG_ERROR("srv_NetrEnumerateTrustedDomainsEx: server_name=%s trusts=%x",
                  pszServerName, trust_flags);

    status = LwRtlWC16StringAllocateFromCString(
                 &pwszNetBiosName,
                 szNetBiosName);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = LwRtlWC16StringAllocateFromCString(
                 &pwszDnsDomainName,
                 pszDnsDomainName);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* Convert DomainGuid to binary form */

    if (dwDomainGuidLen == 16)
    {
        memcpy(domainGuid, pDomainGuid, dwDomainGuidLen);
    }
    else if (dwDomainGuidLen != 36 || uuid_parse(pDomainGuid, domainGuid) != 0)
    {
        status = STATUS_NO_GUID_TRANSLATION;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    /* Populate this value! */
    status = NetlogonSrvAllocateMemory(
                 (VOID **) &pDomainTrustArray,
                 sizeof(NetrDomainTrust) * dwDomainTrustCount);
    BAIL_ON_NTSTATUS_ERROR(status);

    pDomainTrustArray[0].netbios_name = pwszNetBiosName;
    pDomainTrustArray[0].dns_name = pwszDnsDomainName;
    pDomainTrustArray[0].trust_flags = dwTrustFlags;
    pDomainTrustArray[0].parent_index = dwParentIndex;
    pDomainTrustArray[0].trust_type = dwTrustType;
    pDomainTrustArray[0].trust_attrs = dwTrustAttrs; /* TBD:Adam ??? */
    pDomainTrustArray[0].sid = pDomainSid;
    memcpy(&pDomainTrustArray[0].guid, &domainGuid, sizeof(domainGuid));

    trusts->count = dwDomainTrustCount;
    trusts->array = pDomainTrustArray;

cleanup:
    LW_SAFE_FREE_MEMORY(pszServerName);
    NetlogonLdapRelease(hDirectory);

    return status;

error:
    status = status ? status : dwError;
    LW_SAFE_FREE_MEMORY(pwszNetBiosName);
    LW_SAFE_FREE_MEMORY(pwszDnsDomainName);
    LW_SAFE_FREE_MEMORY(pDomainSid);
    LW_SAFE_FREE_MEMORY(pDomainTrustArray);

    goto cleanup;
}



NTSTATUS srv_netr_Function29(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2c(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2d(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2e(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2f(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function30(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function31(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function32(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function33(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function34(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function35(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function36(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function37(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function38(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function39(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function3a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function3b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
