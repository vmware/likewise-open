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
 *        provider-main.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Active Directory Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#ifndef CELLLDAP_H_
#define CELLLDAP_H_

DWORD
CellModeInitializeMiniProvider(
    VOID
    );

DWORD
CellModeFindUserByName(
    PCSTR                pszPseudoDomainName,
    PCSTR                pszPseudoCellDN,
    PCSTR                pszRealDomainName,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PAD_SECURITY_OBJECT *ppUserInfo
    );

DWORD
CellModeGetUserGroupMembership(
    HANDLE  hDirectory,
    PCSTR   pszCellDN,
    PCSTR   pszNetBIOSDomainName,
    DWORD   dwUID,    
    int     *piPrimaryGroupIndex,
    PDWORD  pdwGroupsFound,
    PAD_SECURITY_OBJECT** pppGroupInfoList
    );
    
DWORD
CellModeSchemaGetUserGroupMembership(
    HANDLE  hDirectory,
    PCSTR   pszCellDN,
    PCSTR   pszNetBIOSDomainName,
    DWORD   dwUID,    
    int     *piPrimaryGroupIndex,
    PDWORD  pdwGroupsFound,
    PAD_SECURITY_OBJECT** pppGroupInfoList
    );
    
DWORD
CellModeNonSchemaGetUserGroupMembership(
    HANDLE  hDirectory,
    PCSTR   pszCellDN,
    PCSTR   pszNetBIOSDomainName,
    DWORD   dwUID,   
    int     *piPrimaryGroupIndex,
    PDWORD  pdwGroupsFound,
    PAD_SECURITY_OBJECT** pppGroupInfoList
    );

DWORD
CellModeFindGroupByName(
    PCSTR                pszPseudoDomainName,
    PCSTR                pszPseudoCellDN,
    PCSTR                pszRealDomainName,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PAD_SECURITY_OBJECT *ppGroupInfo
    );

DWORD
CellModeEnumUsers(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumUsers,
    PDWORD         pdwUsersFound,
    PVOID**        pppUserInfoList
    );

DWORD
CellModeSchemaEnumUsers(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumUsers,
    PDWORD         pdwUsersFound,
    PVOID**        pppUserInfoList
    );

DWORD
CellModeNonSchemaEnumUsers(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumUsers,
    PDWORD         pdwUsersFound,
    PVOID**        pppUserInfoList
    );

DWORD
CellModeEnumGroups(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumGroups,
    PDWORD         pdwNumGroupsFound,
    PVOID**        pppGroupInfoList
    );

DWORD
CellModeSchemaEnumGroups(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumGroups,
    PDWORD         pdwNumGroupsFound,
    PVOID**        pppGroupInfoList
    );

DWORD
CellModeNonSchemaEnumGroups(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumGroups,
    PDWORD         pdwNumGroupsFound,
    PVOID**        pppGroupInfoList
    );


DWORD
CellModeEnumNSSArtefacts(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    DWORD          dwMapType,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumNSSArtefacts,
    PDWORD         pdwNumNSSArtefactsFound,
    PVOID**        pppNSSArtefactInfoList
    );

DWORD
CellModeSchemaEnumNSSArtefacts(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    DWORD          dwMapType,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumNSSArtefacts,
    PDWORD         pdwNumNSSArtefactsFound,
    PVOID**        pppNSSArtefactInfoList
    );

DWORD
CellModeNonSchemaEnumNSSArtefacts(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    DWORD          dwMapType,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumNSSArtefacts,
    PDWORD         pdwNumNSSArtefactsFound,
    PVOID**        pppNSSArtefactInfoList
    );

#endif /*CELLLDAP_H_*/
