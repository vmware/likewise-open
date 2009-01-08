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
 *        keytab.h
 *
 * Abstract:
 *
 *        Kerberos 5 keytab management library
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 * 
 */

#ifndef _KEYTAB_H_
#define _KEYTAB_H_

#ifndef _WIN32

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifndef DWORD_DEFINED
#define DWORD_DEFINED 1

typedef uint32_t        DWORD, *PDWORD;

#endif

#ifndef INT_DEFINED
#define INT_DEFINED 1

typedef int             INT, *PINT;

#endif

#ifndef UINT64_DEFINED
#define UINT64_DEFINED 1

typedef uint64_t        UINT64, *PUINT64;

#endif

#ifndef UINT32_DEFINED
#define UINT32_DEFINED 1

typedef uint32_t        UINT32, *PUINT32;

#endif

#ifndef UINT16_DEFINED
#define UINT16_DEFINED 1

typedef uint16_t        UINT16, *PUINT16;

#endif

#ifndef WORD_DEFINED
#define WORD_DEFINED 1

typedef uint16_t WORD, *PWORD;

#endif

#ifndef USHORT_DEFINED
#define USHORT_DEFINED 1

typedef unsigned short  USHORT, *PUSHORT;

#endif

#ifndef ULONG_DEFINED
#define ULONG_DEFINED 1

typedef unsigned long   ULONG, *PULONG;

#endif

#ifndef ULONGLONG_DEFINED
#define ULONGLONG_DEFINED 1

typedef unsigned long long ULONGLONG, *PULONGLONG;

#endif

#ifndef UINT8_DEFINED
#define UINT8_DEFINED 1

typedef uint8_t         UINT8, *PUINT8;

#endif

#ifndef BYTE_DEFINED
#define BYTE_DEFINED

typedef uint8_t BYTE, *PBYTE;

#endif

#ifndef UCHAR_DEFINED
#define UCHAR_DEFINED 1

typedef uint8_t UCHAR, *PUCHAR;

#endif

#ifndef HANDLE_DEFINED
#define HANDLE_DEFINED 1

typedef unsigned long   HANDLE, *PHANDLE;

#endif

#ifndef CHAR_DEFINED
#define CHAR_DEFINED 1

typedef char            CHAR;

#endif

#ifndef PSTR_DEFINED
#define PSTR_DEFINED 1

typedef char *          PSTR;

#endif

#ifndef PCSTR_DEFINED
#define PCSTR_DEFINED 1

typedef const char *    PCSTR;

#endif

#ifndef VOID_DEFINED
#define VOID_DEFINED 1

typedef void            VOID, *PVOID;

#endif

#ifndef PCVOID_DEFINED
#define PCVOID_DEFINED 1

typedef const void      *PCVOID;

#endif

#ifndef BOOLEAN_DEFINED
#define BOOLEAN_DEFINED 1

typedef int             BOOLEAN, *PBOOLEAN;

#endif

#ifndef PWSTR_DEFINED
#define PWSTR_DEFINED 1

typedef wchar16_t *     PWSTR;

#endif

#ifndef PCWSTR_DEFINED
#define PCWSTR_DEFINED 1

typedef const wchar16_t * PCWSTR;

#endif

#endif /* _WIN32 */

#define KT_STATUS_SUCCESS                    (0x00000000)

#define KT_STATUS(code)                      (0x0050 | (code))

/* General errors */
#define KT_STATUS_OUT_OF_MEMORY              KT_STATUS(0x1001)
#define KT_STATUS_INVALID_PARAMETER          KT_STATUS(0x1002)

/* Kerberos errors */
#define KT_STATUS_KRB5_ERROR                 KT_STATUS(0x2000)
#define KT_STATUS_KRB5_CLOCK_SKEW            KT_STATUS(0x2001)
#define KT_STATUS_KRB5_NO_KEYS_FOUND         KT_STATUS(0x2002)
#define KT_STATUS_KRB5_NO_DEFAULT_REALM      KT_STATUS(0x2003)
#define KT_STATUS_KRB5_PASSWORD_EXPIRED      KT_STATUS(0x2004)
#define KT_STATUS_KRB5_PASSWORD_MISMATCH     KT_STATUS(0x2005)
#define KT_STATUS_GSS_CALL_FAILED            KT_STATUS(0x2006)

/* LDAP errors */
#define KT_STATUS_LDAP_ERROR                 KT_STATUS(0x3000)
#define KT_STATUS_LDAP_NO_KVNO_FOUND         KT_STATUS(0x3001)


DWORD
KtKrb5AddKey(
    PCSTR pszPrincipal,
    PVOID pKey,
    DWORD dwKeyLen,
    PCSTR pszSalt,
    PCSTR pszKtPath,
    PCSTR pszDcName,
    DWORD dwKeyVersion
    );


DWORD
KtKrb5AddKeyW(
    PCWSTR pwszPrincipal,
    PVOID pKey,
    DWORD dwKeyLen,
    PCWSTR pwszSalt,
    PCWSTR pwszKtPath,
    PCWSTR pwszDcName,
    DWORD dwKeyVersion
    );


DWORD
KtKrb5GetKey(
    PCSTR pszPrincipal,
    PCSTR pszKtPath,
    DWORD dwEncType,
    PVOID *pKey,
    DWORD *dwKeyLen
    );


DWORD
KtKrb5RemoveKey(
    PSTR pszPrincipal,
    DWORD dwVer,
    PSTR pszKtPath
    );


DWORD
KtKrb5FormatPrincipal(
    PCSTR pszAccount,
    PCSTR pszRealm,
    PSTR *ppszPrincipal
    );


DWORD
KtKrb5FormatPrincipalW(
    PCWSTR pwszAccount,
    PCWSTR pwszRealm,
    PWSTR *ppwszPrincipal
    );

DWORD
KtLdapGetBaseDn(
    PCSTR pszDcName,
    PSTR *pszBaseDn);

DWORD
KtLdapGetBaseDnW(
    PCWSTR pwszDcName,
    PWSTR *ppwszBaseDn);

DWORD
KtLdapGetKeyVersion(
    PCSTR pszDcName,
    PCSTR pszBaseDn,
    PCSTR pszPrincipal,
    DWORD *dwKvno
    );

DWORD
KtLdapGetKeyVersionW(
    PCWSTR pwszDcName,
    PCWSTR pwszBaseDn,
    PCWSTR pwszPrincipal,
    DWORD *dwKvno
    );

DWORD
KtLdapGetSaltingPrincipal(
    PCSTR pszDcName,
    PCSTR pszBaseDn,
    PCSTR pszMachAcctName,
    PSTR *pszSalt);

DWORD
KtGetSaltingPrincipal(
    PCSTR pszMachineName,
    PCSTR pszDnsDomainName,
    PCSTR pszRealmName,
    PCSTR pszDcName,
    PCSTR pszBaseDn,
    PSTR *pszSalt);

DWORD
KtGetSaltingPrincipalW(
    PCWSTR pwszMachineName,
    PCWSTR pwszDnsDomainName,
    PCWSTR pwszRealmName,
    PCWSTR pwszDcName,
    PCWSTR pwszBaseDn,
    PWSTR *pwszSalt);


#endif /* _KEYTAB_H_ */
