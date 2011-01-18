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
 *        dnssock.h
 *
 * Abstract:
 *
 *        Likewise Dynamic DNS Updates (LWDNS)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */


DWORD
DNSTCPOpen(
	PCSTR    pszNameServer,
	PHANDLE  phDNSServer
	);



DWORD
DNSUDPOpen(
    PCSTR    pszNameServer,
	PHANDLE  phDNSServer
	);



DWORD
DNSTCPClose(HANDLE hBindServer);


DWORD
DNSUDPClose(HANDLE hBindServer);

DWORD
DNSClose(
    HANDLE hDNSServer
    );

DWORD
DNSSendTCPRequest(
	HANDLE hDNSHandle,
	PBYTE pDNSSendBuffer,
	DWORD dwBufferSize,
	PDWORD pdwBytesSent
	);

DWORD
DNSSendUDPRequest(
	HANDLE hDNSHandle,
	PBYTE pDNSSendBuffer,
	DWORD dwBufferSize,
	PDWORD pdwBytesSent
	);


DWORD
DNSTCPReceiveBufferContext(
	HANDLE hDNSHandle,
	HANDLE hDNSRecvBuffer,
	PDWORD pdwBytesRead
	);

DWORD
DNSUDPReceiveBufferContext(
	HANDLE hDNSHandle,
	HANDLE hDNSRecvBuffer,
	PDWORD pdwBytesRead
	);

DWORD
DNSReceiveBufferContext(
	HANDLE hDNSHandle,
	HANDLE hDNSRecvBuffer,
	PDWORD pdwBytesRead
	);


DWORD
DNSCreateSendBuffer(
	HANDLE * phDNSSendBuffer
	);


DWORD
DNSMarshallBuffer(
	HANDLE hDNSSendBuffer,
	PBYTE pDNSSendBuffer,
	DWORD dwBufferSize,
	PDWORD pdwBytesWritten
	);

void
DNSFreeSendBuffer(
	HANDLE hDNSSendBuffer
	);

DWORD
DNSSendBufferContext(
	HANDLE hDNSServer,
	HANDLE hSendBuffer,
	PDWORD pdwBytesSent
	);


DWORD
DNSTCPSendBufferContext(
	HANDLE hDNSServer,
	HANDLE hSendBuffer,
	PDWORD pdwBytesSent
	);


DWORD
DNSUDPSendBufferContext(
	HANDLE hDNSServer,
	HANDLE hSendBuffer,
	PDWORD pdwBytesSent
	);

DWORD
DNSDumpSendBufferContext(
	HANDLE hSendBuffer
	);

DWORD
DNSDumpRecvBufferContext(
	HANDLE hRecvBuffer
	);

DWORD
DNSCreateReceiveBuffer(
	HANDLE * phDNSRecvBuffer
	);

DWORD
DNSUnmarshallBuffer(
	HANDLE hDNSRecvBuffer,
	PBYTE pDNSRecvBuffer,
	DWORD dwBufferSize,
	PDWORD pdwBytesRead
	);

DWORD
DNSUnmarshallDomainNameAtOffset(
	HANDLE hRecvBuffer,
	WORD wOffset,
	PDNS_DOMAIN_NAME * ppDomainName
	);

DWORD
DNSReceiveBufferMoveBackIndex(
	HANDLE hRecvBuffer,
	WORD wOffset
	);

VOID
DNSFreeSendBufferContext(
	HANDLE hSendBuffer
	);

VOID
DNSFreeReceiveBufferContext(
	HANDLE hRecvBuffer
	);

DWORD
DNSGetSendBufferContextSize(
	HANDLE hSendBuffer
	);


PBYTE
DNSGetSendBufferContextBuffer(
	HANDLE hSendBuffer
	);


