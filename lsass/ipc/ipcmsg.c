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
 *        lsamsg.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Inter-process Message API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "ipc.h"

DWORD
LsaBuildMessage(
    LsaMessageType msgType,
    uint32_t       msgLen,
    uint16_t       iData,
    uint16_t       nData,
    PLSAMESSAGE*   ppMessage)
{
    DWORD dwError = 0;

    PLSAMESSAGE pMessage = NULL;
    PSTR pData = NULL;

    dwError = LsaAllocateMemory(sizeof(LSAMESSAGE), (PVOID*)&pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    memset(pMessage, 0, sizeof(LSAMESSAGE));
    
    pMessage->header.messageType = msgType;
    pMessage->header.version = 1;
    pMessage->header.reserved[0] = iData;
    pMessage->header.reserved[1] = nData;
    pMessage->header.messageLength = msgLen;
    
    if (pMessage->header.messageLength > 0)
    {
        dwError = LsaAllocateMemory(pMessage->header.messageLength + 1, (PVOID*)&pData);
        BAIL_ON_LSA_ERROR(dwError);
        pMessage->pData = pData;
    }

    *ppMessage = pMessage;
    
    return (dwError);

  error:

    if (pData) {
        LsaFreeMemory(pData);
    }

    if (pMessage) {
        LsaFreeMemory(pMessage);
    }
    
    *ppMessage = NULL;

    return (dwError);
}

void
LsaFreeMessage(
    PLSAMESSAGE pMessage
    )
{
    LSA_SAFE_FREE_MEMORY(pMessage->pData);
    LsaFreeMemory(pMessage);
}

DWORD
LsaReadNextMessage(
    int          fd,
    PLSAMESSAGE* ppMessage
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    PSTR pData = NULL;
    DWORD dwBytesRead = 0;
    DWORD msgLength = 0;

    dwError = LsaAllocateMemory(sizeof(LSAMESSAGE), (PVOID*)&pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    memset(pMessage, 0, sizeof(LSAMESSAGE));
    
    dwError = LsaReadData(fd, (PSTR)&(pMessage->header), sizeof(LSAMESSAGEHEADER), &dwBytesRead);
    BAIL_ON_CONNECTION_INVALID(dwError);

    if (dwBytesRead != sizeof(LSAMESSAGEHEADER)) {
       dwError = EINVAL;
       BAIL_ON_LSA_ERROR(dwError);
    }

    msgLength = pMessage->header.messageLength;
    if (msgLength > 0) {

       dwError = LsaAllocateMemory(msgLength + 1, (PVOID*)&pData);
       BAIL_ON_LSA_ERROR(dwError);

       dwError = LsaReadData(fd, pData, msgLength, &dwBytesRead);
       BAIL_ON_CONNECTION_INVALID(dwError);

       if (msgLength != dwBytesRead) {
          dwError = EINVAL;
          BAIL_ON_LSA_ERROR(dwError);
       }

       // We don't null-terminate the data, because it may not be a plain string
       // We should treat it as a byte stream
       pMessage->pData = pData;
    }

    *ppMessage = pMessage;
    
    return (dwError);

  error:
    
    if (pData) {
        LsaFreeMemory(pData);
    }

    if (pMessage) {
        LsaFreeMemory(pMessage);
    }

    return (dwError);
}

DWORD
LsaSecureReadNextMessage(
    int   fd,
    uid_t peerUID,
    PFNMESSAGESCREENER pFnScreener,
    PLSAMESSAGE* ppMessage
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    PSTR pData = NULL;
    DWORD dwBytesRead = 0;
    DWORD msgLength = 0;
    
    if (!pFnScreener) {
    	dwError = LsaReadNextMessage(fd, ppMessage);
    	goto cleanup;
    }

    dwError = LsaAllocateMemory(sizeof(LSAMESSAGE), (PVOID*)&pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    memset(pMessage, 0, sizeof(LSAMESSAGE));
    
    dwError = LsaReadData(fd, (PSTR)&(pMessage->header), sizeof(LSAMESSAGEHEADER), &dwBytesRead);
    BAIL_ON_CONNECTION_INVALID(dwError);

    if (dwBytesRead != sizeof(LSAMESSAGEHEADER)) {
       dwError = EINVAL;
       BAIL_ON_LSA_ERROR(dwError);
    }

    // This is just checking the message header.
    dwError = pFnScreener(pMessage, peerUID);
    BAIL_ON_LSA_ERROR(dwError);

    msgLength = pMessage->header.messageLength;
    if (msgLength > 0) {

       dwError = LsaAllocateMemory(msgLength + 1, (PVOID*)&pData);
       BAIL_ON_LSA_ERROR(dwError);

       dwError = LsaReadData(fd, pData, msgLength, &dwBytesRead);
       BAIL_ON_CONNECTION_INVALID(dwError);

       if (msgLength != dwBytesRead) {
          dwError = EINVAL;
          BAIL_ON_LSA_ERROR(dwError);
       }

       // We don't null-terminate the data, because it may not be a plain string
       // We should treat it as a byte stream
       pMessage->pData = pData;
    }

    *ppMessage = pMessage;
    
  cleanup:
    
    return (dwError);

  error:
    
    LSA_SAFE_FREE_MEMORY(pData);
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    *ppMessage = NULL;

    goto cleanup;
}

DWORD
LsaWriteMessage(
    int   fd,
    const PLSAMESSAGE pMessage)
{
    DWORD dwError = 0;

    dwError = LsaWriteData(fd,
                        (const PSTR)&pMessage->header,
                        sizeof(LSAMESSAGEHEADER));
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWriteData(fd,
                         pMessage->pData,
                         pMessage->header.messageLength);
    BAIL_ON_LSA_ERROR(dwError);

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
LsaSendCreds(
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
    dwError = LsaSendMsg(fd, &msg);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
LsaRecvCreds(
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

    dwError = LsaRecvMsg(fd, &msg);
    BAIL_ON_LSA_ERROR(dwError);

    /* Extract credential fd */
    
#ifdef MSGHDR_HAS_MSG_CONTROL
    cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg)
    {
        dwError = EBADF;
        LSA_LOG_ERROR("The received local socket authentication message has no headers in it.");
        BAIL_ON_LSA_ERROR(dwError);
    }
    while (cmsg)
    {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
        {
            memcpy(&credFd, CMSG_DATA(cmsg), sizeof(credFd));
            if (credFd == -1)
            {
                dwError = EBADF;
                LSA_LOG_ERROR("The received local socket authentication message has -1 for the file descriptor.");
                BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        }
        cmsg = CMSG_NXTHDR(&msg, cmsg);
    }
#endif

    /* Fail if we couldn't extract a valid fd from message */
    if (credFd == -1)
    {
        dwError = EBADF;
        LSA_LOG_ERROR("The received local socket authentication message did not have a rights header.");
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Stat the fd to find the uid/gid of the peer socket */
    if (fstat(credFd, &statbuf) != 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Get the path of our unix socket */
    if (getsockname(fd, (struct sockaddr*) &localAddr, &localAddrLen))
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Get the path that the peer unix socket is connected to */
    if (getpeername(credFd, (struct sockaddr*) &credPeerAddr, &credPeerAddrLen))
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Fail if these paths are not the same */
    if (strcmp(localAddr.sun_path, credPeerAddr.sun_path))
    {
        dwError = EPERM;
        BAIL_ON_LSA_ERROR(dwError);
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
