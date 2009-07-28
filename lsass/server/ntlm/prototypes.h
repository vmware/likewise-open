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
NtlmServerAcceptSecurityContext(
    IN PLSA_CRED_HANDLE phCredential,
    IN OUT PLSA_CONTEXT_HANDLE phContext,
    IN PSecBufferDesc pInput,
    IN DWORD fContextReq,
    IN DWORD TargetDataRep,
    IN OUT PLSA_CONTEXT_HANDLE phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT PDWORD  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    );

DWORD
NtlmServerAcquireCredentialsHandle(
    IN LWMsgAssoc* pAssoc,
    IN SEC_CHAR *pszPrincipal,
    IN SEC_CHAR *pszPackage,
    IN DWORD fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    OUT PLSA_CRED_HANDLE phCredential,
    OUT PTimeStamp ptsExpiry
    );

DWORD
NtlmServerDecryptMessage(
    IN PLSA_CONTEXT_HANDLE phContext,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOL pbEncrypted
    );

DWORD
NtlmServerDeleteSecurityContext(
    IN PLSA_CONTEXT_HANDLE phContext
    );

DWORD
NtlmServerEncryptMessage(
    IN PLSA_CONTEXT_HANDLE phContext,
    IN BOOL bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    );

DWORD
NtlmServerExportSecurityContext(
    IN PLSA_CONTEXT_HANDLE phContext,
    IN DWORD fFlags,
    OUT PSecBuffer pPackedContext,
    OUT OPTIONAL HANDLE *pToken
    );

DWORD
NtlmServerFreeCredentialsHandle(
    IN PLSA_CRED_HANDLE phCredential
    );

DWORD
NtlmServerImportSecurityContext(
    IN PSECURITY_STRING *pszPackage,
    IN PSecBuffer pPackedContext,
    IN OPTIONAL HANDLE pToken,
    OUT PLSA_CONTEXT_HANDLE phContext
    );

DWORD
NtlmServerInitializeSecurityContext(
    IN OPTIONAL PLSA_CRED_HANDLE phCredential,
    IN OPTIONAL PLSA_CONTEXT_HANDLE phContext,
    IN OPTIONAL SEC_CHAR * pszTargetName,
    IN DWORD fContextReq,
    IN DWORD Reserved1,
    IN DWORD TargetDataRep,
    IN OPTIONAL PSecBufferDesc pInput,
    IN DWORD Reserved2,
    IN OUT OPTIONAL PLSA_CONTEXT_HANDLE phNewContext,
    IN OUT OPTIONAL PSecBufferDesc pOutput,
    OUT PDWORD pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry
    );

DWORD
NtlmServerMakeSignature(
    IN PLSA_CONTEXT_HANDLE phContext,
    IN BOOL bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    );

DWORD
NtlmServerQueryCredentialsAttributes(
    IN PLSA_CRED_HANDLE phCredential,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmServerQueryContextAttributes(
    IN PLSA_CONTEXT_HANDLE phContext,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmServerVerifySignature(
    IN PLSA_CONTEXT_HANDLE phContext,
    IN PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOL pbVerified,
    OUT PBOOL pbEncryted
    );

DWORD
NtlmInitializeContextDatabase(
    VOID
    );

VOID
NtlmShutdownContextDatabase(
    VOID
    );

VOID
NtlmAddContext(
    IN PLSA_CONTEXT         pContext,
    OUT PLSA_CONTEXT_HANDLE pContextHandle
    );

VOID
NtlmReleaseContext(
    IN LSA_CONTEXT_HANDLE hContext
    );

VOID
NtlmGetContextInfo(
    IN LSA_CONTEXT_HANDLE           ContextHandle,
    OUT OPTIONAL PNTLM_STATE        pNtlmState,
    OUT OPTIONAL PVOID*             ppMessage,
    OUT OPTIONAL PDWORD             pdwMessageSize,
    OUT OPTIONAL PLSA_CRED_HANDLE   pCredHandle
    );

DWORD
NtlmInitContext(
    OUT PLSA_CONTEXT *ppNtlmContext
    );

DWORD
NtlmInsertContext(
    PLSA_CONTEXT pNtlmContext
    );

DWORD
NtlmRemoveContext(
    IN PLSA_CONTEXT_HANDLE pCtxtHandle
    );

VOID
NtlmRemoveAllContext(
    VOID
    );

DWORD
NtlmFindContext(
    IN PLSA_CONTEXT_HANDLE pCtxtHandle,
    OUT PLSA_CONTEXT *ppNtlmContext
    );

VOID
NtlmFreeContext(
    PLSA_CONTEXT pNtlmContext
    );

DWORD
NtlmCreateContextFromSecBufferDesc(
    PSecBufferDesc pSecBufferDesc,
    NTLM_STATE nsContextType,
    PLSA_CONTEXT *ppNtlmContext
    );

DWORD
NtlmCopyContextToSecBufferDesc(
    IN PLSA_CONTEXT pNtlmContext,
    IN OUT PSecBufferDesc pSecBufferDesc
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
    OUT PDWORD pdwSize,
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
    OUT PDWORD pdwSize,
    OUT PNTLM_CHALLENGE_MESSAGE *ppChlngMsg
    );

DWORD
NtlmCreateResponseMessage(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCHAR pAuthTargetName,
    IN PCSTR pUserName,
    IN PCHAR pWorkstation,
    IN PBYTE pOsVersion,
    IN PCSTR pPassword,
    IN DWORD dwNtRespType,
    IN DWORD dwLmRespType,
    OUT PDWORD pdwSize,
    OUT PNTLM_RESPONSE_MESSAGE *ppRespMsg
    );

DWORD
NtlmValidateResponseMessage(
    IN PNTLM_RESPONSE_MESSAGE pRespMsg
    );

DWORD
NtlmBuildResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pPassword,
    IN DWORD dwResponseType,
    IN DWORD dwBufferSize,
    OUT PBYTE pBuffer
    );

DWORD
NtlmBuildLmResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pPassword,
    IN DWORD dwLength,
    OUT PBYTE pResponse
    );

DWORD
NtlmBuildNtlmResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pPassword,
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
    IN DWORD dwOptions,
    IN PCHAR pDomain,
    IN PCHAR pWorkstation,
    IN PBYTE pOsVersion,
    OUT PLSA_CONTEXT *ppNtlmContext
    );

DWORD
NtlmCreateChallengeContext(
    IN PLSA_CONTEXT pNtlmNegCtxt,
    OUT PLSA_CONTEXT *ppNtlmContext
    );

DWORD
NtlmCreateResponseContext(
    IN PLSA_CONTEXT pChlngCtxt,
    OUT PLSA_CONTEXT *ppNtlmContext
    );

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

DWORD
NtlmGetProcessSecurity(
    IN LWMsgAssoc* pAssoc,
    OUT uid_t* pUid,
    OUT gid_t* pGid
    );

#endif /* __NTLM_PROTOTYPES_H__ */
