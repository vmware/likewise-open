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
 *      ntlmcrypt.c
 *
 * Abstract:
 *
 * NTLM crypto routines
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "includes.h"


int urand;
/*
 * NTLMInitializeCrypto()
 *
 * @brief Initializes RNG
 *
 * @note requires urandom
 *
 */
DWORD
NTLMInitializeCrypto(void)
{

    /* initialize rng */

    /* @todo - not everyone supports urandom */
    /* @todo - strong enough? */
    urand = open("/dev/urandom", O_RDONLY);
    if (urand < 0)
        return (LSA_ERROR_INTERNAL);

    /* anything else */
    return (LSA_ERROR_SUCCESS);
}

/*
 * NTLMCryptGenRandom()
 *
 * @brief Uses urandom to generate random stream.
 *
 * @param in buf - store rand stream here.
 *
 * @note - requires urandom
 */
DWORD
NTLMCryptGenRandom(
    PSEC_BUFFER buf
    )
{
    /*@todo - not everyone supports urandom */
    if (read(urand,buf->buffer,buf->length) != buf->length)
        return (LSA_ERROR_INTERNAL);

    return (LSA_ERROR_SUCCESS);
}

/* sometimes we don't want to deal w/ secbuffer */
DWORD
NTLMCryptGenRandomBytes(
    PUCHAR buf,
    ULONG len
    )
{
    SEC_BUFFER tmp;
    tmp.length = tmp.maxLength = len;
    tmp.buffer = buf;

    return NTLMCryptGenRandom(&tmp);

}




/*
 * NTLMCryptDES56Encrypt8()
 *
 * @brief Crypt handler for CBC DES 56 
 *
 * @param in key8 - 64 bit key (with parity bit) 
 * @param in buf - block of data to encrypt
 * @param in enc - encrypted block
 *
 */
void
NTLMCryptDES56Encrypt8(
    des_cblock *key8,
    const_des_cblock *buf,
    des_cblock *enc
    )
{
    des_key_schedule keySched;

    /* set the parity byte */
    DES_set_odd_parity(key8);

    /* initialize key + OS dependent key schedule */
    DES_set_key(key8, &keySched);

    /* @todo - test w/ cbc, add in iv - not needed right now.. */
    /* isn't this supposed to be CBC? Oh well, it works the same w/ 8 bytes*/
    DES_ecb_encrypt(buf, enc, &keySched, 1);

    memset(&keySched, 0, sizeof(keySched));

}

void
NTLMCryptHashChallenge(
    UCHAR key7[7],
    PUCHAR challenge,
    PUCHAR result
    )
{
    des_cblock key8[8];

    /* build an 8 byte key */
    ((UCHAR*)key8)[0] =  key7[0];
    ((UCHAR*)key8)[1] = (key7[0] << 7) | (key7[1] >> 1);
    ((UCHAR*)key8)[2] = (key7[1] << 6) | (key7[2] >> 2);
    ((UCHAR*)key8)[3] = (key7[2] << 5) | (key7[3] >> 3);
    ((UCHAR*)key8)[4] = (key7[3] << 4) | (key7[4] >> 4);
    ((UCHAR*)key8)[5] = (key7[4] << 3) | (key7[5] >> 5);
    ((UCHAR*)key8)[6] = (key7[5] << 2) | (key7[6] >> 6);
    ((UCHAR*)key8)[7] = (key7[6] << 1);

    NTLMCryptDES56Encrypt8(
        key8,
        (const_des_cblock*) challenge,
        (des_cblock*) result
        );
}
void
NTLMCryptRC4(
    UCHAR *rc4key,
    PSEC_BUFFER data
    )
{
    RC4_KEY key;
    RC4_set_key(&key, 16, rc4key);
    RC4(&key, data->length, data->buffer, data->buffer);
}

void
NTLMCryptRC4Bytes(
    UCHAR *rc4key,
    PUCHAR data,
    ULONG dataLen
    )
{
    RC4_KEY key;
    RC4_set_key(&key, 16, rc4key);
    RC4(&key, dataLen, data, data);
}



DWORD
NTLMComputeNTOWF(
    PLSA_STRING password,
    NTLM_OWF owf
    )
{
    /* @todo what to do here?  Hash '/0' - I think */
    if (password->length == 0 || password->buffer == NULL)
        return LSA_ERROR_PASSWORD_MISMATCH;

    MD4((UCHAR*) password->buffer, password->length, owf);
    return LSA_ERROR_SUCCESS;

}
