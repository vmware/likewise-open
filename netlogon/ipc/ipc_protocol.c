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
 *        ipc_protocol.c
 *
 * Abstract:
 *
 *        Likewise Netlogon
 *
 *        Active Directory Site API
 *
 * Authors:
 *          Brian koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "includes.h"

static LWMsgTypeSpec gLWNetDCInfoSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_DC_INFO),
    LWMSG_MEMBER_UINT32(LWNET_DC_INFO, dwPingTime),
    LWMSG_MEMBER_UINT32(LWNET_DC_INFO, dwDomainControllerAddressType),
    LWMSG_MEMBER_UINT32(LWNET_DC_INFO, dwFlags),
    LWMSG_MEMBER_UINT32(LWNET_DC_INFO, dwVersion),
    LWMSG_MEMBER_UINT16(LWNET_DC_INFO, wLMToken),
    LWMSG_MEMBER_UINT16(LWNET_DC_INFO, wNTToken),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszDomainControllerName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszDomainControllerAddress),
    LWMSG_MEMBER_ARRAY_BEGIN(LWNET_DC_INFO, pucDomainGUID),
    LWMSG_UINT8(UCHAR),
    LWMSG_ARRAY_END,
    LWMSG_ATTR_LENGTH_STATIC(LWNET_GUID_SIZE),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszNetBIOSDomainName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszFullyQualifiedDomainName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszDnsForestName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszDCSiteName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszClientSiteName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszNetBIOSHostName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszUserName),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetDCAddressSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_DC_ADDRESS),
    LWMSG_MEMBER_PSTR(LWNET_DC_ADDRESS, pszDomainControllerName),
    LWMSG_MEMBER_PSTR(LWNET_DC_ADDRESS, pszDomainControllerAddress),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetIPCErrorSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_ERROR),
    LWMSG_MEMBER_UINT32(LWNET_IPC_ERROR, dwError),
    LWMSG_MEMBER_PSTR(LWNET_IPC_ERROR, pszErrorMessage),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetIPCDCNameReqSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_DCNAME_REQ),
    LWMSG_MEMBER_PSTR(LWNET_IPC_DCNAME_REQ, pszServerFQDN),
    LWMSG_MEMBER_PSTR(LWNET_IPC_DCNAME_REQ, pszDomainFQDN),
    LWMSG_MEMBER_PSTR(LWNET_IPC_DCNAME_REQ, pszSiteName),
    LWMSG_MEMBER_UINT32(LWNET_IPC_DCNAME_REQ, dwFlags),
    LWMSG_MEMBER_UINT32(LWNET_IPC_DCNAME_REQ, dwBlackListCount),
    LWMSG_MEMBER_POINTER_BEGIN(LWNET_IPC_DCNAME_REQ, ppszAddressBlackList),
    LWMSG_PSTR,
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(LWNET_IPC_DCNAME_REQ, dwBlackListCount),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetIPCDCTimeReqSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_DCTIME_REQ),
    LWMSG_MEMBER_PSTR(LWNET_IPC_DCTIME_REQ, pszDomainFQDN),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetIPCDCTimeResSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_DCTIME_RES),
    LWMSG_MEMBER_UINT32(LWNET_IPC_DCTIME_RES, dcTime),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetIPCDCReqSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_DC_REQ),
    LWMSG_MEMBER_PSTR(LWNET_IPC_DC_REQ, pszDomainFQDN),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetIPCDCResSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_DC_RES),
    LWMSG_MEMBER_PSTR(LWNET_IPC_DC_RES, pszDCFQDN),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetIPCCurrentResSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_CURRENT_RES),
    LWMSG_MEMBER_PSTR(LWNET_IPC_CURRENT_RES, pszDomainFQDN),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetIPCDCListResSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_DCLIST_RES),
    LWMSG_MEMBER_UINT32(LWNET_IPC_DCLIST_RES, dwDcCount),
    LWMSG_MEMBER_POINTER_BEGIN(LWNET_IPC_DCLIST_RES, pDcList),
    LWMSG_TYPESPEC(gLWNetDCAddressSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(LWNET_IPC_DCLIST_RES, dwDcCount),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgProtocolSpec gLWNetIPCSpec[] =
{
    LWMSG_MESSAGE(LWNET_Q_DCTIME, gLWNetIPCDCTimeReqSpec),
    LWMSG_MESSAGE(LWNET_R_DCTIME_SUCCESS, gLWNetIPCDCTimeResSpec),
    LWMSG_MESSAGE(LWNET_R_DCTIME_FAILURE, gLWNetIPCErrorSpec),
    LWMSG_MESSAGE(LWNET_Q_DCINFO, gLWNetIPCDCNameReqSpec),
    LWMSG_MESSAGE(LWNET_R_DCINFO_SUCCESS, gLWNetDCInfoSpec),
    LWMSG_MESSAGE(LWNET_R_DCINFO_FAILURE, gLWNetIPCErrorSpec),
    LWMSG_MESSAGE(LWNET_Q_DC, gLWNetIPCDCReqSpec),
    LWMSG_MESSAGE(LWNET_R_DC_SUCCESS, gLWNetIPCDCResSpec),
    LWMSG_MESSAGE(LWNET_R_DC_FAILURE, gLWNetIPCErrorSpec),
    LWMSG_MESSAGE(LWNET_Q_CURRENT_DOMAIN, NULL),
    LWMSG_MESSAGE(LWNET_R_CURRENT_DOMAIN_SUCCESS, gLWNetIPCCurrentResSpec),
    LWMSG_MESSAGE(LWNET_R_CURRENT_DOMAIN_FAILURE, gLWNetIPCErrorSpec),
    LWMSG_MESSAGE(LWNET_Q_DCLIST, gLWNetIPCDCNameReqSpec),
    LWMSG_MESSAGE(LWNET_R_DCLIST_SUCCESS, gLWNetIPCDCListResSpec),
    LWMSG_MESSAGE(LWNET_R_DCLIST_FAILURE, gLWNetIPCErrorSpec),
    LWMSG_PROTOCOL_END
};

LWMsgProtocolSpec*
LWNetIPCGetProtocolSpec(
    void
    )
{
    return gLWNetIPCSpec;
}
