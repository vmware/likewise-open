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
    IN HANDLE Handle,
    IN PNTLM_CRED_HANDLE phCredential,
    IN OUT PNTLM_CONTEXT_HANDLE phContext,
    IN PSecBufferDesc pInput,
    IN DWORD fContextReq,
    IN DWORD TargetDataRep,
    IN OUT PNTLM_CONTEXT_HANDLE phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT PDWORD  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    );

DWORD
NtlmServerAcquireCredentialsHandle(
    IN LWMsgCall* pCall,
    IN SEC_CHAR* pszPrincipal,
    IN SEC_CHAR* pszPackage,
    IN DWORD fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    OUT PNTLM_CRED_HANDLE phCredential,
    OUT PTimeStamp ptsExpiry
    );

DWORD
NtlmGetNameInformation(
    PSTR* ppszServerName,
    PSTR* ppszDomainName,
    PSTR* ppszDnsServerName,
    PSTR* ppszDnsDomainName
    );

DWORD
NtlmServerDecryptMessage(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOLEAN pbEncrypted
    );

DWORD
NtlmServerDeleteSecurityContext(
    IN PNTLM_CONTEXT_HANDLE phContext
    );

DWORD
NtlmServerEncryptMessage(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN BOOLEAN bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    );

VOID
NtlmMakeSignature(
    IN PNTLM_CONTEXT pContext,
    IN PBYTE pData,
    IN DWORD dwDataSize,
    IN DWORD dwMsgSeqNum,
    IN OUT PBYTE pToken
    );

DWORD
NtlmCrc32(
    IN PBYTE pData,
    IN DWORD dwDataSize
    );

DWORD
NtlmServerExportSecurityContext(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN DWORD fFlags,
    OUT PSecBuffer pPackedContext,
    OUT OPTIONAL HANDLE *pToken
    );

DWORD
NtlmServerFreeCredentialsHandle(
    IN PNTLM_CRED_HANDLE phCredential
    );

DWORD
NtlmServerImportSecurityContext(
    IN PSECURITY_STRING *pszPackage,
    IN PSecBuffer pPackedContext,
    IN OPTIONAL HANDLE pToken,
    OUT PNTLM_CONTEXT_HANDLE phContext
    );

DWORD
NtlmServerInitializeSecurityContext(
    IN OPTIONAL PNTLM_CRED_HANDLE phCredential,
    IN OPTIONAL PNTLM_CONTEXT_HANDLE phContext,
    IN OPTIONAL SEC_CHAR * pszTargetName,
    IN DWORD fContextReq,
    IN DWORD Reserved1,
    IN DWORD TargetDataRep,
    IN OPTIONAL PSecBufferDesc pInput,
    IN DWORD Reserved2,
    IN OUT OPTIONAL PNTLM_CONTEXT_HANDLE phNewContext,
    IN OUT OPTIONAL PSecBufferDesc pOutput,
    OUT PDWORD pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry
    );

DWORD
NtlmServerMakeSignature(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN BOOLEAN bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    );

DWORD
NtlmServerQueryCredentialsAttributes(
    IN PNTLM_CRED_HANDLE phCredential,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmServerQueryContextAttributes(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmServerVerifySignature(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOLEAN pbVerified,
    OUT PBOOLEAN pbEncryted
    );

DWORD
NtlmServerQueryNameAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_Names* ppBuffer
    );

DWORD
NtlmServerQuerySessionKeyAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_SessionKey* ppBuffer
    );

DWORD
NtlmServerQuerySizeAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_Sizes* ppBuffer
    );

VOID
NtlmReleaseContext(
    IN PNTLM_CONTEXT_HANDLE phContext
    );

VOID
NtlmGetContextInfo(
    IN NTLM_CONTEXT_HANDLE ContextHandle,
    OUT OPTIONAL PNTLM_STATE pNtlmState,
    OUT OPTIONAL PDWORD pNegotiatedFlags,
    OUT OPTIONAL PVOID* ppMessage,
    OUT OPTIONAL PDWORD pdwMessageSize,
    OUT OPTIONAL PBYTE* ppSessionKey,
    OUT OPTIONAL PNTLM_CRED_HANDLE pCredHandle
    );

DWORD
NtlmCreateContext(
    IN PNTLM_CRED_HANDLE pCredHandle,
    OUT PNTLM_CONTEXT *ppNtlmContext
    );

DWORD
NtlmInsertContext(
    PNTLM_CONTEXT pNtlmContext
    );

DWORD
NtlmRemoveContext(
    IN PNTLM_CONTEXT_HANDLE pCtxtHandle
    );

VOID
NtlmRemoveAllContext(
    VOID
    );

DWORD
NtlmFindContext(
    IN PNTLM_CONTEXT_HANDLE pCtxtHandle,
    OUT PNTLM_CONTEXT *ppNtlmContext
    );

VOID
NtlmFreeContext(
    PNTLM_CONTEXT* ppNtlmContext
    );

VOID
NtlmReleaseCredential(
    IN PNTLM_CRED_HANDLE phCred
    );

DWORD
NtlmCreateCredential(
    IN PLSA_CRED_HANDLE pLsaCredHandle,
    IN DWORD dwDirection,
    IN PSTR pServerName,
    IN PSTR pDomainName,
    IN PSTR pDnsServerName,
    IN PSTR pDnsDomainName,
    OUT PNTLM_CREDENTIALS* ppNtlmCreds
    );

VOID
NtlmGetCredentialInfo(
    IN NTLM_CRED_HANDLE CredHandle,
    OUT OPTIONAL PCSTR* pszUserName,
    OUT OPTIONAL PCSTR* pszPassword,
    OUT OPTIONAL uid_t* pUid,
    OUT OPTIONAL PCSTR* pszServerName,
    OUT OPTIONAL PCSTR* pszDomainName,
    OUT OPTIONAL PCSTR* pszDnsServerName,
    OUT OPTIONAL PCSTR* pszDnsDomainName
    );

VOID
NtlmReferenceCredential(
    IN NTLM_CRED_HANDLE hCredential
    );

VOID
NtlmFreeCredential(
    IN PNTLM_CREDENTIALS pCreds
    );

DWORD
NtlmGetMessageFromSecBufferDesc(
    IN PSecBufferDesc pSecBufferDesc,
    OUT PDWORD pdwMessageSize,
    OUT PVOID *ppMessage
    );

DWORD
NtlmCopyContextToSecBufferDesc(
    IN PNTLM_CONTEXT pNtlmContext,
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
    IN PCSTR pDomain,
    IN PCSTR pWorkstation,
    IN PBYTE pOsVersion,
    OUT PDWORD pdwSize,
    OUT PNTLM_NEGOTIATE_MESSAGE *ppNegMsg
    );

DWORD
NtlmCreateChallengeMessage(
    IN PNTLM_NEGOTIATE_MESSAGE pNegMsg,
    IN PCSTR pServerName,
    IN PCSTR pDomainName,
    IN PCSTR pDnsHostName,
    IN PCSTR pDnsDomainName,
    IN PBYTE  pOsVersion,
    OUT PDWORD pdwSize,
    OUT PNTLM_CHALLENGE_MESSAGE *ppChlngMsg
    );

DWORD
NtlmCreateResponseMessage(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pUserName,
    IN PCSTR pPassword,
    IN PBYTE pOsVersion,
    IN DWORD dwNtRespType,
    IN DWORD dwLmRespType,
    OUT PDWORD pdwSize,
    OUT PNTLM_RESPONSE_MESSAGE *ppRespMsg,
    OUT PBYTE pLmUserSessionKey,
    OUT PBYTE pNtlmUserSessionKey
    );

VOID
NtlmStoreSecondaryKey(
    IN PBYTE pMasterKey,
    IN PBYTE pSecondaryKey,
    IN OUT PNTLM_RESPONSE_MESSAGE pMessage
    );

VOID
NtlmWeakenSessionKey(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN OUT PBYTE pMasterKey,
    OUT PDWORD pcbKeyLength
    );

DWORD
NtlmGetAuthTargetNameFromChallenge(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    OUT PCHAR* ppAuthTargetName
    );

DWORD
NtlmBuildResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pUserName,
    IN PCSTR pPassword,
    IN DWORD dwResponseType,
    IN DWORD dwBufferSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE pBuffer
    );

VOID
NtlmBuildLmResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pPassword,
    IN DWORD dwResponseSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE pResponse
    );

DWORD
NtlmBuildNtlmResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pPassword,
    IN DWORD dwResponseSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE pResponse
    );

DWORD
NtlmBuildNtlmV2Response(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pUserName,
    IN PCSTR pPassword,
    IN DWORD dwResponseSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE pResponse
    );

DWORD
NtlmCreateNtlmV2Blob(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN BYTE NtlmHashV2[MD4_DIGEST_LENGTH],
    OUT PDWORD pdwSize,
    OUT PBYTE* ppBlob
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
    IN PNTLM_CRED_HANDLE pCredHandle,
    IN DWORD dwOptions,
    IN PCSTR pDomain,
    IN PCSTR pWorkstation,
    IN PBYTE pOsVersion,
    OUT PNTLM_CONTEXT *ppNtlmContext
    );

DWORD
NtlmCreateChallengeContext(
    IN PNTLM_NEGOTIATE_MESSAGE pNtlmNegMsg,
    IN PNTLM_CRED_HANDLE pCredHandle,
    OUT PNTLM_CONTEXT *ppNtlmContext
    );

DWORD
NtlmCreateResponseContext(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PNTLM_CRED_HANDLE pCredHandle,
    IN OUT PNTLM_CONTEXT *ppNtlmContext
    );

DWORD
NtlmFixUserName(
    IN PCSTR pOriginalUserName,
    OUT PSTR* ppUserName
    );

DWORD
NtlmCreateValidatedContext(
    IN PNTLM_RESPONSE_MESSAGE pNtlmRespMsg,
    IN DWORD dwMsgSize,
    IN DWORD NegotiatedFlags,
    IN PBYTE pSessionKey,
    IN DWORD dwSessionKeyLen,
    IN PNTLM_CRED_HANDLE pCredHandle,
    OUT PNTLM_CONTEXT *ppNtlmContext
    );

DWORD
NtlmValidateResponse(
    IN HANDLE Handle,
    IN PNTLM_RESPONSE_MESSAGE pRespMsg,
    IN DWORD dwRespMsgSize,
    IN PNTLM_CONTEXT pChlngCtxt,
    OUT BYTE pSessionKey[NTLM_SESSION_KEY_SIZE]
    );

DWORD
NtlmGetUserNameFromResponse(
    IN PNTLM_RESPONSE_MESSAGE pRespMsg,
    IN BOOLEAN bUnicode,
    OUT PSTR* ppUserName
    );

DWORD
NtlmGetDomainNameFromResponse(
    IN PNTLM_RESPONSE_MESSAGE pRespMsg,
    IN BOOLEAN bUnicode,
    OUT PSTR* ppDomainName
    );

DWORD
NtlmGetWorkstationFromResponse(
    IN PNTLM_RESPONSE_MESSAGE pRespMsg,
    IN BOOLEAN bUnicode,
    OUT PSTR* ppWorkstation
    );

DWORD
NtlmCalculateResponseSize(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN DWORD dwResponseType,
    OUT PDWORD pdwSize
    );

VOID
NtlmCalculateNtlmV2ResponseSize(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    OUT PDWORD pdwSize
    );

DWORD
NtlmCreateNtlmV1Hash(
    PCSTR pPassword,
    BYTE Hash[MD4_DIGEST_LENGTH]
    );

DWORD
NtlmCreateNtlmV2Hash(
    PCSTR pUserName,
    PCSTR pDomain,
    BYTE NtlmV1Hash[MD4_DIGEST_LENGTH],
    BYTE NtlmV2Hash[MD4_DIGEST_LENGTH]
    );

VOID
NtlmGenerateLanManagerSessionKey(
    IN PNTLM_RESPONSE_MESSAGE pMessage,
    IN PBYTE pLmUserSessionKey,
    OUT PBYTE pLanManagerSessionKey
    );

DWORD
NtlmCreateMD4Digest(
    IN PBYTE pBuffer,
    IN DWORD dwBufferLen,
    OUT BYTE MD4Digest[MD4_DIGEST_LENGTH]
    );

ULONG64
NtlmCreateKeyFromHash(
    IN PBYTE pBuffer,
    IN DWORD dwLength
    );

VOID
NtlmSetParityBit(
    IN OUT PULONG64 pKey
    );

DWORD
NtlmGetProcessSecurity(
    IN LWMsgCall* pCall,
    OUT uid_t* pUid,
    OUT gid_t* pGid
    );

#endif /* __NTLM_PROTOTYPES_H__ */
