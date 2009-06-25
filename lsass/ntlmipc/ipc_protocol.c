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

static LWMsgTypeSpec gNtlmSecHandle[] =
{
    // ULONG_PTR       dwLower;
    // ULONG_PTR       dwUpper;

    LWMSG_STRUCT_BEGIN(SecHandle),
    LWMSG_MEMBER_POINTER(SecHandle, dwLower, LWMSG_INT64(long)), //LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_POINTER(SecHandle, dwUpper, LWMSG_INT64(long)), //LWMSG_ATTR_NOT_NULL,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmSecBuffer[] =
{
    // ULONG cbBuffer;
    // ULONG BufferType;
    // PVOID pvBuffer;

    LWMSG_STRUCT_BEGIN(SecBuffer),
    LWMSG_MEMBER_UINT32(SecBuffer, cbBuffer),
    LWMSG_MEMBER_UINT32(SecBuffer, BufferType),
    LWMSG_MEMBER_POINTER(SecBuffer, pvBuffer, LWMSG_UINT8(char)), //LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_LENGTH_MEMBER(SecBuffer, cbBuffer),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmSecBufferDesc[] =
{
    // ULONG      ulVersion;
    // ULONG      cBuffers;
    // PSecBuffer pBuffers;

    LWMSG_STRUCT_BEGIN(SecBufferDesc),
    LWMSG_MEMBER_UINT32(SecBufferDesc, ulVersion),
    LWMSG_MEMBER_UINT32(SecBufferDesc, cBuffers),
    LWMSG_MEMBER_POINTER_BEGIN(SecBufferDesc, pBuffers),
    LWMSG_TYPESPEC(gNtlmSecBuffer),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(SecBufferDesc, cBuffers),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmLuid[] =
{
    // DWORD LowPart;
    // LONG  HighPart;

    LWMSG_STRUCT_BEGIN(LUID),
    LWMSG_MEMBER_UINT32(LUID, LowPart),
    LWMSG_MEMBER_INT32(LUID, HighPart),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmSecWinntAuthId[] =
{
    // USHORT *User;
    // ULONG UserLength;
    // USHORT *Domain;
    // ULONG DomainLength;
    // USHORT *Password;
    // ULONG PasswordLength;
    // ULONG Flags;

    LWMSG_STRUCT_BEGIN(SEC_WINNT_AUTH_IDENTITY),

    LWMSG_MEMBER_UINT32(SEC_WINNT_AUTH_IDENTITY, UserLength),

    LWMSG_MEMBER_POINTER(SEC_WINNT_AUTH_IDENTITY, User, LWMSG_UINT16(short)),
    LWMSG_ATTR_LENGTH_MEMBER(SEC_WINNT_AUTH_IDENTITY, UserLength),

    LWMSG_MEMBER_UINT32(SEC_WINNT_AUTH_IDENTITY, DomainLength),

    LWMSG_MEMBER_POINTER(SEC_WINNT_AUTH_IDENTITY, Domain, LWMSG_UINT16(short)),
    LWMSG_ATTR_LENGTH_MEMBER(SEC_WINNT_AUTH_IDENTITY, DomainLength),

    LWMSG_MEMBER_UINT32(SEC_WINNT_AUTH_IDENTITY, PasswordLength),

    LWMSG_MEMBER_POINTER(SEC_WINNT_AUTH_IDENTITY, Password, LWMSG_UINT16(short)),
    LWMSG_ATTR_LENGTH_MEMBER(SEC_WINNT_AUTH_IDENTITY, PasswordLength),

    LWMSG_MEMBER_UINT32(SEC_WINNT_AUTH_IDENTITY, Flags),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmSecurityString[] =
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

static LWMsgTypeSpec gNtlmIpcErrorSpec[] =
{
    // DWORD dwError;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ERROR),
    LWMSG_MEMBER_UINT32(NTLM_IPC_ERROR, dwError),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmAcceptSecCtxt[] =
{
    // PCredHandle phCredential;
    // PCtxtHandle phContext;
    // PSecBufferDesc pInput;
    // ULONG fContextReq;
    // ULONG TargetDataRep;
    // PCtxtHandle phNewContext;
    // PSecBufferDesc pOutput;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, phCredential),
    LWMSG_TYPESPEC(gNtlmSecHandle),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandle),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, pInput),
    LWMSG_TYPESPEC(gNtlmSecBufferDesc),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, fContextReq),

    LWMSG_MEMBER_UINT32(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, TargetDataRep),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, phNewContext),
    LWMSG_TYPESPEC(gNtlmSecHandle),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, pOutput),
    LWMSG_TYPESPEC(gNtlmSecBufferDesc),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmAcquireCreds[] =
{
    // SEC_CHAR *pszPrincipal;
    // SEC_CHAR *pszPackage;
    // ULONG fCredentialUse;
    // PLUID pvLogonID;
    // PVOID pAuthData;
    // NOT USED BY NTLM - SEC_GET_KEY_FN pGetKeyFn;
    // NOT USED BY NTLM - PVOID pvGetKeyArgument;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ACQUIRE_CREDS_REQ),

    LWMSG_MEMBER_PSTR(NTLM_IPC_ACQUIRE_CREDS_REQ, pszPrincipal),

    LWMSG_MEMBER_PSTR(NTLM_IPC_ACQUIRE_CREDS_REQ, pszPackage),

    LWMSG_MEMBER_UINT32(NTLM_IPC_ACQUIRE_CREDS_REQ, fCredentialUse),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACQUIRE_CREDS_REQ, pvLogonID),
    LWMSG_TYPESPEC(gNtlmLuid),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACQUIRE_CREDS_REQ, pAuthData),
    LWMSG_TYPESPEC(gNtlmSecWinntAuthId),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmDecryptMsg[] =
{
    // PCtxtHandle phContext;
    // PSecBufferDesc pMessage;
    // ULONG MessageSeqNo;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_DECRYPT_MSG_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_DECRYPT_MSG_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandle),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_DECRYPT_MSG_REQ, pMessage),
    LWMSG_TYPESPEC(gNtlmSecBufferDesc),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_DECRYPT_MSG_REQ, MessageSeqNo),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmEncryptMsg[] =
{
    // PCtxtHandle phContext;
    // ULONG fQoP;
    // PSecBufferDesc pMessage;
    // ULONG MessageSeqNo;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ENCRYPT_MSG_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ENCRYPT_MSG_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandle),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_ENCRYPT_MSG_REQ, fQoP),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ENCRYPT_MSG_REQ, pMessage),
    LWMSG_TYPESPEC(gNtlmSecBufferDesc),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_ENCRYPT_MSG_REQ, MessageSeqNo),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmExportSecCtxt[] =
{
    // PCtxtHandle phContext;
    // ULONG fFlags;
    // PSecBuffer pPackedContext;
    // HANDLE *pToken;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_EXPORT_SEC_CTXT_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_EXPORT_SEC_CTXT_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandle),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_EXPORT_SEC_CTXT_REQ, fFlags),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_EXPORT_SEC_CTXT_REQ, pPackedContext),
    LWMSG_TYPESPEC(gNtlmSecBuffer),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER(NTLM_IPC_EXPORT_SEC_CTXT_REQ, pToken, LWMSG_INT64(VOID)),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


static LWMsgTypeSpec gNtlmFreeCreds[] =
{
    // PCredHandle phCredential;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_FREE_CREDS_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_FREE_CREDS_REQ, phCredential),
    LWMSG_TYPESPEC(gNtlmSecHandle),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmImportSecCtxt[] =
{
    // PSECURITY_STRING *pszPackage;
    // PSecBuffer pPackedContext;
    // HANDLE pToken;
    // PCtxtHandle phContext;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_IMPORT_SEC_CTXT_REQ),

    LWMSG_MEMBER_POINTER(NTLM_IPC_IMPORT_SEC_CTXT_REQ, pszPackage, LWMSG_TYPESPEC(gNtlmSecurityString)),

    LWMSG_MEMBER_POINTER(NTLM_IPC_IMPORT_SEC_CTXT_REQ, pPackedContext, LWMSG_TYPESPEC(gNtlmSecBuffer)),

    LWMSG_MEMBER_POINTER(NTLM_IPC_IMPORT_SEC_CTXT_REQ, pToken, LWMSG_INT64(VOID)),

    LWMSG_MEMBER_POINTER(NTLM_IPC_IMPORT_SEC_CTXT_REQ, phContext, LWMSG_TYPESPEC(gNtlmSecHandle)),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmInitSecCtxt[] =
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
    LWMSG_TYPESPEC(gNtlmSecHandle),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_INIT_SEC_CTXT_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandle),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_PSTR(NTLM_IPC_INIT_SEC_CTXT_REQ, pszTargetName),

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_REQ, fContextReq),

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_REQ, Reserved1),

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_REQ, TargetDataRep),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_INIT_SEC_CTXT_REQ, pInput),
    LWMSG_TYPESPEC(gNtlmSecBufferDesc),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_REQ, Reserved2),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_INIT_SEC_CTXT_REQ, phNewContext),
    LWMSG_TYPESPEC(gNtlmSecHandle),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_INIT_SEC_CTXT_REQ, pOutput),
    LWMSG_TYPESPEC(gNtlmSecBufferDesc),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmMakeSign[] =
{
    // PCtxtHandle phContext;
    // ULONG fQoP;
    // PSecBufferDesc pMessage;
    // ULONG MessageSeqNo;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_MAKE_SIGN_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_MAKE_SIGN_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandle),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_MAKE_SIGN_REQ, fQoP),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_MAKE_SIGN_REQ, pMessage),
    LWMSG_TYPESPEC(gNtlmSecBufferDesc),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_MAKE_SIGN_REQ, MessageSeqNo),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmQueryCreds[] =
{
    // PCredHandle phCredential;
    // ULONG ulAttribute;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_QUERY_CREDS_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_QUERY_CREDS_REQ, phCredential),
    LWMSG_TYPESPEC(gNtlmSecHandle),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_QUERY_CREDS_REQ, ulAttribute),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmQueryCtxt[] =
{
    // PCtxtHandle phContext;
    // ULONG ulAttribute;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_QUERY_CTXT_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_QUERY_CTXT_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandle),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_QUERY_CTXT_REQ, ulAttribute),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmVerifySign[] =
{
    // PCtxtHandle phContext;
    // PSecBufferDesc pMessage;
    // ULONG MessageSeqNo;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_VERIFY_SIGN_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_VERIFY_SIGN_REQ, phContext),
    LWMSG_TYPESPEC(gNtlmSecHandle),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_VERIFY_SIGN_REQ, pMessage),
    LWMSG_TYPESPEC(gNtlmSecBufferDesc),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_VERIFY_SIGN_REQ, MessageSeqNo),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgProtocolSpec gNtlmIpcSpec[] =
{
    LWMSG_MESSAGE(NTLM_Q_ACCEPT_SEC_CTXT, gNtlmAcceptSecCtxt),
    LWMSG_MESSAGE(NTLM_R_ACCEPT_SEC_CTXT_SUCCESS, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_R_ACCEPT_SEC_CTXT_FAILURE, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_Q_ACQUIRE_CREDS, gNtlmAcquireCreds),
    LWMSG_MESSAGE(NTLM_R_ACQUIRE_CREDS_SUCCESS, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_R_ACQUIRE_CREDS_FAILURE, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_Q_DECRYPT_MSG, gNtlmDecryptMsg),
    LWMSG_MESSAGE(NTLM_R_DECRYPT_MSG_SUCCESS, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_R_DECRYPT_MSG_FAILURE, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_Q_ENCRYPT_MSG, gNtlmEncryptMsg),
    LWMSG_MESSAGE(NTLM_R_ENCRYPT_MSG_SUCCESS, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_R_ENCRYPT_MSG_FAILURE, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_Q_EXPORT_SEC_CTXT, gNtlmExportSecCtxt),
    LWMSG_MESSAGE(NTLM_R_EXPORT_SEC_CTXT_SUCCESS, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_R_EXPORT_SEC_CTXT_FAILURE, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_Q_FREE_CREDS, gNtlmFreeCreds),
    LWMSG_MESSAGE(NTLM_R_FREE_CREDS_SUCCESS, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_R_FREE_CREDS_FAILURE, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_Q_IMPORT_SEC_CTXT, gNtlmImportSecCtxt),
    LWMSG_MESSAGE(NTLM_R_IMPORT_SEC_CTXT_SUCCESS, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_R_IMPORT_SEC_CTXT_FAILURE, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_Q_INIT_SEC_CTXT, gNtlmInitSecCtxt),
    LWMSG_MESSAGE(NTLM_R_INIT_SEC_CTXT_SUCCESS, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_R_INIT_SEC_CTXT_FAILURE, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_Q_MAKE_SIGN, gNtlmMakeSign),
    LWMSG_MESSAGE(NTLM_R_MAKE_SIGN_SUCCESS, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_R_MAKE_SIGN_FAILURE, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_Q_QUERY_CREDS, gNtlmQueryCreds),
    LWMSG_MESSAGE(NTLM_R_QUERY_CREDS_SUCCESS, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_R_QUERY_CREDS_FAILURE, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_Q_QUERY_CTXT, gNtlmQueryCtxt),
    LWMSG_MESSAGE(NTLM_R_QUERY_CTXT_SUCCESS, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_R_QUERY_CTXT_FAILURE, gNtlmIpcErrorSpec),
    LWMSG_MESSAGE(NTLM_Q_VERIFY_SIGN, gNtlmVerifySign),
    LWMSG_MESSAGE(NTLM_R_VERIFY_SIGN_SUCCESS, gNtlmIpcErrorSpec),
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
        return NTLM_ERROR_INTERNAL;
    case LWMSG_STATUS_SUCCESS:
        return NTLM_ERROR_SUCCESS;
    case LWMSG_STATUS_ERROR:
    case LWMSG_STATUS_MEMORY:
    case LWMSG_STATUS_MALFORMED:
    case LWMSG_STATUS_OVERFLOW:
    case LWMSG_STATUS_UNDERFLOW:
    case LWMSG_STATUS_EOF:
    case LWMSG_STATUS_UNIMPLEMENTED:
    case LWMSG_STATUS_SYSTEM:
        return NTLM_ERROR_INTERNAL;
    case LWMSG_STATUS_INVALID_PARAMETER:
        return EINVAL;
    case LWMSG_STATUS_INVALID_STATE:
        return EINVAL;
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
