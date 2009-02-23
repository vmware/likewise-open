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

#define LWMSG_SPEC_DEBUG

#include "includes.h"

LWMsgTypeSpec gSecurityTokenRepSpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(IO_ACCESS_TOKEN),
    /* Discriminator */
    LWMSG_MEMBER_UINT8(IO_ACCESS_TOKEN, type),
    /* Begin union */
    LWMSG_MEMBER_UNION_BEGIN(IO_ACCESS_TOKEN, payload),
    /* Union arm -- plain */
    LWMSG_MEMBER_STRUCT_BEGIN(union _LW_IO_ACCESS_TOKEN_U, plain),
    LWMSG_MEMBER_PWSTR(struct _LW_IO_ACCESS_TOKEN_PLAIN, pwszUsername),
    LWMSG_MEMBER_PWSTR(struct _LW_IO_ACCESS_TOKEN_PLAIN, pwszPassword),
    LWMSG_STRUCT_END,
    LWMSG_ATTR_TAG(IO_ACCESS_TOKEN_TYPE_PLAIN),
    /* Union arm -- krb5 */
    LWMSG_MEMBER_STRUCT_BEGIN(union _LW_IO_ACCESS_TOKEN_U, krb5),
    LWMSG_MEMBER_PWSTR(struct _LW_IO_ACCESS_TOKEN_KRB5, pwszPrincipal),
    LWMSG_MEMBER_PWSTR(struct _LW_IO_ACCESS_TOKEN_KRB5, pwszCachePath),
    LWMSG_STRUCT_END,
    LWMSG_ATTR_TAG(IO_ACCESS_TOKEN_TYPE_KRB5),
    /* End union */
    LWMSG_UNION_END,
    LWMSG_ATTR_DISCRIM(IO_ACCESS_TOKEN, type),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRequestConfigSpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(SMB_REQUEST),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(SMB_REQUEST, dwCurTime),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gStatusReplySpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(SMB_STATUS_REPLY),
    /* err - marshal as 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(SMB_STATUS_REPLY, dwError),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLogInfoSpec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(SMB_LOG_INFO),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(SMB_LOG_INFO, maxAllowedLogLevel),
    /* 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(SMB_LOG_INFO, logTarget),
    /* path - marshal as pointer to string */
    LWMSG_MEMBER_PSTR(SMB_LOG_INFO, pszPath),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgProtocolSpec gLwIoDaemonProtocolSpec[] =
{
    LWMSG_MESSAGE(SMB_REFRESH_CONFIG,              gRequestConfigSpec),
    LWMSG_MESSAGE(SMB_REFRESH_CONFIG_SUCCESS,      gStatusReplySpec),
    LWMSG_MESSAGE(SMB_REFRESH_CONFIG_FAILED,       gStatusReplySpec),
    LWMSG_MESSAGE(SMB_SET_LOG_INFO,                gLogInfoSpec),
    LWMSG_MESSAGE(SMB_SET_LOG_INFO_SUCCESS,        gStatusReplySpec),
    LWMSG_MESSAGE(SMB_SET_LOG_INFO_FAILED,         gStatusReplySpec),
    LWMSG_MESSAGE(SMB_GET_LOG_INFO,                gRequestConfigSpec),
    LWMSG_MESSAGE(SMB_GET_LOG_INFO_SUCCESS,        gLogInfoSpec),
    LWMSG_MESSAGE(SMB_GET_LOG_INFO_FAILED,         gStatusReplySpec),
    LWMSG_PROTOCOL_END
};

NTSTATUS
LwIoDaemonIpcAddProtocolSpec(
    IN OUT LWMsgProtocol* pProtocol
    )
{
    return LwIoDaemonIpcAddProtocolSpecEx(pProtocol, NULL);
}

NTSTATUS
LwIoDaemonIpcAddProtocolSpecEx(
    IN OUT LWMsgProtocol* pProtocol,
    OUT OPTIONAL PCSTR* ppszError
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    LWMsgStatus msgStatus = 0;
    PCSTR pszError = NULL;

    msgStatus = lwmsg_protocol_add_protocol_spec(pProtocol, gLwIoDaemonProtocolSpec);

    if (msgStatus && ppszError)
    {
        pszError = lwmsg_protocol_get_error_message(pProtocol, msgStatus);
    }

    status = NtIpcLWMsgStatusToNtStatus(msgStatus);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (ppszError)
    {
        *ppszError = pszError;
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

