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
 *        processmsg.c
 *
 * Abstract:
 *
 *        Code for processing NTLM messages - core of the protocol
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "client.h"
void
NTLMDumpNegotiateMessage(
    DWORD lvl,
    PNTLM_CONTEXT pCtxt,
    PNEGOTIATE_MESSAGE negMsg
    )
{
    if ((lvl & db_level) == 0)
        return;

    DBG(lvl,("Negotiate Message - ctxt(0x%p)\n", pCtxt));
    NTLMDumpNegotiateFlags(lvl, "negotiate message", negMsg->negotiateFlags);

}

/*
 * NTLMBuildTargetInfo()
 *
 * @brief If the client is requesting target info, build
 * it here.  For standalone / non-joined machines, we can only
 * offer local names.  For domain joined, we should also
 * include netbios and dns names (if requested).
 *
 * @todo - this should be global, and tied into change notification
 * scheme domain, server name changes.  Right now, we compute every time.
 *
 * @param out targetInfo -  secbuffer (absolute), to be tacked on the end
 *                          of the challenge messge, with buffer
 *                          relative to the base of the containing message.
 */

static DWORD
NTLMBuildTargetInfo(
    PSEC_BUFFER targetInfoOut
    )
{
    DWORD dwError = 0;
    DWORD dwSize;
    PBYTE copyTo;
    DWORD ofs = 0;

    SEC_BUFFER targetInfo;
    LSA_STRING server;
    LSA_STRING serverDNSName;
    LSA_STRING dnsDomain;
    LSA_STRING nbDomain;

    ZERO_STRUCT(targetInfo);
    ZERO_STRUCT(server);
    ZERO_STRUCT(serverDNSName);
    ZERO_STRUCT(dnsDomain);
    ZERO_STRUCT(nbDomain);

    dwError = NTLMGetWorkstationName(&server);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMGetDNSWorkstationName(&serverDNSName);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMGetDNSDomainName(&dnsDomain);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMGetNBDomainName(&nbDomain);
    BAIL_ON_NTLM_ERROR(dwError);

    dwSize = server.length + serverDNSName.length +
        dnsDomain.length + nbDomain.length + (10 * sizeof(USHORT));

    targetInfo.buffer = NTLMAllocateMemory(dwSize);
    if (!targetInfo.buffer)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    targetInfo.length = targetInfo.maxLength = dwSize;
    copyTo = targetInfo.buffer;

    PUT_USHORT(copyTo,ofs,NAME_TYPE_DOMAIN);
    ofs += sizeof(USHORT);
    PUT_USHORT(copyTo,ofs,nbDomain.length);
    ofs += sizeof(USHORT);
    PUT_BYTES(copyTo, ofs, nbDomain.buffer, nbDomain.length);
    ofs += nbDomain.length;

    PUT_USHORT(copyTo,ofs,NAME_TYPE_SERVER);
    ofs += sizeof(USHORT);
    PUT_USHORT(copyTo,ofs,server.length);
    ofs += sizeof(USHORT);
    PUT_BYTES(copyTo, ofs, server.buffer, server.length);
    ofs += server.length;

    PUT_USHORT(copyTo,ofs,NAME_TYPE_DOMAIN_DNS);
    ofs += sizeof(USHORT);
    PUT_USHORT(copyTo,ofs,dnsDomain.length);
    ofs += sizeof(USHORT);
    PUT_BYTES(copyTo, ofs, dnsDomain.buffer, dnsDomain.length);
    ofs += dnsDomain.length;

    PUT_USHORT(copyTo,ofs,NAME_TYPE_SERVER_DNS);
    ofs += sizeof(USHORT);
    PUT_USHORT(copyTo,ofs,serverDNSName.length);
    ofs += sizeof(USHORT);
    PUT_BYTES(copyTo, ofs, serverDNSName.buffer, serverDNSName.length);
    ofs += serverDNSName.length;

    /* @todo - put bit in here to say if server does force guest */

    PUT_USHORT(copyTo,ofs,NAME_TYPE_END);
    ofs += sizeof(USHORT);
    PUT_USHORT(copyTo,ofs,0);

    memcpy(targetInfoOut, &targetInfo, sizeof(SEC_BUFFER));
    targetInfo.buffer = NULL;

error:

    NTLM_SAFE_FREE(targetInfo.buffer);
    NTLM_SAFE_FREE(server.buffer);
    NTLM_SAFE_FREE(nbDomain.buffer);
    NTLM_SAFE_FREE(serverDNSName.buffer);
    NTLM_SAFE_FREE(dnsDomain.buffer);

    return dwError;
}

void
NTLMFreeTargetInfo(PSEC_BUFFER targetInfo)
{
    if (!targetInfo)
        return;

    NTLM_SAFE_FREE(targetInfo->buffer);

}

DWORD
NTLMServerValidateNegotiateFlags(
    PNTLM_CONTEXT pCtxt,
    ULONG negotiateFlags,
    ULONG *newFlags
    )
{
    /* eventually the default should be altered via policy / oids */
    ULONG flags = ( NEGOTIATE_SRV_DEFAULT & negotiateFlags );
    ULONG dwError = LSA_ERROR_SUCCESS;

    NTLMDumpNegotiateFlags(D_ERROR, "inbound flags", negotiateFlags);

    /* remove flags which we support, but client doesn't want */
    /* right now, only do NTLM, no LM */
    if ((flags & NEGOTIATE_NTLM) == 0)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_UNSUPPORTED_SUBPROTO);

    /* @todo - we need to eventually support ANSI, or fail here
     * if we aren't provided a UNICODE flags */
    if (flags & NEGOTIATE_UNICODE)
        flags &= ~NEGOTIATE_OEM;
    else
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_UNSUPPORTED_SUBPROTO);

    /* store these flags in context */
    NTLM_LOCK_CONTEXT(pCtxt);
    pCtxt->negotiateFlags = flags;
    NTLM_UNLOCK_CONTEXT(pCtxt);

    /* add new flags, specifically for challenge msg */
    if (flags & NEGOTIATE_REQUEST_TARGET)
        flags |= (NEGOTIATE_REQUEST_TARGET | CHALLENGE_TARGET_INFO);

    *newFlags = flags;

    NTLMDumpNegotiateFlags(D_ERROR, "new flags", negotiateFlags);

error:

    return dwError;
}

DWORD
NTLMFinalizedMessage(
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    )
{

    /* @todo debug */
    /* this message SHOULD NEVER be called */
    return LSA_ERROR_INVALID_CONTEXT;
}

ULONG
NTLMGetDefaultNegFlags(PNTLM_CONTEXT pCtxt)
{
    /*
     * @todo - this should be alterable via policy, and gss api
     * OID set during gss?
     */
    return NEGOTIATE_CLI_DEFAULT;
}

/*
 * NEGOTIATE
 */
DWORD
NTLMBuildNegotiateMessage(
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    )
{
    DWORD dwError = 0;
    DWORD ofs = 0;
    DWORD dwSize = sizeof(NEGOTIATE_MESSAGE) + NTLM_VERSION_SIZE;
    ULONG negotiateFlags;
    PBYTE negotiateMsg = NULL;

    /* @todo what should the input token have (mechs?)? */

    negotiateMsg = NTLMAllocateMemory(dwSize);

    if (!negotiateMsg)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    PUT_BYTES(negotiateMsg, 0, signature, NTLM_SIGNATURE_SIZE);
    ofs += NTLM_SIGNATURE_SIZE;

    PUT_ULONG(negotiateMsg, ofs, NEGOTIATE_MSG);
    ofs += sizeof(ULONG);

    negotiateFlags = NTLMGetDefaultNegFlags(pCtxt);

    PUT_ULONG(negotiateMsg, ofs, negotiateFlags);
    ofs += sizeof(ULONG);

    /* calling workstation and domain should be 0s */
    SET_BYTES(negotiateMsg, ofs, 0, 16);
    ofs += 16;

    /* debug version info - send 2k3 version */
    if (negotiateFlags & NEGOTIATE_VERSION_DEBUG) {

        PUT_BYTES(negotiateMsg, ofs, version, NTLM_VERSION_SIZE);
        ofs += NTLM_VERSION_SIZE;

    } else
        dwSize -= NTLM_VERSION_SIZE;

    pOutputToken->length = pOutputToken->maxLength = dwSize;
    pOutputToken->buffer = negotiateMsg;
    negotiateMsg = NULL;

    /*
     * set flags on context
     * no lock required - only we have ctxt handle
     */
    pCtxt->negotiateFlags = NEGOTIATE_CLI_DEFAULT;
    pCtxt->processNextMessage = NTLMProcessChallengeMessage;

    DBGDumpSecBuffer(D_ERROR, "negotiate msg", pOutputToken);

error:

    /* if we err'd, this context is doa */
    if (dwError)
        pCtxt->processNextMessage = NTLMFinalizedMessage;
    else
        dwError = LSA_WARNING_CONTINUE_NEEDED;

    NTLM_SAFE_FREE(negotiateMsg);
    return dwError;
}


DWORD
NTLMParseNegotiateMessage(
    PSEC_BUFFER pBuf,
    ULONG *negotiateFlags
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    ULONG ofs = 0;

    if (pBuf->length < sizeof(NEGOTIATE_MESSAGE))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INSUFFICIENT_BUFFER);

    dwError = NTLMParseMessageHeader(
                    pBuf,
                    &ofs,
                    NEGOTIATE_MSG
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    /* only interesting part is negotiate flag */
    (*negotiateFlags) = GET_ULONG(pBuf->buffer, ofs);

    /* @todo NTLMDumpNegotiateMessage */

error:

    return dwError;
}



DWORD
NTLMProcessNegotiateMessage(
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    )
{
    DWORD dwError = 0;
    ULONG newFlags = 0;
    ULONG negotiateFlags;

    /* parse the message */
    dwError = NTLMParseNegotiateMessage(
                pInputToken,
                &negotiateFlags
                );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMServerValidateNegotiateFlags(
                    pCtxt,
                    negotiateFlags,
                    &newFlags
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMBuildChallengeMessage(
                    pCtxt,
                    newFlags,
                    pOutputToken
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LSA_WARNING_CONTINUE_NEEDED;

error:

    /* @todo debug */

    return dwError;
}


/*
 *  CHALLENGE MESSAGE HANDLERS
 */

void
NTLMDumpChallengeMessage(
    PSEC_BUFFER challengeMsg
    )
{
    /* @todo - parse and dump individual elements */
    DBGDumpSecBuffer(D_ERROR, "challenge msg", challengeMsg);
}

DWORD
NTLMBuildChallengeMessage(
    PNTLM_CONTEXT pCtxt,
    DWORD challengeFlags,
    PSEC_BUFFER challengeMsg
    )
{
    DWORD dwError = 0;
    DWORD dwSize = sizeof(CHALLENGE_MESSAGE);
    ULONG ofs = 0;
    ULONG bufofs = sizeof(CHALLENGE_MESSAGE);
    LSA_STRING workstation;
    SEC_BUFFER targetInfo;
    UCHAR challenge[NTLM_CHALLENGE_LENGTH];

    ZERO_STRUCT(workstation);
    ZERO_STRUCT(targetInfo);
    ZERO_STRUCTP(challengeMsg);

    /* build target name, and target info */
    if (challengeFlags & NEGOTIATE_REQUEST_TARGET) {

        dwError = NTLMGetWorkstationName(&workstation);
        BAIL_ON_NTLM_ERROR(dwError);

        /* add in target info buffer */
        dwSize += sizeof(SEC_BUFFER) + workstation.max;
        bufofs += sizeof(SEC_BUFFER);

        /* @todo we don't support domain currently */
        challengeFlags |= CHALLENGE_TARGET_SERVER;

        dwError = NTLMBuildTargetInfo(&targetInfo);
        BAIL_ON_NTLM_ERROR(dwError);

        dwSize += targetInfo.maxLength;
        challengeFlags |= CHALLENGE_TARGET_INFO;
    }

    if (challengeFlags & NEGOTIATE_VERSION_DEBUG)
    {
        dwSize += sizeof(version);
        bufofs += sizeof(version);
    }

    challengeMsg->buffer =  NTLMAllocateMemory(dwSize);
    if (!challengeMsg->buffer)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    PUT_BYTES(challengeMsg->buffer, ofs, signature, NTLM_SIGNATURE_SIZE);
    ofs += NTLM_SIGNATURE_SIZE;

    PUT_ULONG(challengeMsg->buffer, ofs, CHALLENGE_MSG);
    ofs += sizeof(ULONG);

    if (challengeFlags & NEGOTIATE_REQUEST_TARGET) {
        NTLMPutLsaString(&workstation, challengeMsg->buffer, &bufofs, &ofs);
    }

    /* these flags aren't persisted */
    NTLMDumpNegotiateFlags(D_ERROR, "challenge flags", challengeFlags);
    PUT_ULONG(challengeMsg->buffer, ofs, challengeFlags);
    ofs += sizeof(ULONG);

    /* compute challenge */
    dwError = NTLMCryptGenRandomBytes(
                challenge,
                NTLM_CHALLENGE_LENGTH
                );

    BAIL_ON_NTLM_ERROR(dwError);

    PUT_BYTES(challengeMsg->buffer, ofs, challenge, NTLM_CHALLENGE_LENGTH);
    ofs += NTLM_CHALLENGE_LENGTH;

    /* huh - what are these here for? */
    SET_BYTES(challengeMsg->buffer, ofs, 0, 8);
    ofs += 8;


    if (challengeFlags & CHALLENGE_TARGET_INFO)
        NTLMPutSecBuffer(&targetInfo, challengeMsg->buffer, &bufofs, &ofs);

    /* debug version info - send 2k3 version */
    if (challengeFlags & NEGOTIATE_VERSION_DEBUG) {
        PUT_BYTES(challengeMsg->buffer, ofs, version, NTLM_VERSION_SIZE);
        ofs += NTLM_VERSION_SIZE;
    }

    /* update context */
    NTLM_LOCK_CONTEXT(pCtxt);
    pCtxt->processNextMessage = NTLMProcessAuthenticateMessage;
    COPY_CHALLENGE(pCtxt->challenge, challenge);

    /* add in target info flags, but not name type */
    pCtxt->negotiateFlags |= (challengeFlags & CHALLENGE_TARGET_INFO);
    /* @todo - store off, or version, targetInfo */
    NTLM_UNLOCK_CONTEXT(pCtxt);
    challengeMsg->length = challengeMsg->maxLength = dwSize;

    DBGDumpSecBuffer(D_ERROR, "challenge msg", challengeMsg);

error:

    NTLM_SAFE_FREE(workstation.buffer);
    NTLM_SAFE_FREE(targetInfo.buffer);

    return dwError;
}

DWORD
NTLMBuildAuthenticateMessage(
    PNTLM_CONTEXT pCtxt,
    CHALLENGE_MESSAGE *challengeMsg,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER pOutputToken
    )
{

    DWORD dwError = 0;
    ULONG negFlags;
    HANDLE hLsaConnection = (HANDLE)NULL;
    SEC_BUFFER marshaledAuthUser;
    SEC_BUFFER_S baseSessionKey;
    SEC_BUFFER_S serverChallenge;

    INIT_SEC_BUFFER_S_VAL(&serverChallenge, 8, challengeMsg->challenge);

    ZERO_STRUCT(marshaledAuthUser);
    ZERO_STRUCT(baseSessionKey);

    negFlags = NTLMContextGetNegotiateFlags(pCtxt);

    dwError = NTLMContextGetMarshaledCreds(
                    pCtxt,
                    &marshaledAuthUser
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    /* call LSA to build the message */
    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaGSSBuildAuthMessage(
                    hLsaConnection,
                    &marshaledAuthUser,
                    &serverChallenge,
                    targetInfo,
                    negFlags,
                    pOutputToken,
                    &baseSessionKey
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    /* @todo - dumb down */
    DBGDumpSecBuffer(D_ERROR, "authenticate msg", pOutputToken);
    DBGDumpSecBufferS(D_ERROR, "sessionkey", &baseSessionKey);

    NTLM_LOCK_CONTEXTS();
    memcpy(&pCtxt->baseSessionKey, &baseSessionKey, sizeof(SEC_BUFFER_S));
    NTLM_UNLOCK_CONTEXTS();

error:

    NTLM_SAFE_FREE(marshaledAuthUser.buffer);

    return dwError;
}

DWORD
NTLMCheckChallengeNegotiateFlags(
    PNTLM_CONTEXT pCtxt,
    PCHALLENGE_MESSAGE challengeMessage,
    PSEC_BUFFER targetInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    ULONG cliNegFlags = NTLMContextGetNegotiateFlags(pCtxt);

    /*@ todo - dumb down D_ERROR to D_TRACE */
    NTLMDumpNegotiateFlags(
        D_ERROR,
        "challenge msg flags",
        challengeMessage->negotiateFlags
        );

    NTLMDumpNegotiateFlags(
        D_ERROR,
        "context flags",
        cliNegFlags
        );

    /* check various target constructs */
    if ((challengeMessage->negotiateFlags & CHALLENGE_TARGET_INFO) &&
        (targetInfo->length == 0))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_TOKEN);

    if ((challengeMessage->negotiateFlags & NEGOTIATE_REQUEST_TARGET) &&
        (challengeMessage->target.length == 0))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_TOKEN);

    /* @todo -hacky - we don't support weak keys, protocols, or ANSI */
    if ((challengeMessage->negotiateFlags & NEGOTIATE_REQUIRED) !=
        NEGOTIATE_REQUIRED)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_NOT_SUPPORTED);

    if ((challengeMessage->negotiateFlags & NEGOTIATE_VERSION_REQUIRED) == 0)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_NOT_SUPPORTED);

    /* finally, make sure all flags are accounted for */
    if ((cliNegFlags | NEGOTIATE_VARIATIONS) !=
        (challengeMessage->negotiateFlags | NEGOTIATE_VARIATIONS))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_TOKEN);

    /* @todo - update client context flags */

    if (challengeMessage->negotiateFlags & CHALLENGE_TARGET_INFO)
    {
        NTLM_LOCK_CONTEXTS();
        pCtxt->negotiateFlags |= CHALLENGE_TARGET_INFO;
        NTLM_UNLOCK_CONTEXTS();
    }

error:

    return dwError;
}

void
NTLMFreeChallengeMessage(
    PCHALLENGE_MESSAGE msg
)
{
    NTLM_SAFE_FREE(msg->target.buffer);
}

DWORD
NTLMParseChallengeMessage(
    PSEC_BUFFER pBuf,
    PCHALLENGE_MESSAGE pChallengeMsg,
    PSEC_BUFFER pTargetInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    ULONG ofs = 0;

    ZERO_STRUCTP(pChallengeMsg);
    ZERO_STRUCTP(pTargetInfo);

    if (pBuf->length < sizeof(CHALLENGE_MESSAGE))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INSUFFICIENT_BUFFER);

    dwError = NTLMParseMessageHeader(
                    pBuf,
                    &ofs,
                    CHALLENGE_MSG
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    /* I think this is always present */
    dwError = NTLMGetSecBuffer(&pChallengeMsg->target, pBuf, &ofs);
    BAIL_ON_NTLM_ERROR(dwError);

    pChallengeMsg->negotiateFlags = GET_ULONG(pBuf->buffer, ofs);
    ofs += sizeof(ULONG);

    GET_BYTES(pChallengeMsg->challenge, ofs, pBuf->buffer,
        NTLM_CHALLENGE_LENGTH);
    ofs += NTLM_CHALLENGE_LENGTH;

    /* handle reserved 0 padded bytes */
    ofs += 8;

    if (pChallengeMsg->negotiateFlags & CHALLENGE_TARGET_INFO)
    {
        dwError = NTLMGetSecBuffer(pTargetInfo, pBuf, &ofs);
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* skip version */
    if (pChallengeMsg->negotiateFlags & NEGOTIATE_VERSION_DEBUG)
        ofs += NTLM_VERSION_SIZE;
error:

    return dwError;
}


DWORD
NTLMProcessChallengeMessage(
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    )
{
    DWORD dwError = 0;
    CHALLENGE_MESSAGE challengeMessage;
    SEC_BUFFER targetInfo;

    ZERO_STRUCT(challengeMessage);
    ZERO_STRUCT(targetInfo);

    dwError = NTLMParseChallengeMessage(
                pInputToken,
                &challengeMessage,
                &targetInfo
                );

    BAIL_ON_NTLM_ERROR(dwError);

    /* check final negotiated flags */
    dwError = NTLMCheckChallengeNegotiateFlags(
                pCtxt,
                &challengeMessage,
                &targetInfo
                );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMBuildAuthenticateMessage(
                pCtxt,
                &challengeMessage,
                &targetInfo,
                pOutputToken
                );

    BAIL_ON_NTLM_ERROR(dwError);

    /* ? dwError = LSA_WARNING_CONTINUE_NEEDED; */
    /* should be OK - clients need to continue sending */

error:

    NTLM_SAFE_FREE(targetInfo.buffer);
    return dwError;
}

DWORD
NTLMProcessAuthenticateMessage(
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pInputToken,
    PSEC_BUFFER pOutputToken
    )

{
    DWORD dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    ULONG negFlags = NTLMContextGetNegotiateFlags(pCtxt);
    SEC_BUFFER_S    baseSessionKey;
    SEC_BUFFER_S    serverChallenge;
    SEC_BUFFER      targetInfo;

    INIT_SEC_BUFFER_S_VAL(
        &serverChallenge,
        NTLM_CHALLENGE_LENGTH,
        pCtxt->challenge
        );

    /*@todo - global, cached, target info? */
    dwError = NTLMBuildTargetInfo(&targetInfo);
    BAIL_ON_NTLM_ERROR(dwError);

    /* call LSA to build the message */
    dwError = LsaOpenServer(&hLsaConnection);
        BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaGSSValidateAuthMessage(
                hLsaConnection,
                negFlags,
                &serverChallenge,
                &targetInfo,
                pInputToken,
                &baseSessionKey
                );

    BAIL_ON_NTLM_ERROR(dwError);

    /* update context */
    NTLM_LOCK_CONTEXTS();
    memcpy(&pCtxt->baseSessionKey, &baseSessionKey, sizeof(SEC_BUFFER_S));
    pCtxt->processNextMessage = NTLMFinalizedMessage;
    NTLM_UNLOCK_CONTEXTS();


error:

    return dwError;

}
