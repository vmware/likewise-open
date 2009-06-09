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
 *        clientipc.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Header (Library)
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __CLIENTIPC_P_H__
#define __CLIENTIPC_P_H__

#include <lwmsg/lwmsg.h>
#include <ntlmipc.h>

#if 0
typedef struct __NTLM_CLIENT_CONNECTION_CONTEXT {
    int    fd;
} NTLM_CLIENT_CONNECTION_CONTEXT, *PNTLM_CLIENT_CONNECTION_CONTEXT;
#endif

typedef struct __NTLM_CLIENT_CONNECTION_CONTEXT
{
    LWMsgProtocol* pProtocol;
    LWMsgAssoc* pAssoc;
} NTLM_CLIENT_CONNECTION_CONTEXT, *PNTLM_CLIENT_CONNECTION_CONTEXT;

typedef struct _SecHandle {
  ULONG_PTR       dwLower;
  ULONG_PTR       dwUpper;
} SecHandle, * PSecHandle;

typedef SecHandle    CredHandle;
typedef PSecHandle   PCredHandle;

typedef SecHandle    CtxtHandle;
typedef PSecHandle   PCtxtHandle;

typedef char*   SEC_CHAR;

typedef struct _SecBuffer {
  ULONG cbBuffer;
  ULONG BufferType;
  PVOID pvBuffer;
}SecBuffer, *PSecBuffer;

typedef struct _SecBufferDesc {
  ULONG      ulVersion;
  ULONG      cBuffers;
  PSecBuffer pBuffers;
}SecBufferDesc, *PSecBufferDesc;

DWORD
NtlmTransactClientAcceptSecurityContext(
    IN PCredHandle phCredential,
    IN OUT PCtxtHandle phContext,
    IN PSecBufferDesc pInput,
    IN ULONG fContextReq,
    IN ULONG TargetDataRep,
    IN OUT PCtxtHandle phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT ULONG  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    );

DWORD
NtlmTransactClientAcquireCredentialsHandle(
    IN SEC_CHAR *pszPrincipal,
    IN SEC_CHAR *pszPackage,
    IN ULONG fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    IN SEC_GET_KEY_FN pGetKeyFn,
    IN PVOID pvGetKeyArgument,
    OUT PCredHandle phCredential,
    OUT PTimeStamp ptsExpiry
    );

DWORD
NtlmTransactClientDecryptMessage(
    IN PCtxtHandle phContext,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQoP
    );

DWORD
NtlmTransactClientEncryptMessage(
    IN PCtxtHandle phContext,
    IN ULONG fQoP,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    );

DWORD
NtlmTransactClientExportSecurityContext(
    IN PCtxtHandle phContext,
    IN ULONG fFlags,
    OUT PSecBuffer pPackedContext,
    OUT OPTIONAL HANDLE *pToken
    );

DWORD
NtlmTransactClientFreeCredentialsHandle(
    IN PCredHandle phCredential
    );

DWORD
NtlmTransactClientImportSecurityContext(
    IN PSECURITY_STRING *pszPackage,
    IN PSecBuffer pPackedContext,
    IN OPTIONAL HANDLE pToken,
    OUT PCtxtHandle phContext
    );

DWORD
NtlmTransactClientInitializeSecurityContext(
    IN OPTIONAL PCredHandle phCredential,
    IN OPTIONAL PCtxtHandle phContext,
    IN OPTIONAL SEC_CHAR * pszTargetName,
    IN ULONG fContextReq,
    IN ULONG Reserverd1,
    IN ULONG TargetDataRep,
    IN OPTIONAL PSecBufferDesc pInput,
    IN ULONG Reserved2,
    IN OUT OPTIONAL PCtxtHandle phNewContext,
    IN OUT OPTIONAL PSecBufferDesc pOutput,
    OUT PULONG pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry
    );

DWORD
NtlmTransactClientMakeSignature(
    IN PCtxtHandle phContext,
    IN ULONG fQoP,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    );

DWORD
NtlmTransactClientQueryCredentialsAttributes(
    IN PCredHandle phCredential,
    IN ULONG ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmTransactClientQuerySecurityContextAttributes(
    IN PCtxtHandle phContext,
    IN ULONG ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmTransactClientVerifySignature(
    IN PCtxtHandle phContext,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    );