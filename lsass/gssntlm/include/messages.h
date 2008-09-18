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
 *        messages.h
 *
 * Abstract:
 *
 *        NTLM protocol message definitions and flags
 *
 * Author: Todd Stecher (2007)
 *
 */

#ifndef _MESSAGES_H_
#define _MESSAGES_H_

/* message signature */
#define NTLM_SIGNATURE                  "NTLMSSP"
#define NTLM_SIGNATURE_SIZE             8
#define NTLM_VERSION_SIZE               8


/* helpful macros */
#define COPY_CHALLENGE(_d_,_s_) memcpy(_d_,_s_,NTLM_CHALLENGE_LENGTH)

/* message type */
typedef DWORD NTLM_MESSAGE_TYPE;

#define NEGOTIATE_MSG           1
#define CHALLENGE_MSG           2
#define AUTHENTICATE_MSG        3
#define MAX_MSG                 AUTHENTICATE_MSG
#define UNKNOWN_MSG             8
/* target types */
#define TARGET_TYPE_DOMAIN    0x00010000
#define TARGET_TYPE_SERVER    0x00020000

/* name types */
#define NAME_TYPE_END         0x0000
#define NAME_TYPE_SERVER      0x0001
#define NAME_TYPE_DOMAIN      0x0002
#define NAME_TYPE_SERVER_DNS  0x0003
#define NAME_TYPE_DOMAIN_DNS  0x0004

 

/******************************
 *
 *     NEGOTIATE MESSAGE 
 *
 *****************************/

#define NEGOTIATE_UNICODE               0x00000001  /* unicode charset */
#define NEGOTIATE_OEM                   0x00000002  /* oem charset */
#define NEGOTIATE_REQUEST_TARGET        0x00000004  /* return target in
                                                       challenge */

#define NEGOTIATE_SIGN                  0x00000010  /* sign requested */
#define NEGOTIATE_SEAL                  0x00000020  /* encryption requested */
#define NEGOTIATE_DATAGRAM              0x00000040  /* udp message - @todo */
#define NEGOTIATE_LM_KEY                0x00000080  /* use LM key for crypto */
#define NEGOTIATE_NETWARE               0x00000100  /* netware - not supported */
#define NEGOTIATE_NTLM                  0x00000200  /* use NTLM authentication */
#define NEGOTIATE_NT_ONLY               0x00000400  /* use ONLY NT authentication */
#define NEGOTIATE_DOMAIN                0x00001000  /* domain supplied */
#define NEGOTIATE_WORKSTATION           0x00002000  /* wks supplied */
#define NEGOTIATE_LOCAL_CALL            0x00004000  /* loopback auth */
#define NEGOTIATE_ALWAYS_SIGN           0x00008000  /* use dummy sig */
#define NEGOTIATE_NTLM2                 0x00080000  /* use NTLMv2 key */
#define NEGOTIATE_VERSION_DEBUG	        0x02000000  /* used for debug */
#define NEGOTIATE_128                   0x20000000  /* 128-bit encryption */
#define NEGOTIATE_KEY_EXCH              0x40000000  /* perform key exchange */
#define NEGOTIATE_56                    0x80000000  /* 56-bit encryption @todo */


/* default negotiate flags - modifiable via gssapi */
#define NEGOTIATE_CLI_DEFAULT ( NEGOTIATE_UNICODE | NEGOTIATE_SIGN | NEGOTIATE_SEAL |\
    NEGOTIATE_WORKSTATION | NEGOTIATE_ALWAYS_SIGN | NEGOTIATE_NTLM | NEGOTIATE_NT_ONLY|\
    NEGOTIATE_NTLM2 | NEGOTIATE_128 | NEGOTIATE_56 | NEGOTIATE_REQUEST_TARGET |\
    NEGOTIATE_KEY_EXCH | NEGOTIATE_VERSION_DEBUG) 

#define NEGOTIATE_SRV_DEFAULT ( NEGOTIATE_UNICODE | NEGOTIATE_SIGN | NEGOTIATE_SEAL |\
    NEGOTIATE_WORKSTATION | NEGOTIATE_ALWAYS_SIGN | NEGOTIATE_NTLM |\
    NEGOTIATE_NTLM2 | NEGOTIATE_128 | NEGOTIATE_56 | NEGOTIATE_REQUEST_TARGET |\
    NEGOTIATE_KEY_EXCH |NEGOTIATE_VERSION_DEBUG) 

#define NEGOTIATE_KNOWN ( NEGOTIATE_UNICODE | NEGOTIATE_OEM |\
    NEGOTIATE_REQUEST_TARGET | NEGOTIATE_SIGN | NEGOTIATE_SEAL |\
    NEGOTIATE_DATAGRAM | NEGOTIATE_LM_KEY | NEGOTIATE_NETWARE |\
    NEGOTIATE_DOMAIN | NEGOTIATE_WORKSTATION | NEGOTIATE_LOCAL_CALL |\
    NEGOTIATE_ALWAYS_SIGN | NEGOTIATE_NTLM | NEGOTIATE_NTLM2 |\
    NEGOTIATE_VERSION_DEBUG | NEGOTIATE_128 | NEGOTIATE_KEY_EXCH |\
    NEGOTIATE_56 | CHALLENGE_TARGET_INFO | CHALLENGE_INIT_RESPONSE |\
    CHALLENGE_ACCEPT_RESPONSE | NEGOTIATE_NT_ONLY ) 

#define NEGOTIATE_REQUIRED ( NEGOTIATE_UNICODE | NEGOTIATE_128 ) 
#define NEGOTIATE_VERSION_REQUIRED ( NEGOTIATE_NTLM | NEGOTIATE_NTLM2 )

#define NEGOTIATE_VARIATIONS ( NEGOTIATE_DOMAIN | NEGOTIATE_WORKSTATION |\
    CHALLENGE_ACCEPT_RESPONSE | CHALLENGE_TARGET_INFO |\
    CHALLENGE_TARGET_SERVER | CHALLENGE_TARGET_DOMAIN | NEGOTIATE_NTLM2 |\
    NEGOTIATE_NTLM | NEGOTIATE_NT_ONLY )


typedef struct _NEGOTIATE_MESSAGE {
	UCHAR             signature[NTLM_SIGNATURE_SIZE];
	ULONG             messageType;
	ULONG             negotiateFlags;
        SEC_BUFFER        workstation;
        SEC_BUFFER        domain;
} NEGOTIATE_MESSAGE, *PNEGOTIATE_MESSAGE;


/******************************
 *
 *    CHALLENGE MESSAGE 
 *
 *****************************/
#define CHALLENGE_INIT_RESPONSE         0x00010000
#define CHALLENGE_ACCEPT_RESPONSE       0x00020000
#define CHALLENGE_NON_NT_SESSION_KEY    0x00040000

#define CHALLENGE_TARGET_DOMAIN         TARGET_TYPE_DOMAIN
#define CHALLENGE_TARGET_SERVER         TARGET_TYPE_SERVER
#define CHALLENGE_TARGET_INFO           0x00800000

typedef struct _CHALLENGE_MESSAGE {
	UCHAR             signature[8];
	ULONG             messageType;
	SEC_BUFFER        target;
	ULONG             negotiateFlags;
	UCHAR             challenge[8];
        UCHAR             reserved[8];   
} CHALLENGE_MESSAGE, *PCHALLENGE_MESSAGE;

/******************************
 *
 *    AUTHENTICATE MESSAGE 
 *
 *****************************/
typedef struct _AUTHENTICATE_MESSAGE {
        UCHAR                   signature[NTLM_SIGNATURE_SIZE];
        NTLM_MESSAGE_TYPE       messageType;
        SEC_BUFFER              lmResponse;
        SEC_BUFFER              ntResponse;
        LSA_STRING              domainName;
        LSA_STRING              userName;
        LSA_STRING              workstation;
        SEC_BUFFER              sessionKey; 
        ULONG                   negotiateFlags;
} AUTHENTICATE_MESSAGE, *PAUTHENTICATE_MESSAGE;

#define NTLMV2_RESPONSE_BLOB_VERSION 0x00000101

typedef struct _NTLMV2_RESPONSE_BLOB {
        UCHAR                   hmac[MD5_DIGEST_LENGTH];
        ULONG                   version;
        ULONG                   zeros1;
        UINT64                  time;
        UCHAR                   clientChallenge[8];
        ULONG                   zeros2;
} NTLMV2_RESPONSE_BLOB, *PNTLMV2_RESPONSE_BLOB;






#endif
