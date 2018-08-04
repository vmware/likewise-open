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

uint32 schn_wrap_md5(
                 void                 *sec_ctx,
                 uint32                sec_level,
                 struct schn_blob     *in,
                 struct schn_blob     *out,
                 struct schn_tail     *tail);

uint32 schn_wrap_aes(
                 void                 *sec_ctx,
                 uint32                sec_level,
                 struct schn_blob     *in,
                 struct schn_blob     *out,
                 struct schn_tail     *tail);

/*
 * Wrap the packet prior to sending
 */

uint32 schn_wrap(void                 *sec_ctx,
                 uint32                sec_level,
                 struct schn_blob     *in,
                 struct schn_blob     *out,
                 struct schn_tail     *tail)
{
    uint32 status = schn_s_ok;
    struct schn_auth_ctx *schn_ctx = NULL;

    schn_ctx = (struct schn_auth_ctx*)sec_ctx;

    out->len  = in->len;
    out->base = calloc(sizeof(unsigned char), out->len ? out->len : 1);
    if (out->base == NULL) {
        status = schn_s_no_memory;
        goto error;
    }
    memcpy(out->base, in->base, out->len);

    switch(schn_ctx->seal_type)
    {
    case SCHANNEL_SEAL_ALG_AES:
        status = schn_wrap_aes(sec_ctx,
                 sec_level,
                 in,
                 out,
                 tail);
        break;

    case SCHANNEL_SEAL_ALG_MD4:
        status = schn_wrap_md5(sec_ctx,
                 sec_level,
                 in,
                 out,
                 tail);
        break;

    default:
        /* TBD:Adam-Return error here */
        goto error;
        break;
    }

cleanup:
    return status;

error:
    goto cleanup;
}

uint32 schn_wrap_md5(
                 void                 *sec_ctx,
                 uint32                sec_level,
                 struct schn_blob     *in,
                 struct schn_blob     *out,
                 struct schn_tail     *tail)
{
    uint32 status = schn_s_ok;
    struct schn_auth_ctx *schn_ctx = NULL;
    unsigned char *schannel_sig = NULL;
    unsigned char sess_key[16], nonce[8], seq_number[8], digest[8];
    uint32 sender_flags;
    unsigned char seal_key[16];

    memset(sess_key, 0, sizeof(digest));
    memset(nonce, 0, sizeof(nonce));
    memset(seq_number, 0, sizeof(seq_number));
    memset(digest, 0, sizeof(digest));

    /* Nonce ("pseudo_bytes" call is to be replaced with "bytes"
       once we're ready to properly reseed the generator) */
    RAND_pseudo_bytes((unsigned char*)nonce, sizeof(nonce));

    memcpy(sess_key, schn_ctx->session_key, 16);

    /* Select proper schannel signature */
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

    /* Digest */
    schn_sign_digest(sess_key, nonce, schannel_sig, out, digest);

    sender_flags = schn_ctx->sender_flags;
    schn_sign_get_seq_number(schn_ctx, sender_flags, seq_number);

    if (sec_level == SCHANNEL_SEC_LEVEL_PRIVACY) {
        RC4_KEY key_nonce, key_data;

        memset(&key_nonce, 0, sizeof(key_nonce));
        memset(&key_data, 0, sizeof(key_data));

        /* Prepare sealing key */
        schn_seal_generate_key(schn_ctx->session_key, seq_number, seal_key);

        /* Encrypt the key */
        RC4_set_key(&key_nonce, sizeof(seal_key), (unsigned char*)seal_key);
        RC4(&key_nonce, sizeof(nonce), (unsigned char*)nonce,
            (unsigned char*)nonce);

        /* Encrypt the payload */
        RC4_set_key(&key_data, sizeof(seal_key), (unsigned char*)seal_key);
        RC4(&key_data, out->len, (unsigned char*)out->base,
            (unsigned char*)out->base);
    }

    /* Sequence number */
    schn_sign_update_seqnum(digest, sess_key, &schn_ctx->seq_num, seq_number);

    memcpy(tail->signature,  schannel_sig, 8);
    memcpy(tail->digest,     digest,       8);
    memcpy(tail->seq_number, seq_number,   8);
    memcpy(tail->nonce,      nonce,        8);

cleanup:
    return status;

error:
    goto cleanup;
}

static
uint32 schn_encrypt_aes_128_cfb8(
    EVP_CIPHER_CTX *enc_ctx,
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
    int enc_ctx_init = 0;
    int encryptedDataFinalLen = 0;
    int encryptedDataLen = 0;

    if (!EVP_CIPHER_CTX_cipher(enc_ctx))
    {
        /* Double check IV and key length is correct */
        if (IVlen != EVP_CIPHER_iv_length(EVP_aes_128_cfb8()) &&
            IVlen != keylen)
        {
            status = STATUS_INVALID_PARAMETER;
            goto cleanup;
        }

        /* Returns 1=SUCCESS, 0=FAILURE */
        status = EVP_EncryptInit_ex(
                       enc_ctx,
                       EVP_aes_128_cfb8(),
                       NULL, /* Engine implementation */
                       key,
                       IV);
        if (status == 0)
        {
            status = STATUS_INVALID_PARAMETER;
            goto cleanup;
        }
        enc_ctx_init = 1;
    }
    status = EVP_CIPHER_CTX_set_padding(enc_ctx, 0);
    if (status == 0)
    {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }


    status = EVP_EncryptUpdate(
                 enc_ctx,
                 outdata,
                 &encryptedDataLen,
                 indata,
                 indata_len);
    if (status == 0)
    {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    status = EVP_EncryptFinal_ex(enc_ctx,
                                 &outdata[encryptedDataLen],
                                 &encryptedDataFinalLen);
    encryptedDataLen += encryptedDataFinalLen;
    if (status == 0)
    {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    status = 0;
    *outdata_len = encryptedDataLen;

cleanup:
    if (enc_ctx_init && status)
    {
        EVP_CIPHER_CTX_cleanup(enc_ctx);
        memset(enc_ctx, 0, sizeof(*enc_ctx));
    }

    return status;
}

/*
 * 3.3.4.2.3 Generating a Server Netlogon Signature Token
 */
uint32 schn_wrap_aes(
                 void                 *sec_ctx,
                 uint32                sec_level,
                 struct schn_blob     *in,
                 struct schn_blob     *out,
                 struct schn_tail     *tail)
{
    uint32 status = schn_s_ok;
    uint8 IV[16] = {0}; /* Assuming IV is same length as key */
    struct schn_auth_ctx *schn_ctx = NULL;
    uint8 session_key[16] = {0};
    uint8 confounder[8] = {0};
    uint8 enc_seq_number[8] = {0};
    uint8 copy_seqnum[8] = {0};
    uint8 enc_nonce[8] = {0};
    uint8 signature[EVP_MAX_MD_SIZE] = {0};
    /*
     * First 8 bytes of Signature is checksum
     *     a.k.a. tail->digest
     */
    unsigned char checksum[8] = {0};
    int encrypt_len = 0;
    EVP_CIPHER_CTX enc_ctx = {0};
    struct schn_nl_auth_sha2_signature sha2_signature_header = {0};
    unsigned int signature_len = 0;

    uint8 confounder_seskey[16] = {0};
    int i = 0;

    if (!sec_ctx || !in || !out || !tail)
    {
        status = STATUS_INVALID_PARAMETER;
        goto error;
    }

    schn_ctx = (struct schn_auth_ctx*)sec_ctx;
    memcpy(session_key, schn_ctx->session_key, sizeof(session_key));

    /*
     * 1-4. Same as steps 1-4 in section 3.3.4.2.1
     * first 8 bytes of SCHN_NL_SHA2_HEADER
     */
    sha2_signature_header.sign_alg = schn_ctx->sign_type;
    sha2_signature_header.seal_alg = schn_ctx->seal_type;
    sha2_signature_header.pad = schn_ctx->pad;
    sha2_signature_header.flags = schn_ctx->flags;

    /* 3.3.4.2.3 paragraph 5. Set to TRUE; OR in 4th byte 0x80 */
    schn_construct_seqnum_aes(
        FALSE,   /* Construct server seq_num (NOT initiator) */
        schn_ctx->seq_num_aes,
        copy_seqnum);

    RAND_pseudo_bytes((uint8 *) confounder, sizeof(confounder));

    /*
     * 3.3.4.2.3 #6 Generating a Server Netlogon Signature Token
     */
    status = schn_compute_signature_aes(
                 session_key,
                 sizeof(session_key),
                 (unsigned char *) &sha2_signature_header,
                 sizeof(sha2_signature_header),
                 confounder,
                 sizeof(confounder),
                 out->base,
                 out->len,
                 signature,
                 &signature_len);
    if (status)
    {
        goto cleanup;
    }
    memcpy(checksum, signature, sizeof(checksum));

    /* The ServerSequenceNumber MUST be incremented by one. */
    schn_ctx->seq_num_aes++;

    /* Construct the Confounder session key */
    for (i=0; i<sizeof(confounder_seskey); i++)
    {
        confounder_seskey[i] = session_key[i] ^ 0xf0;
    }

    /*
     * "...the initialization vector constructed by concatenating the sequence
     * number with itself twice."
     */
    memcpy(&IV[0], copy_seqnum, sizeof(copy_seqnum));
    memcpy(&IV[8], copy_seqnum, sizeof(copy_seqnum));

    /*
     *  8. If the Confidentiality option is requested, the Confounder field
     * and the *data* MUST be encrypted.
     *
     * ------------------------------------------------
     *
     * Encrypt the confounder: tail->nonce contains the encrypted confounder
     */
    status = schn_encrypt_aes_128_cfb8(
                 &enc_ctx,
                 confounder_seskey,
                 sizeof(confounder_seskey),
                 IV,
                 sizeof(IV),
                 (uint8 *) &confounder,
                 sizeof(confounder),
                 enc_nonce,
                 &encrypt_len);
    if (status)
    {
        goto cleanup;
    }

    /* encrypt_len must equal sizeof(tail->nonce) */
    if (encrypt_len != sizeof(tail->nonce))
    {
        status = SEC_E_MESSAGE_ALTERED;
        goto error;
    }

    /* Encrypt the data, using the same enc_ctx (confounder IV) */
    status = schn_encrypt_aes_128_cfb8(
                 &enc_ctx,
                 NULL,  /* enc_ctx initialized; these 4 args unused */
                 0,
                 NULL,
                 0,
                 in->base,
                 in->len,
                 out->base,
                 &encrypt_len);
    if (status)
    {
        goto cleanup;
    }
    out->len = encrypt_len;

    /* Destroy the confounder/data encryption context */
    schn_cleanup_aes_128_cfb8(&enc_ctx);

    /*
     * 3.3.4.2.1 paragraph 9.
     * "...initialization vector constructed by concatenating the first 8 bytes
     * of the checksum with itself twice."
     *
     * Terminology: checksum is signature; and aka tail->digest
     */
    memcpy(&IV[0], checksum, sizeof(checksum));
    memcpy(&IV[8], checksum, sizeof(checksum));

    /*
     * 3.3.4.2.1 paragraph 9. The SequenceNumber MUST be encrypted.
     */
    status = schn_encrypt_aes_128_cfb8(
                 &enc_ctx,
                 session_key,
                 sizeof(session_key),
                 IV,
                 sizeof(IV),
                 copy_seqnum,
                 sizeof(copy_seqnum),
                 enc_seq_number,
                 &encrypt_len);
    if (status)
    {
        goto cleanup;
    }
    schn_cleanup_aes_128_cfb8(&enc_ctx);

    /* encrypt_len must equal sizeof(tail_seq_number) */
    if (encrypt_len != sizeof(enc_seq_number))
    {
        status = SEC_E_MESSAGE_ALTERED;
        goto error;
    }

    /*
     * The Netlogon Signature token MUST then be sent to the client along
     * with the data.
     *
     *  Look at schn_discover_crypto_algorithm(), tail->signature is
     * "Secure Channel Verifier", or sha2_signature_header [8 bytes]
     */
    memcpy(tail->signature,  &sha2_signature_header, sizeof(sha2_signature_header));
    memcpy(tail->digest,     checksum,               sizeof(checksum));
    memcpy(tail->seq_number, enc_seq_number,         sizeof(enc_seq_number));
    memcpy(tail->nonce,      enc_nonce,              sizeof(enc_nonce));

    status = 0;

cleanup:
    return status;

error:
    schn_cleanup_aes_128_cfb8(&enc_ctx);
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
