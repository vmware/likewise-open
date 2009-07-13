/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ipc_protocol.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        NTLM IPC
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include "ipc.h"

/******************************************************************************/

static LWMsgTypeSpec gNtlmSecHandleSpec[] =
{
    // DWORD       dwLower;
    // DWORD       dwUpper;

    LWMSG_STRUCT_BEGIN(SecHandle),
    LWMSG_MEMBER_POINTER(SecHandle, dwLower, LWMSG_UINT32(DWORD)),
    LWMSG_MEMBER_POINTER(SecHandle, dwUpper, LWMSG_UINT32(DWORD)),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmSecBufferSpec[] =
{
    // DWORD cbBuffer;
    // DWORD BufferType;
    // PVOID pvBuffer;

    LWMSG_STRUCT_BEGIN(SecBuffer),
    LWMSG_MEMBER_UINT32(SecBuffer, cbBuffer),
    LWMSG_MEMBER_UINT32(SecBuffer, BufferType),
    LWMSG_MEMBER_POINTER(SecBuffer, pvBuffer, LWMSG_UINT8(CHAR)),
    LWMSG_ATTR_LENGTH_MEMBER(SecBuffer, cbBuffer),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmSecBufferDescSpec[] =
{
    // For now, I don't believe we need this version information
    // DWORD      ulVersion;
    // DWORD      cBuffers;
    // PSecBuffer pBuffers;

    LWMSG_STRUCT_BEGIN(SecBufferDesc),
    //LWMSG_MEMBER_UINT32(SecBufferDesc, ulVersion),
    LWMSG_MEMBER_UINT32(SecBufferDesc, cBuffers),
    LWMSG_MEMBER_POINTER_BEGIN(SecBufferDesc, pBuffers),
    LWMSG_TYPESPEC(gNtlmSecBufferSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(SecBufferDesc, cBuffers),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmLuidSpec[] =
{
    // DWORD LowPart;
    // INT  HighPart;

    LWMSG_STRUCT_BEGIN(LUID),
    LWMSG_MEMBER_UINT32(LUID, LowPart),
    LWMSG_MEMBER_INT32(LUID, HighPart),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmSecWinntAuthIdSpec[] =
{
    // USHORT *User;
    // DWORD UserLength;
    // USHORT *Domain;
    // DWORD DomainLength;
    // USHORT *Password;
    // DWORD PasswordLength;
    // DWORD Flags;

    LWMSG_STRUCT_BEGIN(SEC_WINNT_AUTH_IDENTITY),

    LWMSG_MEMBER_UINT32(SEC_WINNT_AUTH_IDENTITY, UserLength),

    LWMSG_MEMBER_POINTER(SEC_WINNT_AUTH_IDENTITY, User, LWMSG_UINT16(USHORT)),
    LWMSG_ATTR_LENGTH_MEMBER(SEC_WINNT_AUTH_IDENTITY, UserLength),

    LWMSG_MEMBER_UINT32(SEC_WINNT_AUTH_IDENTITY, DomainLength),

    LWMSG_MEMBER_POINTER(SEC_WINNT_AUTH_IDENTITY, Domain, LWMSG_UINT16(USHORT)),
    LWMSG_ATTR_LENGTH_MEMBER(SEC_WINNT_AUTH_IDENTITY, DomainLength),

    LWMSG_MEMBER_UINT32(SEC_WINNT_AUTH_IDENTITY, PasswordLength),

    LWMSG_MEMBER_POINTER(
        SEC_WINNT_AUTH_IDENTITY,
        Password,
        LWMSG_UINT16(USHORT)
        ),
    LWMSG_ATTR_LENGTH_MEMBER(SEC_WINNT_AUTH_IDENTITY, PasswordLength),

    LWMSG_MEMBER_UINT32(SEC_WINNT_AUTH_IDENTITY, Flags),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmSecurityStringSpec[] =
{
    // USHORT      Length;
    // USHORT      MaximumLength;
    // PUSHORT     Buffer;

    LWMSG_STRUCT_BEGIN(SECURITY_STRING),

    LWMSG_MEMBER_UINT16(SECURITY_STRING, Length),

    LWMSG_MEMBER_UINT16(SECURITY_STRING, MaximumLength),

    LWMSG_MEMBER_POINTER(SECURITY_STRING, Buffer, LWMSG_INT64(USHORT)),
    LWMSG_ATTR_LENGTH_MEMBER(SECURITY_STRING, Length),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmIpcErrorSpec[] =
{
    // DWORD dwError;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ERROR),
    LWMSG_MEMBER_UINT32(NTLM_IPC_ERROR, dwError),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmAcceptSecCtxtSpec[] =
{
    // PCredHandle phCredential;
    // PCtxtHandle phContext;
    // PSecBufferDesc pInput;
    // DWORD fContextReq;
    // DWORD TargetDataRep;
    // PCtxtHandle phNewContext;
    // PSecBufferDesc pOutput;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, phCredential),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, pInput),
    LWMSG_TYPESPEC(gNtlmSecBufferDescSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, fContextReq),

    LWMSG_MEMBER_UINT32(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, TargetDataRep),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, phNewContext),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, pOutput),
    LWMSG_TYPESPEC(gNtlmSecBufferDescSpec),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmAcceptSecCtxtRespSpec[] =
{
    //CtxtHandle hContext;
    //CtxtHandle hNewContext;
    //SecBufferDesc Output;
    //DWORD  fContextAttr;
    //TimeStamp tsTimeStamp;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE,
        hContext,
        gNtlmSecHandleSpec
        ),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE,
        hNewContext,
        gNtlmSecHandleSpec
        ),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE,
        Output,
        gNtlmSecBufferDescSpec
        ),

    LWMSG_MEMBER_UINT32(NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE, fContextAttr),

    LWMSG_MEMBER_UINT64(NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE, tsTimeStamp),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmAcquireCredsSpec[] =
{
    // SEC_CHAR *pszPrincipal;
    // SEC_CHAR *pszPackage;
    // DWORD fCredentialUse;
    // PLUID pvLogonID;
    // PVOID pAuthData;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ACQUIRE_CREDS_REQ),

    LWMSG_MEMBER_PSTR(NTLM_IPC_ACQUIRE_CREDS_REQ, pszPrincipal),

    LWMSG_MEMBER_PSTR(NTLM_IPC_ACQUIRE_CREDS_REQ, pszPackage),

    LWMSG_MEMBER_UINT32(NTLM_IPC_ACQUIRE_CREDS_REQ, fCredentialUse),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACQUIRE_CREDS_REQ, pvLogonID),
    LWMSG_TYPESPEC(gNtlmLuidSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACQUIRE_CREDS_REQ, pAuthData),
    LWMSG_TYPESPEC(gNtlmSecWinntAuthIdSpec),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmAcquireCredsRespSpec[] =
{
    //CredHandle hCredential;
    //TimeStamp tsExpiry;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ACQUIRE_CREDS_RESPONSE),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_ACQUIRE_CREDS_RESPONSE,
        hCredential,
        gNtlmSecHandleSpec
        ),

    LWMSG_MEMBER_UINT64(NTLM_IPC_ACQUIRE_CREDS_RESPONSE, tsExpiry),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmDecryptMsgSpec[] =
{
    // PCtxtHandle phContext;
    // PSecBufferDesc pMessage;
    // ULONG MessageSeqNo;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_DECRYPT_MSG_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_DECRYPT_MSG_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_DECRYPT_MSG_REQ, pMessage),
    LWMSG_TYPESPEC(gNtlmSecBufferDescSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_DECRYPT_MSG_REQ, MessageSeqNo),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmDecryptMsgRespSpec[] =
{
    //SecBufferDesc pMessage;
    //BOOL pbEncrypted;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_DECRYPT_MSG_RESPONSE),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_DECRYPT_MSG_RESPONSE,
        Message,
        gNtlmSecBufferDescSpec
        ),

    LWMSG_MEMBER_UINT32(NTLM_IPC_DECRYPT_MSG_RESPONSE, bEncrypted),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmDeleteSecCtxtSpec[] =
{
    // PCtxtHandle phContext

    LWMSG_STRUCT_BEGIN(NTLM_IPC_DELETE_SEC_CTXT_REQ),

	LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_DELETE_SEC_CTXT_REQ, phContext),
	LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};
    // No Response

/******************************************************************************/

static LWMsgTypeSpec gNtlmEncryptMsgSpec[] =
{
    // PCtxtHandle phContext;
    // BOOL bEncrypt;
    // PSecBufferDesc pMessage;
    // ULONG MessageSeqNo;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ENCRYPT_MSG_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ENCRYPT_MSG_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_ENCRYPT_MSG_REQ, bEncrypt),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ENCRYPT_MSG_REQ, pMessage),
    LWMSG_TYPESPEC(gNtlmSecBufferDescSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_ENCRYPT_MSG_REQ, MessageSeqNo),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmEncryptMsgRespSpec[] =
{
    //SecBufferDesc Message;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ENCRYPT_MSG_RESPONSE),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_ENCRYPT_MSG_RESPONSE,
        Message,
        gNtlmSecBufferDescSpec
        ),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmExportSecCtxtSpec[] =
{
    // PCtxtHandle phContext;
    // ULONG fFlags;
    // PSecBuffer pPackedContext;
    // HANDLE *pToken;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_EXPORT_SEC_CTXT_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_EXPORT_SEC_CTXT_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_EXPORT_SEC_CTXT_REQ, fFlags),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_EXPORT_SEC_CTXT_REQ, pPackedContext),
    LWMSG_TYPESPEC(gNtlmSecBufferSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_EXPORT_SEC_CTXT_REQ, pToken),
    LWMSG_INT64(VOID),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmExportSecCtxtRespSpec[] =
{
    //SecBuffer PackedContext;
    //HANDLE hToken;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_EXPORT_SEC_CTXT_RESPONSE),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_EXPORT_SEC_CTXT_RESPONSE,
        PackedContext,
        gNtlmSecBufferSpec
        ),

    LWMSG_MEMBER_INT64(NTLM_IPC_EXPORT_SEC_CTXT_RESPONSE, hToken)

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmFreeCredsSpec[] =
{
    // PCredHandle phCredential;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_FREE_CREDS_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_FREE_CREDS_REQ, phCredential),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

    // No Response

/******************************************************************************/

static LWMsgTypeSpec gNtlmImportSecCtxtSpec[] =
{
    // PSECURITY_STRING *pszPackage;
    // PSecBuffer pPackedContext;
    // HANDLE pToken;
    // PCtxtHandle phContext;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_IMPORT_SEC_CTXT_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_IMPORT_SEC_CTXT_REQ, pszPackage),
    LWMSG_TYPESPEC(gNtlmSecurityStringSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_IMPORT_SEC_CTXT_REQ, pPackedContext),
    LWMSG_TYPESPEC(gNtlmSecBufferSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_IMPORT_SEC_CTXT_REQ, pToken),
    LWMSG_INT64(VOID),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_IMPORT_SEC_CTXT_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmImportSecCtxtRespSpec[] =
{
    //CtxtHandle hContext;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_IMPORT_SEC_CTXT_RESPONSE),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_IMPORT_SEC_CTXT_RESPONSE,
        hContext,
        gNtlmSecHandleSpec
        ),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmInitSecCtxtSpec[] =
{
    //PCredHandle phCredential;
    //PCtxtHandle phContext;
    //SEC_CHAR * pszTargetName;
    //ULONG fContextReq;
    //ULONG Reserverd1;
    //ULONG TargetDataRep;
    //PSecBufferDesc pInput;
    //ULONG Reserved2;
    //PCtxtHandle phNewContext;
    //PSecBufferDesc pOutput;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_INIT_SEC_CTXT_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_INIT_SEC_CTXT_REQ, phCredential),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_INIT_SEC_CTXT_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_PSTR(NTLM_IPC_INIT_SEC_CTXT_REQ, pszTargetName),

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_REQ, fContextReq),

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_REQ, Reserved1),

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_REQ, TargetDataRep),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_INIT_SEC_CTXT_REQ, pInput),
    LWMSG_TYPESPEC(gNtlmSecBufferDescSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_REQ, Reserved2),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_INIT_SEC_CTXT_REQ, phNewContext),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_INIT_SEC_CTXT_REQ, pOutput),
    LWMSG_TYPESPEC(gNtlmSecBufferDescSpec),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmInitSecCtxtRespSpec[] =
{
    //CtxtHandle hNewContext;
    //SecBufferDesc Output;
    //DWORD fContextAttr;
    //TimeStamp tsExpiry;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_INIT_SEC_CTXT_RESPONSE),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_INIT_SEC_CTXT_RESPONSE,
        hNewContext,
        gNtlmSecHandleSpec
        ),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_INIT_SEC_CTXT_RESPONSE,
        Output,
        gNtlmSecBufferDescSpec
        ),

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_RESPONSE, fContextAttr),

    LWMSG_MEMBER_UINT64(NTLM_IPC_INIT_SEC_CTXT_RESPONSE, tsExpiry),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmMakeSignSpec[] =
{
    // PCtxtHandle phContext;
    // DWORD bEncrypt;
    // PSecBufferDesc pMessage;
    // ULONG MessageSeqNo;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_MAKE_SIGN_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_MAKE_SIGN_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_MAKE_SIGN_REQ, bEncrypt),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_MAKE_SIGN_REQ, pMessage),
    LWMSG_TYPESPEC(gNtlmSecBufferDescSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_MAKE_SIGN_REQ, MessageSeqNo),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmMakeSignRespSpec[] =
{
    //SecBufferDesc Message;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_MAKE_SIGN_RESPONSE),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_MAKE_SIGN_RESPONSE,
        Message,
        gNtlmSecBufferDescSpec
        ),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmQueryCredsSpec[] =
{
    // PCredHandle phCredential;
    // ULONG ulAttribute;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_QUERY_CREDS_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_QUERY_CREDS_REQ, phCredential),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_QUERY_CREDS_REQ, ulAttribute),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmQueryCredsRespSpec[] =
{
    //DWORD dwBufferSize;
    //PVOID pBuffer;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_QUERY_CREDS_RESPONSE),

    LWMSG_MEMBER_UINT32(NTLM_IPC_QUERY_CREDS_RESPONSE, dwBufferSize),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_QUERY_CREDS_RESPONSE, pBuffer),
    LWMSG_UINT8(CHAR),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(NTLM_IPC_QUERY_CREDS_RESPONSE, dwBufferSize),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmQueryCtxtSpec[] =
{
    // PCtxtHandle phContext;
    // ULONG ulAttribute;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_QUERY_CTXT_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_QUERY_CTXT_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_QUERY_CTXT_REQ, ulAttribute),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmQueryCtxtRespSpec[] =
{
    //DWORD dwBufferSize;
    //PVOID pBuffer;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_QUERY_CTXT_RESPONSE),

    LWMSG_MEMBER_UINT32(NTLM_IPC_QUERY_CTXT_RESPONSE, dwBufferSize),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_QUERY_CTXT_RESPONSE, pBuffer),
    LWMSG_UINT8(CHAR),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(NTLM_IPC_QUERY_CTXT_RESPONSE, dwBufferSize),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmVerifySignSpec[] =
{
    // PCtxtHandle phContext;
    // PSecBufferDesc pMessage;
    // ULONG MessageSeqNo;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_VERIFY_SIGN_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_VERIFY_SIGN_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandleSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_VERIFY_SIGN_REQ, pMessage),
    LWMSG_TYPESPEC(gNtlmSecBufferDescSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_VERIFY_SIGN_REQ, MessageSeqNo),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmVerifySignRespSpec[] =
{
    //BOOL bVerified;
    //BOOL bEncryted;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_VERIFY_SIGN_RESPONSE),

    LWMSG_MEMBER_UINT32(NTLM_IPC_VERIFY_SIGN_RESPONSE, bVerified),

    LWMSG_MEMBER_UINT32(NTLM_IPC_VERIFY_SIGN_RESPONSE, bEncrypted),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgProtocolSpec gNtlmIpcSpec[] =
{
    LWMSG_MESSAGE(NTLM_Q_ACCEPT_SEC_CTXT, gNtlmAcceptSecCtxtSpec),
    LWMSG_MESSAGE(NTLM_R_ACCEPT_SEC_CTXT_SUCCESS, gNtlmAcceptSecCtxtRespSpec),
    LWMSG_MESSAGE(NTLM_R_ACCEPT_SEC_CTXT_FAILURE, gNtlmIpcErrorSpec),

    LWMSG_MESSAGE(NTLM_Q_ACQUIRE_CREDS, gNtlmAcquireCredsSpec),
    LWMSG_MESSAGE(NTLM_R_ACQUIRE_CREDS_SUCCESS, gNtlmAcquireCredsRespSpec),
    LWMSG_MESSAGE(NTLM_R_ACQUIRE_CREDS_FAILURE, gNtlmIpcErrorSpec),

    LWMSG_MESSAGE(NTLM_Q_DECRYPT_MSG, gNtlmDecryptMsgSpec),
    LWMSG_MESSAGE(NTLM_R_DECRYPT_MSG_SUCCESS, gNtlmDecryptMsgRespSpec),
    LWMSG_MESSAGE(NTLM_R_DECRYPT_MSG_FAILURE, gNtlmIpcErrorSpec),

    LWMSG_MESSAGE(NTLM_Q_DELETE_SEC_CTXT, gNtlmDeleteSecCtxtSpec),
    LWMSG_MESSAGE(NTLM_R_DELETE_SEC_CTXT_SUCCESS, NULL),
    LWMSG_MESSAGE(NTLM_R_DELETE_SEC_CTXT_FAILURE, gNtlmIpcErrorSpec),

    LWMSG_MESSAGE(NTLM_Q_ENCRYPT_MSG, gNtlmEncryptMsgSpec),
    LWMSG_MESSAGE(NTLM_R_ENCRYPT_MSG_SUCCESS, gNtlmEncryptMsgRespSpec),
    LWMSG_MESSAGE(NTLM_R_ENCRYPT_MSG_FAILURE, gNtlmIpcErrorSpec),

    LWMSG_MESSAGE(NTLM_Q_EXPORT_SEC_CTXT, gNtlmExportSecCtxtSpec),
    LWMSG_MESSAGE(NTLM_R_EXPORT_SEC_CTXT_SUCCESS, gNtlmExportSecCtxtRespSpec),
    LWMSG_MESSAGE(NTLM_R_EXPORT_SEC_CTXT_FAILURE, gNtlmIpcErrorSpec),

    LWMSG_MESSAGE(NTLM_Q_FREE_CREDS, gNtlmFreeCredsSpec),
    LWMSG_MESSAGE(NTLM_R_FREE_CREDS_SUCCESS, NULL),
    LWMSG_MESSAGE(NTLM_R_FREE_CREDS_FAILURE, gNtlmIpcErrorSpec),

    LWMSG_MESSAGE(NTLM_Q_IMPORT_SEC_CTXT, gNtlmImportSecCtxtSpec),
    LWMSG_MESSAGE(NTLM_R_IMPORT_SEC_CTXT_SUCCESS, gNtlmImportSecCtxtRespSpec),
    LWMSG_MESSAGE(NTLM_R_IMPORT_SEC_CTXT_FAILURE, gNtlmIpcErrorSpec),

    LWMSG_MESSAGE(NTLM_Q_INIT_SEC_CTXT, gNtlmInitSecCtxtSpec),
    LWMSG_MESSAGE(NTLM_R_INIT_SEC_CTXT_SUCCESS, gNtlmInitSecCtxtRespSpec),
    LWMSG_MESSAGE(NTLM_R_INIT_SEC_CTXT_FAILURE, gNtlmIpcErrorSpec),

    LWMSG_MESSAGE(NTLM_Q_MAKE_SIGN, gNtlmMakeSignSpec),
    LWMSG_MESSAGE(NTLM_R_MAKE_SIGN_SUCCESS, gNtlmMakeSignRespSpec),
    LWMSG_MESSAGE(NTLM_R_MAKE_SIGN_FAILURE, gNtlmIpcErrorSpec),

    LWMSG_MESSAGE(NTLM_Q_QUERY_CREDS, gNtlmQueryCredsSpec),
    LWMSG_MESSAGE(NTLM_R_QUERY_CREDS_SUCCESS, gNtlmQueryCredsRespSpec),
    LWMSG_MESSAGE(NTLM_R_QUERY_CREDS_FAILURE, gNtlmIpcErrorSpec),

    LWMSG_MESSAGE(NTLM_Q_QUERY_CTXT, gNtlmQueryCtxtSpec),
    LWMSG_MESSAGE(NTLM_R_QUERY_CTXT_SUCCESS, gNtlmQueryCtxtRespSpec),
    LWMSG_MESSAGE(NTLM_R_QUERY_CTXT_FAILURE, gNtlmIpcErrorSpec),

    LWMSG_MESSAGE(NTLM_Q_VERIFY_SIGN, gNtlmVerifySignSpec),
    LWMSG_MESSAGE(NTLM_R_VERIFY_SIGN_SUCCESS, gNtlmVerifySignRespSpec),
    LWMSG_MESSAGE(NTLM_R_VERIFY_SIGN_FAILURE, gNtlmIpcErrorSpec),

    LWMSG_PROTOCOL_END
};

LWMsgProtocolSpec*
NtlmIpcGetProtocolSpec(
    VOID
    )
{
    return gNtlmIpcSpec;
}

DWORD
NtlmMapLwmsgStatus(
    LWMsgStatus status
    )
{
    switch (status)
    {
    default:
        return LW_ERROR_INTERNAL;
    case LWMSG_STATUS_SUCCESS:
        return LW_ERROR_SUCCESS;
    case LWMSG_STATUS_ERROR:
        return LW_ERROR_INTERNAL;
    case LWMSG_STATUS_MEMORY:
        return LW_ERROR_OUT_OF_MEMORY;
    case LWMSG_STATUS_MALFORMED:
    case LWMSG_STATUS_OVERFLOW:
    case LWMSG_STATUS_UNDERFLOW:
    case LWMSG_STATUS_EOF:
        return LW_ERROR_INVALID_MESSAGE;
    case LWMSG_STATUS_INVALID_PARAMETER:
        return EINVAL;
    case LWMSG_STATUS_INVALID_STATE:
        return EINVAL;
    case LWMSG_STATUS_UNIMPLEMENTED:
        return LW_ERROR_NOT_IMPLEMENTED;
    case LWMSG_STATUS_SYSTEM:
        return LW_ERROR_INTERNAL;
    case LWMSG_STATUS_SECURITY:
        return EACCES;
    case LWMSG_STATUS_CANCELLED:
        return EINTR;
    case LWMSG_STATUS_FILE_NOT_FOUND:
        return ENOENT;
    case LWMSG_STATUS_CONNECTION_REFUSED:
        return ECONNREFUSED;
    case LWMSG_STATUS_PEER_RESET:
        return ECONNRESET;
    case LWMSG_STATUS_PEER_ABORT:
        return ECONNABORTED;
    case LWMSG_STATUS_PEER_CLOSE:
        return EPIPE;
    case LWMSG_STATUS_SESSION_LOST:
        return EPIPE;
    }
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
