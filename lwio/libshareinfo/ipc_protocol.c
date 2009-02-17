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
 *        ipc_protocol.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *
 */

#include "includes.h"


typedef union _SHARE_INFO_UNION
{
    PSHARE_INFO_0   p0;
    PSHARE_INFO_1   p1;
    PSHARE_INFO_501 p501;
    PSHARE_INFO_502 p502;
} SHARE_INFO_UNION, *PSHARE_INFO_UNION;


typedef struct _SHARE_INFO_ADD_PARAMS
{
    DWORD dwInfoLevel;
    SHARE_INFO_UNION info;
} SHARE_INFO_ADD_PARAMS, *PSHARE_INFO_ADD_PARAMS;

typedef struct _SHARE_INFO_DELETE_PARAMS
{
    PWSTR servername;
    PWSTR netname;
    DWORD reserved;
}SHARE_INFO_DELETE_PARAMS, *PSHARE_INFO_DELETE_PARAMS;

typedef struct _SHARE_INFO_SETINFO_PARAMS
{
    PWSTR servername;
    PWSTR netname;
    DWORD level;
    SHARE_INFO_UNION info;
}SHARE_INFO_SETINFO_PARAMS, *PSHARE_INFO_SETINFO_PARAMS;

static LWMsgTypeSpec gShareInfo0Spec[] =
{
    LWMSG_STRUCT_BEGIN(SHARE_INFO_502),
    LWMSG_MEMBER_PWSTR(SHARE_INFO_502, shi502_netname),
    LWMSG_MEMBER_UINT32(SHARE_INFO_502, shi502_type),
    LWMSG_MEMBER_PWSTR(SHARE_INFO_502, shi502_remark),
    LWMSG_MEMBER_UINT32(SHARE_INFO_502, shi502_permissions),
    LWMSG_MEMBER_UINT32(SHARE_INFO_502, shi502_max_uses),
    LWMSG_MEMBER_UINT32(SHARE_INFO_502, shi502_current_uses),
    LWMSG_MEMBER_PWSTR(SHARE_INFO_502, shi502_path),
    LWMSG_MEMBER_PWSTR(SHARE_INFO_502, shi502_password),
    LWMSG_MEMBER_UINT32(SHARE_INFO_502, shi502_reserved),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


static LWMsgTypeSpec gShareInfo502Spec[] =
{
    LWMSG_STRUCT_BEGIN(SHARE_INFO_502),
    LWMSG_MEMBER_PWSTR(SHARE_INFO_502, shi502_netname),
    LWMSG_MEMBER_UINT32(SHARE_INFO_502, shi502_type),
    LWMSG_MEMBER_PWSTR(SHARE_INFO_502, shi502_remark),
    LWMSG_MEMBER_UINT32(SHARE_INFO_502, shi502_permissions),
    LWMSG_MEMBER_UINT32(SHARE_INFO_502, shi502_max_uses),
    LWMSG_MEMBER_UINT32(SHARE_INFO_502, shi502_current_uses),
    LWMSG_MEMBER_PWSTR(SHARE_INFO_502, shi502_path),
    LWMSG_MEMBER_PWSTR(SHARE_INFO_502, shi502_password),
    LWMSG_MEMBER_UINT32(SHARE_INFO_502, shi502_reserved),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


static LWMsgTypeSpec gShareInfoUnionSpec[] =
{
    LWMSG_UNION_BEGIN(SHARE_INFO_UNION),
    LWMSG_MEMBER_POINTER(SHARE_INFO_UNION, p502, LWMSG_TYPESPEC(gShareInfo502Spec)),
    LWMSG_ATTR_TAG(502),
    LWMSG_UNION_END,
    LWMSG_TYPE_END
};


static LWMsgTypeSpec gShareInfoAddParamsSpec[] =
{
    LWMSG_STRUCT_BEGIN(SHARE_INFO_ADD_PARAMS),
    LWMSG_MEMBER_UINT32(SHARE_INFO_ADD_PARAMS, dwInfoLevel),
    //    LWMSG_MEMBER_TYPESPEC(SHARE_INFO_ADD_PARAMS, gShareInfoUnionSpec),
    LWMSG_ATTR_DISCRIM(SHARE_INFO_ADD_PARAMS, dwInfoLevel),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gShareInfoDeleteParamsSpec[] =
{
    LWMSG_STRUCT_BEGIN(SHARE_INFO_DELETE_PARAMS),
    LWMSG_MEMBER_PWSTR(SHARE_INFO_DELETE_PARAMS, servername),
    LWMSG_MEMBER_PWSTR(SHARE_INFO_DELETE_PARAMS, netname),
    LWMSG_MEMBER_UINT32(SHARE_INFO_DELETE_PARAMS, reserved),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LW_NTSTATUS
LwShareInfoMarshalAddParameters(
    PSHARE_INFO_ADD_PARAMS pParams,
    PBYTE* ppBuffer,
    ULONG* pulBufferSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    VOID* pBuffer = NULL;
    size_t szBufferSize = 0;
    LWMsgContext* pContext = NULL;
    NT_IPC_MESSAGE_CREATE_FILE *pInfo = NULL;

    Status = MAP_LWMSG_STATUS(
        lwmsg_context_new(&pContext));
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
        lwmsg_marshal_alloc(
            pContext,
            gShareInfoAddParamsSpec,
	    pInfo,
	    &pBuffer,
	    &szBufferSize));
    BAIL_ON_NT_STATUS(Status);

    *ppBuffer = pBuffer;
    *pulBufferSize = (ULONG) szBufferSize;

cleanup:

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

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
LwShareInfoUnmarshalAddParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PSHARE_INFO_ADD_PARAMS* ppParams
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSHARE_INFO_ADD_PARAMS pParams = NULL;
    LWMsgContext* pContext = NULL;

    Status = MAP_LWMSG_STATUS(
        lwmsg_context_new(&pContext));
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
        lwmsg_unmarshal_simple(
            pContext,
            gShareInfoAddParamsSpec,
	    pBuffer,
            ulBufferSize,
            OUT_PPVOID(&pParams)));
    BAIL_ON_NT_STATUS(Status);

    *ppParams = pParams;

cleanup:

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    return Status;

error:

    *ppParams = NULL;

    if (pParams)
    {
        lwmsg_context_free_graph(pContext, gShareInfoAddParamsSpec, pParams);
    }

    goto cleanup;
}

LW_NTSTATUS
LwShareInfoMarshalDeleteParameters(
    PSHARE_INFO_ADD_PARAMS pParams,
    PBYTE* ppBuffer,
    ULONG* pulBufferSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    VOID* pBuffer = NULL;
    size_t szBufferSize = 0;
    LWMsgContext* pContext = NULL;
    NT_IPC_MESSAGE_CREATE_FILE *pInfo = NULL;

    Status = MAP_LWMSG_STATUS(
        lwmsg_context_new(&pContext));
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
        lwmsg_marshal_alloc(
            pContext,
            gShareInfoDeleteParamsSpec,
	    pInfo,
	    &pBuffer,
	    &szBufferSize));
    BAIL_ON_NT_STATUS(Status);

    *ppBuffer = pBuffer;
    *pulBufferSize = (ULONG) szBufferSize;

cleanup:

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

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
LwShareInfoUnmarshalDeleteParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PSHARE_INFO_DELETE_PARAMS* ppParams
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSHARE_INFO_DELETE_PARAMS pParams = NULL;
    LWMsgContext* pContext = NULL;

    Status = MAP_LWMSG_STATUS(
        lwmsg_context_new(&pContext));
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
        lwmsg_unmarshal_simple(
            pContext,
            gShareInfoDeleteParamsSpec,
	    pBuffer,
            ulBufferSize,
            OUT_PPVOID(&pParams)));
    BAIL_ON_NT_STATUS(Status);

    *ppParams = pParams;

cleanup:

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    return Status;

error:

    *ppParams = NULL;

    if (pParams)
    {
        lwmsg_context_free_graph(pContext, gShareInfoDeleteParamsSpec, pParams);
    }

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
