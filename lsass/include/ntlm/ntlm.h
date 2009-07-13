/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        ntlm.h
 *
 * Abstract:
 *
 *          Common structure definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */
#ifndef __NTLM_H__
#define __NTLM_H__

#include <config.h>
#include <lw/types.h>
#include <lw/attrs.h>
#include <lwmsg/lwmsg.h>
#include <lsasystem.h>
#include <lwdef.h>
#include <lwerror.h>
#include <lwmem.h>
#include <lwsecurityidentifier.h>

//******************************************************************************
//
// S T R U C T S
//

typedef struct _SecHandle
{
    DWORD       dwLower;
    DWORD       dwUpper;
} SecHandle, * PSecHandle;

typedef SecHandle    CredHandle;
typedef PSecHandle   PCredHandle;

typedef SecHandle    CtxtHandle;
typedef PSecHandle   PCtxtHandle;

typedef CHAR SEC_CHAR;

// Again, under windows, these are ULONG values, which would be equivalent to
// DWORD values here.  Just noting the structure "change".
typedef struct _SecBuffer
{
    DWORD cbBuffer;
    DWORD BufferType;
    PVOID pvBuffer;
}SecBuffer, *PSecBuffer;

typedef struct _SecBufferDesc
{
    // At this point, we do not require version information
    // DWORD      ulVersion;
    DWORD      cBuffers;
    PSecBuffer pBuffers;
}SecBufferDesc, *PSecBufferDesc;

typedef struct _SecPkgContext_Sizes
{
  DWORD cbMaxToken;
  DWORD cbMaxSignature;
  DWORD cbBlockSize;
  DWORD cbSecurityTrailer;
}SecPkgContext_Sizes, *PSecPkgContext_Sizes;

// Again, under windows, HighPart would be a long which is 32 bit; here we have
// to adjust the size accordingly.
typedef struct _LUID
{
    DWORD LowPart;
    INT  HighPart;
}LUID, *PLUID;

// Again... windows... 32 bits
typedef struct _SEC_WINNT_AUTH_IDENTITY
{
  USHORT *User;
  DWORD UserLength;
  USHORT *Domain;
  DWORD DomainLength;
  USHORT *Password;
  DWORD PasswordLength;
  DWORD Flags;
}SEC_WINNT_AUTH_IDENTITY, *PSEC_WINNT_AUTH_IDENTITY;

typedef INT64 SECURITY_INTEGER, *PSECURITY_INTEGER;
//typedef LARGE_INTEGER _SECURITY_INTEGER, SECURITY_INTEGER, *PSECURITY_INTEGER;

typedef SECURITY_INTEGER TimeStamp;                 // ntifs
typedef SECURITY_INTEGER * PTimeStamp;      // ntifs

//
// If we are in 32 bit mode, define the SECURITY_STRING structure,
// as a clone of the base UNICODE_STRING structure.  This is used
// internally in security components, an as the string interface
// for kernel components (e.g. FSPs)
//
// I'm going to default this to always be the non-unicode string
// type so that its marshalling is predictable.  It can be converted
// on either side if need be.
//
typedef struct _SECURITY_STRING
{
    USHORT      Length;
    USHORT      MaximumLength;
    PUSHORT     Buffer;
} SECURITY_STRING, * PSECURITY_STRING;

typedef struct _NTLM_SEC_BUFFER
{
    USHORT usLength;
    USHORT usMaxLength;
    DWORD  dwOffset;
} NTLM_SEC_BUFFER, *PNTLM_SEC_BUFFER;

typedef struct _WIN_VERSION_INFO
{
    BYTE    bMajor;
    BYTE    bMinor;
    SHORT   sBuild;
    DWORD   dwReserved;
} WIN_VERSION_INFO, *PWIN_VERSION_INFO;

//******************************************************************************
//
// D E F I N E S
//

#define BAIL_ON_NTLM_ERROR(dwError) \
    if (dwError)                    \
    {                               \
        goto error;                 \
    }

#define BAIL_ON_NTLM_INVALID_POINTER(p) \
    if (NULL == p)                      \
    {                                   \
       dwError = LW_ERROR_INTERNAL;   \
       BAIL_ON_NTLM_ERROR(dwError);     \
    }

#define INVALID_HANDLE  ((HANDLE)~0)

#define SECPKG_ATTR_SIZES           0

#define SECBUFFER_DATA    0
#define SECBUFFER_PADDING 1
#define SECBUFFER_TOKEN   2
#define SECBUFFER_STREAM  3

//  NTLM FLAGS
//
#define NTLM_FLAG_UNICODE               0x00000001  /* unicode charset */
#define NTLM_FLAG_OEM                   0x00000002  /* oem charset */
#define NTLM_FLAG_REQUEST_TARGET        0x00000004  /* ret trgt in challenge */
#define NTLM_FLAG_SIGN                  0x00000010  /* sign requested */
#define NTLM_FLAG_SEAL                  0x00000020  /* encryption requested */
#define NTLM_FLAG_DATAGRAM              0x00000040  /* udp message */
#define NTLM_FLAG_LM_KEY                0x00000080  /* use LM key for crypto */
#define NTLM_FLAG_NETWARE               0x00000100  /* netware - unsupported */
#define NTLM_FLAG_NTLM                  0x00000200  /* use NTLM auth */
#define NTLM_FLAG_DOMAIN                0x00001000  /* domain supplied */
#define NTLM_FLAG_WORKSTATION           0x00002000  /* wks supplied */
#define NTLM_FLAG_LOCAL_CALL            0x00004000  /* loopback auth */
#define NTLM_FLAG_ALWAYS_SIGN           0x00008000  /* use dummy sig */
#define NTLM_FLAG_TYPE_DOMAIN           0x00010000  /* domain authenticator */
#define NTLM_FLAG_TYPE_SERVER           0x00020000  /* server authenticator */
#define NTLM_FLAG_TYPE_SHARE            0x00040000  /* share authenticator */
#define NTLM_FLAG_NTLM2                 0x00080000  /* use NTLMv2 key */
#define NTLM_FLAG_INIT_RESPONSE         0x00100000  /* unknown */
#define NTLM_FLAG_ACCEPT_RESPONSE       0x00200000  /* unknown */
#define NTLM_FLAG_NON_NT_SESSION_KEY    0x00400000  /* unknown */
#define NTLM_FLAG_TARGET_INFO           0x00800000  /* target info used */
#define NTLM_FLAG_UNKNOWN_02000000      0x02000000  /* needed, for what? */
#define NTLM_FLAG_128                   0x20000000  /* 128-bit encryption */
#define NTLM_FLAG_KEY_EXCH              0x40000000  /* perform key exchange */
#define NTLM_FLAG_56                    0x80000000  /* 56-bit encryption */

#define NTLM_FLAG_NEGOTIATE_DEFAULT ( \
    NTLM_FLAG_UNICODE         | \
    NTLM_FLAG_OEM             | \
    NTLM_FLAG_REQUEST_TARGET  | \
    NTLM_FLAG_NTLM            | \
    NTLM_FLAG_WORKSTATION     | \
    NTLM_FLAG_NTLM2           | \
    NTLM_FLAG_128             | \
    NTLM_FLAG_56              )

#define NTLM_FLAG_SRV_SUPPORTS ( \
    NTLM_FLAG_UNICODE          | \
    NTLM_FLAG_OEM              | \
    NTLM_FLAG_REQUEST_TARGET   | \
    NTLM_FLAG_NTLM             | \
    NTLM_FLAG_LOCAL_CALL       | \
    NTLM_FLAG_ALWAYS_SIGN      | \
    NTLM_FLAG_WORKSTATION      | \
    NTLM_FLAG_NTLM2            | \
    NTLM_FLAG_TARGET_INFO      | \
    NTLM_FLAG_128              | \
    NTLM_FLAG_56               )

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

//******************************************************************************
//
// E X T E R N S
//

//******************************************************************************
//
// P R O T O T Y P E S
//

DWORD
NtlmClientAcceptSecurityContext(
    IN PCredHandle phCredential,
    IN OUT PCtxtHandle phContext,
    IN PSecBufferDesc pInput,
    IN DWORD fContextReq,
    IN DWORD TargetDataRep,
    IN OUT PCtxtHandle phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT PDWORD  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    );

DWORD
NtlmClientAcquireCredentialsHandle(
    IN SEC_CHAR *pszPrincipal,
    IN SEC_CHAR *pszPackage,
    IN DWORD fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    OUT PCredHandle phCredential,
    OUT PTimeStamp ptsExpiry
    );

DWORD
NtlmClientDecryptMessage(
    IN PCtxtHandle phContext,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOL pbEncrypted
    );

DWORD
NtlmClientDeleteSecurityContext(
    IN PCtxtHandle phContext
    );

DWORD
NtlmClientEncryptMessage(
    IN PCtxtHandle phContext,
    IN BOOL bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    );

DWORD
NtlmClientExportSecurityContext(
    IN PCtxtHandle phContext,
    IN DWORD fFlags,
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
    IN DWORD fContextReq,
    IN DWORD Reserved1,
    IN DWORD TargetDataRep,
    IN OPTIONAL PSecBufferDesc pInput,
    IN DWORD Reserved2,
    IN OUT OPTIONAL PCtxtHandle phNewContext,
    IN OUT OPTIONAL PSecBufferDesc pOutput,
    OUT PDWORD pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry
    );

DWORD
NtlmClientMakeSignature(
    IN PCtxtHandle phContext,
    IN BOOL bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    );

DWORD
NtlmClientQueryCredentialsAttributes(
    IN PCredHandle phCredential,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmClientQueryContextAttributes(
    IN PCtxtHandle phContext,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmClientVerifySignature(
    IN PCtxtHandle phContext,
    IN PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOL pbVerified,
    OUT PBOOL pbEncryted
    );

DWORD
NtlmServerAcceptSecurityContext(
    IN PCredHandle phCredential,
    IN OUT PCtxtHandle phContext,
    IN PSecBufferDesc pInput,
    IN DWORD fContextReq,
    IN DWORD TargetDataRep,
    IN OUT PCtxtHandle phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT PDWORD  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    );

DWORD
NtlmServerAcquireCredentialsHandle(
    IN SEC_CHAR *pszPrincipal,
    IN SEC_CHAR *pszPackage,
    IN DWORD fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    OUT PCredHandle phCredential,
    OUT PTimeStamp ptsExpiry
    );

DWORD
NtlmServerDecryptMessage(
    IN PCtxtHandle phContext,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOL pbEncrypted
    );

DWORD
NtlmServerDeleteSecurityContext(
    IN PCtxtHandle phContext
    );

DWORD
NtlmServerEncryptMessage(
    IN PCtxtHandle phContext,
    IN BOOL bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    );

DWORD
NtlmServerExportSecurityContext(
    IN PCtxtHandle phContext,
    IN DWORD fFlags,
    OUT PSecBuffer pPackedContext,
    OUT OPTIONAL HANDLE *pToken
    );

DWORD
NtlmServerFreeCredentialsHandle(
    IN PCredHandle phCredential
    );

DWORD
NtlmServerImportSecurityContext(
    IN PSECURITY_STRING *pszPackage,
    IN PSecBuffer pPackedContext,
    IN OPTIONAL HANDLE pToken,
    OUT PCtxtHandle phContext
    );

DWORD
NtlmServerInitializeSecurityContext(
    IN OPTIONAL PCredHandle phCredential,
    IN OPTIONAL PCtxtHandle phContext,
    IN OPTIONAL SEC_CHAR * pszTargetName,
    IN DWORD fContextReq,
    IN DWORD Reserved1,
    IN DWORD TargetDataRep,
    IN OPTIONAL PSecBufferDesc pInput,
    IN DWORD Reserved2,
    IN OUT OPTIONAL PCtxtHandle phNewContext,
    IN OUT OPTIONAL PSecBufferDesc pOutput,
    OUT PDWORD pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry
    );

DWORD
NtlmServerMakeSignature(
    IN PCtxtHandle phContext,
    IN BOOL bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    );

DWORD
NtlmServerQueryCredentialsAttributes(
    IN PCredHandle phCredential,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmServerQueryContextAttributes(
    IN PCtxtHandle phContext,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmServerVerifySignature(
    IN PCtxtHandle phContext,
    IN PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOL pbVerified,
    OUT PBOOL pbEncryted
    );

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

#endif // __NTLM_H__
