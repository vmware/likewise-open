/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        ipc_protocol.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process communication API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "ipc.h"

extern LWMsgTypeSpec gLsaUserInfoListSpec[];
extern LWMsgTypeSpec gLsaGroupInfoListSpec[];

LWMsgTypeSpec gLsaAdIPCEnumUsersFromCacheReqSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ),
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ, pszResume),
    LWMSG_MEMBER_UINT32(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ, dwInfoLevel),
    LWMSG_MEMBER_UINT32(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ, dwMaxNumUsers),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec gLsaAdIPCEnumUsersFromCacheRespSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP),
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP, pszResume),
    LWMSG_MEMBER_POINTER_BEGIN(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP, pUserInfoList),
    LWMSG_TYPESPEC(gLsaUserInfoListSpec),
    LWMSG_POINTER_END,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec gLsaAdIPCEnumGroupsFromCacheReqSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ),
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ, pszResume),
    LWMSG_MEMBER_UINT32(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ, dwInfoLevel),
    LWMSG_MEMBER_UINT32(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ, dwMaxNumGroups),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec gLsaAdIPCEnumGroupsFromCacheRespSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP),
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP, pszResume),
    LWMSG_MEMBER_POINTER_BEGIN(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP, pGroupInfoList),
    LWMSG_TYPESPEC(gLsaGroupInfoListSpec),
    LWMSG_POINTER_END,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec*
LsaAdIPCGetEnumUsersFromCacheReqSpec(
    void
    )
{
    return gLsaAdIPCEnumUsersFromCacheReqSpec;
}

LWMsgTypeSpec*
LsaAdIPCGetEnumUsersFromCacheRespSpec(
    void
    )
{
    return gLsaAdIPCEnumUsersFromCacheRespSpec;
}

LWMsgTypeSpec*
LsaAdIPCGetEnumGroupsFromCacheReqSpec(
    void
    )
{
    return gLsaAdIPCEnumGroupsFromCacheReqSpec;
}

LWMsgTypeSpec*
LsaAdIPCGetEnumGroupsFromCacheRespSpec(
    void
    )
{
    return gLsaAdIPCEnumGroupsFromCacheRespSpec;
}

