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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 *          Adam Bernstein (abernstein@vmware.com)
 */

#include "includes.h"

static
uint32 schn_unwrap_md5(void              *sec_ctx,
                   uint32                sec_level,
                   struct schn_blob     *in,
                   struct schn_blob     *out,
                   struct schn_tail     *tail);

static
uint32 schn_unwrap_aes(void              *sec_ctx,
                   uint32                sec_level,
                   struct schn_blob     *in,
                   struct schn_blob     *out,
                   struct schn_blob     *confounder,
                   struct schn_tail     *tail);
/*
 * Unwrap the packet after receiving
 */

uint32 schn_unwrap(void                 *sec_ctx,
                   uint32                sec_level,
                   struct schn_blob     *in,
                   struct schn_blob     *out,
                   struct schn_tail     *tail)
{
    uint32 status = schn_s_ok;
    uint16 sign_alg = 0;
    uint16 seal_alg = 0;
    struct schn_blob confounder_decrypted = {0};
    unsigned char confounder_decrypted_buf[8] = {0};
    struct schn_auth_ctx *schn_ctx = NULL;

    schn_ctx = (struct schn_auth_ctx*)sec_ctx;

    status = schn_discover_crypto_algorithm(tail, &sign_alg, &seal_alg);
    if (status == SEC_E_MESSAGE_ALTERED)
    {
        /* This is a hard fail and cannot proceed */
        goto error;
    }
    schn_ctx->sign_type = sign_alg;
    schn_ctx->seal_type = seal_alg;

    /* pad value, must be 0xffff for AES */
    memcpy(&schn_ctx->pad,  &tail->signature[4], sizeof(uint16));

    /* flags value, ignored, must be 0x0000 */
    memcpy(&schn_ctx->flags,  &tail->signature[6], sizeof(uint16));

    out->len  = in->len;
    out->base = malloc(out->len ? out->len + /*TBD*/ 16 : 1);
    if (out->base == NULL) {
        status = schn_s_no_memory;
        goto error;
    }

    switch (seal_alg)
    {
    case SCHANNEL_SEAL_ALG_AES:
        /* Return confounder argument */
        confounder_decrypted.base = confounder_decrypted_buf;
        confounder_decrypted.len = sizeof(confounder_decrypted_buf);

        status = schn_unwrap_aes(sec_ctx,
                                 sec_level,
                                 in,
                                 out,
                                 &confounder_decrypted,
                                 tail);
        break;

    case SCHANNEL_SEAL_ALG_MD4:
        memcpy(out->base, in->base, out->len);
        status = schn_unwrap_md5(sec_ctx,
                                 sec_level,
                                 in,
                                 out,
                                 tail);
        break;

    default:
        /* TBD:Adam-Return error here */
        break;
    }

cleanup:
    return status;

error:
    goto cleanup;
}


static
uint32 schn_unwrap_md5(void              *sec_ctx,
                   uint32                sec_level,
                   struct schn_blob     *in,
                   struct schn_blob     *out,
                   struct schn_tail     *tail)
{
    uint32 status = schn_s_ok;
    struct schn_auth_ctx *schn_ctx = NULL;
    uint8 seq_number[8], digest[8];
    uint32 sender_flags;
    unsigned char *schannel_sig;
    uint8 seal_key[16];

    schn_ctx = (struct schn_auth_ctx*)sec_ctx;

    switch (sec_level) {
    case SCHANNEL_SEC_LEVEL_INTEGRITY:
        schannel_sig = (unsigned char*)schannel_sig_sign;
        break;

    case SCHANNEL_SEC_LEVEL_PRIVACY:
        schannel_sig = (unsigned char*)schannel_sig_seal;
        break;

    default:
        status = schn_s_unsupported_protect_level;
        goto error;
    }

    /* if we're an initiator we should expect a packet from acceptor */
    sender_flags = (schn_ctx->sender_flags == SCHANNEL_INITIATOR_FLAGS) ?
                    SCHANNEL_ACCEPTOR_FLAGS : SCHANNEL_INITIATOR_FLAGS;

    /* create original sequence number that should be used
       to sign the received packet */
    schn_sign_get_seq_number(schn_ctx, sender_flags, seq_number);

    /* decode received sequence number and compare the result
       with expected sequence number */
    schn_sign_update_seqnum(tail->digest,
                            schn_ctx->session_key,
                            &schn_ctx->seq_num,
                            tail->seq_number);

    if (memcmp((void*)tail->seq_number,
               (void*)seq_number,
               sizeof(tail->seq_number))) {
        status = schn_s_invalid_credentials;
        goto error;
    }

    /* check whether schannel signature is correct */
    if (memcmp((void*)tail->signature,
               (void*)schannel_sig,
               sizeof(tail->signature))) {
        status = schn_s_invalid_credentials;
        goto error;
    }

    if (sec_level == SCHANNEL_SEC_LEVEL_PRIVACY) {
        RC4_KEY key_nonce, key_data;

        memset(&key_nonce, 0, sizeof(key_nonce));
        memset(&key_data, 0, sizeof(key_data));

        /* Prepare sealing key */
        schn_seal_generate_key(schn_ctx->session_key,
                               tail->seq_number, seal_key);

        /* Decrypt nonce */
        RC4_set_key(&key_nonce, sizeof(seal_key), (unsigned char*)seal_key);
        RC4(&key_nonce, sizeof(tail->nonce), (unsigned char*)tail->nonce,
            (unsigned char*)tail->nonce);

        /* Decrypt the payload */
        RC4_set_key(&key_data, sizeof(seal_key), (unsigned char*)seal_key);
        RC4(&key_data, out->len, (unsigned char*)out->base,
            (unsigned char*)out->base);

    }

    /* check the packet payload digest */
    schn_sign_digest(schn_ctx->session_key,
                     tail->nonce, schannel_sig,
                     out, digest);

    if (memcmp((void*)tail->digest,
               (void*)digest,
               sizeof(tail->digest))) {
        status = schn_s_invalid_credentials;
        goto error;
    }

cleanup:
    return status;

error:
    goto cleanup;
}

static
uint32
schn_decrypt_aes_128_cfb8(
    EVP_CIPHER_CTX *dec_ctx,
    uint8 key[16],
    int keylen,
    uint8 IV[16],
    int IVlen,
    uint8 *indata,
    int indata_len,
    uint8 *outdata,
    int *outdata_len)
{
    uint32 status = schn_s_ok;
    int dec_ctx_init = 0;
    int decryptedDataFinalLen = 0;
    int decryptedDataLen = 0;

    if (!EVP_CIPHER_CTX_cipher(dec_ctx))
    {
        /* Double check IV and key length is correct */
        if (IVlen != EVP_CIPHER_iv_length(EVP_aes_128_cfb8()) &&
            IVlen != keylen)
        {
            status = STATUS_INVALID_PARAMETER;
            goto cleanup;
        }

        /* Returns 1=SUCCESS, 0=FAILURE */
        status = EVP_DecryptInit_ex(
                       dec_ctx,
                       EVP_aes_128_cfb8(),
                       NULL, /* Engine implementation */
                       key,
                       IV);
        if (status == 0)
        {
            status = STATUS_INVALID_PARAMETER;
            goto cleanup;
        }
        dec_ctx_init = 1;
    }

    status = EVP_DecryptUpdate(
                 dec_ctx,
                 outdata,
                 &decryptedDataLen,
                 indata,
                 indata_len);
    if (status == 0)
    {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    status = EVP_DecryptFinal_ex(dec_ctx,
                                 &outdata[decryptedDataLen],
                                 &decryptedDataFinalLen);
    decryptedDataLen += decryptedDataFinalLen;
    if (status == 0)
    {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    status = 0;
    *outdata_len = decryptedDataLen;

cleanup:
    if (dec_ctx_init && status)
    {
        EVP_CIPHER_CTX_cleanup(dec_ctx);
        memset(dec_ctx, 0, sizeof(*dec_ctx));
    }

    return status;
}

static
uint32 schn_unwrap_aes(void             *sec_ctx,
                       uint32           sec_level,
                       struct schn_blob *in,
                       struct schn_blob *out,
                       struct schn_blob *confounder, /* Decrypted confounder */
                       struct schn_tail *tail)
{
    uint32 status = schn_s_ok;
    uint8 IV[16] = {0}; /* Assuming IV is same length as key */
    struct schn_auth_ctx *schn_ctx = NULL;
    uint8 decryptedData[8] = {0};
    int decryptedDataLen = sizeof(decryptedData);
    uint8 copy_seqnum[8] = {0};
    int decrypt_len = 0;
    EVP_CIPHER_CTX dec_ctx = {0};
    struct schn_nl_auth_sha2_signature sha2_signature_header = {0};
    uint8 signature[EVP_MAX_MD_SIZE] = {0};
    unsigned int signature_len = 0;

    uint8 confounder_seskey[16] = {0};
    int i = 0;

    if (!sec_ctx || !in || !out || !confounder || !tail)
    {
        status = STATUS_INVALID_PARAMETER;
        goto error;
    }

    schn_ctx = (struct schn_auth_ctx*)sec_ctx;

    /*
     * Construct IV: initialization vector constructed by concatenating
     * the checksum with itself (thus getting 16 bytes of data).
     * Ref: MS-NRPC 3.3.4.2.2, bullet item 5.
     */
    memcpy(&IV[0], &tail->digest[0], 8);
    memcpy(&IV[8], &tail->digest[0], 8);

    status = schn_decrypt_aes_128_cfb8(
                 &dec_ctx,
                 schn_ctx->session_key,
                 sizeof(schn_ctx->session_key),
                 IV,
                 sizeof(IV),
                 tail->seq_number,
                 sizeof(tail->seq_number),
                 decryptedData,
                 &decryptedDataLen); /* TBD:Adam-Should equal sizeof(seq_num_aes) */
    if (status)
    {
        goto error;
    }
    schn_cleanup_aes_128_cfb8(&dec_ctx);

    schn_construct_seqnum_aes(
        TRUE,   /* Construct client seq_num (initiator) */
        schn_ctx->seq_num_aes,
        copy_seqnum);
    if (memcmp(decryptedData, copy_seqnum, sizeof(copy_seqnum)) != 0)
    {
        status = SEC_E_OUT_OF_SEQUENCE;
        goto error;
    }

    /* The ClientSequenceNumber MUST be incremented by one. */
    schn_ctx->seq_num_aes++;

    /*
     * MS-NETR: 3.3.4.2.2, paragraph #9: 
     * Confidentiality option is requested, the Confounder and the *data*
     * MUST be decrypted.
     *
     * decrypt using an initialization vector constructed by 
     * concatenating twice the sequence number (thus getting 16 bytes of data).
     */
    memcpy(&IV[0], copy_seqnum, sizeof(copy_seqnum));
    memcpy(&IV[8], copy_seqnum, sizeof(copy_seqnum));

    /* Construct the Confounder session key */
    for (i=0; i<sizeof(confounder_seskey); i++)
    {
        confounder_seskey[i] = schn_ctx->session_key[i] ^ 0xf0;
    }

    /* Decrypt confounder */
    status = schn_decrypt_aes_128_cfb8(
                 &dec_ctx,
                 confounder_seskey,
                 sizeof(schn_ctx->session_key),
                 IV,
                 sizeof(IV),
                 tail->nonce,
                 sizeof(tail->nonce),
                 confounder->base,
                 &decrypt_len); /* length of data decrypted */
    if (status)
    {
        goto error;
    }
    confounder->len = decrypt_len;

    /* Decrypt data payload, chained after confounder decrypt */
    status = schn_decrypt_aes_128_cfb8(
                 &dec_ctx,
                 NULL, /* dec_ctx initialized; these 4 args unused */
                 0,
                 NULL,
                 0,
                 in->base,
                 in->len, /* length of data to decrypt */
                 out->base,
                 &decrypt_len); /* length of data decrypted */
    if (status)
    {
        goto error;
    }
    out->len = decrypt_len;

    /* Destroy the confounder/data decryption context */
    schn_cleanup_aes_128_cfb8(&dec_ctx);

    /* first 8 bytes of SCHN_NL_SHA2_HEADER */
    sha2_signature_header.sign_alg = schn_ctx->sign_type;
    sha2_signature_header.seal_alg = schn_ctx->seal_type;
    sha2_signature_header.pad = schn_ctx->pad;
    sha2_signature_header.flags = schn_ctx->flags;

    status = schn_compute_signature_aes(
                 schn_ctx->session_key,
                 sizeof(schn_ctx->session_key),
                 (unsigned char *) &sha2_signature_header,
                 sizeof(sha2_signature_header),
                 confounder->base,
                 confounder->len,
                 out->base, /* This is the decrypted message */
                 out->len,  /* This is the decrypted message len */
                 signature,
                 &signature_len);
    if (status)
    {
        goto error;
    }
                 
    /*
     * 11. The first 8 bytes of the computed signature MUST be compared to the 
     * checksum. If these two do not match, the SEC_E_MESSAGE_ALTERED
     */
    if (memcmp(signature, tail->digest, sizeof(tail->digest)) != 0)
    {
        status = SEC_E_OUT_OF_SEQUENCE;
        goto error;
    }

    status = 0; /* 1=OK for OpenSSL, 0=Success for this function */

cleanup:
    return status;

error:
    schn_cleanup_aes_128_cfb8(&dec_ctx);
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
