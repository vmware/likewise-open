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
 *        processmsg.h
 *
 * Abstract:
 * 
 *       NTLM message processing functions.
 *
 * Author: Todd Stecher (2007)
 *
 */
#ifndef _PROCESSMSG_H_
#define _PROCESSMSG_H_

typedef struct _TARGET_INFO_ELEM 
{
    USHORT nameType;
    USHORT length;
    PBYTE buffer;
} TARGET_INFO_ELEM, *PTARGET_INFO_ELEM;

DWORD
NTLMBuildNegotiateMessage( 
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    );

DWORD
NTLMProcessNegotiateMessage( 
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    );

DWORD
NTLMBuildChallengeMessage(
    PNTLM_CONTEXT pCtxt,
    DWORD dwChallengeFlags,
    PSEC_BUFFER pOutputToken
    );

DWORD
NTLMProcessChallengeMessage(
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    );


DWORD
NTLMProcessAuthenticateMessage(
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    );

DWORD
NTLMMessageFinalized(
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    );


DWORD
NTLMBuildAuthenticateMessage(
    PNTLM_CONTEXT pCtxt,
    CHALLENGE_MESSAGE *challengeMessage,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER pOutputToken
    );

DWORD
NTLMBuildResponse(
    PNTLM_CONTEXT pCtxt,
    PUCHAR challenge,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER ntResponse
    );

DWORD
NTLMComputeNTLMv2Response(
    PNTLM_CONTEXT pCtxt,
    PUCHAR challenge,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER response
    );


DWORD
NTLMComputeNTLMv1Response(
    PNTLM_CONTEXT pCtxt,
    PUCHAR challenge,
    PSEC_BUFFER pNTResponse
    );

DWORD
NTLMComputeSessionKey(
    PNTLM_CONTEXT pCtxt,
    PUCHAR serverChallenge,
    SEC_BUFFER_S *clientChallenge,
    SEC_BUFFER_S *sessionKey,
    BOOLEAN client
    );


void
NTLMDumpNegotiateMessage(
    DWORD lvl,
    PNTLM_CONTEXT pCtxt,
    PNEGOTIATE_MESSAGE negMsg
    );

#endif /* _PROCESS_MSG_H_ */
