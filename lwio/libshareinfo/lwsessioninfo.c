/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lwsessioninfo.c
 *
 * Abstract:
 *
 *        Likewise I/O subsystem (LWIO)
 *
 *        Session Information IPC
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static LWMsgTypeSpec gSessionInfo0Spec[] =
{
    LWMSG_STRUCT_BEGIN(SESSION_INFO_0),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_0, sesi0_cname),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSessionInfo1Spec[] =
{
    LWMSG_STRUCT_BEGIN(SESSION_INFO_1),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_1,  sesi1_cname),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_1,  sesi1_username),
    LWMSG_MEMBER_UINT32(SESSION_INFO_1, sesi1_num_opens),
    LWMSG_MEMBER_UINT32(SESSION_INFO_1, sesi1_time),
    LWMSG_MEMBER_UINT32(SESSION_INFO_1, sesi1_idle_time),
    LWMSG_MEMBER_UINT32(SESSION_INFO_1, sesi1_user_flags),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSessionInfo2Spec[] =
{
    LWMSG_STRUCT_BEGIN(SESSION_INFO_2),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_2,  sesi2_cname),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_2,  sesi2_username),
    LWMSG_MEMBER_UINT32(SESSION_INFO_2, sesi2_num_opens),
    LWMSG_MEMBER_UINT32(SESSION_INFO_2, sesi2_time),
    LWMSG_MEMBER_UINT32(SESSION_INFO_2, sesi2_idle_time),
    LWMSG_MEMBER_UINT32(SESSION_INFO_2, sesi2_user_flags),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_2,  sesi2_cltype_name),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSessionInfo10Spec[] =
{
    LWMSG_STRUCT_BEGIN(SESSION_INFO_10),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_10,  sesi10_cname),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_10,  sesi10_username),
    LWMSG_MEMBER_UINT32(SESSION_INFO_10, sesi10_time),
    LWMSG_MEMBER_UINT32(SESSION_INFO_10, sesi10_idle_time),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSessionInfo502Spec[] =
{
    LWMSG_STRUCT_BEGIN(SESSION_INFO_502),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_502,  sesi502_cname),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_502,  sesi502_username),
    LWMSG_MEMBER_UINT32(SESSION_INFO_502, sesi502_num_opens),
    LWMSG_MEMBER_UINT32(SESSION_INFO_502, sesi502_time),
    LWMSG_MEMBER_UINT32(SESSION_INFO_502, sesi502_idle_time),
    LWMSG_MEMBER_UINT32(SESSION_INFO_502, sesi502_user_flags),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_502,  sesi502_cltype_name),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_502,  sesi502_transport),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

#define SESSION_INFO_LEVEL_0     0
#define SESSION_INFO_LEVEL_1     1
#define SESSION_INFO_LEVEL_2     2
#define SESSION_INFO_LEVEL_10   10
#define SESSION_INFO_LEVEL_502 502

static LWMsgTypeSpec gSessionInfoUnionSpec[] =
{
    LWMSG_UNION_BEGIN(SESSION_INFO_UNION),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p0, LWMSG_TYPESPEC(gSessionInfo0Spec)),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_0),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p1, LWMSG_TYPESPEC(gSessionInfo1Spec)),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_1),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p2, LWMSG_TYPESPEC(gSessionInfo2Spec)),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_2),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p10, LWMSG_TYPESPEC(gSessionInfo10Spec)),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_10),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p502, LWMSG_TYPESPEC(gSessionInfo502Spec)),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_502),
    LWMSG_UNION_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSessionInfoEnumParamsSpec[] =
{
    LWMSG_STRUCT_BEGIN(SESSION_INFO_ENUM_PARAMS),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_ENUM_PARAMS,  pwszServername),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_ENUM_PARAMS,  pwszUncClientname),
    LWMSG_MEMBER_UINT32(SESSION_INFO_ENUM_PARAMS, dwInfoLevel),
    LWMSG_MEMBER_UINT32(SESSION_INFO_ENUM_PARAMS, dwPreferredMaxLength),
    LWMSG_MEMBER_UINT32(SESSION_INFO_ENUM_PARAMS, dwEntriesRead),
    LWMSG_MEMBER_UINT32(SESSION_INFO_ENUM_PARAMS, dwTotalEntries),
    LWMSG_MEMBER_POINTER_BEGIN(SESSION_INFO_ENUM_PARAMS, pdwResumeHandle),
    LWMSG_UINT8(UINT32),
    LWMSG_POINTER_END,
    LWMSG_MEMBER_UNION_BEGIN(SESSION_INFO_ENUM_PARAMS, info),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p0, LWMSG_TYPESPEC(gSessionInfo0Spec)),
    LWMSG_ATTR_LENGTH_MEMBER(SESSION_INFO_ENUM_PARAMS, dwEntriesRead),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_0),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p1, LWMSG_TYPESPEC(gSessionInfo1Spec)),
    LWMSG_ATTR_LENGTH_MEMBER(SESSION_INFO_ENUM_PARAMS, dwEntriesRead),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_1),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p2, LWMSG_TYPESPEC(gSessionInfo2Spec)),
    LWMSG_ATTR_LENGTH_MEMBER(SESSION_INFO_ENUM_PARAMS, dwEntriesRead),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_2),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p10, LWMSG_TYPESPEC(gSessionInfo10Spec)),
    LWMSG_ATTR_LENGTH_MEMBER(SESSION_INFO_ENUM_PARAMS, dwEntriesRead),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_10),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p502, LWMSG_TYPESPEC(gSessionInfo502Spec)),
    LWMSG_ATTR_LENGTH_MEMBER(SESSION_INFO_ENUM_PARAMS, dwEntriesRead),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_502),
    LWMSG_UNION_END,
    LWMSG_ATTR_DISCRIM(SESSION_INFO_ENUM_PARAMS, dwInfoLevel),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSessionInfoDeleteParamsSpec[] =
{
    LWMSG_STRUCT_BEGIN(SESSION_INFO_DELETE_PARAMS),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_DELETE_PARAMS, pwszServername),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_DELETE_PARAMS, pwszUncClientname),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_DELETE_PARAMS, pwszUncUsername),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LW_NTSTATUS
LwSessionInfoMarshalEnumParameters(
    PSESSION_INFO_ENUM_PARAMS pParams,
    PBYTE*                    ppBuffer,
    ULONG*                    pulBufferSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    VOID* pBuffer = NULL;
    size_t ulBufferSize = 0;
    LWMsgDataContext* pDataContext = NULL;

    status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(status);

    status = MAP_LWMSG_STATUS(
        lwmsg_data_marshal_flat_alloc(
            pDataContext,
            gSessionInfoEnumParamsSpec,
            pParams,
            &pBuffer,
            &ulBufferSize));
    BAIL_ON_NT_STATUS(status);

    *ppBuffer = pBuffer;
    *pulBufferSize = (ULONG) ulBufferSize;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return status;

error:
    *ppBuffer = NULL;
    *pulBufferSize = 0;

    if (pBuffer)
    {
        RtlMemoryFree(pBuffer);
    }

    goto cleanup;
}


LW_NTSTATUS
LwSessionInfoUnmarshalEnumParameters(
    PBYTE                      pBuffer,
    ULONG                      ulBufferSize,
    PSESSION_INFO_ENUM_PARAMS* ppParams
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSESSION_INFO_ENUM_PARAMS pParams = NULL;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
        lwmsg_data_unmarshal_flat(
            pDataContext,
            gSessionInfoEnumParamsSpec,
            pBuffer,
            ulBufferSize,
            OUT_PPVOID(&pParams)));
    BAIL_ON_NT_STATUS(Status);

    *ppParams = pParams;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return Status;

error:

    *ppParams = NULL;

    if (pParams)
    {
        lwmsg_data_free_graph(pDataContext, gSessionInfoEnumParamsSpec, pParams);
    }

    goto cleanup;
}

LW_NTSTATUS
LwSessionInfoMarshalDeleteParameters(
    PSESSION_INFO_DELETE_PARAMS pParams,
    PBYTE* ppBuffer,
    ULONG* pulBufferSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    VOID* pBuffer = NULL;
    size_t ulBufferSize = 0;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
        lwmsg_data_marshal_flat_alloc(
            pDataContext,
            gSessionInfoDeleteParamsSpec,
            pParams,
            &pBuffer,
            &ulBufferSize));
    BAIL_ON_NT_STATUS(Status);

    *ppBuffer = pBuffer;
    *pulBufferSize = (ULONG) ulBufferSize;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return Status;

error:
    *ppBuffer = NULL;
    *pulBufferSize = 0;

    if (pBuffer)
    {
        RtlMemoryFree(pBuffer);
    }

    goto cleanup;
}


LW_NTSTATUS
LwSessionInfoUnmarshalDeleteParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PSESSION_INFO_DELETE_PARAMS* ppParams
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSESSION_INFO_DELETE_PARAMS pParams = NULL;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
        lwmsg_data_unmarshal_flat(
            pDataContext,
            gSessionInfoDeleteParamsSpec,
            pBuffer,
            ulBufferSize,
            OUT_PPVOID(&pParams)));
    BAIL_ON_NT_STATUS(Status);

    *ppParams = pParams;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return Status;

error:

    *ppParams = NULL;

    if (pParams)
    {
        lwmsg_data_free_graph(pDataContext, gSessionInfoDeleteParamsSpec, pParams);
    }

    goto cleanup;
}


