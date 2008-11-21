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
 *        ad_enum_objects.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Wei Fu (wfu@likewisesoftware.com)
 */


#ifndef AD_ENUM_OBJECTS_H_
#define AD_ENUM_OBJECTS_H_

typedef DWORD LSA_AD_MARSHAL_INFO_LIST_MODE;

#define LSA_AD_MARSHAL_MODE_DEFAULT_SCHEMA 1
#define LSA_AD_MARSHAL_MODE_UNPROVISIONED  2
// Include Cell mode (schema/non-schema) and Default non-schema
#define LSA_AD_MARSHAL_MODE_OTHER          3


typedef DWORD LSA_AD_ENUM_TYPE;

#define LSA_AD_ENUM_TYPE_GROUP 1
#define LSA_AD_ENUM_TYPE_USER  2


typedef DWORD (*LSA_AD_MARSHAL_OBJECTS_INFOLIST_CALLBACK)(
    IN OPTIONAL HANDLE hProvider,
    IN HANDLE hDirectory,    
    IN OPTIONAL PCSTR pszDomainDnsName,
    IN LSA_AD_MARSHAL_INFO_LIST_MODE InfoListMarshalMode,
    IN LDAPMessage* pMessagePseudo,
    IN DWORD dwUserInfoLevel,
    OUT PVOID** pppUserInfoList,
    OUT PDWORD pdwNumUsers
    );

typedef void (*LSA_FREE_OBJECTS_INFOLIST_CALLBACK)(
    DWORD  dwLevel,
    PVOID* pGroupInfoList,
    DWORD  dwNumGroups
    );

DWORD
ADMarshalObjectSidListFromPseudo(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessagePseudo,
    OUT PSTR** pppSidsList,
    OUT PDWORD pdwNumObjectsFound
    );

DWORD
DefaultModeSchemaOrUnprovisionEnumObjects(
    // Enumerate either Groups or Users
    IN LSA_AD_ENUM_TYPE EnumType,
    // AdMode can be DEFAULT_MODE (schema only) or UNPROVISIONED_MODE
    IN DWORD AdMode,
    IN LSA_AD_MARSHAL_OBJECTS_INFOLIST_CALLBACK pMarshalObjectsInfoListCallback,
    IN LSA_FREE_OBJECTS_INFOLIST_CALLBACK pFreeObjectsInfoListCallback,
    IN PCSTR pszDomainDnsName,
    IN PAD_ENUM_STATE pEnumState,
    IN DWORD dwMaxNumObjects,
    OUT PDWORD pdwNumObjectsFound,
    OUT PVOID** pppObjectsInfoList
    );

DWORD
CellModeOrDefaultNonSchemaEnumObjects(
    IN HANDLE hProvider,    
    IN ADConfigurationMode adConfMode,
    // Enumerate either Groups or Users
    IN LSA_AD_ENUM_TYPE EnumType,    
    IN LSA_AD_MARSHAL_OBJECTS_INFOLIST_CALLBACK pMarshalObjectsInfoListCallback,
    IN LSA_FREE_OBJECTS_INFOLIST_CALLBACK pFreeObjectsInfoListCallback,
    IN PCSTR pszCellDN,    
    IN PAD_ENUM_STATE pEnumState,
    IN DWORD dwMaxNumObjects,
    OUT PDWORD pdwNumObjectsFound,
    OUT PVOID** pppObjectsInfoList
    );


#endif /*AD_ENUM_OBJECTS_H_*/
