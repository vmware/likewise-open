/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        ipc-protocol.c
 *
 * Abstract:
 *
 *        IPC protocol definitions
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

#if defined(WORDS_BIGENDIAN)
#  define UCS2_NATIVE "UCS-2BE"
#else
#  define UCS2_NATIVE "UCS-2LE"
#endif

#define LWMSG_MEMBER_PWSTR(_type, _field)           \
    LWMSG_MEMBER_POINTER_BEGIN(_type, _field),      \
    LWMSG_UINT16(WCHAR),                            \
    LWMSG_POINTER_END,                              \
    LWMSG_ATTR_ZERO_TERMINATED,                     \
    LWMSG_ATTR_ENCODING(UCS2_NATIVE)

#define LWMSG_PWSTR                    \
    LWMSG_POINTER_BEGIN,               \
    LWMSG_UINT16(WCHAR),               \
    LWMSG_POINTER_END,                 \
    LWMSG_ATTR_ZERO_TERMINATED,        \
    LWMSG_ATTR_ENCODING(UCS2_NATIVE)

static LWMsgTypeSpec gStringSpec[] =
{
    LWMSG_PWSTR,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gStringListSpec[] =
{
    LWMSG_POINTER_BEGIN,
    LWMSG_PWSTR,
    LWMSG_POINTER_END,
    LWMSG_ATTR_ZERO_TERMINATED,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gExistingHandleSpec[] =
{
    LWMSG_HANDLE(LW_SERVICE_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,
    LWMSG_TYPE_END
};


static LWMsgTypeSpec gNewHandleSpec[] =
{
    LWMSG_HANDLE(LW_SERVICE_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gErrorSpec[] =
{
    LWMSG_UINT32(DWORD),
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gStatusSpec[] =
{
    LWMSG_UINT8(LW_SERVICE_STATUS),
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gServiceInfoSpec[] =
{
    LWMSG_STRUCT_BEGIN(LW_SERVICE_INFO),
    LWMSG_MEMBER_PWSTR(LW_SERVICE_INFO, pwszName),
    LWMSG_MEMBER_PWSTR(LW_SERVICE_INFO, pwszDescription),
    LWMSG_MEMBER_UINT8(LW_SERVICE_INFO, type),
    LWMSG_ATTR_RANGE(LW_SERVICE_EXECUTABLE, LW_SERVICE_DRIVER),
    LWMSG_MEMBER_PWSTR(LW_SERVICE_INFO, pwszPath),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_TYPESPEC(LW_SERVICE_INFO, ppwszArgs, gStringListSpec),
    LWMSG_MEMBER_TYPESPEC(LW_SERVICE_INFO, ppwszDependencies, gStringListSpec),
    LWMSG_MEMBER_UINT8(LW_SERVICE_INFO, bStartupService),
    LWMSG_ATTR_RANGE(0, 1),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgProtocolSpec gIpcSpec[] =
{
    LWMSG_MESSAGE(SM_IPC_ERROR, gErrorSpec),
    LWMSG_MESSAGE(SM_IPC_ACQUIRE_SERVICE_HANDLE_REQ, gStringSpec),
    LWMSG_MESSAGE(SM_IPC_ACQUIRE_SERVICE_HANDLE_RES, gNewHandleSpec),
    LWMSG_MESSAGE(SM_IPC_RELEASE_SERVICE_HANDLE_REQ, gExistingHandleSpec),
    LWMSG_MESSAGE(SM_IPC_RELEASE_SERVICE_HANDLE_RES, NULL),
    LWMSG_MESSAGE(SM_IPC_ENUMERATE_SERVICES_REQ, NULL),
    LWMSG_MESSAGE(SM_IPC_ENUMERATE_SERVICES_RES, gStringListSpec),
    LWMSG_MESSAGE(SM_IPC_ADD_SERVICE_REQ, gServiceInfoSpec),
    LWMSG_MESSAGE(SM_IPC_ADD_SERVICE_RES, NULL),
    LWMSG_MESSAGE(SM_IPC_REMOVE_SERVICE_REQ, gStringSpec),
    LWMSG_MESSAGE(SM_IPC_REMOVE_SERVICE_RES, NULL),
    LWMSG_MESSAGE(SM_IPC_START_SERVICE_REQ, gExistingHandleSpec),
    LWMSG_MESSAGE(SM_IPC_START_SERVICE_RES, NULL),
    LWMSG_MESSAGE(SM_IPC_STOP_SERVICE_REQ, gExistingHandleSpec),
    LWMSG_MESSAGE(SM_IPC_STOP_SERVICE_RES, NULL),
    LWMSG_MESSAGE(SM_IPC_REFRESH_SERVICE_REQ, gExistingHandleSpec),
    LWMSG_MESSAGE(SM_IPC_REFRESH_SERVICE_RES, NULL),
    LWMSG_MESSAGE(SM_IPC_GET_SERVICE_STATUS_REQ, gExistingHandleSpec),
    LWMSG_MESSAGE(SM_IPC_GET_SERVICE_STATUS_RES, gStatusSpec),
    LWMSG_MESSAGE(SM_IPC_GET_SERVICE_INFO_REQ, gExistingHandleSpec),
    LWMSG_MESSAGE(SM_IPC_GET_SERVICE_INFO_RES, gServiceInfoSpec),
    LWMSG_PROTOCOL_END,
};

LWMsgProtocolSpec*
LwSmIpcGetProtocolSpec(
    VOID
    )
{
    return gIpcSpec;
}
