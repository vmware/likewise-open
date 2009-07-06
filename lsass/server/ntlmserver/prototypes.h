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
 *        prototypes.h
 *
 * Abstract:
 *
 *
 * Authors:
 *
 */

#ifndef __NTLM_PROTOTYPES_H__
#define __NTLM_PROTOTYPES_H__

DWORD
NtlmInitContext(
    OUT PNTLM_CONTEXT *ppNtlmContext
    );

DWORD
NtlmInsertContext(
    PNTLM_CONTEXT pNtlmContext
    );

DWORD
NtlmRemoveContext(
    IN PCtxtHandle pCtxtHandle
    );

DWORD
NtlmRemoveAllContext(
    VOID
    );

DWORD
NtlmFindContext(
    IN PCtxtHandle pCtxtHandle,
    OUT PNTLM_CONTEXT *ppNtlmContext
    );

DWORD
NtlmFreeContext(
    PNTLM_CONTEXT pNtlmContext
    );

DWORD
NtlmInitCredentials(
    OUT PNTLM_CREDENTIALS *ppNtlmCreds
    );

DWORD
NtlmInsertCredentials(
    PNTLM_CREDENTIALS pNtlmCreds
    );

DWORD
NtlmRemoveCredentials(
    IN PCredHandle pCredHandle
    );

DWORD
NtlmRemoveAllCredentials(
    VOID
    );

DWORD
NtlmFindCredentials(
    IN PCredHandle pCredHandle,
    OUT PNTLM_CREDENTIALS *ppNtlmCreds
    );

DWORD
NtlmFreeCredentials(
    PNTLM_CREDENTIALS pNtlmCreds
    );

DWORD
NtlmCreateContextFromSecBufferDesc(
    PSecBufferDesc pSecBufferDesc,
    NTLM_STATE nsContextType,
    PNTLM_CONTEXT *ppNtlmContext
    );

DWORD
NtlmGetRandomBuffer(
    PBYTE pBuffer,
    DWORD dwLen
    );

DWORD
NtlmCreateNegotiateMessage(
    IN DWORD dwOptions,
    IN PCHAR pDomain,
    IN PCHAR pWorkstation,
    IN PBYTE pOsVersion,
    OUT PNTLM_NEGOTIATE_MESSAGE *ppNegMsg
    );

DWORD
NtlmCreateChallengeMessage(
    IN PNTLM_NEGOTIATE_MESSAGE pNegMsg,
    IN PCHAR pServerName,
    IN PCHAR pDomainName,
    IN PCHAR pDnsHostName,
    IN PCHAR pDnsDomainName,
    IN PBYTE  pOsVersion,
    OUT PNTLM_CHALLENGE_MESSAGE *ppChlngMsg
    );

DWORD
NtlmCreateResponseMessage(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCHAR pAuthTargetName,
    IN PCHAR pUserName,
    IN PCHAR pWorkstation,
    IN PBYTE pOsVersion,
    IN PCHAR pPassword,
    IN DWORD dwNtRespType,
    IN DWORD dwLmRespType,
    OUT PNTLM_RESPONSE_MESSAGE *ppRespMsg
    );

DWORD
NtlmValidateResponseMessage(
    IN PNTLM_RESPONSE_MESSAGE pRespMsg
    );

DWORD
NtlmBuildResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCHAR pPassword,
    IN DWORD dwResponseType,
    IN DWORD dwBufferSize,
    OUT PBYTE pBuffer
    );

DWORD
NtlmBuildLmResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCHAR pPassword,
    IN DWORD dwLength,
    OUT PBYTE pResponse
    );

DWORD
NtlmBuildNtlmResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCHAR pPassword,
    IN DWORD dwLength,
    OUT PBYTE pResponse
    );

DWORD
NtlmBuildNtlmV2Response(
    VOID
    );

DWORD
NtlmBuildLmV2Response(
    VOID
    );

DWORD
NtlmBuildNtlm2Response(
    VOID
    );

DWORD
NtlmBuildAnonymousResponse(
    VOID
    );

DWORD
NtlmCreateNegotiateContext(
    IN OUT PNTLM_CONTEXT *ppNtlmContext
    );

DWORD
NtlmCreateChallengeContext(
    IN PNTLM_CONTEXT pNtlmNegCtxt,
    OUT PNTLM_CONTEXT *ppNtlmContext
    );

DWORD
NtlmCreateResponseContext();

DWORD
NtlmValidateResponse();

DWORD
NtlmCalculateResponseSize(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN DWORD dwResponseType,
    OUT PDWORD pdwSize
    );

DWORD
NtlmCalculateNtlmV2ResponseSize(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    OUT PDWORD pdwSize
    );

DWORD
NtlmCreateMD4Digest(
    PBYTE pBuffer,
    DWORD dwBufferLen,
    BYTE MD4Digest[MD4_DIGEST_LENGTH]
    );

DWORD
NtlmSetParityBit(
    PULONG64 pKey
    );

#endif /* __NTLM_PROTOTYPES_H__ */
