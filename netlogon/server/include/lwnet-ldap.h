/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        lwnet-ldap.h
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *        
 *        LDAP API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LWNETLDAP_H__
#define __LWNETLDAP_H__
#ifndef LDAP_DEPRECATED
#define LDAP_DEPRECATED 1
#include <ldap.h>
#endif
#include <lber.h>

typedef struct _AD_DIRECTORY_CONTEXT {
    LDAP *ld;
} AD_DIRECTORY_CONTEXT, *PAD_DIRECTORY_CONTEXT;

DWORD
LWNetCLdapOpenDirectory(
    IN PCSTR pszServerName,
    OUT PHANDLE phDirectory
    );

DWORD
LWNetLdapBindDirectoryAnonymous(
    HANDLE hDirectory
    );

DWORD
LWNetLdapConvertDomainToDN(
    PCSTR pszDomainName,
    PSTR * ppszDomainDN
    );

void
LWNetLdapCloseDirectory(
    HANDLE hDirectory
    );

DWORD
LWNetLdapReadObject(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    PSTR*  ppszAttributeList,
    LDAPMessage **ppMessage
    );

DWORD
LWNetLdapGetParentDN(
    PCSTR pszObjectDN,
    PSTR* ppszParentDN
    );

DWORD
LWNetLdapDirectorySearch(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    int    scope,
    PCSTR  pszQuery,
    PSTR * ppszAttributeList,
    LDAPMessage **ppMessage
    );

DWORD
LWNetLdapDirectorySearchEx(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    int    scope,
    PCSTR  pszQuery,
    PSTR * ppszAttributeList,
    DWORD  dwNumMaxEntries,
    LDAPMessage **ppMessage
    );

LDAPMessage*
LWNetLdapFirstEntry(
    HANDLE hDirectory,
    LDAPMessage *pMessage
    );

LDAPMessage*
LWNetLdapNextEntry(
    HANDLE hDirectory,
    LDAPMessage* pMessage
    );

LDAP *
LWNetLdapGetSession(
    HANDLE hDirectory
    );

DWORD
LWNetLdapGetBytes(
        HANDLE hDirectory,
        LDAPMessage* pMessage,
        PSTR pszFieldName,
        PBYTE* ppszByteValue,
        PDWORD pszByteLen        
        );

DWORD
LWNetLdapGetString(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PCSTR pszFieldName,
    PSTR* ppszValue
    );

DWORD
LWNetLdapGetUInt32(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PCSTR pszFieldName,
    PDWORD pdwValue
    );

DWORD
LWNetLdapGetStrings(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PCSTR pszFieldName,
    PSTR** pppszValues,
    PDWORD pdwNumValues
    );

#endif /* __LWNETLDAP_H__ */
