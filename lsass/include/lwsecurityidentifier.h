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
 *        lwsecurityidentifier.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LWSS)
 *
 *        Security Identifier API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __LWSECURITY_IDENTIFIER_H__
#define __LWSECURITY_IDENTIFIER_H__

#define WELLKNOWN_SID_DOMAIN_USER_GROUP_RID 513

typedef struct __LSA_SECURITY_IDENTIFIER {
    UCHAR* pucSidBytes;  //byte representation of multi-byte Security Identification Descriptor
    DWORD dwByteLength;
} LSA_SECURITY_IDENTIFIER, *PLSA_SECURITY_IDENTIFIER;
DWORD
LwAllocSecurityIdentifierFromBinary(
    UCHAR* pucSidBytes,
    DWORD dwSidBytesLength,
    PLSA_SECURITY_IDENTIFIER* ppSecurityIdentifier
    );

DWORD
LwAllocSecurityIdentifierFromString(
    PCSTR pszSidString,
    PLSA_SECURITY_IDENTIFIER* ppSecurityIdentifier
    );

VOID
LwFreeSecurityIdentifier(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier
    );

DWORD
LwGetSecurityIdentifierRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD pdwRid
    );

DWORD
LwSetSecurityIdentifierRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    DWORD dwRid
    );

DWORD
LwReplaceSidRid(
    IN PCSTR pszSid,
    IN DWORD dwNewRid,
    OUT PSTR* ppszNewSid
    );

DWORD
LwGetSecurityIdentifierHashedRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD dwHashedRid
    );

DWORD
LwHashSecurityIdentifierToId(
    IN PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    OUT PDWORD pdwId
    );

DWORD
LwHashSidStringToId(
    IN PCSTR pszSidString,
    OUT PDWORD pdwId
    );

DWORD
LwGetSecurityIdentifierBinary(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    UCHAR** ppucSidBytes,
    DWORD* pdwSidBytesLength
    );

DWORD
LwGetSecurityIdentifierString(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PSTR* ppszSidStr
    );

DWORD
LwGetDomainSecurityIdentifier(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PLSA_SECURITY_IDENTIFIER* ppDomainSID
    );

DWORD
LwByteArrayToHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    );

DWORD
LwByteArrayToLdapFormatHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    );

DWORD
LwSidStrToLdapFormatHexStr(
    IN PCSTR pszSid,
    OUT PSTR* ppszHexSid
    );

void
LwUidHashCalc(
    PDWORD pdwAuthorities,
    DWORD dwAuthorityCount,
    PDWORD pdwHash
    );

DWORD
LwSidStringToBytes(
    IN PCSTR pszSidString,
    OUT UCHAR** ppucSidBytes,
    OUT DWORD* pdwSidBytesLength
    );

DWORD
LwSidBytesToString(
    UCHAR* pucSidBytes,
    DWORD  dwSidBytesLength,
    PSTR*  ppszSidString
    );

DWORD
LwBuildSIDString(
    PCSTR pszRevision,
    PCSTR pszAuth,
    PBYTE pucSidBytes,
    DWORD dwWordCount,
    PSTR* ppszSidString
    );

#endif /* __LWSECURITY_IDENTIFIER_H__ */
