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
 *        client.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        API (Client)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#ifndef __PROTOTYPES_H__
#define __PROTOTYPES_H__

DWORD
NtlmClientAcceptSecurityContext(
    IN PCredHandle phCredential,
    IN OUT PCtxtHandle phContext,
    IN PSecBufferDesc pInput,
    IN ULONG fContextReq,
    IN ULONG TargetDataRep,
    IN OUT PCtxtHandle phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT PULONG  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    );

DWORD
NtlmClientAcquireCredentialsHandle(
    IN SEC_CHAR *pszPrincipal,
    IN SEC_CHAR *pszPackage,
    IN ULONG fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    // NOT USED BY NTLM - IN SEC_GET_KEY_FN pGetKeyFn,
    // NOT USED BY NTLM - IN PVOID pvGetKeyArgument,
    OUT PCredHandle phCredential,
    OUT PTimeStamp ptsExpiry
    );

DWORD
NtlmClientDecryptMessage(
    IN PCtxtHandle phContext,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQoP
    );

DWORD
NtlmClientEncryptMessage(
    IN PCtxtHandle phContext,
    IN ULONG fQoP,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    );

DWORD
NtlmClientExportSecurityContext(
    IN PCtxtHandle phContext,
    IN ULONG fFlags,
    OUT PSecBuffer pPackedContext,
    OUT OPTIONAL HANDLE *pToken
    );

DWORD
NtlmClientFreeCredentialsHandle(
    IN PCredHandle phCredential
    );

DWORD
NtlmClientImportSecurityContext(
    IN PSECURITY_STRING *pszPackage,
    IN PSecBuffer pPackedContext,
    IN OPTIONAL HANDLE pToken,
    OUT PCtxtHandle phContext
    );

DWORD
NtlmClientInitializeSecurityContext(
    IN OPTIONAL PCredHandle phCredential,
    IN OPTIONAL PCtxtHandle phContext,
    IN OPTIONAL SEC_CHAR * pszTargetName,
    IN ULONG fContextReq,
    IN ULONG Reserved1,
    IN ULONG TargetDataRep,
    IN OPTIONAL PSecBufferDesc pInput,
    IN ULONG Reserved2,
    IN OUT OPTIONAL PCtxtHandle phNewContext,
    IN OUT OPTIONAL PSecBufferDesc pOutput,
    OUT PULONG pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry
    );

DWORD
NtlmClientMakeSignature(
    IN PCtxtHandle phContext,
    IN ULONG fQoP,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    );

DWORD
NtlmClientQueryCredentialsAttributes(
    IN PCredHandle phCredential,
    IN ULONG ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmClientQuerySecurityContextAttributes(
    IN PCtxtHandle phContext,
    IN ULONG ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmClientVerifySignature(
    IN PCtxtHandle phContext,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    );

#endif // __PROTOTYPES_H__
