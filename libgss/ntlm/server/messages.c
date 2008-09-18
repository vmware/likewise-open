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
 *        messages.c
 *
 * Abstract:
 *
 *        Credential handle management functions.
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "server.h"

/* @todo */
void
NTLMDumpNegotiateFlags(DWORD dwNegFlags);

void
NTLMDumpNegotiateMessage();

void
NTLMDumpChallengeMessage();


void
NTLMDumpResponseMessage();

/* end @todo */
    
DWORD
NTLMParseMessageHeader(
    NTLM_CONTEXT *pCtxt,
    PSEC_BUFFER pIn,
    PSEC_BUFFER pMessage,
    DWORD *pMessageType
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    SEC_BUFFER tmp = *pIn;
    UCHAR sig[NTLM_SIGNATURE_SIZE];

    PARSE_TEST(ParseBytes(&tmp, NTLM_SIGNATURE_SIZE, (PBYTE*) &sig));
    PARSE_TEST(ParseUINT32(&tmp, (UINT32*) pMessageType)); 

    /* move pointer */
    *pMessage = tmp;

error:

    /* @todo debug */
    return dwError;
}


DWORD
NTLMParseNegotiateMessage(
    NTLM_CONTEXT *pCtxt, 
    PSEC_BUFFER pBuf,
    PNEGOTIATE_MESSAGE pNegMsg
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    DWORD negFlags;
    DWORD messageType;
    SEC_BUFFER message;

    dwError = NTLMParseMessageHeader(
                    pCtxt, 
                    pBuf, 
                    &message,
                    &messageType
                    );

    BAIL_ON_NTLM_ERROR(dwError);
    if (messageType != NEGOTIATE_MSG) {
        dwError = LSA_ERROR_INVALID_TOKEN;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    PARSE_TEST(ParseUINT32(&message,(UINT32*)&pNegMsg->negotiateFlags));

    /* @todo NTLMDumpNegotiateMessage */

error:

    return dwError;
}

DWORD
NTLMParseChallengeMessage(
    NTLM_CONTEXT *pCtxt, 
    PSEC_BUFFER pBuf,
    PCHALLENGE_MESSAGE pChallengeMessage
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    ULONG negFlags;
    SEC_BUFFER message;
    SEC_BUFFER base = *pBuf;

    dwError = NTLMParseMessageHeader(
                    pCtxt, 
                    pBuf, 
                    &message,
                    (DWORD*)&pChallengeMessage->messageType
                    );

    BAIL_ON_NTLM_ERROR(dwError);
    if (pChallengeMessage->messageType != NEGOTIATE_MSG) {
        dwError = LSA_ERROR_INVALID_TOKEN;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    PARSE_TEST(ParseUINT32(&message,&pChallengeMessage->negotiateFlags));

    /* compare with expected pCtxt flags */

    /* 
     * if target name is provided, it must be in both the negotiate flags
     * in the context, in addition to what is provided in the message
     */ 
    if (pChallengeMessage->targetInfo.length &&
       (!CHECK_FLAG(pCtxt->negotiateFlags, NEGOTIATE_REQUEST_TARGET) ||
        !CHECK_FLAG(pChallengeMessage->negotiateFlags, 
            NEGOTIATE_REQUEST_TARGET) ||
        !CHECK_FLAG(pChallengeMessage->negotiateFlags, 
            (CHALLENGE_TARGET_SERVER | CHALLENGE_TARGET_DOMAIN)))) {
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_TOKEN);
    }


    /* NTLMDumpChallengeMessage */

error:

    return dwError;
}
DWORD
NTLMParseAuthenticateMessage(
    NTLM_CONTEXT *pCtxt, 
    PSEC_BUFFER pBuf,
    PAUTHENTICATE_MESSAGE pAuthMsg
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    ULONG negFlags;
    SEC_BUFFER message;
    SEC_BUFFER base = *pBuf;

    dwError = NTLMParseMessageHeader(
                    pCtxt, 
                    pBuf, 
                    &message,
                    (DWORD*)&pAuthMsg->messageType
                    );

    BAIL_ON_NTLM_ERROR(dwError);
    if (pAuthMsg->messageType != AUTHENTICATE_MSG) {
        dwError = LSA_ERROR_INVALID_TOKEN;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* compare with expected pCtxt flags */


error:

    return dwError;
}

