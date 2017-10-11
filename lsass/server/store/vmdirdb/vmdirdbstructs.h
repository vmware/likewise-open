/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        samdbstructs.h
 *
 * Abstract:
 *
 *
 *      Likewise VMDIR Database Provider
 *
 *      VMDIR database structures
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#ifndef __VMDIRDBSTRUCTS_H__
#define __VMDIRDBSTRUCTS_H__

typedef struct _VMDIRDB_OBJECTCLASS_TO_ATTR_MAP_INFO
{
    VMDIRDB_OBJECT_CLASS        objectClass;
    PVMDIRDB_ATTRIBUTE_MAP_INFO pAttributeMaps;
    DWORD                     dwNumMaps;

} VMDIRDB_OBJECTCLASS_TO_ATTR_MAP_INFO, *PVMDIRDB_OBJECTCLASS_TO_ATTR_MAP_INFO;

typedef struct _VMDIR_DB_CONTEXT
{
    struct _VMDIR_DB_CONTEXT* pNext;

} VMDIR_DB_CONTEXT, *PVMDIR_DB_CONTEXT;

typedef struct _VMDIR_DB_ATTR_LOOKUP
{
    PLWRTL_RB_TREE pLookupTable;

} VMDIR_DB_ATTR_LOOKUP, *PVMDIR_DB_ATTR_LOOKUP;

typedef DWORD (*VMDIRDB_LDAPQUERY_MAP_ENTRY_TRANSFORM_FUNC)(
    DWORD dwNumEntries,
    PDIRECTORY_ENTRY in,
    PDIRECTORY_ENTRY *out /* Allocated by transform function */
    );

/* Printf-like function. Data parsed from the SQL filter
   is passed in as arguments. LDAP filter syntax like the
   following will use that data:
   (&(ObjectClass=dcObject)(entryDn=%dn))
*/
typedef PSTR (*VMDIR_LDAPQUERY_FILTER_FORMAT_FUNC)(
    PSTR pszLdapFilterTemplate,
    ...
    );

typedef struct _VMDIRDB_LDAPQUERY_MAP_ENTRY
{
    PSTR pszSqlQuery;
    PSTR pszLdapQuery;
    PSTR pszLdapBase;
    PSTR *ppszLdapAttributes; /* optional */
    PDWORD pdwLdapAttributesType; /* optional */
 
    /* Optional: Construct ldap filter helper */
    VMDIR_LDAPQUERY_FILTER_FORMAT_FUNC pfnLdapFilterPrintf;  

    /* Optional: DIRECTORY_ENTRY constructor helper function */
    VMDIRDB_LDAPQUERY_MAP_ENTRY_TRANSFORM_FUNC pfnTransform;
    ULONG uScope;
} VMDIRDB_LDAPQUERY_MAP_ENTRY, *PVMDIRDB_LDAPQUERY_MAP_ENTRY;

typedef struct _VMDIRDB_LDAPQUERY_MAP
{
    DWORD dwNumEntries;
    DWORD dwMaxEntries;
    VMDIRDB_LDAPQUERY_MAP_ENTRY queryMap[];
} VMDIRDB_LDAPQUERY_MAP, *PVMDIRDB_LDAPQUERY_MAP;

typedef struct _VMDIRDB_LDAPATTR_MAP_ENTRY
{
    PWSTR pwszAttribute;
    PSTR pszAttribute;
    DWORD dwType;
} VMDIRDB_LDAPATTR_MAP_ENTRY, *PVMDIRDB_LDAPATTR_MAP_ENTRY;

typedef struct _VMDIRDB_LDAPATTR_MAP
{
    DWORD dwNumEntries;
    DWORD dwMaxEntries;
    VMDIRDB_LDAPATTR_MAP_ENTRY attrMap[];
} VMDIRDB_LDAPATTR_MAP, *PVMDIRDB_LDAPATTR_MAP;

typedef struct _VMDIR_DIRECTORY_CONTEXT
{
    PWSTR    pwszDistinguishedName;
    PWSTR    pwszCredential;
    ULONG    ulMethod;

    PVMDIR_DB_CONTEXT         pDbContext;

    PVMDIRDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassAttrMaps;
    DWORD                               dwNumObjectClassAttrMaps;
    PVMDIR_DB_ATTR_LOOKUP   pAttrLookup;

} VMDIR_DIRECTORY_CONTEXT, *PVMDIR_DIRECTORY_CONTEXT;

typedef struct _VMDIR_GLOBALS
{
    pthread_mutex_t mutex;
    pthread_rwlock_t  rwLock;
    pthread_rwlock_t* pRwLock;
    PSTR            pszProviderName;
    DIRECTORY_PROVIDER_FUNCTION_TABLE providerFunctionTable;
    VMDIRDB_BIND_PROTOCOL bindProtocol;
    PVMDIRDB_LDAPQUERY_MAP pLdapMap;
    PVMDIRDB_LDAPATTR_MAP pLdapAttrMap;
    PSTR pszDomainDn;
} VMDIR_GLOBALS, *PVMDIR_GLOBALS;

typedef struct _VMDIR_DB_DOMAIN_INFO
{
    ULONG ulDomainRecordId;

    PWSTR pwszDomainName;
    PWSTR pwszNetBIOSName;
    PWSTR pwszDomainSID;

} VMDIR_DB_DOMAIN_INFO, *PVMDIR_DB_DOMAIN_INFO;

typedef struct _VMDIRDB_DN_TOKEN
{
    VMDIRDB_DN_TOKEN_TYPE tokenType;
    PWSTR               pwszDN;
    PWSTR               pwszToken;
    DWORD               dwLen;

    struct _VMDIRDB_DN_TOKEN * pNext;

} VMDIRDB_DN_TOKEN, *PVMDIRDB_DN_TOKEN;

typedef struct _VMDIR_DB_DN
{
    PWSTR pwszDN;

    PVMDIRDB_DN_TOKEN pTokenList;

} VMDIR_DB_DN, *PVMDIR_DB_DN;

typedef struct _VMDIR_DB_COLUMN_VALUE
{
    PVMDIRDB_ATTRIBUTE_MAP_INFO pAttrMapInfo;
    PVMDIR_DB_ATTRIBUTE_MAP     pAttrMap;

    PDIRECTORY_MOD            pDirMod;

    ULONG            ulNumValues;
    PATTRIBUTE_VALUE pAttrValues;

    struct _VMDIR_DB_COLUMN_VALUE* pNext;

} VMDIR_DB_COLUMN_VALUE, *PVMDIR_DB_COLUMN_VALUE;

#endif /* __VMDIRDBSTRUCTS_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
