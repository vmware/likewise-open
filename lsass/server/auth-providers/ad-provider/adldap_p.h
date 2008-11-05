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
 *        AD LDAP helper functions (private header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */
#ifndef __ADLDAP_P_H__
#define __ADLDAP_P_H__
 
typedef DWORD (*PFN_DEFAULT_FINDUSER_NAMEBYALIAS_FUNC)(PCSTR pszAlias, PCSTR pszRootDomainName, PSTR* ppszUpnName);

typedef struct _LSA_AD_DN_LISTS_ENTRY {
    DWORD dwDNCount;
    PSTR* ppszDNValues;    
} LSA_AD_DN_LISTS_ENTRY, *PLSA_AD_DN_LISTS_ENTRY;

DWORD
ADBuildGetUsersByDNQuery(
    PSTR*  ppszUserDNList,
    DWORD  dwNumUsersFound,
    PDWORD pdwNumUsersProcessed, /* in/out parameter */
    PSTR*  ppszQuery
    );

DWORD
ADBuildGetPseudoGroupsBySIDQuery(
    PCSTR* ppszRealSIDList,
    DWORD  dwRealGroupsFound,
    PDWORD pdwRealSIDsProcessed, /* in/out parameter */
    PSTR*  ppszQuery
    );

DWORD
ADCopyAttributeList(
    PSTR   szAttributeList[],
    PSTR** pppOutputAttributeList);

DWORD
UnprovisionedModeMakeLocalSID(
    PCSTR pszDomainSID,
    DWORD dwID,
    PSTR* ppszLocalSID
    );

DWORD
ADUnprovisionalModeGetSid(
    HANDLE  hDirectory,    
    DWORD   dwUID, //this is assumed to be a hashed UID.
    PSTR*   ppszObjectSID      
    );

DWORD
ADLdap_DefaultFindUserNameByAlias(
    PCSTR pszAlias,
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_CellFindUserNameByAlias(        
    PCSTR pszAlias,
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_DefaultFindUserNameByAliasInDomain(
    PCSTR pszAlias,   
    PCSTR pszDomainName,    
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_DefaultFindUserNameById(
    uid_t uid,
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_DefaultFindUserNameByIdInDomain(
    uid_t uid,  
    PCSTR pszDomainName,    
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_DefaultSchemaFindUserNameByAlias(
    PCSTR pszAlias,    
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_DefaultSchemaFindUserNameById(
    uid_t uid,    
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_DefaultSchemaFindUserNameHelper(
    HANDLE hGCDirectory,    
    PCSTR pszQuery,
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_DefaultNonSchemaFindUserNameByAlias(        
    PCSTR pszAlias,
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_DefaultNonSchemaFindUserNameById(        
    uid_t uid,
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_DefaultNonSchemaFindUserNameHelper(
    HANDLE hGCDirectory,
    PCSTR  pszRootDomainName,
    PCSTR  pszQuery,
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_CellFindUserNameByAlias(        
    PCSTR pszAlias,    
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_CellFindUserNameByAliasInOneCell(        
    PCSTR pszAlias,    
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_CellFindUserNameById(        
    uid_t uid,    
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_CellFindUserNameByIdInOneCell(        
    uid_t uid,    
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_CellFindUserNameHelper(
    HANDLE hDirectory,    
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PCSTR pszQuery,
    PSTR* ppszNT4Name);

DWORD
ADLdap_UnprovisionedFindUserNameById( 
    uid_t uid,
    PSTR* ppszNT4Name);

DWORD
ADLdap_UnprovisionedFindGroupNameById( 
    gid_t gid,
    PSTR* ppszNT4Name);

DWORD
ADLdap_UnprovisionedFindObjectNameByIdInDomain( 
    DWORD dwId,
    PCSTR pszDomainName,
    PSTR* ppszNT4Name);

DWORD
ADLdap_UnprovisionedFindObjectNameByIdInDomainHelper(    
    DWORD dwId,
    PCSTR pszPrimaryDomainName,
    PCSTR pszDomainName,
    PSTR* ppszNT4Name);

DWORD
ADLdap_DefaultFindGroupNameByAlias(
    PCSTR pszAlias,
    PSTR* ppszNT4Name);

DWORD
ADLdap_DefaultFindGroupNameByAliasInDomain(
    PCSTR pszAlias,   
    PCSTR pszDomainName,    
    PSTR* ppszNT4Name);

DWORD
ADLdap_CellFindGroupNameByAlias(        
    PCSTR pszAlias,    
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name);

DWORD
ADLdap_CellFindGroupNameByAliasInOneCell(        
    PCSTR pszAlias,    
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name);

DWORD
ADLdap_CellFindGroupNameById(        
    gid_t gid,    
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name);

DWORD
ADLdap_CellFindGroupNameByIdInOneCell(        
    gid_t gid,    
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name
    );

DWORD
ADLdap_DefaultSchemaFindGroupNameByAlias(
    PCSTR pszAlias,    
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name);

DWORD
ADLdap_DefaultSchemaFindGroupNameById(
    gid_t gid,    
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name);

DWORD
ADLdap_CellFindGroupNameHelper(
    HANDLE hDirectory,    
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PCSTR pszQuery,
    PSTR* ppszNT4Name);

DWORD
ADLdap_DefaultSchemaFindGroupNameHelper(
    HANDLE hGCDirectory,    
    PCSTR pszQuery,
    PSTR* ppszNT4Name);

DWORD
ADLdap_DefaultNonSchemaFindGroupNameByAlias(        
    PCSTR pszAlias,
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name);

DWORD
ADLdap_DefaultNonSchemaFindGroupNameById(        
    gid_t gid,
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name);

DWORD
ADLdap_DefaultNonSchemaFindGroupNameHelper(
    HANDLE hGCDirectory,
    PCSTR  pszRootDomainName,
    PCSTR  pszQuery,
    PSTR* ppszNT4Name);

DWORD
ADLdap_DefaultFindGroupNameById(
    gid_t gid,
    PSTR* ppszNT4Name);

DWORD
ADLdap_DefaultFindGroupNameByIdInDomain(
    uid_t uid,  
    PCSTR pszDomainName,    
    PSTR* ppszNT4Name);

DWORD
ADFindUserByNameNonAliasHelper(
    HANDLE  hPseudoDirectory,
    HANDLE  hRealDirectory,    
    PCSTR   pszCellDN,
    DWORD   dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PAD_SECURITY_OBJECT *ppUserInfo);

DWORD
ADFindGroupByNameNT4Helper(
    HANDLE  hPseudoDirectory,
    HANDLE  hRealDirectory, 
    PCSTR   pszCellDN,
    DWORD   dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PAD_SECURITY_OBJECT* ppGroupInfo);

DWORD
ADLdap_GetGroupMembersDNList(
    IN HANDLE hDirectory,
    IN PCSTR pszGroupDN,
    OUT PDWORD pdwTotalDNCount,
    OUT PSTR** pppszDNValues
    );

#endif 

