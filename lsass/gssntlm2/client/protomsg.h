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
 *        protomsg.h
 *
 * Abstract:
 *
 *       NTLM message processing functions.
 *
 * Author: Todd Stecher (2007)
 *
 */
#ifndef _PROTOMSG_H_
#define _PROTOMSG_H_


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
NTLMMessageFinalized(
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
NTLMBuildAuthenticateMessage(
    PNTLM_CONTEXT pCtxt,
    CHALLENGE_MESSAGE *challengeMsg,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER pOutputToken
    );

void
NTLMDumpNegotiateMessage(
    DWORD lvl,
    PNTLM_CONTEXT pCtxt,
    PNEGOTIATE_MESSAGE negMsg
    );

#endif /* _MESSAGES_H_ */
