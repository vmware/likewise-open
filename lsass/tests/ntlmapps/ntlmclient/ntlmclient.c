/*
 * Copyright 1994 by OpenVision Technologies, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of OpenVision not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission. OpenVision makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * OPENVISION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OPENVISION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */


#include "ntlmclient.h"

INT
main(
    IN INT argc,
    IN PCHAR *argv
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCHAR pServiceName = NULL;
    PCHAR pServerHost = NULL;
    PCHAR pMsg = NULL;
    PCHAR pSecPkgName = "NTLM";
    USHORT usPort = 4444;
    INT nSignOnly = 0;

    // These are the negotiation flags... we'll update these later
    //
    DWORD DelegFlag =
        NTLM_FLAG_NEGOTIATE_DEFAULT |
        NTLM_FLAG_SIGN              |
        NTLM_FLAG_SEAL;

    /* Parse arguments. */
    argc--; argv++;
    while(argc > 3)
    {

        if(strcmp(*argv, "-port") == 0)
        {
            argc--; argv++;

            if(!argc)
            {
                dwError = Usage();
                BAIL_ON_LW_ERROR(dwError);
            }

            usPort = (USHORT)atoi(*argv);
        }
        else if(strcmp(*argv, "-sign") == 0)
        {
            nSignOnly = TRUE;
        }
        else
        {
            fprintf(
                stderr,
                "Invalid parameter: %s\n",
                *argv
                );

            dwError = Usage();
            BAIL_ON_LW_ERROR(dwError);
        }
        argc--; argv++;
    }

    if(argc != 3)
    {
        dwError = Usage();
        BAIL_ON_LW_ERROR(dwError);
    }

    if(nSignOnly)
    {
        DelegFlag &= ~NTLM_FLAG_SEAL;
        printf("No encryption - sign only\n");
    }

    pServerHost = *argv++;
    pServiceName = *argv++;
    pMsg = *argv++;

    dwError = CallServer(
        pServerHost,
        usPort,
        pServiceName,
        DelegFlag,
        pMsg,
        pSecPkgName,
        nSignOnly
        );

    BAIL_ON_LW_ERROR(dwError);

error:
    return dwError;
}

DWORD
Usage(VOID)
{
    fprintf(
        stderr,
        "Usage: ntlm-client [-port port] [-sign] host service msg\n"
        );

    return LW_ERROR_INVALID_PARAMETER;
}

DWORD
CallServer(
    IN PCHAR pHost,
    IN USHORT usPort,
    IN PCHAR pServiceName,
    IN DWORD DelegFlag,
    IN PCHAR pMsg,
    IN PCHAR pSecPkgName,
    IN INT nSignOnly
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    SecBuffer WrapBuffers[3];
    INT nSocket = INVALID_SOCKET;
    DWORD RetFlags = 0;
    BOOL bVerified = FALSE;
    BOOL bEncrypted = FALSE;
    INT nContextAcquired = 0;

    CtxtHandle Context;
    SecBuffer InBuffer;
    SecBuffer OutBuffer;
    SecBufferDesc InBufferDesc;
    SecPkgContext_Sizes Sizes;

    memset(&Context, 0, sizeof(CtxtHandle));
    memset(&InBuffer, 0, sizeof(SecBuffer));
    memset(&OutBuffer, 0, sizeof(SecBuffer));
    memset(&InBufferDesc, 0, sizeof(SecBufferDesc));
    memset(&Sizes, 0, sizeof(SecPkgContext_Sizes));
    memset(WrapBuffers, 0, sizeof(SecBuffer)*3);

    /* Open connection */

    dwError = ConnectToServer(pHost, usPort, &nSocket);
    BAIL_ON_LW_ERROR(dwError);

    /* Establish context */
    dwError = ClientEstablishContext(
            nSocket,
            pServiceName,
            DelegFlag,
            &Context,
            pSecPkgName,
            &RetFlags
            );

    BAIL_ON_LW_ERROR(dwError);

    nContextAcquired = 1;

    dwError = NtlmClientQueryContextAttributes(
        &Context,
        SECPKG_ATTR_SIZES,
        &Sizes
        );

    BAIL_ON_LW_ERROR(dwError);

    /* Seal the message */
    InBuffer.pvBuffer = pMsg;
    InBuffer.cbBuffer = (DWORD)strlen(pMsg) + 1;

    //
    // Prepare to encrypt the message
    //

    InBufferDesc.cBuffers = 3;
    InBufferDesc.pBuffers = WrapBuffers;
    //InBufferDesc.ulVersion = SECBUFFER_VERSION;

    WrapBuffers[0].cbBuffer = Sizes.cbSecurityTrailer;
    WrapBuffers[0].BufferType = SECBUFFER_TOKEN;
    WrapBuffers[0].pvBuffer = malloc(Sizes.cbSecurityTrailer);

    if(WrapBuffers[0].pvBuffer == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LW_ERROR(dwError);
    }

    WrapBuffers[1].BufferType = SECBUFFER_DATA;
    WrapBuffers[1].cbBuffer = InBuffer.cbBuffer;
    WrapBuffers[1].pvBuffer = malloc(WrapBuffers[1].cbBuffer);

    if(WrapBuffers[1].pvBuffer == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LW_ERROR(dwError);
    }

    memcpy(
        WrapBuffers[1].pvBuffer,
        InBuffer.pvBuffer,
        InBuffer.cbBuffer
        );

    WrapBuffers[2].BufferType = SECBUFFER_PADDING;
    WrapBuffers[2].cbBuffer = Sizes.cbBlockSize;
    WrapBuffers[2].pvBuffer = malloc(WrapBuffers[2].cbBuffer);

    if(WrapBuffers[2].pvBuffer == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = NtlmClientEncryptMessage(
        &Context,
        !nSignOnly,
        &InBufferDesc,
        0
        );

    BAIL_ON_LW_ERROR(dwError);

    //
    // Create the mesage to send to server
    //

    OutBuffer.cbBuffer = WrapBuffers[0].cbBuffer + WrapBuffers[1].cbBuffer + WrapBuffers[2].cbBuffer;
    OutBuffer.pvBuffer = malloc(OutBuffer.cbBuffer);

    if(OutBuffer.pvBuffer == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LW_ERROR(dwError);
    }

    memcpy(
        OutBuffer.pvBuffer,
        WrapBuffers[0].pvBuffer,
        WrapBuffers[0].cbBuffer
        );
    memcpy(
        (PUCHAR) OutBuffer.pvBuffer + (int) WrapBuffers[0].cbBuffer,
        WrapBuffers[1].pvBuffer,
        WrapBuffers[1].cbBuffer
        );
    memcpy(
        (PUCHAR) OutBuffer.pvBuffer + WrapBuffers[0].cbBuffer + WrapBuffers[1].cbBuffer,
        WrapBuffers[2].pvBuffer,
        WrapBuffers[2].cbBuffer
        );

    /* Send to server */
    dwError = SendToken(nSocket, &OutBuffer);
    BAIL_ON_LW_ERROR(dwError);

    free(OutBuffer.pvBuffer);
    OutBuffer.pvBuffer = NULL;
    OutBuffer.cbBuffer = 0;
    free(WrapBuffers[0].pvBuffer);
    WrapBuffers[0].pvBuffer = NULL;
    free(WrapBuffers[1].pvBuffer);
    WrapBuffers[1].pvBuffer = NULL;

    /* Read signature block into OutBuffer */
    dwError = RecvToken(nSocket, &OutBuffer);
    BAIL_ON_LW_ERROR(dwError);

    /* Verify signature block */

    InBufferDesc.cBuffers = 2;
    WrapBuffers[0] = InBuffer;
    WrapBuffers[0].BufferType = SECBUFFER_DATA;
    WrapBuffers[1] = OutBuffer;
    WrapBuffers[1].BufferType = SECBUFFER_TOKEN;

    dwError = NtlmClientVerifySignature(
        &Context,
        &InBufferDesc,
        0,
        &bVerified,
        &bEncrypted
        );
    BAIL_ON_LW_ERROR(dwError);

    free(OutBuffer.pvBuffer);
    OutBuffer.pvBuffer = NULL;

    /* Delete context */
    dwError = NtlmClientDeleteSecurityContext(&Context);
    BAIL_ON_LW_ERROR(dwError);

    close(nSocket);

finish:
    return dwError;
error:
    if(nContextAcquired)
    {
        NtlmClientDeleteSecurityContext(&Context);
    }
    if(INVALID_SOCKET != nSocket)
    {
        close(nSocket);
    }
    if(WrapBuffers[0].pvBuffer)
    {
        free(WrapBuffers[0].pvBuffer);
    }
    if(WrapBuffers[1].pvBuffer)
    {
        free(WrapBuffers[1].pvBuffer);
    }
    if(WrapBuffers[2].pvBuffer)
    {
        free(WrapBuffers[2].pvBuffer);
    }
    if(OutBuffer.pvBuffer)
    {
        free(OutBuffer.pvBuffer);
    }

    goto finish;
}

DWORD
ConnectToServer(
    IN PCHAR pHost,
    IN USHORT usPort,
    OUT PINT pSocket
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    struct sockaddr_in sAddr;
    struct hostent *pHostEnt;

    memset(&sAddr, 0, sizeof(struct sockaddr_in));

    *pSocket = INVALID_SOCKET;

    pHostEnt = gethostbyname(pHost);

    if(pHostEnt == NULL)
    {
        dwError = LwMapErrnoToLwError(h_errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    sAddr.sin_family = pHostEnt->h_addrtype;
    memcpy((PCHAR)&sAddr.sin_addr, pHostEnt->h_addr, sizeof(sAddr.sin_addr));
    sAddr.sin_port = htons(usPort);

    *pSocket = (INT)socket(PF_INET, SOCK_STREAM, 0);
    if(INVALID_SOCKET == *pSocket)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = connect(*pSocket, (struct sockaddr *)&sAddr, sizeof(sAddr));
    if(dwError == SOCKET_ERROR)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

finish:
    return dwError;
error:
    if(INVALID_SOCKET != *pSocket)
    {
        close(*pSocket);
        *pSocket = INVALID_SOCKET;
    }
    goto finish;
}

DWORD
ClientEstablishContext(
    IN INT nSocket,
    IN PCHAR pServiceName,
    IN DWORD DelegFlag,
    OUT CtxtHandle *pSspiContext,
    IN PCHAR pSecPkgName,
    OUT DWORD *pRetFlags
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLoopError = LW_ERROR_SUCCESS;
    PCtxtHandle pContextHandle = NULL;
    INT nCredentialsAcquired = 0;
    INT nContextAcquired = 0;

    SecBuffer SendTokenBuffer;
    SecBuffer RecvTokenBuffer;
    SecBufferDesc InputDesc;
    SecBufferDesc OutputDesc;
    CredHandle CredHandle;
    TimeStamp Expiry;

    memset(&SendTokenBuffer, 0, sizeof(SecBuffer));
    memset(&RecvTokenBuffer, 0, sizeof(SecBuffer));
    memset(&InputDesc, 0, sizeof(SecBufferDesc));
    memset(&OutputDesc, 0, sizeof(SecBufferDesc));
    memset(&CredHandle, 0, sizeof(CredHandle));
    memset(&Expiry, 0, sizeof(TimeStamp));

    memset(pSspiContext, 0, sizeof(CtxtHandle));
    *pRetFlags = 0;

    InputDesc.cBuffers = 1;
    InputDesc.pBuffers = &RecvTokenBuffer;
    //InputDesc.ulVersion = SECBUFFER_VERSION;

    RecvTokenBuffer.BufferType = SECBUFFER_TOKEN;
    RecvTokenBuffer.cbBuffer = 0;
    RecvTokenBuffer.pvBuffer = NULL;

    OutputDesc.cBuffers = 1;
    OutputDesc.pBuffers = &SendTokenBuffer;
    //OutputDesc.ulVersion = SECBUFFER_VERSION;

    SendTokenBuffer.BufferType = SECBUFFER_TOKEN;
    SendTokenBuffer.cbBuffer = 0;
    SendTokenBuffer.pvBuffer = NULL;

    CredHandle.dwLower = 0;
    CredHandle.dwUpper = 0;

    dwError = NtlmClientAcquireCredentialsHandle(
        NULL,                       // no principal name
        pSecPkgName,                // package name
        0, //SECPKG_CRED_OUTBOUND,
        NULL,                       // no logon id
        NULL,                       // no auth data
        &CredHandle,
        &Expiry
        );

    BAIL_ON_LW_ERROR(dwError);

    nCredentialsAcquired = 1;

   /*
    * Perform the context-establishement loop.
    */

    pSspiContext->dwLower = 0;
    pSspiContext->dwUpper = 0;

    do
    {
        // we need to use dwLoopErr in this case because we may get
        // back a "continue" command.  In those cases, we still
        // need dwError to be used and set seperatly based on other
        // calls.
        dwLoopError =
            NtlmClientInitializeSecurityContext(
                &CredHandle,
                pContextHandle,
                pServiceName,
                DelegFlag,
                0,          // reserved
                0, //SECURITY_NATIVE_DREP,
                &InputDesc,
                0,          // reserved
                pSspiContext,  // <-- this is the handle to the data returned in OutputDesc... it's used to make look ups faster.
                &OutputDesc,   // <-- this is the data the above handle represents
                pRetFlags,
                &Expiry
                );

        if(LW_ERROR_SUCCESS != dwLoopError && LW_WARNING_CONTINUE_NEEDED != dwLoopError)
        {
            dwError = dwLoopError;
            BAIL_ON_LW_ERROR(dwError);
        }

        nContextAcquired = 1;

        pContextHandle = pSspiContext;

        if(RecvTokenBuffer.pvBuffer)
        {
            free(RecvTokenBuffer.pvBuffer);
            RecvTokenBuffer.pvBuffer = NULL;
            RecvTokenBuffer.cbBuffer = 0;
        }

        if(SendTokenBuffer.cbBuffer != 0)
        {
            dwError = SendToken(nSocket, &SendTokenBuffer);
            BAIL_ON_LW_ERROR(dwError);
        }

        FreeContextBuffer(SendTokenBuffer.pvBuffer);
        SendTokenBuffer.pvBuffer = NULL;
        SendTokenBuffer.cbBuffer = 0;

        if(LW_WARNING_CONTINUE_NEEDED == dwLoopError)
        {
            dwError = RecvToken(nSocket, &RecvTokenBuffer);
            BAIL_ON_LW_ERROR(dwError);
        }

    } while (dwLoopError == LW_WARNING_CONTINUE_NEEDED);

    NtlmClientFreeCredentialsHandle(&CredHandle);

finish:
    return dwError;
error:
    if(nCredentialsAcquired)
    {
        NtlmClientFreeCredentialsHandle(&CredHandle);
    }
    if(nContextAcquired)
    {
        NtlmClientDeleteSecurityContext(pSspiContext);
        memset(pSspiContext, 0, sizeof(CtxtHandle));
    }
    if(RecvTokenBuffer.pvBuffer)
    {
        free(RecvTokenBuffer.pvBuffer);
    }
    if(SendTokenBuffer.cbBuffer)
    {
        FreeContextBuffer(SendTokenBuffer.pvBuffer);
    }

    goto finish;
}

DWORD
FreeContextBuffer(
    IN PVOID pBuffer
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    if(pBuffer)
    {
        free(pBuffer);
    }

    return dwError;
}

DWORD
SendToken(
    IN INT nSocket,
    IN PSecBuffer pToken
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLen = 0;
    INT nBytesWritten = 0;

    dwLen = htonl(pToken->cbBuffer);

    dwError = WriteAll(
        nSocket,
        (PCHAR)&dwLen,
        4,
        &nBytesWritten
        );

    BAIL_ON_LW_ERROR(dwError);

    if(4 != nBytesWritten)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = WriteAll(
        nSocket,
        pToken->pvBuffer,
        pToken->cbBuffer,
        &nBytesWritten
        );

    BAIL_ON_LW_ERROR(dwError);

    if(nBytesWritten != pToken->cbBuffer)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LW_ERROR(dwError);
    }

error:
    return dwError;
}

DWORD
WriteAll(
    IN INT nSocket,
    IN PCHAR pBuffer,
    IN UINT nBytes,
    OUT PINT nBytesWritten
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    INT nReturn = 0;
    PCHAR pTrav = NULL;

    *nBytesWritten = 0;

    for(pTrav = pBuffer; nBytes; pTrav += nReturn, nBytes -= nReturn)
    {
        nReturn = send(nSocket, pTrav, nBytes, 0);

        if(nReturn < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LW_ERROR(dwError);
        }

        if(nReturn == 0)
        {
            break;
        }
    }

    *nBytesWritten = pTrav - pBuffer;

error:
    return dwError;
}

DWORD
RecvToken(
    IN INT nSocket,
    OUT PSecBuffer pToken
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    INT nBytesRead = 0;

    memset(pToken, 0, sizeof(SecBuffer));

    dwError = ReadAll(
        nSocket,
        (PCHAR)&pToken->cbBuffer,
        4,
        &nBytesRead
        );

    BAIL_ON_LW_ERROR(dwError);

    if(4 != nBytesRead)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LW_ERROR(dwError);
    }

    pToken->cbBuffer = ntohl(pToken->cbBuffer);
    pToken->pvBuffer = (PCHAR) malloc(pToken->cbBuffer);

    if(pToken->pvBuffer == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = ReadAll(
        nSocket,
        (PCHAR)pToken->pvBuffer,
        pToken->cbBuffer,
        &nBytesRead
        );

    BAIL_ON_LW_ERROR(dwError);

    if(nBytesRead != pToken->cbBuffer)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LW_ERROR(dwError);
    }

finish:
    return dwError;
error:
    if(pToken->pvBuffer)
    {
        free(pToken->pvBuffer);
        pToken->pvBuffer = NULL;
    }
    goto finish;
}

DWORD
ReadAll(
    IN INT nSocket,
    OUT PCHAR pBuffer,
    IN UINT nBytes,
    OUT PINT nBytesRead
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    int nReturn = 0;
    char *pTrav = NULL;

    memset(pBuffer, 0, nBytes);
    *nBytesRead = 0;

    for(pTrav = pBuffer; nBytes; pTrav += nReturn, nBytes -= nReturn)
    {
        nReturn = recv(nSocket, pTrav, nBytes, 0);

        if(nReturn < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LW_ERROR(dwError);
        }

        if(nReturn == 0)
        {
            break;
        }
    }

    *nBytesRead = pTrav - pBuffer;

error:
    return dwError;
}
