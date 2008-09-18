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

#include "lwnet-utils.h"

typedef enum {
    LWNET_ERROR            =   0,
    LWNET_Q_DCTIME         =   1,
    LWNET_R_DCTIME         =   2,
    LWNET_Q_DCINFO         =   3,
    LWNET_R_DCINFO         =   4,
    LWNET_Q_DC             =   5,
    LWNET_R_DC             =   6,
    LWNET_Q_CURRENT_DOMAIN =   7,
    LWNET_R_CURRENT_DOMAIN =   8,
    LWNET_MESSAGE_SENTINEL
} LWNetMessageType;

typedef struct LWNetMessageHeaderTag {
    /* type of group policy message */
    uint8_t   messageType;
    /* protocol version */
    uint8_t   version;
    /* This may be used for sequencing
     * For instance, 1 of 10, 2 of 10
     */
    uint16_t  reserved[2];
    /* The length of the attached message
     * This is in network format
     */
    uint32_t  messageLength;
} LWNETMESSAGEHEADER, *PLWNETMESSAGEHEADER;

typedef struct LWNetMessageTag {
    LWNETMESSAGEHEADER header;
    PSTR pData;
} LWNETMESSAGE, *PLWNETMESSAGE;

#define LWNET_SAFE_FREE_MESSAGE(pMessage) \
    if (pMessage) {                     \
        LWNetFreeMessage(pMessage);    \
        pMessage = NULL;                \
    }

typedef DWORD (*PFNMESSAGESCREENER) (PLWNETMESSAGE pMessage, uid_t peerUID);

typedef struct __LWNETDATACOORDINATES {
    DWORD offset;
    DWORD length;
} LWNETDATACOORDINATES, *PLWNETDATACOORDINATES;

typedef struct __LWNET_DC_NAME_REQ_HEADER
{
    LWNETDATACOORDINATES serverFQDN;
    LWNETDATACOORDINATES domainFQDN;
    LWNETDATACOORDINATES siteName;
    DWORD flags;
} LWNET_DC_NAME_REQ_HEADER, *PLWNET_DC_NAME_REQ_HEADER;

typedef struct __LWNET_DC_INFO_HEADER
{
    DWORD domainControllerAddressType;
    DWORD flags;
    DWORD version;
    WORD LMToken;
    WORD NTToken;
    UCHAR domainGUID[LWNET_GUID_SIZE];
    LWNETDATACOORDINATES domainControllerName;
    LWNETDATACOORDINATES domainControllerAddress;
    LWNETDATACOORDINATES netBIOSDomainName;
    LWNETDATACOORDINATES fullyQualifiedDomainName;
    LWNETDATACOORDINATES DNSForestName;
    LWNETDATACOORDINATES DCSiteName;
    LWNETDATACOORDINATES clientSiteName;
    LWNETDATACOORDINATES netBIOSHostName;
    LWNETDATACOORDINATES userName;
} LWNET_DC_INFO_HEADER, *PLWNET_DC_INFO_HEADER;

typedef struct __LWNETERRORRECORDHEADER {
    DWORD              errorCode;
    LWNETDATACOORDINATES message;
} LWNETERRORRECORDHEADER, *PLWNETERRORRECORDHEADER;

DWORD
LWNetOpenServer(
    PHANDLE phConnection
    );

DWORD
LWNetCloseServer(
    HANDLE hConnection
    );

/* Builds a message object with the data field allocated - but, not filled in */
DWORD
LWNetBuildMessage(
    LWNetMessageType msgType,
    uint32_t       msgLen,
    uint16_t       iData,
    uint16_t       nData,
    PLWNETMESSAGE*   ppMessage
    );

void
LWNetFreeMessage(
    PLWNETMESSAGE pMessage
    );

DWORD
LWNetReadNextMessage(
    int         fd,
    PLWNETMESSAGE *ppMessage
    );

DWORD
LWNetSecureReadNextMessage(
    int                fd,
    uid_t              peerUID,
    PFNMESSAGESCREENER pFnScreener,
    PLWNETMESSAGE        *ppMessage
    );

DWORD
LWNetWriteMessage(
    int   fd,
    const PLWNETMESSAGE pMessage
    );

DWORD
LWNetSendCreds(
    int fd
    );

DWORD
LWNetRecvCreds(
    int fd,
    uid_t* pUid,
    gid_t* pGid
    );

DWORD
LWNetWriteData(
    DWORD dwFd,
    PSTR  pszBuf,
    DWORD dwLen);

DWORD
LWNetReadData(
    DWORD  dwFd,
    PSTR   pszBuf,
    DWORD  dwBytesToRead,
    PDWORD pdwBytesRead);

DWORD
LWNetSendMsg(
    DWORD dwFd,
    const struct msghdr *pMsg
    );

DWORD
LWNetRecvMsg(
    DWORD dwFd,
    struct msghdr *pMsg
    );

DWORD
LWNetMarshalError(
    DWORD errorCode,
    PCSTR pszErrorMessage,
    PSTR  pszBuf,
    PDWORD pdwBufLen
    );

DWORD
LWNetUnmarshalError(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PDWORD pdwError,
    PSTR*  ppszError
    );

DWORD
LWNetMarshalDCNameReq(
    PCSTR   pszServerFQDN,
    PCSTR   pszDomainFQDN,
    PCSTR   pszSiteName,
    DWORD   dwFlags,
    PSTR    pszBuf,
    PDWORD  pdwBufLen
    );

DWORD
LWNetComputeDCNameReqLength(
    PCSTR   pszServerFQDN,
    PCSTR   pszDomainFQDN,
    PCSTR   pszSiteName
    );

DWORD
LWNetUnmarshalDCNameReq(
    PCSTR   pszMsgBuf,
    DWORD   dwMsgLen,
    PSTR*   ppszServerFQDN,
    PSTR*   ppszDomainFQDN,
    PSTR*   ppszSiteName,
    PDWORD  pdwFlags
    );

DWORD
LWNetMarshalDCInfo(
    PLWNET_DC_INFO  pDCInfo,
    PSTR            pszBuffer,
    PDWORD          pdwBufLen
    );

DWORD
LWNetUnmarshalDCInfo(
    PCSTR         pszMsgBuf,
    DWORD         dwBufLen,
    PLWNET_DC_INFO* ppDCInfo
    );

DWORD
LWNetComputeBufferLength(
    PLWNET_DC_INFO pDCInfo
    );

#endif /*__LWNETIPC_H__*/
