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
 *        dnsresponse.h
 *
 * Abstract:
 *
 *        Likewise Dynamic DNS Updates (LWDNS)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __DNSRESPONSE_H__
#define __DNSRESPONSE_H__

DWORD
DNSStdReceiveStdResponse(
    HANDLE         hDNSHandle,
    PDNS_RESPONSE* ppDNSResponse
    );

DWORD
DNSUpdateGetResponseCode(
    PDNS_UPDATE_RESPONSE pDNSUpdateResponse,
    PDWORD pdwResponseCode
    );

DWORD
DNSStdGetResponseCode(
    PDNS_RESPONSE pDNSResponse
    );

VOID
DNSStdFreeResponse(
    PDNS_RESPONSE pDNSResponse
    );

DWORD
DNSUnmarshallDomainName(
    HANDLE hRecvBuffer, 
    PDNS_DOMAIN_NAME * ppDomainName
    );

DWORD
DNSUnmarshallRRHeader(
    HANDLE         hRecvBuffer,
    PDNS_RR_HEADER pRRHeader
    );

DWORD
DNSUnmarshallRData(
    HANDLE hRecvBuffer,
    DWORD  dwSize,
    PBYTE* ppRData,
    PDWORD pdwRead
    );

#endif /* __DNSRESPONSE_H__ */

