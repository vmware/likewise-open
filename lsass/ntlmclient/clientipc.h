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
 *        Inter-process Communication (NTLM Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#ifndef __CLIENTIPC_H__
#define __CLIENTIPC_H__

#include <lwmsg/lwmsg.h>

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

DWORD
NtlmTransactAcceptSecurityContext(
    IN HANDLE hServer,
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
NtlmTransactAcquireCredentialsHandle(
    IN HANDLE hServer,
    IN SEC_CHAR *pszPrincipal,
    IN SEC_CHAR *pszPackage,
    IN ULONG fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    // NOT NEEDED BY NTLM - IN SEC_GET_KEY_FN pGetKeyFn,
    // NOT NEEDED BY NTLM - IN PVOID pvGetKeyArgument,
    OUT PCredHandle phCredential,
    OUT PTimeStamp ptsExpiry
    );

DWORD
NtlmTransactDecryptMessage(
    IN HANDLE hServer,
    IN PCtxtHandle phContext,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQoP
    );

DWORD
NtlmTransactEncryptMessage(
    IN HANDLE hServer,
    IN PCtxtHandle phContext,
    IN ULONG fQoP,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    );

DWORD
NtlmTransactExportSecurityContext(
    IN HANDLE hServer,
    IN PCtxtHandle phContext,
    IN ULONG fFlags,
    OUT PSecBuffer pPackedContext,
    OUT OPTIONAL HANDLE *pToken
    );

DWORD
NtlmTransactFreeCredentialsHandle(
    IN HANDLE hServer,
    IN PCredHandle phCredential
    );

DWORD
NtlmTransactImportSecurityContext(
    IN HANDLE hServer,
    IN PSECURITY_STRING *pszPackage,
    IN PSecBuffer pPackedContext,
    IN OPTIONAL HANDLE pToken,
    OUT PCtxtHandle phContext
    );

DWORD
NtlmTransactInitializeSecurityContext(
    IN HANDLE hServer,
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
NtlmTransactMakeSignature(
    IN HANDLE hServer,
    IN PCtxtHandle phContext,
    IN ULONG fQoP,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    );

DWORD
NtlmTransactQueryCredentialsAttributes(
    IN HANDLE hServer,
    IN PCredHandle phCredential,
    IN ULONG ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmTransactQueryContextAttributes(
    IN HANDLE hServer,
    IN PCtxtHandle phContext,
    IN ULONG ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmTransactVerifySignature(
    IN HANDLE hServer,
    IN PCtxtHandle phContext,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    );

#endif
