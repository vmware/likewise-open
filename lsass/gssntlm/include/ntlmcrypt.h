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
 *        ntlmcrypt.h
 *
 * Abstract:
 *
 *       NTLM crypto routines
 *
 * Author: Todd Stecher (2007)
 *
 */
#ifndef _NTLMCRYPT_H_
#define _NTLMCRYPT_H_


typedef unsigned char NTLM_OWF[16];

DWORD
NTLMInitializeCrypto(void);


DWORD
NTLMCryptGenRandom(
    PSEC_BUFFER buf
);

DWORD
NTLMCryptGenRandomBytes(
    PUCHAR buf,
    ULONG len
    );
void
NTLMCryptRC4Decrypt(
    UCHAR *rc4key,
    PSEC_BUFFER data
);

void
NTLMCryptRC4(
    UCHAR *rc4key,
    PSEC_BUFFER data
);

void
NTLMCryptRC4Bytes(
    UCHAR *rc4key,
    PUCHAR data,
    ULONG dataLen
);

void
NTLMCryptRC4Encrypt(
    UCHAR *rc4key,
    PSEC_BUFFER data
);

void
NTLMCryptDES56Encrypt8(
    des_cblock *key8,
    const_des_cblock *buf,
    des_cblock *enc
    );


void
NTLMCryptHashChallenge(
    UCHAR key7[7],
    PUCHAR challenge,
    PUCHAR result
    );


DWORD
NTLMComputeNTOWF(
    PLSA_STRING password,
    NTLM_OWF owf
    );
#endif /* _NTLMCRYPT_H_ */
