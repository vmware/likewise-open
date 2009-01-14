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

    // Look at LsaRecvCreds for a description of why the fd is sent in a loop
    while (bServerReply == 0)
    {
        /* Send message */
        dwError = LsaSendMsg(fd, &msg);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaWriteData(
                        fd,
                        (PSTR)&bFdTerminator,
                        sizeof(bFdTerminator));
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaReadData(
                        fd,
                        (PSTR)&bServerReply,
                        sizeof(bServerReply),
                        &dwServerReplyLen);
        BAIL_ON_LSA_ERROR(dwError);

        if (dwServerReplyLen != 1 || bServerReply > 1)
        {
            dwError = EBADF;
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (bServerReply == 0)
        {
            LSA_LOG_WARNING("The local socket authentication message was not received by the server. Resending.");
        }
    }

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
    char payload[] = {0, 0};
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

        dwError = LsaRecvMsg(fd, &msg);
        BAIL_ON_LSA_ERROR(dwError);
        if (msg.msg_iovlen < 1 || msg.msg_iov[0].iov_len < 1)
        {
            dwError = EBADF;
            BAIL_ON_LSA_ERROR(dwError);
        }

        switch (((char *)msg.msg_iov[0].iov_base)[0])
        {
            case 0:
                bReply = 1;
                break;
            case 1:
                LSA_LOG_WARNING("The client did not send a local socket authentication message. Requesting a retry from the client.");
                bReply = 0;
                break;
            default:
                dwError = EBADF;
                LSA_LOG_ERROR("Received an invalid fd terminator of %X",
                        ((char *)msg.msg_iov[0].iov_base)[0]);
                BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaWriteData(
            fd,
            (PSTR)&bReply,
            sizeof(bReply));
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaReadData(
        fd,
        payload,
        1,
        &dwReadPayload);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwReadPayload != 1 || payload[0] != 1)
    {
        dwError = EBADF;
        BAIL_ON_LSA_ERROR(dwError);
    }

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
