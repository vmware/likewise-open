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

#ifndef __MARSHAL_GROUP_P_H__
#define __MARSHAL_GROUP_P_H__

DWORD
LsaMarshalGroupInfoList(
    PVOID* ppGroupInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumGroups,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaComputeGroupBufferSize(
    PVOID* ppGroupInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumGroups,
    PDWORD pdwBufLen
    );

DWORD
LsaComputeBufferSize_Group0(
    PVOID* ppGroupInfoList,
    DWORD  dwNumGroups
    );

DWORD
LsaComputeBufferSize_Group1(
    PVOID* ppGroupInfoList,
    DWORD  dwNumGroups
    );

DWORD
LsaMarshalGroup_0_InfoList(
    PVOID* ppGroupInfoList,
    DWORD  dwNumGroups,
    DWORD  dwBeginOffset,
    PSTR   pszBuffer,
    PDWORD pdwDataBytesWritten
    );

DWORD
LsaMarshalGroup_0(
    PLSA_GROUP_INFO_0 pGroupInfo,
    PSTR   pszHeaderBuffer,
    PSTR   pszDataBuffer,
    DWORD  dwDataBeginOffset,
    PDWORD pdwDataBytesWritten
    );

DWORD
LsaMarshalGroup_1_InfoList(
    PVOID* ppGroupInfoList,
    DWORD  dwNumGroups,
    DWORD  dwBeginOffset,
    PSTR   pszBuffer,
    PDWORD pdwDataBytesWritten
    );

DWORD
LsaMarshalGroup_1(
    PLSA_GROUP_INFO_1 pGroupInfo,
    PSTR   pszHeaderBuffer,
    PSTR   pszDataBuffer,
    DWORD  dwDataBeginOffset,
    PDWORD pdwDataBytesWritten
    );

DWORD
LsaGetGroupMemberBufferLength(
    PSTR* ppszGroupMembers
    );

DWORD
LsaUnmarshalGroupInfoList(
    PCSTR   pszMsgBuf,
    DWORD   dwMsgLen,
    PDWORD  pdwInfoLevel,
    PVOID** pppGroupInfoList,
    PDWORD  pdwNumGroups
    );

DWORD
LsaUnmarshalGroup_0_InfoList(
    PCSTR pszMsgBuf,
    PCSTR pszHdrBuf,
    PVOID** pppGroupInfoList,
    DWORD dwNumGroups
    );

DWORD
LsaUnmarshalGroup_0(
    PCSTR pszMsgBuf,
    PLSA_GROUP_0_RECORD_HEADER pHeader,
    PLSA_GROUP_INFO_0* ppGroupInfo
    );

DWORD
LsaUnmarshalGroup_1_InfoList(
    PCSTR pszMsgBuf,
    PCSTR pszHdrBuf,
    PVOID** pppGroupInfoList,
    DWORD dwNumGroups
    );

DWORD
LsaUnmarshalGroup_1(
    PCSTR pszMsgBuf,
    PLSA_GROUP_1_RECORD_HEADER pHeader,
    PLSA_GROUP_INFO_1* ppGroupInfo
    );

DWORD
LsaUnmarshalGroupMembers(
    PCSTR  pszGroupMembers,
    PSTR** pppszMembers
    );

DWORD
LsaFindNumberOfMembers(
    PCSTR  pszGroupMembers
    );

DWORD
LsaComputeBufferSize_FindGroupByNameQuery(
    PCSTR pszGroupName,
    DWORD dwInfoLevel
    );

DWORD
LsaComputeBufferSize_FindGroupByIdQuery(
    gid_t gid,
    DWORD dwInfoLevel
    );

DWORD
LsaMarshalDeleteGroupByIdQuery(
    gid_t  gid,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalDeleteGroupByIdQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    gid_t* pGid
    );

DWORD
LsaComputeBufferSize_GetGroupsForUserQuery(
    uid_t uid,
    DWORD dwInfoLevel
    );

#endif /* __MARSHAL_GROUP_P_H__ */

