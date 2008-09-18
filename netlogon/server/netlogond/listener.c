/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        listener.c
 *
 * Abstract:
 *
 *        Likewise Site Manager Listener
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 * 
 */
#include "includes.h"

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#define LISTEN_Q (5)
#define MAX_SOCKET_STALETIME 30

static
void*
LWNetSrvHandleConnectionThreadRoutine(
    PVOID pData
    )
{
    if (pData != NULL) {
        pthread_detach(pthread_self());

        // This routine is responsible for closing the descriptor
        LWNetSrvHandleConnection((HANDLE)pData);
    }

    return 0;
}

static
PVOID
LWNetSrvListenerThreadRoutine(
    PVOID pData
    )
{
    DWORD dwError = 0;
    int sockfd = *(int*)pData;
    uid_t peerUID = 0;
    gid_t peerGID = 0;
    HANDLE hConnection = (HANDLE)NULL;
    pthread_t threadId;
    struct sockaddr_un cliaddr;
    SOCKLEN_T len = 0;
    int connfd = -1;
    BOOLEAN bInvalidConnection = FALSE;

    for(;;) {
        bInvalidConnection = FALSE;
        memset(&cliaddr, 0, sizeof(cliaddr));
        len = sizeof(cliaddr);
        connfd = accept(sockfd, (struct sockaddr*)&cliaddr, &len);

        if (LWNetSrvShouldProcessExit())
           goto cleanup;

        if (connfd < 0) {
            if (errno == EPROTO || errno == ECONNABORTED) {

               continue;

            } else if (errno == EINTR) {

               if (LWNetSrvShouldProcessExit()) {

                   goto cleanup;

               } else {

                 continue;

               }

            } else {

               dwError = errno;
               BAIL_ON_LWNET_ERROR(dwError);

            }
        }

        if (!bInvalidConnection) {

           dwError = LWNetSrvOpenConnection(
                           connfd,
                           peerUID,
                           peerGID,
                           &hConnection);
           BAIL_ON_LWNET_ERROR(dwError);

           connfd = -1;

           do
           {
               dwError = pthread_create(&threadId, NULL,
                                        LWNetSrvHandleConnectionThreadRoutine,
                                        (PVOID)hConnection);
               if ( dwError == ENOMEM || dwError == EAGAIN )
               {
                   LWNET_LOG_DEBUG(
                       "LWNET listener out of resources at %s:%d, pthread_create returned %d",
                       __FILE__,
                       __LINE__,
                       dwError);
               }
           } while ( dwError == ENOMEM || dwError == EAGAIN );
           BAIL_ON_LWNET_ERROR(dwError);
           hConnection = (HANDLE)NULL;
           
        }
    }

cleanup:

    if (connfd >= 0) {
        close(connfd);
    }

    if (hConnection != (HANDLE)NULL) {
        LWNetSrvCloseConnection(hConnection);
    }

    if (sockfd != -1)
    {
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
    }

    LWNetFreeMemory(pData);

    return NULL;

error:

    LWNET_LOG_INFO("LWNET listener stopped due to error [Code:%d]", dwError);

    // Make sure to indicate to any signal handling that
    // we are supposed to terminate since the listener
    // is bailing out.

    kill(getpid(),SIGTERM);

    goto cleanup;
}

DWORD
LWNetSrvStartListenThread(
    pthread_t* pThreadId,
    pthread_t** ppThreadId
    )
{
    DWORD dwError = 0;
    PSTR pszCommPath = NULL;
    int sockfd = -1;
    pthread_t threadId;
    BOOLEAN bDirExists = FALSE;
    PSTR pszCachePath = NULL;
    int* pContext = NULL;

    dwError = LWNetSrvGetCachePath(&pszCachePath);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetCheckDirectoryExists(pszCachePath, &bDirExists);
    BAIL_ON_LWNET_ERROR(dwError);

    if (!bDirExists) {
        // Directory should be RWX for root and accessible to all
        // (so they can see the socket.
        mode_t mode = S_IRWXU | S_IRGRP| S_IXGRP | S_IROTH | S_IXOTH;
        dwError = LWNetCreateDirectory(pszCachePath, mode);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetAllocateStringPrintf(&pszCommPath, "%s/%s",
                                        pszCachePath, LWNET_SERVER_FILENAME);
    BAIL_ON_LWNET_ERROR(dwError);

    unlink(pszCommPath);

    sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)    {
        dwError = errno;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    struct sockaddr_un servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strncpy(servaddr.sun_path, pszCommPath, sizeof(servaddr.sun_path) - 1);
    
    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        dwError = errno;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    /*
     * Allow anyone to write to the socket.
     * We check the uids against the messages.
     */
    dwError = LWNetChangeOwnerAndPermissions(
                           pszCommPath,
                           0,
                           0,
                           S_IRWXU|S_IRWXG|S_IRWXO
                           );
    BAIL_ON_LWNET_ERROR(dwError);
    
    if (listen(sockfd, LISTEN_Q) < 0) {
        dwError = errno;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    // Spin up acceptor thread, passing in socket
    dwError = LWNetAllocateMemory(sizeof(*pContext), (PVOID*)&pContext);
    BAIL_ON_LWNET_ERROR(dwError);

    *pContext = sockfd;

    dwError = pthread_create(&threadId, NULL,
                             LWNetSrvListenerThreadRoutine,
                             pContext);
    BAIL_ON_LWNET_ERROR(dwError);
    pContext = NULL;
    sockfd = -1;

error:
    LWNET_SAFE_FREE_STRING(pszCachePath);
    LWNET_SAFE_FREE_STRING(pszCommPath);
    LWNET_SAFE_FREE_MEMORY(pContext);

    if (sockfd != -1)
    {
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
    }

    if (dwError)
    {
        *ppThreadId = NULL;
    }
    else
    {
        *pThreadId = threadId;
        *ppThreadId = pThreadId;
    }

    return dwError;
}

