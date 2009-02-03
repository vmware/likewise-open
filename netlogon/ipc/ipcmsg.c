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
    unsigned char payload[] = {0};
    int credFd = fd;
    struct iovec payload_vec = {0};
    struct msghdr msg = {0};
    BYTE bFdTerminator = 1;
    BYTE bServerReply = 0;
    DWORD dwServerReplyLen = 0;

#ifdef MSGHDR_HAS_MSG_CONTROL
    union
    {
        struct cmsghdr cm;
        char buf[CMSG_SPACE(sizeof(credFd))];
    } buf_un;
    struct cmsghdr *cmsg = NULL;
#endif

    /* Set up dummy payload */
    payload_vec.iov_base = payload;
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

    // Look at LWNetRecvCreds for a description of why the fd is sent in a loop
    while (bServerReply == 0)
    {
        /* Send message */
        dwError = LWNetSendMsg(fd, &msg);
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LWNetWriteData(
                        fd,
                        (PSTR)&bFdTerminator,
                        sizeof(bFdTerminator));
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LWNetReadData(
                        fd,
                        (PSTR)&bServerReply,
                        sizeof(bServerReply),
                        &dwServerReplyLen);
        BAIL_ON_LWNET_ERROR(dwError);

        if (dwServerReplyLen != 1 || bServerReply > 1)
        {
            dwError = EBADF;
            BAIL_ON_LWNET_ERROR(dwError);
        }

        if (bServerReply == 0)
        {
            LWNET_LOG_WARNING("The local socket authentication message was not received by the server. Resending.");
        }
    }

error:

    return dwError;
}

void
LWNetFreeMessageControlFds(
    struct msghdr *pMsg)
{
    int *pFds = NULL;
#ifdef MSGHDR_HAS_MSG_CONTROL
    struct cmsghdr *cmsg = NULL;
    DWORD dwIndex = 0;

    if (pMsg != NULL && (ssize_t)pMsg->msg_controllen > 0)
    {
        cmsg = CMSG_FIRSTHDR(pMsg);
        while (cmsg)
        {
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
            {
                pFds = (int *)CMSG_DATA(cmsg);
                for (dwIndex = 0;
                    dwIndex*sizeof(pFds[0]) <
                            cmsg->cmsg_len - ((char *)pFds - (char *)cmsg);
                    dwIndex++)
                {
                    if (pFds[dwIndex] != -1)
                    {
                        close(pFds[dwIndex]);
                        pFds[dwIndex] = -1;
                    }
                }
            }
            cmsg = CMSG_NXTHDR(pMsg, cmsg);
        }
    }
#else
    if (pMsg->msg_accrightslen == sizeof(int))
    {
        pFds = (int *)msg.msg_accrights;
        if (pFds[0] != -1)
        {
            close(pFds[0]);
            pFds[0] = -1;
        }
    }
#endif
}

DWORD
LWNetRecvCreds(
    int fd,
    uid_t* pUid,
    gid_t* pGid)
{
    DWORD dwError = 0;
    char payload[] = {0, 0};
    // Do not free. This is copied from the control section of msg
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
    DWORD dwReadPayload = 0;
    BYTE bReply = 0;

#ifdef MSGHDR_HAS_MSG_CONTROL
    /* Initialize the fd space to all -1 to indicate it does not have any
     * fds in it.
     */
    memset(&buf_un, -1, sizeof(buf_un));
    msg.msg_control = buf_un.buf;
    msg.msg_controllen = sizeof(buf_un.buf);
#endif

    /* sendmsg is used on the client side to send a file descriptor.
     * Unfortunately, on RHEL 4, the function can fail without reporting
     * any error on the client side. If sendmsg fails, then neither the
     * iov data nor the control data is sent.
     *
     * To compensate for sendmsg, the lsass client follows this behavior:
     * 1. Send fd and 0 (using sendmsg)
     * 2. Send 1 (using send)
     * 3. Read byte. If it is 0, go back to step 1
     *
     * The lsass server follows this behavior:
     * 1. Read fd and byte
     *      If byte is 0 -> reply with 1 and go to step 2
     *      If byte is 1 -> reply with 0 and repeat step 1.
     * 2. Read a 1 back from the client
     */
    while (bReply == 0)
    {
        /* Set up area to receive dummy payload */
        payload_vec.iov_base = &payload;
        payload_vec.iov_len = 1;
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
        if (msg.msg_iovlen < 1 || msg.msg_iov[0].iov_len < 1)
        {
            dwError = EBADF;
            BAIL_ON_LWNET_ERROR(dwError);
        }

        switch (((char *)msg.msg_iov[0].iov_base)[0])
        {
            case 0:
                bReply = 1;
                break;
            case 1:
                LWNET_LOG_WARNING("The client did not send a local socket authentication message. Requesting a retry from the client.");
                bReply = 0;
                LWNetFreeMessageControlFds(&msg);
                break;
            default:
                dwError = EBADF;
                LWNET_LOG_ERROR("Received an invalid fd terminator of %X",
                        ((char *)msg.msg_iov[0].iov_base)[0]);
                BAIL_ON_LWNET_ERROR(dwError);
        }

        dwError = LWNetWriteData(
            fd,
            (PSTR)&bReply,
            sizeof(bReply));
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetReadData(
        fd,
        payload,
        1,
        &dwReadPayload);
    BAIL_ON_LWNET_ERROR(dwError);

    if (dwReadPayload != 1 || payload[0] != 1)
    {
        dwError = EBADF;
        BAIL_ON_LWNET_ERROR(dwError);
    }

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

    LWNetFreeMessageControlFds(&msg);

    return dwError;

error:
	
    *pUid = 0;
    *pGid = 0;

    goto cleanup;
}
