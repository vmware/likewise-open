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
 *        lwnet-ipc.h
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        Interprocess Communication
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __LWNETIPC_H__
#define __LWNETIPC_H__

#include <lwmsg/lwmsg.h>

#include "lwnet-utils.h"

typedef struct _LWNET_IPC_ERROR
{
    DWORD dwError;
    PCSTR pszErrorMessage;
} LWNET_IPC_ERROR, *PLWNET_IPC_ERROR;

typedef struct _LWNET_IPC_DCNAME_REQ
{
    PCSTR   pszServerFQDN;
    PCSTR   pszDomainFQDN;
    PCSTR   pszSiteName;
    DWORD   dwFlags;
    DWORD   dwBlackListCount;
    PSTR*   ppszAddressBlackList;
} LWNET_IPC_DCNAME_REQ, *PLWNET_IPC_DCNAME_REQ;

typedef struct _LWNET_IPC_DCTIME_REQ
{
    PCSTR pszDomainFQDN;
} LWNET_IPC_DCTIME_REQ, *PLWNET_IPC_DCTIME_REQ;

typedef struct _LWNET_IPC_DCTIME_RES
{
    LWNET_UNIX_TIME_T dcTime;
} LWNET_IPC_DCTIME_RES, *PLWNET_IPC_DCTIME_RES;

typedef struct _LWNET_IPC_DC_REQ
{
    PCSTR pszDomainFQDN;
} LWNET_IPC_DC_REQ, *PLWNET_IPC_DC_REQ;

typedef struct _LWNET_IPC_DC_RES
{
    PSTR pszDCFQDN;
} LWNET_IPC_DC_RES, *PLWNET_IPC_DC_RES;

typedef struct _LWNET_IPC_CURRENT_RES
{
    PSTR pszDomainFQDN;
} LWNET_IPC_CURRENT_RES, *PLWNET_IPC_CURRENT_RES;

typedef struct _LWNET_IPC_DCLIST_RES {
    PLWNET_DC_ADDRESS pDcList;
    DWORD dwDcCount;
} LWNET_IPC_DCLIST_RES, *PLWNET_IPC_DCLIST_RES;

typedef enum _LWNET_IPC_TAG
{
    LWNET_Q_DCTIME,
    LWNET_R_DCTIME_SUCCESS,
    LWNET_R_DCTIME_FAILURE,
    LWNET_Q_DCINFO,
    LWNET_R_DCINFO_SUCCESS,
    LWNET_R_DCINFO_FAILURE,
    LWNET_Q_DC,
    LWNET_R_DC_SUCCESS,
    LWNET_R_DC_FAILURE,
    LWNET_Q_CURRENT_DOMAIN,
    LWNET_R_CURRENT_DOMAIN_SUCCESS,
    LWNET_R_CURRENT_DOMAIN_FAILURE,
    LWNET_Q_DCLIST,
    LWNET_R_DCLIST_SUCCESS,
    LWNET_R_DCLIST_FAILURE
} LWNET_IPC_TAG;

LWMsgProtocolSpec*
LWNetIPCGetProtocolSpec(
    void
    );

#define MAP_LWMSG_ERROR(_e_) ((_e_) ? -1 : 0)
#define MAP_LWNET_ERROR(_e_) ((_e_) ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS)

#endif /*__LWNETIPC_H__*/
