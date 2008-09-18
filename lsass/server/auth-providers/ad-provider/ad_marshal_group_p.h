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
 *        adldap_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        AD LDAP Group Marshalling functions (private header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */
#ifndef __LSALDAP_MARSHAL_GROUP_P_H__
#define __LSALDAP_MARSHAL_GROUP_P_H__

DWORD
ADSchemaMarshalGroupInfoList_0(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,
    LDAPMessage *pMessagePseudo,
    PVOID**     pppGroupInfoList,
    PDWORD      pNumGroups
    );

DWORD
ADSchemaMarshalGroupInfo_0(
    HANDLE       hDirectory,
    PCSTR        pszNetBIOSDomainName,
    LDAPMessage *pMessageReal,
    LDAPMessage *pMessagePseudo,
    PVOID*       ppGroupInfo
    );

DWORD
ADNonSchemaMarshalGroupInfoList_0(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,    
    LDAPMessage *pMessagePseudo,
    PVOID**     pppGroupInfoList,
    PDWORD      pwdNumGroups
    );

DWORD
ADNonSchemaMarshalGroupInfo_0(
    HANDLE       hDirectory,
    PCSTR        pszNetBIOSDomainName,
    LDAPMessage *pMessageReal,
    LDAPMessage *pMessagePseudo,
    PVOID*       ppGroupInfo
    );

DWORD
ADUnprovisionedMarshalGroupInfoList_0(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,
    LDAPMessage *pMessage,
    PVOID**     pppGroupInfoList,
    PDWORD      pdwNumGroupsFound
    );

DWORD
ADUnprovisionedMarshalGroupInfo_0(
    HANDLE       hDirectory,
    PCSTR        pszNetBIOSDomainName,
    LDAPMessage* pMessage,
    PVOID*       ppGroupInfo
    );

DWORD
ADSchemaMarshalToGroupCache(
    HANDLE                  hDirectory,
    PCSTR                   pszNetBIOSDomainName,
    LDAPMessage*            pMessageReal,
    LDAPMessage*            pMessagePseudo,
    PAD_SECURITY_OBJECT*    ppGroupInfo    
    );

DWORD
ADSchemaMarshalToGroupCacheEx(
    HANDLE                  hPseudoDirectory,
    HANDLE                  hRealDirectory,
    PLSA_LOGIN_NAME_INFO    pGroupNameInfo,
    LDAPMessage*            pMessageReal,
    LDAPMessage*            pMessagePseudo,
    PAD_SECURITY_OBJECT*    ppGroupInfo    
    );

DWORD
ADNonSchemaMarshalToGroupCache(
    HANDLE                  hDirectory,
    PCSTR                   pszNetBIOSDomainName,
    LDAPMessage*            pMessageReal,
    LDAPMessage*            pMessagePseudo,
    PAD_SECURITY_OBJECT*    ppGroupInfo
    );

DWORD
ADNonSchemaMarshalToGroupCacheEx(
    HANDLE                  hPseudoDirectory,
    HANDLE                  hRealDirectory,    
    PLSA_LOGIN_NAME_INFO    pGroupNameInfo,
    LDAPMessage*            pMessageReal,
    LDAPMessage*            pMessagePseudo,
    PAD_SECURITY_OBJECT*    ppGroupInfo    
    );

DWORD
ADUnprovisionedMarshalToGroupCache(
    HANDLE                  hDirectory,
    PCSTR                   pszNetBIOSDomainName,
    LDAPMessage*            pMessage,
    PAD_SECURITY_OBJECT*    ppGroupInfo
    );

#endif //__LSALDAP_MARSHAL_GROUP_P_H__

