/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *
 */
#include "ipc.h"

extern LWMsgTypeSpec gLsaGroupInfo0Spec[];
extern LWMsgTypeSpec gLsaGroupInfo1Spec[];

LWMsgTypeSpec gLsaLocalIPCGetGroupMembershipReq[] =
{
    LWMSG_STRUCT_BEGIN(LSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REQ),
    LWMSG_MEMBER_PSTR(LSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REQ, pszSID),
    LWMSG_MEMBER_PSTR(LSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REQ, pszDN),
    LWMSG_MEMBER_UINT32(LSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REQ, dwGroupInfoLevel),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


#define GROUP_INFO_LEVEL_0 0
#define GROUP_INFO_LEVEL_1 1

LWMsgTypeSpec gLsaLocalIPCGetGroupMembershipRep[] =
{
    LWMSG_STRUCT_BEGIN(LSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REP),
    LWMSG_MEMBER_UINT32(LSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REP, dwNumGroups),
    LWMSG_MEMBER_UINT32(LSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REP, dwGroupInfoLevel),
    LWMSG_MEMBER_UNION_BEGIN(LSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REP, Groups),
    LWMSG_MEMBER_POINTER_BEGIN(union _member_info_list, ppInfo0),
    LWMSG_POINTER_BEGIN,
    LWMSG_TYPESPEC(gLsaGroupInfo0Spec),
    LWMSG_POINTER_END,
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(LSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REP, dwNumGroups),
    LWMSG_ATTR_TAG(GROUP_INFO_LEVEL_0),
    LWMSG_MEMBER_POINTER_BEGIN(union _member_info_list, ppInfo1),
    LWMSG_POINTER_BEGIN,
    LWMSG_TYPESPEC(gLsaGroupInfo1Spec),
    LWMSG_POINTER_END,
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(LSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REP, dwNumGroups),
    LWMSG_ATTR_TAG(GROUP_INFO_LEVEL_1),
    LWMSG_UNION_END,
    LWMSG_ATTR_DISCRIM(LSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REP, dwGroupInfoLevel),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


LWMsgTypeSpec*
LsaLocalIPCGetGroupMembershipReqSpec(
    void
    )
{
    return gLsaLocalIPCGetGroupMembershipReq;
}


LWMsgTypeSpec*
LsaLocalIPCGetGroupMembershipRepSpec(
    void
    )
{
    return gLsaLocalIPCGetGroupMembershipRep;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
