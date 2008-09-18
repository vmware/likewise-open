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
 *        marshal_user.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Marshal/Unmarshal API for Messages related to Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#ifndef __MARSHAL_USER_P_H__
#define __MARSHAL_USER_P_H__

DWORD
LsaMarshalUserInfoList(
    PVOID* ppUserInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumUsers,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaComputeUserBufferSize(
    PVOID* ppUserInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumUsers,
    PDWORD pdwBufLen
    );

DWORD
LsaComputeBufferSize_User0(
    PVOID* ppUserInfoList,
    DWORD  dwNumUsers
    );

DWORD
LsaComputeBufferSize_User1(
    PVOID* ppUserInfoList,
    DWORD  dwNumUsers
    );

DWORD
LsaComputeBufferSize_User2(
    PVOID* ppUserInfoList,
    DWORD  dwNumUsers
    );

DWORD
LsaMarshalUser_0_InfoList(
    PVOID* ppUserInfoList,
    DWORD  dwNumUsers,
    DWORD  dwBeginOffset,
    PSTR   pszBuffer,
    PDWORD pdwDataBytesWritten
    );

DWORD
LsaMarshalUser_0(
    PLSA_USER_INFO_0 pUserInfo,
    PSTR   pszHeaderBuffer,
    PSTR   pszDataBuffer,
    DWORD  dwDataBeginOffset,
    PDWORD pdwDataBytesWritten
    );

DWORD
LsaMarshalUser_1_InfoList(
    PVOID* ppUserInfoList,
    DWORD  dwNumUsers,
    DWORD  dwBeginOffset,
    PSTR   pszBuffer,
    PDWORD pdwDataBytesWritten
    );

DWORD
LsaMarshalUser_1(
    PLSA_USER_INFO_1 pUserInfo,
    PSTR   pszHeaderBuffer,
    PSTR   pszDataBuffer,
    DWORD  dwDataBeginOffset,
    PDWORD pdwDataBytesWritten
    );

DWORD
LsaMarshalUser_2_InfoList(
    PVOID* ppUserInfoList,
    DWORD  dwNumUsers,
    DWORD  dwBeginOffset,
    PSTR   pszBuffer,
    PDWORD pdwDataBytesWritten
    );

DWORD
LsaMarshalUser_2(
    PLSA_USER_INFO_2 pUserInfo,
    PSTR   pszHeaderBuffer,
    PSTR   pszDataBuffer,
    DWORD  dwDataBeginOffset,
    PDWORD pdwDataBytesWritten
    );

DWORD
LsaUnmarshalUserInfoList(
    PCSTR   pszMsgBuf,
    DWORD   dwMsgLen,
    PDWORD  pdwInfoLevel,
    PVOID** pppUserInfoList,
    PDWORD  pdwNumUsers
    );

DWORD
LsaUnmarshalUser_0_InfoList(
    PCSTR   pszMsgBuf,
    PCSTR   pszHdrBuf,
    DWORD   dwNumUsers,
    PVOID** pppUserInfoList
    );

DWORD
LsaUnmarshalUserInPlace_0(
    PCSTR                     pszMsgBuf,
    PLSA_USER_INFO_0          pUserInfo,
    PLSA_USER_0_RECORD_HEADER pUserInfoHeader
    );

DWORD
LsaUnmarshalUser_1_InfoList(
    PCSTR   pszMsgBuf,
    PCSTR   pszHdrBuf,
    DWORD   dwNumUsers,
    PVOID** pppUserInfoList
    );

DWORD
LsaUnmarshalUserInPlace_1(
    PCSTR                     pszMsgBuf,
    PLSA_USER_1_RECORD_HEADER pUserInfoHeader,
    PLSA_USER_INFO_1          pUserInfo
    );

DWORD
LsaUnmarshalUser_2_InfoList(
    PCSTR   pszMsgBuf,
    PCSTR   pszHdrBuf,
    DWORD   dwNumUsers,
    PVOID** pppUserInfoList
    );

DWORD
LsaUnmarshalUserInPlace_2(
    PCSTR                     pszMsgBuf,
    PLSA_USER_2_RECORD_HEADER pUserInfoHeader,
    PLSA_USER_INFO_2          pUserInfo
    );

DWORD
LsaMarshalFindUserByNameQuery(
    PCSTR  pszLoginId,
    DWORD  dwInfoLevel,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaComputeBufferSize_FindUserByNameQuery(
    PCSTR pszLoginId,
    DWORD dwInfoLevel
    );

DWORD
LsaUnmarshalFindUserByNameQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PSTR*  ppszLoginId,
    PDWORD pdwInfoLevel
    );

DWORD
LsaMarshalFindUserByIdQuery(
    uid_t  uid,
    DWORD  dwInfoLevel,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaComputeBufferSize_FindUserByIdQuery(
    uid_t uid,
    DWORD dwInfoLevel
    );

DWORD
LsaUnmarshalFindUserByIdQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    uid_t* pUid,
    PDWORD pdwInfoLevel
    );

DWORD
LsaMarshalDeleteUserByIdQuery(
    uid_t  uid,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalDeleteUserByIdQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    uid_t* pUid
    );

DWORD
LsaMarshalUserModInfo(
    PLSA_USER_MOD_INFO pUserModInfo,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaComputeBufferSize_UserModInfo(
    PLSA_USER_MOD_INFO pUserModInfo
    );

DWORD
LsaUnmarshalUserModInfo(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PLSA_USER_MOD_INFO* ppUserModInfo
    );

#endif /* __MARSHAL_USER_P_H__ */
