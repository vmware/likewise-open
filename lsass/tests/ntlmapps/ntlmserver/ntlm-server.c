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

#include "ntlm-server.h"

INT
main(
    IN INT      argc,
    IN PCHAR*   argv
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    INT nListenSocket = INVALID_SOCKET;
    INT nAcceptSocket = INVALID_SOCKET;
    USHORT usPort = 4444;
    INT nOnce = 0;
    PCHAR pServiceName = NULL;
    PCHAR pServicePassword = NULL;
    PCHAR pServiceRealm = NULL;
    LSA_CRED_HANDLE ServerCreds;
    INT nServerCredsAcquired = 0;
    DWORD AscFlags = 0; //ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_MUTUAL_AUTH;
    PCHAR pSecPkgName = "NTLM";
    BOOL bFound = FALSE;

    ServerCreds = NULL;

    argc--; argv++;

    while(argc)
    {
        if(strcmp(*argv, "-port") == 0)
        {
            argc--; argv++;
            if(!argc)
            {
                dwError = Usage();
                BAIL_ON_LW_ERROR(dwError);
            }
            usPort = (u_short)atoi(*argv);
        }
        else if(strcmp(*argv, "-once") == 0)
        {
            nOnce = 1;
        }
        else
        {
            /*
            for(i = 0; i < (sizeof(FlagMappings)/sizeof(FLAGMAPPING)); i++)
            {
                if(_strcmpi( *argv, FlagMappings[ i ].name ) == 0)
                {
                    bFound = TRUE;
                    AscFlags |= FlagMappings[ i ].value ;
                    break;
                }
            }
            */

            if(!bFound)
            {
                break;
            }
        }

        argc--; argv++;
    }
    if(argc != 3)
    {
        dwError = Usage();
        BAIL_ON_LW_ERROR(dwError);
    }

    if((*argv)[0] == '-')
    {
        dwError = Usage();
        BAIL_ON_LW_ERROR(dwError);
    }

    pServiceName = *argv;
    argv++;
    pServicePassword = *argv;
    argv++;
    pServiceRealm = *argv;

    dwError = ServerAcquireCreds(
        pServiceName,
        pServicePassword,
        pServiceRealm,
        pSecPkgName,
        &ServerCreds
        );

    BAIL_ON_LW_ERROR(dwError);

    nServerCredsAcquired = 1;

    dwError = CreateSocket(usPort, &nListenSocket);

    BAIL_ON_LW_ERROR(dwError);

    do
    {
        /* Accept a TCP connection */
        nAcceptSocket = accept(nListenSocket, NULL, 0);

        if(INVALID_SOCKET == nAcceptSocket)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LW_ERROR(dwError);
        }

        dwError = SignServer(
            nAcceptSocket,
            &ServerCreds,
            AscFlags
            );

        BAIL_ON_LW_ERROR(dwError);

        close(nAcceptSocket);

    } while (!nOnce);

    close(nListenSocket);

    NtlmClientFreeCredentialsHandle(&ServerCreds);

finish:
    return dwError;
error:
    if(nServerCredsAcquired)
    {
        NtlmClientFreeCredentialsHandle(&ServerCreds);
    }
    if(INVALID_SOCKET != nListenSocket)
    {
        close(nListenSocket);
    }
    if(INVALID_SOCKET != nAcceptSocket)
    {
        close(nAcceptSocket);
    }
    goto finish;
}

DWORD
Usage(
    VOID
    )
{
    fprintf(stderr, "Usage: sspi-server [-port port] [-k]\n");
    fprintf(stderr, "       [service_name] [service_password] [service_realm]\n");

    return LW_ERROR_INVALID_PARAMETER;
}

DWORD
ServerAcquireCreds(
    IN PCHAR                pServiceName,
    IN PCHAR                pServicePassword,
    IN PCHAR                pServiceRealm,
    IN PCHAR                pSecPkgName,
    OUT PLSA_CRED_HANDLE    pServerCreds
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    TimeStamp Expiry;

    SEC_WINNT_AUTH_IDENTITY AuthIdentity;

    memset(&Expiry, 0, sizeof(TimeStamp));
    memset(&AuthIdentity, 0, sizeof(SEC_WINNT_AUTH_IDENTITY));

    pServerCreds = NULL;

    AuthIdentity.Password = pServicePassword;
    AuthIdentity.PasswordLength = (DWORD)strlen(pServicePassword);

    AuthIdentity.Domain = pServiceRealm;
    AuthIdentity.DomainLength = (DWORD)strlen(pServiceRealm);
    //AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;


    dwError = NtlmClientAcquireCredentialsHandle(
        pServiceName,
        pSecPkgName,
        0,//SECPKG_CRED_INBOUND,
        NULL,                       // no logon id
        &AuthIdentity,              // auth data
        pServerCreds,
        &Expiry
        );

    BAIL_ON_LW_ERROR(dwError);

error:
    return dwError;
}

DWORD
CreateSocket(
    IN USHORT   uPort,
    OUT PINT    pSocket
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    struct sockaddr_in sAddr;
    INT nOn = 1;

    *pSocket = INVALID_SOCKET;
    memset(&sAddr, 0, sizeof(struct sockaddr_in));

    sAddr.sin_family = AF_INET;
    sAddr.sin_port = htons(uPort);
    sAddr.sin_addr.s_addr = INADDR_ANY;

    *pSocket = (int)socket(PF_INET, SOCK_STREAM, 0);

    if(INVALID_SOCKET == *pSocket)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    /* Let the socket be reused right away */
    dwError = setsockopt
        (
        *pSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        (char *)&nOn,
        sizeof(nOn)
        );

    if(dwError != 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = bind(
        *pSocket,
        (struct sockaddr *) &sAddr,
        sizeof(sAddr)
        );

    if(SOCKET_ERROR == dwError)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = listen(*pSocket, 5);

    if(SOCKET_ERROR == dwError)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

finish:
    return dwError;
error:
    if(*pSocket != INVALID_SOCKET)
    {
        close(*pSocket);
        *pSocket = INVALID_SOCKET;
    }

    goto finish;
}

DWORD
SignServer(
    IN INT              nSocket,
    IN PLSA_CRED_HANDLE pServerCreds,
    IN DWORD            AscFlags
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOL bEncrypted = 0;
    INT nContextAcquired = 0;
    SecBuffer TransmitBuffer;
    SecBuffer MsgBuffer;
    SecBuffer WrapBuffers[2];
    SecBufferDesc WrapBufferDesc;
    LSA_CONTEXT_HANDLE Context = NULL;
    SecPkgContext_Sizes Sizes;

    memset(&TransmitBuffer, 0, sizeof(SecBuffer));
    memset(&MsgBuffer, 0, sizeof(SecBuffer));
    memset(WrapBuffers, 0, sizeof(SecBuffer) * 2);
    memset(&WrapBufferDesc, 0, sizeof(SecBufferDesc));
    memset(&Sizes, 0, sizeof(SecPkgContext_Sizes));

    /* Establish a context with the client */
    dwError = ServerEstablishContext(
        nSocket,
        pServerCreds,
        &Context,
        AscFlags
        );

    BAIL_ON_LW_ERROR(dwError);

    // for clean up... once we've established a context, we must clean it up on
    // future failures.
    nContextAcquired = 1;

    dwError = NtlmClientQueryContextAttributes(
        &Context,
        SECPKG_ATTR_SIZES,
        &Sizes
        );

    BAIL_ON_LW_ERROR(dwError);

    /* Receive the sealed message token */
    dwError = RecvToken(nSocket, &TransmitBuffer);
    BAIL_ON_LW_ERROR(dwError);

    WrapBufferDesc.cBuffers = 2;
    WrapBufferDesc.pBuffers = WrapBuffers;
    //WrapBufferDesc.ulVersion = SECBUFFER_VERSION;
    WrapBuffers[0].BufferType = SECBUFFER_STREAM;
    WrapBuffers[0].pvBuffer = TransmitBuffer.pvBuffer;
    WrapBuffers[0].cbBuffer = TransmitBuffer.cbBuffer;
    WrapBuffers[1].BufferType = SECBUFFER_DATA;
    WrapBuffers[1].cbBuffer = 0;
    WrapBuffers[1].pvBuffer = NULL;

    dwError = NtlmClientDecryptMessage(
        &Context,
        &WrapBufferDesc,
        0,                  // no sequence number
        &bEncrypted
        );

    BAIL_ON_LW_ERROR(dwError);

    MsgBuffer = WrapBuffers[1];

    /* Produce a signature block for the message */

    WrapBuffers[0] = MsgBuffer;

    WrapBuffers[1].BufferType = SECBUFFER_TOKEN;
    WrapBuffers[1].cbBuffer = Sizes.cbMaxSignature;
    WrapBuffers[1].pvBuffer = malloc(Sizes.cbMaxSignature);

    if(WrapBuffers[1].pvBuffer == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = NtlmClientMakeSignature(
        &Context,
        FALSE,
        &WrapBufferDesc,
        0
        );

    BAIL_ON_LW_ERROR(dwError);

    free(TransmitBuffer.pvBuffer);

    TransmitBuffer = WrapBuffers[1];
    WrapBuffers[1].pvBuffer = NULL;
    WrapBuffers[1].cbBuffer = 0;

    /* Send the signature block to the client */

    dwError = SendToken(nSocket, &TransmitBuffer);
    BAIL_ON_LW_ERROR(dwError);

    free(TransmitBuffer.pvBuffer);
    TransmitBuffer.pvBuffer = NULL;
    TransmitBuffer.cbBuffer = 0;

    /* Delete context */

    dwError = NtlmClientDeleteSecurityContext( &Context );
    BAIL_ON_LW_ERROR(dwError);

finish:
    return dwError;
error:
    if(TransmitBuffer.pvBuffer)
    {
        free(TransmitBuffer.pvBuffer);
        TransmitBuffer.pvBuffer = NULL;
        TransmitBuffer.cbBuffer = 0;
    }
    if(WrapBuffers[1].pvBuffer)
    {
        free(WrapBuffers[1].pvBuffer);
        WrapBuffers[1].pvBuffer = NULL;
        WrapBuffers[1].cbBuffer = 0;
    }
    if(nContextAcquired)
    {
        NtlmClientDeleteSecurityContext(&Context);
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
ServerEstablishContext(
    IN INT                  nSocket,
    IN PLSA_CRED_HANDLE     pServerCreds,
    OUT PLSA_CONTEXT_HANDLE pContext,
    IN DWORD                AscFlags
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLoopError = LW_ERROR_SUCCESS;
    DWORD nRetFlags = 0;
    SecBufferDesc InputDesc;
    SecBufferDesc OutputDesc;
    SecBuffer SendTokenBuffer;
    SecBuffer RecvTokenBuffer;
    TimeStamp Expiry;
    PLSA_CONTEXT_HANDLE pContextHandle = NULL;
    INT nContextAcquired = 0;

    memset(&InputDesc, 0, sizeof(SecBufferDesc));
    memset(&OutputDesc, 0, sizeof(SecBufferDesc));
    memset(&SendTokenBuffer, 0, sizeof(SecBuffer));
    memset(&RecvTokenBuffer, 0, sizeof(SecBuffer));
    memset(&Expiry, 0, sizeof(TimeStamp));

    InputDesc.cBuffers = 1;
    //InputDesc.ulVersion = SECBUFFER_VERSION;
    InputDesc.pBuffers = &RecvTokenBuffer;

    OutputDesc.cBuffers = 1;
    //OutputDesc.ulVersion = SECBUFFER_VERSION;
    OutputDesc.pBuffers = &SendTokenBuffer;

    do
    {
        dwError = RecvToken(nSocket, &RecvTokenBuffer);
        BAIL_ON_LW_ERROR(dwError);

        RecvTokenBuffer.BufferType = SECBUFFER_TOKEN;
        SendTokenBuffer.cbBuffer = 0;
        SendTokenBuffer.pvBuffer = NULL;
        SendTokenBuffer.BufferType = SECBUFFER_TOKEN;

        // we need to use dwLoopErr in this case because we may get
        // back a "continue" command.  In those cases, we still
        // need dwError to be used and set seperatly based on other
        // calls.
        dwLoopError = NtlmClientAcceptSecurityContext(
            pServerCreds,
            pContextHandle,
            &InputDesc,
            AscFlags,
            0, //SECURITY_NATIVE_DREP,
            pContext,
            &OutputDesc,
            &nRetFlags,
            &Expiry
            );

        if(LW_ERROR_SUCCESS != dwLoopError && LW_WARNING_CONTINUE_NEEDED != dwLoopError)
        {
            dwError = dwLoopError;
            BAIL_ON_LW_ERROR(dwError);
        }

        nContextAcquired = 1;

        pContextHandle = pContext;
        free(RecvTokenBuffer.pvBuffer);
        RecvTokenBuffer.pvBuffer = NULL;

        if(SendTokenBuffer.cbBuffer != 0)
        {
            dwError = SendToken(nSocket, &SendTokenBuffer);
            BAIL_ON_LW_ERROR(dwError);

            FreeContextBuffer(SendTokenBuffer.pvBuffer);
            SendTokenBuffer.pvBuffer = NULL;
        }

    } while(dwLoopError == LW_WARNING_CONTINUE_NEEDED);

finish:
    return dwError;
error:
    if(RecvTokenBuffer.pvBuffer)
    {
        free(RecvTokenBuffer.pvBuffer);
        RecvTokenBuffer.pvBuffer = NULL;
    }
    if(SendTokenBuffer.cbBuffer)
    {
        FreeContextBuffer(SendTokenBuffer.pvBuffer);
        SendTokenBuffer.pvBuffer = NULL;
        SendTokenBuffer.cbBuffer = 0;
    }
    if(nContextAcquired)
    {
        NtlmClientDeleteSecurityContext(pContext);
    }
    goto finish;
}

DWORD
SendToken(
    IN INT          nSocket,
    IN PSecBuffer   pToken
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
    IN INT      nSocket,
    IN PCHAR    pBuffer,
    IN UINT     nBytes,
    OUT PINT    nBytesWritten
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
    IN INT          nSocket,
    OUT PSecBuffer  pToken
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
    pToken->pvBuffer = (char *) malloc(pToken->cbBuffer);

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
    }
    goto finish;
}

DWORD
ReadAll(
    IN INT      nSocket,
    OUT PCHAR   pBuffer,
    IN UINT     nBytes,
    OUT PINT    nBytesRead
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    INT nReturn = 0;
    PCHAR pTrav = NULL;

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
