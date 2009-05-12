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
 *      crypt.h
 *
 * Abstract:
 *
 *      GSS sign seal handler routines
 *
 * Authors: Todd Stecher
 */
 #ifndef _CRYPT_H_
 #define _CRYPT_H_


#define NTLM_CHECKSUM_LENGTH       16

#define CLIENT_TO_SERVER_SEALKEY "session key to client-to-server sealing key magic constant"
#define SERVER_TO_CLIENT_SEALKEY "session key to server-to-client sealing key magic constant"
#define CLIENT_TO_SERVER_SIGNKEY "session key to client-to-server signing key magic constant"
#define SERVER_TO_CLIENT_SIGNKEY "session key to server-to-client signing key magic constant"

typedef struct _NTLM_GSS_SIGNATURE
{
    ULONG version;
    BYTE checksum[8]; /* bytes 0 - 7 of HMACMD5 */
    ULONG seqNum;
} NTLM_GSS_SIGNATURE, *PNTLM_GSS_SIGNATURE;

/*
 * Sign and seal session key generation
 */
DWORD
NTLMCreateKeys(
    PNTLM_CONTEXT context
);


/*
 * Signature (mic) routines.
 */
DWORD
NTLMV2Sign(
    PNTLM_CONTEXT context,
    ULONG qop,
    gss_buffer_t inputToken,
    gss_buffer_t outputToken
);

DWORD
NTLMV2Verify(
    PNTLM_CONTEXT context,
    ULONG *qop,
    gss_buffer_t inputToken,
    gss_buffer_t sigToken
);


/*
 * Encryption routines.
 */

DWORD
NTLMV2Seal(
    PNTLM_CONTEXT context,
    ULONG qop,
    gss_buffer_t data,
    gss_buffer_t encData
);


DWORD
NTLMV2Unseal(
    PNTLM_CONTEXT context,
    ULONG *qop,
    gss_buffer_t data,
    gss_buffer_t decData
);

DWORD
NTLMV1Seal56(
    PNTLM_CONTEXT context,
    ULONG qop,
    gss_buffer_t data,
    gss_buffer_t encData
);

DWORD
NTLMV1Unseal56(
    PNTLM_CONTEXT context,
    ULONG *qop,
    gss_buffer_t data,
    gss_buffer_t decData
);



/*
 * Unsupported stubs
 */

DWORD
NTLMSealUnsupported(
    PNTLM_CONTEXT context,
    ULONG qop,
    gss_buffer_t data,
    gss_buffer_t encData
);

DWORD
NTLMUnsealUnsupported(
    PNTLM_CONTEXT context,
    ULONG *qop,
    gss_buffer_t data,
    gss_buffer_t decData
);

DWORD
NTLMSignUnsupported(
    PNTLM_CONTEXT context,
    ULONG qop,
    gss_buffer_t inputToken,
    gss_buffer_t sigToken
);

DWORD
NTLMVerifyUnsupported(
    PNTLM_CONTEXT context,
    ULONG *qop,
    gss_buffer_t inputToken,
    gss_buffer_t sigToken
);


#endif



