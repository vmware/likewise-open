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
 *        lwfileinfo.c
 *
 * Abstract:
 *
 *        Likewise I/O Subsystem (LWIO)
 *
 *        File Information IPC
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static LWMsgTypeSpec gFileInfo2Spec[] =
{
    LWMSG_STRUCT_BEGIN(FILE_INFO_2),
    LWMSG_MEMBER_UINT32(FILE_INFO_2, fi2_id),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gFileInfo3Spec[] =
{
    LWMSG_STRUCT_BEGIN(FILE_INFO_3),
    LWMSG_MEMBER_UINT32(FILE_INFO_3, fi3_idd),
    LWMSG_MEMBER_UINT32(FILE_INFO_3, fi3_permissions),
    LWMSG_MEMBER_UINT32(FILE_INFO_3, fi3_num_locks),
    LWMSG_MEMBER_PWSTR(FILE_INFO_3,  fi3_path_name),
    LWMSG_MEMBER_PWSTR(FILE_INFO_3,  fi3_username),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

#define FILE_INFO_LEVEL_2   2
#define FILE_INFO_LEVEL_3   3

static LWMsgTypeSpec gFileInfoUnionSpec[] =
{
    LWMSG_UNION_BEGIN(FILE_INFO_UNION),
    LWMSG_MEMBER_POINTER(FILE_INFO_UNION, p2, LWMSG_TYPESPEC(gFileInfo2Spec)),
    LWMSG_ATTR_TAG(FILE_INFO_LEVEL_2),
    LWMSG_MEMBER_POINTER(FILE_INFO_UNION, p3, LWMSG_TYPESPEC(gFileInfo3Spec)),
    LWMSG_ATTR_TAG(FILE_INFO_LEVEL_3),
    LWMSG_UNION_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gFileInfoEnumParamsSpec[] =
{
    LWMSG_STRUCT_BEGIN(FILE_INFO_ENUM_PARAMS),
    LWMSG_MEMBER_PWSTR(FILE_INFO_ENUM_PARAMS, pwszBasepath),
    LWMSG_MEMBER_PWSTR(FILE_INFO_ENUM_PARAMS, pwszUsername),
    LWMSG_MEMBER_UINT32(FILE_INFO_ENUM_PARAMS, dwInfoLevel),
    LWMSG_MEMBER_UINT32(FILE_INFO_ENUM_PARAMS, dwNumEntries),
    LWMSG_MEMBER_UINT32(FILE_INFO_ENUM_PARAMS, dwTotalEntries),
    LWMSG_MEMBER_POINTER_BEGIN(FILE_INFO_ENUM_PARAMS, pdwResumeHandle),
    LWMSG_UINT8(UINT32),
    LWMSG_POINTER_END,
    LWMSG_MEMBER_UNION_BEGIN(FILE_INFO_ENUM_PARAMS, info),
    LWMSG_MEMBER_POINTER(FILE_INFO_UNION, p2, LWMSG_TYPESPEC(gFileInfo2Spec)),
    LWMSG_ATTR_LENGTH_MEMBER(FILE_INFO_ENUM_PARAMS, dwNumEntries),
    LWMSG_ATTR_TAG(FILE_INFO_LEVEL_2),
    LWMSG_MEMBER_POINTER(FILE_INFO_UNION, p3, LWMSG_TYPESPEC(gFileInfo3Spec)),
    LWMSG_ATTR_LENGTH_MEMBER(FILE_INFO_ENUM_PARAMS, dwNumEntries),
    LWMSG_ATTR_TAG(FILE_INFO_LEVEL_3),
    LWMSG_UNION_END,
    LWMSG_ATTR_DISCRIM(FILE_INFO_ENUM_PARAMS, dwInfoLevel),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gFileInfoGetParamsSpec[] =
{
    LWMSG_STRUCT_BEGIN(FILE_INFO_GET_INFO_PARAMS),
    LWMSG_MEMBER_UINT32(FILE_INFO_GET_INFO_PARAMS, dwInfoLevel),
    LWMSG_MEMBER_UINT32(FILE_INFO_GET_INFO_PARAMS, dwFileId),
    LWMSG_MEMBER_UNION_BEGIN(FILE_INFO_GET_INFO_PARAMS, info),
    LWMSG_MEMBER_POINTER(FILE_INFO_UNION, p2, LWMSG_TYPESPEC(gFileInfo2Spec)),
    LWMSG_ATTR_TAG(FILE_INFO_LEVEL_2),
    LWMSG_MEMBER_POINTER(FILE_INFO_UNION, p3, LWMSG_TYPESPEC(gFileInfo3Spec)),
    LWMSG_ATTR_TAG(FILE_INFO_LEVEL_3),
    LWMSG_UNION_END,
    LWMSG_ATTR_DISCRIM(FILE_INFO_GET_INFO_PARAMS, dwInfoLevel),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gFileInfoCloseParamsSpec[] =
{
    LWMSG_STRUCT_BEGIN(FILE_INFO_CLOSE_PARAMS),
    LWMSG_MEMBER_UINT32(FILE_INFO_CLOSE_PARAMS, dwFileId),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LW_NTSTATUS
LwFileInfoMarshalEnumParameters(
    PFILE_INFO_ENUM_PARAMS pParams,
    PBYTE* ppBuffer,
    ULONG* pulBufferSize
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
            gFileInfoEnumParamsSpec,
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
LwFileInfoUnmarshalEnumParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PFILE_INFO_ENUM_PARAMS* ppParams
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_INFO_ENUM_PARAMS pParams = NULL;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
        lwmsg_data_unmarshal_flat(
            pDataContext,
            gFileInfoEnumParamsSpec,
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
        lwmsg_data_free_graph(pDataContext, gFileInfoEnumParamsSpec, pParams);
    }

    goto cleanup;
}

LW_NTSTATUS
LwFileInfoMarshalGetInfoParameters(
    PFILE_INFO_GET_INFO_PARAMS pParams,
    PBYTE*                     ppBuffer,
    ULONG*                     pulBufferSize
    )
{
    NTSTATUS ntStatus = 0;
    VOID* pBuffer = NULL;
    size_t ulBufferSize = 0;
    LWMsgDataContext* pDataContext = NULL;

    ntStatus = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = MAP_LWMSG_STATUS(
        lwmsg_data_marshal_flat_alloc(
            pDataContext,
            gFileInfoGetParamsSpec,
            pParams,
            &pBuffer,
            &ulBufferSize));
    BAIL_ON_NT_STATUS(ntStatus);

    *ppBuffer = pBuffer;
    *pulBufferSize = (ULONG) ulBufferSize;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return ntStatus;

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
LwFileInfoUnmarshalGetInfoParameters(
    PBYTE                       pBuffer,
    ULONG                       ulBufferSize,
    PFILE_INFO_GET_INFO_PARAMS* ppParams
    )
{
    NTSTATUS ntStatus = 0;
    PFILE_INFO_GET_INFO_PARAMS pParams = NULL;
    LWMsgDataContext* pDataContext = NULL;

    ntStatus = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = MAP_LWMSG_STATUS(
        lwmsg_data_unmarshal_flat(
            pDataContext,
            gFileInfoGetParamsSpec,
            pBuffer,
            ulBufferSize,
            OUT_PPVOID(&pParams)));
    BAIL_ON_NT_STATUS(ntStatus);

    *ppParams = pParams;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return ntStatus;

error:

    *ppParams = NULL;

    if (pParams)
    {
        lwmsg_data_free_graph(pDataContext, gFileInfoGetParamsSpec, pParams);
    }

    goto cleanup;
}

LW_NTSTATUS
LwFileInfoMarshalCloseParameters(
    PFILE_INFO_CLOSE_PARAMS pParams,
    PBYTE*                  ppBuffer,
    ULONG*                  pulBufferSize
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
            gFileInfoCloseParamsSpec,
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
LwFileInfoUnmarshalCloseParameters(
    PBYTE                    pBuffer,
    ULONG                    ulBufferSize,
    PFILE_INFO_CLOSE_PARAMS* ppParams
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_INFO_CLOSE_PARAMS pParams = NULL;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
        lwmsg_data_unmarshal_flat(
            pDataContext,
            gFileInfoCloseParamsSpec,
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
        lwmsg_data_free_graph(pDataContext, gFileInfoCloseParamsSpec, pParams);
    }

    goto cleanup;
}

