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
 *        ipcmsg.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Inter-process Message API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "includes.h"

#define BAIL_ON_CONNECTION_INVALID(errCode)              \
        if (errCode != LWNET_ERROR_CONNECTION_CLOSED) {  \
           BAIL_ON_LWNET_ERROR(errCode);                 \
        }                                                \
        else {                                           \
           goto error;                                   \
        }

DWORD
LWNetBuildMessage(
    LWNetMessageType msgType,
    uint32_t       msgLen,
    uint16_t       iData,
    uint16_t       nData,
    PLWNETMESSAGE*   ppMessage)
{
    DWORD dwError = 0;

    PLWNETMESSAGE pMessage = NULL;
    PSTR pData = NULL;

    dwError = LWNetAllocateMemory(sizeof(LWNETMESSAGE), (PVOID*)&pMessage);
    BAIL_ON_LWNET_ERROR(dwError);

    memset(pMessage, 0, sizeof(LWNETMESSAGE));
    
    pMessage->header.messageType = msgType;
    pMessage->header.version = 1;
    pMessage->header.reserved[0] = iData;
    pMessage->header.reserved[1] = nData;
    pMessage->header.messageLength = msgLen;
    
    if (pMessage->header.messageLength > 0)
    {
        dwError = LWNetAllocateMemory(pMessage->header.messageLength + 1, (PVOID*)&pData);
        BAIL_ON_LWNET_ERROR(dwError);
        pMessage->pData = pData;
    }

    *ppMessage = pMessage;
    
    return (dwError);

  error:

    if (pData) {
        LWNetFreeMemory(pData);
    }

    if (pMessage) {
        LWNetFreeMemory(pMessage);
    }
    
    *ppMessage = NULL;

    return (dwError);
}

void
LWNetFreeMessage(
    PLWNETMESSAGE pMessage
    )
{
    LWNET_SAFE_FREE_MEMORY(pMessage->pData);
    LWNetFreeMemory(pMessage);
}

DWORD
LWNetReadNextMessage(
    int          fd,
    PLWNETMESSAGE* ppMessage
    )
{
    DWORD dwError = 0;
    PLWNETMESSAGE pMessage = NULL;
    PSTR pData = NULL;
    DWORD dwBytesRead = 0;
    DWORD msgLength = 0;

    dwError = LWNetAllocateMemory(sizeof(LWNETMESSAGE), (PVOID*)&pMessage);
    BAIL_ON_LWNET_ERROR(dwError);
    memset(pMessage, 0, sizeof(LWNETMESSAGE));
    
    dwError = LWNetReadData(fd, (PSTR)&(pMessage->header), sizeof(LWNETMESSAGEHEADER), &dwBytesRead);
    BAIL_ON_CONNECTION_INVALID(dwError);

    if (dwBytesRead != sizeof(LWNETMESSAGEHEADER)) {
       dwError = EINVAL;
       BAIL_ON_LWNET_ERROR(dwError);
    }

    msgLength = pMessage->header.messageLength;
    if (msgLength > 0) {

       dwError = LWNetAllocateMemory(msgLength + 1, (PVOID*)&pData);
       BAIL_ON_LWNET_ERROR(dwError);

       dwError = LWNetReadData(fd, pData, msgLength, &dwBytesRead);
       BAIL_ON_CONNECTION_INVALID(dwError);

       if (msgLength != dwBytesRead) {
          dwError = EINVAL;
          BAIL_ON_LWNET_ERROR(dwError);
       }

       // We don't null-terminate the data, because it may not be a plain string
       // We should treat it as a byte stream
       pMessage->pData = pData;
    }

    *ppMessage = pMessage;
    
    return (dwError);

  error:
    
    if (pData) {
        LWNetFreeMemory(pData);
    }

    if (pMessage) {
        LWNetFreeMemory(pMessage);
    }

    return (dwError);
}

DWORD
LWNetSecureReadNextMessage(
    int   fd,
    uid_t peerUID,
    PFNMESSAGESCREENER pFnScreener,
    PLWNETMESSAGE* ppMessage
    )
{
    DWORD dwError = 0;
    PLWNETMESSAGE pMessage = NULL;
    PSTR pData = NULL;
    DWORD dwBytesRead = 0;
    DWORD msgLength = 0;
    
    if (!pFnScreener) {
        dwError = LWNetReadNextMessage(fd, ppMessage);
        goto cleanup;
    }

    dwError = LWNetAllocateMemory(sizeof(LWNETMESSAGE), (PVOID*)&pMessage);
    BAIL_ON_LWNET_ERROR(dwError);
    memset(pMessage, 0, sizeof(LWNETMESSAGE));
    
    dwError = LWNetReadData(fd, (PSTR)&(pMessage->header), sizeof(LWNETMESSAGEHEADER), &dwBytesRead);
    BAIL_ON_CONNECTION_INVALID(dwError);

    if (dwBytesRead != sizeof(LWNETMESSAGEHEADER)) {
       dwError = EINVAL;
       BAIL_ON_LWNET_ERROR(dwError);
    }

    // This is just checking the message header.
    dwError = pFnScreener(pMessage, peerUID);
    BAIL_ON_LWNET_ERROR(dwError);

    msgLength = pMessage->header.messageLength;
    if (msgLength > 0) {

       dwError = LWNetAllocateMemory(msgLength + 1, (PVOID*)&pData);
       BAIL_ON_LWNET_ERROR(dwError);

       dwError = LWNetReadData(fd, pData, msgLength, &dwBytesRead);
       BAIL_ON_CONNECTION_INVALID(dwError);

       if (msgLength != dwBytesRead) {
          dwError = EINVAL;
          BAIL_ON_LWNET_ERROR(dwError);
       }

       // We don't null-terminate the data, because it may not be a plain string
       // We should treat it as a byte stream
       pMessage->pData = pData;
    }

    *ppMessage = pMessage;
    
  cleanup:
    
    return (dwError);

  error:
    
    LWNET_SAFE_FREE_MEMORY(pData);
    LWNET_SAFE_FREE_MESSAGE(pMessage);
    
    *ppMessage = NULL;

    goto cleanup;
}

DWORD
LWNetWriteMessage(
    int   fd,
    const PLWNETMESSAGE pMessage)
{
    DWORD dwError = 0;

    dwError = LWNetWriteData(fd,
                        (const PSTR)&pMessage->header,
                        sizeof(LWNETMESSAGEHEADER));
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LWNetWriteData(fd,
                         pMessage->pData,
                         pMessage->header.messageLength);
    BAIL_ON_LWNET_ERROR(dwError);

  error:
    
    return (dwError);
}

#ifndef CMSG_ALIGN
#    if defined(_CMSG_DATA_ALIGN)
#        define CMSG_ALIGN _CMSG_DATA_ALIGN
#    elif defined(_CMSG_ALIGN)
#        define CMSG_ALIGN _CMSG_ALIGN
#    endif
#endif

#ifndef CMSG_SPACE
#    define CMSG_SPACE(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + CMSG_ALIGN(len))
#endif

#ifndef CMSG_LEN
#    define CMSG_LEN(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#endif


DWORD
LWNetSendCreds(
    int fd
    )
{
    DWORD dwError = 0;
    char payload = 0xff;
    int credFd = fd;
    struct iovec payload_vec = {0};
    struct msghdr msg = {0};

#ifdef MSGHDR_HAS_MSG_CONTROL
    union
    {
        struct cmsghdr cm;
        char buf[CMSG_SPACE(sizeof(credFd))];
    } buf_un;
    struct cmsghdr *cmsg = NULL;
#endif

    /* Set up dummy payload */
    payload_vec.iov_base = &payload;
    payload_vec.iov_len = sizeof(payload);
    msg.msg_iov = &payload_vec;
    msg.msg_iovlen = 1;

#ifdef MSGHDR_HAS_MSG_CONTROL
    /* Set up ancillary data */
    msg.msg_control = buf_un.buf;
    msg.msg_controllen = sizeof(buf_un.buf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(credFd));

    memcpy(CMSG_DATA(cmsg), &credFd, sizeof(credFd));
#else
    msg.msg_accrights = (char*) &credFd;
    msg.msg_accrightslen = sizeof(credFd);
#endif

    /* Send message */
    dwError = LWNetSendMsg(fd, &msg);
    BAIL_ON_LWNET_ERROR(dwError);

error:

    return dwError;
}

DWORD
LWNetRecvCreds(
    int fd,
    uid_t* pUid,
    gid_t* pGid)
{
    DWORD dwError = 0;
    char payload = 0;
    int credFd = -1;
    struct iovec payload_vec = {0};
    struct msghdr msg = {0};
#ifdef MSGHDR_HAS_MSG_CONTROL
    union
    {
        struct cmsghdr cm;
        char buf[CMSG_SPACE(sizeof(credFd))];
    } buf_un;
    struct cmsghdr *cmsg = NULL;
#endif
    struct stat statbuf = {0};
    struct sockaddr_un localAddr;
    SOCKLEN_T localAddrLen = sizeof(localAddr);
    struct sockaddr_un credPeerAddr;
    SOCKLEN_T credPeerAddrLen = sizeof(credPeerAddr);

    /* Set up area to receive dummy payload */
    payload_vec.iov_base = &payload;
    payload_vec.iov_len = sizeof(payload);
    msg.msg_iov = &payload_vec;
    msg.msg_iovlen = 1;

#ifdef MSGHDR_HAS_MSG_CONTROL
    /* Set up area to receive ancillary data */
    msg.msg_control = buf_un.buf;
    msg.msg_controllen = sizeof(buf_un.buf);
#else
    msg.msg_accrights = (char*) &credFd;
    msg.msg_accrightslen = sizeof(credFd);
#endif

    dwError = LWNetRecvMsg(fd, &msg);
    BAIL_ON_LWNET_ERROR(dwError);

    /* Extract credential fd */
    
#ifdef MSGHDR_HAS_MSG_CONTROL
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg))
    {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
        {
            memcpy(&credFd, CMSG_DATA(cmsg), sizeof(credFd));
            break;
        }
    }
#endif

    /* Fail if we couldn't extract a valid fd from message */
    if (credFd == -1)
    {
        dwError = EBADF;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    /* Stat the fd to find the uid/gid of the peer socket */
    if (fstat(credFd, &statbuf) != 0)
    {
        dwError = errno;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    /* Get the path of our unix socket */
    if (getsockname(fd, (struct sockaddr*) &localAddr, &localAddrLen))
    {
        dwError = errno;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    /* Get the path that the peer unix socket is connected to */
    if (getpeername(credFd, (struct sockaddr*) &credPeerAddr, &credPeerAddrLen))
    {
        dwError = errno;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    /* Fail if these paths are not the same */
    if (strcmp(localAddr.sun_path, credPeerAddr.sun_path))
    {
        dwError = EPERM;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    *pUid = statbuf.st_uid;
    *pGid = statbuf.st_gid;

cleanup:

    if (credFd != -1)
        close(credFd);

    return dwError;

error:
	
	*pUid = 0;
	*pGid = 0;

	goto cleanup;
}
