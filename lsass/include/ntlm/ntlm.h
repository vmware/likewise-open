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

#include <lw/types.h>
#include <lw/attrs.h>

#define BAIL_ON_NTLM_ERROR(dwError) \
    if (dwError)               \
    {                          \
        goto error;            \
    }

#define BAIL_ON_INVALID_POINTER(p)                \
        if (NULL == p) {                          \
           dwError = NTLM_ERROR_INTERNAL; \
           BAIL_ON_NTLM_ERROR(dwError);            \
        }

typedef struct _SecHandle
{
    ULONG_PTR       dwLower;
    ULONG_PTR       dwUpper;
} SecHandle, * PSecHandle;

typedef SecHandle    CredHandle;
typedef PSecHandle   PCredHandle;

typedef SecHandle    CtxtHandle;
typedef PSecHandle   PCtxtHandle;

typedef CHAR SEC_CHAR;

typedef struct _SecBuffer
{
    ULONG cbBuffer;
    ULONG BufferType;
    PVOID pvBuffer;
}SecBuffer, *PSecBuffer;

typedef struct _SecBufferDesc
{
    ULONG      ulVersion;
    ULONG      cBuffers;
    PSecBuffer pBuffers;
}SecBufferDesc, *PSecBufferDesc;

typedef struct _LUID
{
    DWORD LowPart;
    LONG  HighPart;
}LUID, *PLUID;

typedef struct _SEC_WINNT_AUTH_IDENTITY
{
  USHORT *User;
  ULONG UserLength;
  USHORT *Domain;
  ULONG DomainLength;
  USHORT *Password;
  ULONG PasswordLength;
  ULONG Flags;
}SEC_WINNT_AUTH_IDENTITY, *PSEC_WINNT_AUTH_IDENTITY;

#define INVALID_HANDLE  ((HANDLE)~0)

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

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

#endif // __NTLM_H__
