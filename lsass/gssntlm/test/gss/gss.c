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
 *        gss.c
 *
 * Abstract:
 *
 *        GSS API tests for GSSNTLM
 *
 * Authors: Todd Stecher (v-todds@likewise.com)
 *
 */

#include <server.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "../include/framework.h"
#include <gssntlm.h>

#define USERNAME "Freddy"
#define DOMAINNAME "xyz" 
#define PASSWORD "password123"
#define NULLPASSWORD ""
#define MAX_TOKEN 2000
#define TEST_MESSAGE_SERVER "Not much, fool!!"
#define TEST_MESSAGE_CLIENT "What's up, sucker!!"

DWORD g_tn = 0xFFFF;
DWORD cur_tn;
DWORD g_verbosity = TRACE_2;

#ifndef INADDR_NONE
#define INADDR_NONE ((in_addr_t) -1)
#endif


BYTE v2Response[]=
{

    0x29,0xca,0xf6,0x6c,0x86,0x64,0x51,0xe7,
    0xb7,0xd5,0xc0,0xa3,0xc8,0x51,0xc0,0xdf,
    0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
    0x5f,0x12,0x08,0x05,0x03,0x78,0xc8,0x01,
    0xf6,0x9a,0x97,0x69,0x68,0x1d,0x43,0xf8,
    0x00,0x00,0x00,0x00,0x02,0x00,0x0c,0x00,
    0x54,0x00,0x53,0x00,0x32,0x00,0x30,0x00,
    0x30,0x00,0x33,0x00,0x01,0x00,0x0a,0x00,
    0x54,0x00,0x53,0x00,0x30,0x00,0x30,0x00,
    0x33,0x00,0x04,0x00,0x22,0x00,0x74,0x00,
    0x73,0x00,0x32,0x00,0x30,0x00,0x30,0x00,
    0x33,0x00,0x2e,0x00,0x69,0x00,0x73,0x00,
    0x69,0x00,0x6c,0x00,0x6f,0x00,0x6e,0x00,
    0x2e,0x00,0x63,0x00,0x6f,0x00,0x6d,0x00,
    0x03,0x00,0x2e,0x00,0x74,0x00,0x73,0x00,
    0x30,0x00,0x30,0x00,0x33,0x00,0x2e,0x00,
    0x74,0x00,0x73,0x00,0x32,0x00,0x30,0x00,
    0x30,0x00,0x33,0x00,0x2e,0x00,0x69,0x00,
    0x73,0x00,0x69,0x00,0x6c,0x00,0x6f,0x00,
    0x6e,0x00,0x2e,0x00,0x63,0x00,0x6f,0x00,
    0x6d,0x00,0x05,0x00,0x22,0x00,0x74,0x00,
    0x73,0x00,0x32,0x00,0x30,0x00,0x30,0x00,
    0x33,0x00,0x2e,0x00,0x69,0x00,0x73,0x00,
    0x69,0x00,0x6c,0x00,0x6f,0x00,0x6e,0x00,
    0x2e,0x00,0x63,0x00,0x6f,0x00,0x6d,0x00,
    0x08,0x00,0x30,0x00,0x30,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x00,0x20,0x00,0x00,0x10,0x46,0x08,0x0e,
    0x7f,0xb3,0x55,0xec,0x03,0x56,0x56,0xb7,
    0x92,0x70,0xd9,0xbb,0x4f,0xb8,0x19,0x34,
    0xc6,0x1d,0xd3,0xff,0x44,0x67,0x4f,0xbd,
    0xc5,0x6b,0x90,0x35,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00
};

USHORT v2ResponseSize = sizeof(v2Response)/sizeof(v2Response[0]);

BYTE v2PartResponse[]=
{

    0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
    0x5f,0x12,0x08,0x05,0x03,0x78,0xc8,0x01,
    0xf6,0x9a,0x97,0x69,0x68,0x1d,0x43,0xf8,
    0x00,0x00,0x00,0x00,0x02,0x00,0x0c,0x00,
    0x54,0x00,0x53,0x00,0x32,0x00,0x30,0x00,
    0x30,0x00,0x33,0x00,0x01,0x00,0x0a,0x00,
    0x54,0x00,0x53,0x00,0x30,0x00,0x30,0x00,
    0x33,0x00,0x04,0x00,0x22,0x00,0x74,0x00,
    0x73,0x00,0x32,0x00,0x30,0x00,0x30,0x00,
    0x33,0x00,0x2e,0x00,0x69,0x00,0x73,0x00,
    0x69,0x00,0x6c,0x00,0x6f,0x00,0x6e,0x00,
    0x2e,0x00,0x63,0x00,0x6f,0x00,0x6d,0x00,
    0x03,0x00,0x2e,0x00,0x74,0x00,0x73,0x00,
    0x30,0x00,0x30,0x00,0x33,0x00,0x2e,0x00,
    0x74,0x00,0x73,0x00,0x32,0x00,0x30,0x00,
    0x30,0x00,0x33,0x00,0x2e,0x00,0x69,0x00,
    0x73,0x00,0x69,0x00,0x6c,0x00,0x6f,0x00,
    0x6e,0x00,0x2e,0x00,0x63,0x00,0x6f,0x00,
    0x6d,0x00,0x05,0x00,0x22,0x00,0x74,0x00,
    0x73,0x00,0x32,0x00,0x30,0x00,0x30,0x00,
    0x33,0x00,0x2e,0x00,0x69,0x00,0x73,0x00,
    0x69,0x00,0x6c,0x00,0x6f,0x00,0x6e,0x00,
    0x2e,0x00,0x63,0x00,0x6f,0x00,0x6d,0x00,
    0x08,0x00,0x30,0x00,0x30,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x00,0x20,0x00,0x00,0x10,0x46,0x08,0x0e,
    0x7f,0xb3,0x55,0xec,0x03,0x56,0x56,0xb7,
    0x92,0x70,0xd9,0xbb,0x4f,0xb8,0x19,0x34,
    0xc6,0x1d,0xd3,0xff,0x44,0x67,0x4f,0xbd,
    0xc5,0x6b,0x90,0x35,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00
};

USHORT v2PartResponseSize = sizeof(v2PartResponse)/sizeof(v2PartResponse[0]);


BYTE v2Challenge[]=
{
    0x50,0x9e,0xb1,0xc3,0x80,0x48,0xd8,0x1d
};

USHORT v2ChallengeSize = sizeof(v2Challenge)/sizeof(v2Challenge[0]);




#define USER "ADMINISTRATOR"
#define DOMAIN "TS08"
#define R_PASSWORD "l3bb#"

BYTE v2CliChal[] = {
        0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
        0x80,0x77,0x8d,0xdd,0x58,0xed,0xc8,0x01,
        0xca,0xd3,0x5c,0xd6,0x10,0xa2,0x02,0xde,
        0x00,0x00,0x00,0x00,0x02,0x00,0x08,0x00,
        0x54,0x00,0x53,0x00,0x30,0x00,0x38,0x00,
        0x01,0x00,0x08,0x00,0x54,0x00,0x53,0x00,
        0x30,0x00,0x38,0x00,0x04,0x00,0x08,0x00,
        0x74,0x00,0x73,0x00,0x30,0x00,0x38,0x00,
        0x03,0x00,0x08,0x00,0x74,0x00,0x73,0x00,
        0x30,0x00,0x38,0x00,0x07,0x00,0x08,0x00,
        0xb1,0xdf,0x9c,0xd9,0x58,0xed,0xc8,0x01,
        0x00,0x00,0x00,0x00
}; 

int v2CliChalSize = sizeof(v2CliChal)/sizeof(v2CliChal[0]);

BYTE v2SrvChal[] = {
    0xe9,0xfc,0x9f,0xcc,0x22,0x95,0x66,0xe2
};

/*
owf
0xbfffe750:,0x86,0xc9,0x96,0x80,0xa3,0xca,0x31,0x1e
0xbfffe758:,0x06,0xaa,0xd8,0x26,0x62,0xe7,0xc6,0xd4

(gdb) x /16b resp_buf
0x7e,0x2f,0x07,0xd2,0xdb,0x57,0xf9,0xed,
0xc0,0x87,0x1e,0x39,0xa8,0xa7,0x85,0x94
*/


#define SESSION_USER "User"
#define SESSION_PASSWORD "Password"
#define SESSION_DOMAIN "Domain"

BYTE random_key[] = {
    0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55
};

BYTE client_challenge[] = {
    0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa
};

BYTE server_challenge[] = {
    0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef
};




DWORD
RunSessionKeyTest(void)
{

    DWORD           dwError;
    LSA_STRING      upperUser;
    LSA_STRING      upperDomain;
    LSA_STRING      password;
    NTLM_OWF        ntv1OWF;
    UCHAR           key[16];
    UCHAR           key2[16];
    UCHAR           randKey[16];

    RC4_KEY         rc4Key;
    HMAC_CTX        hmacCtxt;
    DWORD           mdLen;


    dwError = LsaInitializeLsaStringA(
                    SESSION_USER,
                    &upperUser
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaInitializeLsaStringA(
                    SESSION_DOMAIN,
                    &upperDomain
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaInitializeLsaStringA(
                    SESSION_PASSWORD,
                    &password
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMComputeNTOWF(
                    &password,
                    ntv1OWF
                    );
                    
    
    BAIL_ON_NTLM_ERROR(dwError);


    MD4(ntv1OWF, 16, key);


    /* ntlm2 */
    HMAC_CTX_init(&hmacCtxt);
    HMAC_Init_ex(
        &hmacCtxt, 
        key,
        16,
        EVP_md5(),
        NULL
    );

    HMAC_Update(&hmacCtxt, server_challenge, 8);
    HMAC_Update(&hmacCtxt, client_challenge, 8);
    HMAC_Final(&hmacCtxt, key2, &mdLen);
    HMAC_CTX_cleanup(&hmacCtxt);


    RC4_set_key(&rc4Key, 16, key2);
    RC4(&rc4Key, 16, random_key, randKey);


error:

    return dwError;

}



extern DWORD db_level;
BOOLEAN verbose; 

DWORD
RunV2ChallengeTest(void)
{
    DWORD           dwError = 0;
    unsigned int    mdLen = sizeof(NTLM_OWF);
    NTLM_OWF        ntv1OWF;
    NTLM_OWF        ntv2OWF;
    HMAC_CTX        hmacCtxt;
    LSA_STRING      upperUser;
    LSA_STRING      upperDomain;
    LSA_STRING      password;
    UCHAR           digest[16];
    UCHAR           expected[16];


    dwError = LsaInitializeLsaStringA(
                    USER,
                    &upperUser
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaInitializeLsaStringA(
                    DOMAIN,
                    &upperDomain
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaInitializeLsaStringA(
                    R_PASSWORD,
                    &password
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMComputeNTOWF(
                    &password,
                    ntv1OWF
                    );
                    
    
    BAIL_ON_NTLM_ERROR(dwError);

   /* do the hashing */
    HMAC_CTX_init(&hmacCtxt);
    HMAC_Init_ex(
        &hmacCtxt, 
        ntv1OWF,
        16,
        EVP_md5(),
        NULL
        );

    HMAC_Update(
        &hmacCtxt, 
        (UCHAR*) upperUser.buffer,
        upperUser.length
        );

    HMAC_Update(
        &hmacCtxt, 
        (UCHAR*) upperDomain.buffer,
        upperDomain.length
        );

    HMAC_Final(&hmacCtxt, ntv2OWF, &mdLen);
    HMAC_CTX_cleanup(&hmacCtxt);


    ZERO_STRUCT(hmacCtxt);

   /* do the ntresponse hashing */
    HMAC_CTX_init(&hmacCtxt);
    HMAC_Init_ex(
        &hmacCtxt, 
        ntv2OWF,
        16,
        EVP_md5(),
        NULL
        );

    HMAC_Update(
        &hmacCtxt, 
        (UCHAR*) &v2SrvChal,
        8
        );


    HMAC_Update(
        &hmacCtxt, 
        (UCHAR*) &v2CliChal,
        v2CliChalSize
        );

    HMAC_Final(&hmacCtxt, digest, &mdLen);
    HMAC_CTX_cleanup(&hmacCtxt);

    memcpy(expected, v2Response, 16);

#if 0
    /* session key = HMAC of HMAC */
    HMAC(
        EVP_md5(), 
        ntv2OWF, 
        16, 
        (const UCHAR*) ntResponseBlob->buffer, 
        MD5_DIGEST_LENGTH, 
        baseSessionKey->buffer,
        &mdLen); 

    baseSessionKey->length = baseSessionKey->maxLength = mdLen;

    /* LM response is also filled in, as follows */
    INIT_SEC_BUFFER_S(lmResponseBlob, 24);

    HMAC_CTX_init(&hmacCtxt);
    HMAC_Init_ex(
        &hmacCtxt, 
        ntv2OWF,
        16,
        EVP_md5(),
        NULL
        );

    HMAC_Update(
        &hmacCtxt, 
        (UCHAR*) serverChallenge->buffer, 
        serverChallenge->length
        );

    HMAC_Update(
        &hmacCtxt, 
        (UCHAR*) clientChallenge->buffer,
        clientChallenge->length
        );

    HMAC_Final(&hmacCtxt,lmResponseBlob->buffer, &mdLen);
    HMAC_CTX_cleanup(&hmacCtxt);

    memcpy(&lmResponseBlob->buffer[16], clientChallenge->buffer, 8);

#endif

error:

    return dwError;
}

DWORD
SendBytes(
    SOCKET s, 
    PBYTE pBuf, 
    DWORD cbBuf
)
{
    DWORD dwError;
    PBYTE pTemp = pBuf;
    int   cbSent;
    int   cbRemaining = cbBuf;

    if (0 == cbBuf)
        return(LW_ERROR_SUCCESS);

    while (cbRemaining) 
    {
        cbSent = send(
            s, 
            (const char *)pTemp, 
            cbRemaining, 
            0);

        if (cbSent == -1) 
        {
            dwError = errno;
            fprintf(stderr, "send failed: %u\n", dwError);
            return dwError;
        }

        pTemp += cbSent;
        cbRemaining -= cbSent;
    }

    return LW_ERROR_SUCCESS;
}

DWORD
ReceiveBytes(
    SOCKET  s, 
    PBYTE   pBuf, 
    DWORD   cbBuf, 
    DWORD  *pcbRead
)
{
    DWORD dwError;
    PBYTE pTemp = pBuf;
    int cbRead, cbRemaining = cbBuf;

    while (cbRemaining) 
    {
        cbRead = recv(
                    s, 
                    (char *)pTemp, 
                    cbRemaining, 
                    0
                    );

        if (0 == cbRead)
            break;

        if (-1 == cbRead) 
        {
            dwError = errno;
            fprintf (stderr, "recv failed: %u\n", dwError);
            return dwError;
        }

        cbRemaining -= cbRead;
        pTemp += cbRead;
    }

    *pcbRead = cbBuf - cbRemaining;

    return LW_ERROR_SUCCESS;
}  


DWORD 
SendMsg(
    SOCKET s,
    PBYTE pBuf, 
    DWORD cbBuf
)
{
    DWORD dwError;
    if (0 == cbBuf)
        return(LW_ERROR_SUCCESS);

    /* Send the size of the message. */
    dwError = SendBytes(s, (PBYTE)&cbBuf, sizeof (cbBuf));
    if (!dwError) {
        /*  Send the body of the message. */
        dwError = SendBytes(s, pBuf, cbBuf);
    }

    return (dwError);
}    


DWORD
SendError(
    SOCKET s,
    OM_uint32 majorStatus
)
{
    return SendMsg(s, (PBYTE)&majorStatus, sizeof(OM_uint32));
}





    

DWORD 
ReceiveMsg(
    SOCKET  s, 
    PBYTE   pBuf, 
    DWORD   cbBuf, 
    DWORD  *pcbRead
)
{
    DWORD dwError;
    DWORD cbRead;
    DWORD cbData;

    dwError = ReceiveBytes(
                    s, 
                    (PBYTE)&cbData, 
                    sizeof (cbData), 
                    &cbRead);

    if (dwError)
    {
        printf("ReceiveBytes failed %u\n", dwError);
        return dwError;
    }
    if (sizeof(cbData) != cbRead || cbData > cbBuf)
        return(LW_ERROR_INSUFFICIENT_BUFFER);

    dwError = ReceiveBytes(
                s, 
                pBuf, 
                cbData, 
                &cbRead);

    if (dwError)
    {
        printf("ReceiveBytes(2) failed %u\n", dwError);
        return dwError;
    }

    if (cbRead != cbData)
        return(LW_ERROR_INSUFFICIENT_BUFFER);

    *pcbRead = cbRead;
    return(LW_ERROR_SUCCESS);
}  


DWORD
ConnectToServer(
    char *server, 
    USHORT port,
    int *s
    )
{
    DWORD dwError;
    unsigned long   ulAddress;
    struct hostent *pHost;
    struct sockaddr_in sin;

    ulAddress = inet_addr(server);

    if (INADDR_NONE == ulAddress) 
    {
        pHost = gethostbyname (server);
        if (NULL == pHost) 
        {
            return LW_ERROR_INTERNAL;
        }
        memcpy((char*)&ulAddress, pHost->h_addr, pHost->h_length);
    }

    *s = socket(
            PF_INET, 
            SOCK_STREAM, 
            0
            );

    if (-1 ==  *s) 
    {
        dwError = errno;
        printf("Failed to create socket: %u\n", dwError);
        return(dwError);
    }

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ulAddress;
    sin.sin_port = htons(port);

    //--------------------------------------------------------------------
    //  Connect to the server.

    if(connect(*s, (struct sockaddr*) &sin, sizeof (sin)) == 0)
    {
        printf("connected - %s\n", server);
        return(LW_ERROR_SUCCESS);
    }
        
    dwError = errno;
    printf("Failed to create socket: %u\n", dwError);
    printf("connect failed - 0x%x\n", dwError);
    close(*s);
    return dwError;        
}  

#define CHK_GSS_ERROR(_s_, _m_) do {if ((_s_) || (_m_)) goto out;} while (0)

DWORD
GSSClientSignTest(
    int s,
    gss_ctx_id_t hInitCtxt
)
{

    OM_uint32 status;
    OM_uint32 minorStatus;
    DWORD sockStatus = LW_ERROR_SUCCESS;

    int confState;
    int confRet;
    gss_qop_t qopRet;

    gss_buffer_desc ciphertext = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc cleartext = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc msg = GSS_C_EMPTY_BUFFER;


    msg.value = NTLMAllocateMemory(MAX_TOKEN);
    msg.length = strlen(TEST_MESSAGE_CLIENT) + 1;
    memcpy(msg.value, TEST_MESSAGE_CLIENT, msg.length);

    status = ntlm_gss_wrap(
                &minorStatus,
                hInitCtxt,
                1,
                GSS_C_QOP_DEFAULT,
                &msg,
                &confState,
                &ciphertext
                );

    if (status)
    {
        printf("ntlm_gss_wrap failed! - 0x%x\n", status);
        goto out;
    }

    DBGDumpGSSBuffer(D_ERROR, "wrap output: ", &ciphertext);

    /* send it */
    sockStatus = SendMsg(s, (PBYTE)ciphertext.value, ciphertext.length);
    if (sockStatus) 
    {
        printf("send failure - 0x%x\n", sockStatus);
        goto out;
    }

    /* get response */
    msg.length = MAX_TOKEN;
    sockStatus = ReceiveMsg(
                    s, 
                    (PBYTE)msg.value,
                    msg.length, 
                    (DWORD*)&msg.length
                    );

    if (sockStatus)
    {
        printf("recv failure - 0x%x\n", sockStatus);
        goto out;
    }

    status = ntlm_gss_unwrap(
                &minorStatus,
                hInitCtxt,
                &msg,
                &confRet,
                &qopRet,
                &cleartext
                );

    if (status)
    {
        printf("ntlm_gss_unwrap failed! - 0x%x\n", status);
        goto out;
    }

    DBGDumpGSSBuffer(D_ERROR, "unsealed server message: ", &cleartext);

out:

    ntlm_gss_release_buffer(&minorStatus,&cleartext);
    ntlm_gss_release_buffer(&minorStatus,&ciphertext);
    return status;
}


DWORD
GSSClientAuthenticate(
    int s,
    char *user,
    char *domain,
    char *password
)
{
    DWORD sockStatus = LW_ERROR_SUCCESS;
    OM_uint32 majorStatus;
    OM_uint32 minorStatus;
    OM_uint32 retTime;
    gss_cred_id_t hCred = GSS_C_NO_CREDENTIAL;
    gss_ctx_id_t hInitCtxt = GSS_C_NO_CONTEXT;
    gss_buffer_desc inputToken = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc outputToken = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc creds = GSS_C_EMPTY_BUFFER;
    gss_OID_desc ntlmMech = {GSS_MECH_NTLM_LEN, GSS_MECH_NTLM};
    gss_OID actualMech;
    gss_OID_set_desc ntlmMechs = {1, &ntlmMech};
    gss_OID_set actualMechs;
    SEC_BUFFER msg;
    OM_uint32 flags;
    BOOLEAN firstPass = TRUE;
    BOOLEAN allDone = FALSE;

    msg.buffer = NTLMAllocateMemory(MAX_TOKEN);
    msg.length = msg.maxLength = MAX_TOKEN;

    majorStatus = ntlm_gss_init(&minorStatus);
    CHK_GSS_ERROR(majorStatus, minorStatus);

    if (user)
    {
        majorStatus = ntlm_gss_marshal_supplied_cred(
                        &minorStatus,
                        user,
                        domain,
                        password,
                        &creds
                        );

        CHK_GSS_ERROR(majorStatus, minorStatus);
    }


    majorStatus = ntlm_gss_acquire_supplied_cred(
                        &minorStatus,
                        GSS_C_NO_NAME,
                        &creds,
                        GSS_C_INDEFINITE,
                        &ntlmMechs,
                        GSS_C_INITIATE,
                        &hCred,
                        &actualMechs,
                        &retTime
                        );

    CHK_GSS_ERROR(majorStatus, minorStatus);

    
    do {

        if (!firstPass)
            DBGDumpGSSBuffer(D_ERROR, "ISC input: ", &inputToken);

        majorStatus = ntlm_gss_init_sec_context(
                        &minorStatus,
                        hCred,
                        &hInitCtxt,
                        GSS_C_NO_NAME,
                        &ntlmMech,
                        GSS_C_CONF_FLAG | GSS_C_INTEG_FLAG,
                        GSS_C_INDEFINITE,
                        GSS_C_NO_CHANNEL_BINDINGS, /* not supported */
                        &inputToken,
                        &actualMech,
                        &outputToken,
                        &flags,
                        &retTime
                        );


        if (majorStatus != GSS_S_COMPLETE &&
            majorStatus != GSS_S_CONTINUE_NEEDED)
        {
            /* failure */
            printf("ISC failed - 0x%x\n", majorStatus);
            goto out;
        }

        firstPass = FALSE;

        /* all done - no more to send */
        if (outputToken.length == 0)
            break;

        DBGDumpGSSBuffer(D_ERROR, "ISC output: ", &outputToken);

        /* send it */
        sockStatus = SendMsg(s, (PBYTE)outputToken.value, outputToken.length);

        if (sockStatus) 
        {
            printf("send failure - 0x%x\n", sockStatus);
            goto out;
        }

        ntlm_gss_release_buffer(&minorStatus,&outputToken);

        /* final blob sent - nothing expected in return */
        allDone = (majorStatus == GSS_S_COMPLETE);

        /* get response from server - this is input for next pass */
        msg.length = MAX_TOKEN;

        /* note overflow below */
        sockStatus = ReceiveMsg(
                        s, 
                        (PBYTE)msg.buffer, 
                        msg.length, 
                        (DWORD*)&msg.length
                        );

        if (sockStatus)
        {
            printf("recv failure - 0x%x\n", sockStatus);
            goto out;
        }

        if (msg.length == sizeof(OM_uint32))
        {
            majorStatus = (*(OM_uint32*) msg.buffer);
            if (majorStatus)
            {
                printf("server sent error 0x%x\n", majorStatus);
                goto out;
            }
        }

        inputToken.value = (PVOID) msg.buffer;
        inputToken.length = msg.length;

    } while (!allDone);

    printf("authenticated client\n");

    GSSClientSignTest(s, hInitCtxt);


out:

    ntlm_gss_release_buffer(&minorStatus, &outputToken);
    ntlm_gss_release_cred(&minorStatus, &hCred);
    ntlm_gss_delete_sec_context(&minorStatus, hInitCtxt, NULL);

    if (sockStatus) 
        majorStatus = GSS_S_FAILURE;

    return(majorStatus);
}

#if 0
DWORD
ClientAuthenticate(
        int s,
        SEC_BUFFER *id
        )
{
    DWORD status;
    DWORD sockStatus = LW_ERROR_SUCCESS;
    DWORD dwMinor;
    PVOID hCred = NULL;
    PVOID hInitCtxt = NULL;
    SEC_BUFFER input;
    SEC_BUFFER output;
    DWORD timeReq = 0;
    DWORD flags;
    DWORD timeValid;
    OID_SET desiredMech;
    OID targetMech;
    OID_SET *gotMechs = NULL;
    OID *mechUsed = NULL;
    BOOLEAN firstPass = TRUE;
    BOOLEAN allDone = FALSE;

    input.length = MAX_TOKEN;
    input.buffer = NTLMAllocateMemory(MAX_TOKEN);
    if (!input.buffer)
    {
        status = LW_ERROR_OUT_OF_MEMORY;
        goto out;
    }

    output.length = MAX_TOKEN;
    output.buffer = NTLMAllocateMemory(MAX_TOKEN);
    if (!output.buffer)
    {
        status = LW_ERROR_OUT_OF_MEMORY;
        goto out;
    }


    status = NTLMGssAcquireSuppliedCred(
                &dwMinor,
                id,
                timeReq,
                &desiredMech,
                NTLM_CREDENTIAL_BOTH,
                &hCred,
                &gotMechs,
                &timeValid
                );

    NTLMDumpCredential(D_ERROR, (PNTLM_CREDENTIAL) hCred);

    if (status)
    {
        printf("ACH failed - 0x%x\n", status);
        goto out;
    }
    
    do {

        output.length = MAX_TOKEN;
        if (!firstPass)
            DBGDumpSecBuffer(D_ERROR, "ISC input: ", &input);

        status = NTLMGssInitSecContext(
                    &dwMinor,
                    hCred,
                    &hInitCtxt,
                    NULL, /* @todo - fill this in */
                    &targetMech,
                    0, /* @todo - flags? */
                    timeReq,
                    (firstPass ? NULL : &input),
                    &mechUsed,
                    &output,
                    &flags,
                    &timeValid
                    ); 


        if (status != LW_ERROR_SUCCESS &&
            status != LW_WARNING_CONTINUE_NEEDED)
        {
            /* failure */
            printf("ISC failed - 0x%x\n", status);
            goto out;
        }

        firstPass = FALSE;

        /* all done - no more to send */
        if (output.length == 0)
            break;

        DBGDumpSecBuffer(D_ERROR, "ISC output: ", &output);

        /* send it */
        sockStatus = SendMsg(s, (PBYTE)output.buffer, output.length);

        NTLM_SAFE_FREE(output.buffer);
        ZERO_STRUCT(output);

        if (sockStatus) 
        {
            printf("send failure - 0x%x\n", sockStatus);
            goto out;
        }

        /* final blob sent - nothing expected in return */
        allDone = (status == LW_ERROR_SUCCESS);

        /* get response from server - this is input for next pass */
        input.length = MAX_TOKEN;

        /* note overflow below */
        sockStatus = ReceiveMsg(
                        s, 
                        (PBYTE)input.buffer, 
                        input.length, 
                        (DWORD*)&input.length
                        );

        if (sockStatus)
        {
            printf("recv failure - 0x%x\n", sockStatus);
            goto out;
        }



    } while (!allDone);

    printf("authenticated client\n");

out:
    
    NTLM_SAFE_FREE(input.buffer);
    NTLM_SAFE_FREE(output.buffer);

    NTLMGssReleaseCred(&dwMinor, &hCred);
    NTLMGssDeleteSecContext(&dwMinor, hInitCtxt);

    if (sockStatus) 
        status = LW_ERROR_INTERNAL;

    return(status);
}
#endif

DWORD
AcceptClientConnection(
   USHORT port,
   int *s
   )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    int sListener;
    int sClient;
    struct sockaddr_in sIn;

    sListener = socket(
                    PF_INET, 
                    SOCK_STREAM, 
                    0
                    );

    if (-1 == sListener)  
    {
        dwError = errno;
        printf("Failed to create socket: %u\n", dwError);
        return(dwError);
    }


    sIn.sin_family = AF_INET;
    sIn.sin_addr.s_addr = 0;
    sIn.sin_port = htons(port);

    if (-1 == bind(
            sListener, 
            (struct sockaddr *) &sIn, 
            sizeof (sIn)))  
    {
        dwError = errno;
        printf("Failed to create socket: %u\n", dwError);
        goto error;
    }

    if (-1 == listen(sListener, 1))  
    {
        dwError = errno;
        printf("Listen failed: %u\n", dwError);
        goto error;
    }

    printf("Listening ! \n");

    sClient = accept(
                sListener, 
                NULL, 
                NULL
                );

    if (-1 == sClient)  
    {
        dwError = errno;
        printf("accept failed: %u\n", dwError);
        goto error;
    }

    *s = sClient;

error:

    close(sListener);
    return dwError;
}  

DWORD
GSSServerAuthenticate(
   int s,
   ULONG capabilities
   )
{
    DWORD sockStatus = LW_ERROR_SUCCESS;
    OM_uint32 majorStatus;
    OM_uint32 minorStatus;
    OM_uint32 retTime;
    gss_cred_id_t hCred = GSS_C_NO_CREDENTIAL;
    gss_ctx_id_t hCtxt = GSS_C_NO_CONTEXT;
    gss_buffer_desc inputToken = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc outputToken = GSS_C_EMPTY_BUFFER;
    gss_OID_desc ntlmMech = {GSS_MECH_NTLM_LEN, GSS_MECH_NTLM};
    gss_OID actualMech;
    gss_OID_set_desc ntlmMechs = {1, &ntlmMech};
    gss_OID_set actualMechs;
    SEC_BUFFER msg;
    OM_uint32 flags;
    BOOLEAN firstPass = TRUE;
    gss_name_t srcName;


    msg.buffer = NTLMAllocateMemory(MAX_TOKEN);
    msg.length = msg.maxLength = MAX_TOKEN;

    majorStatus = ntlm_gss_init(&minorStatus);
    CHK_GSS_ERROR(majorStatus, minorStatus);

    /* note overflow below */
    sockStatus = ReceiveMsg(
                s, 
                (PBYTE)msg.buffer, 
                msg.length, 
                (DWORD*) &msg.length
                );

    if (sockStatus)
    {
        printf("recv failure - 0x%x\n", sockStatus);
        goto out;
    }

    inputToken.length = msg.length;
    inputToken.value = (PVOID) msg.buffer;

    majorStatus = ntlm_gss_acquire_cred(
                        &minorStatus,
                        GSS_C_NO_NAME,
                        GSS_C_INDEFINITE,
                        &ntlmMechs,
                        GSS_C_ACCEPT,
                        &hCred,
                        &actualMechs,
                        &retTime
                        );

    CHK_GSS_ERROR(majorStatus, minorStatus);

    do {

        DBGDumpGSSBuffer(D_ERROR, "ASC input: ", &inputToken);

        majorStatus = ntlm_gss_accept_sec_context(
                            &minorStatus,
                            &hCtxt,
                            hCred,
                            &inputToken,
                            GSS_C_NO_CHANNEL_BINDINGS,
                            &srcName,
                            &actualMech,
                            &outputToken,
                            &flags,
                            &retTime,
                            GSS_C_NO_CREDENTIAL
                            );


        if (majorStatus != GSS_S_COMPLETE &&
            majorStatus != GSS_S_CONTINUE_NEEDED)
        {
            /* failure */
            printf("ASC failed - 0x%x\n", majorStatus);
            goto out;
        }

        firstPass = FALSE;

        /* all done - no more to send */
        if (outputToken.length == 0)
            break;

        DBGDumpGSSBuffer(D_ERROR, "ISC output: ", &outputToken);

        /* send it */
        sockStatus = SendMsg(s, (PBYTE)outputToken.value, outputToken.length);

        if (sockStatus) 
        {
            printf("send failure - 0x%x\n", sockStatus);
            goto out;
        }

        if (sockStatus) 
        {
            printf("send failure - 0x%x\n", sockStatus);
            goto out;
        }

        /*ntlm_gss_release_buffer(&minorStatus,&outputToken)*/

        /* final blob sent - nothing expected in return */
        if (majorStatus == GSS_S_COMPLETE)
            break;


        /* get response from server - this is input for next pass */
        msg.length = MAX_TOKEN;

        /* note overflow below */
        sockStatus = ReceiveMsg(
                        s, 
                        (PBYTE)msg.buffer, 
                        msg.length, 
                        (DWORD*)&msg.length
                        );


        if (sockStatus)
        {
            printf("recv failure - 0x%x\n", sockStatus);
            goto out;
        }

        inputToken.value = (PVOID) msg.buffer;
        inputToken.length = msg.length;

    } while (majorStatus);

    printf("authenticated!!!\n");


    /* try out encryption */




out:

    ntlm_gss_release_buffer(&minorStatus, &outputToken);
    ntlm_gss_release_cred(&minorStatus, &hCred);
    ntlm_gss_delete_sec_context(&minorStatus, hCtxt, NULL);

    if (sockStatus) 
        majorStatus = GSS_S_FAILURE;

    return(majorStatus);
}
#if 0
DWORD
ServerAuthenticate(
   int s,
   ULONG capabilities
   )
{
    DWORD sockStatus = LW_ERROR_SUCCESS;
    DWORD status = LW_ERROR_SUCCESS;
    BOOLEAN  firstPass = TRUE;
    DWORD dwMinor;
    PVOID hCred = NULL;
    PVOID hAcceptCtxt = NULL;
    DWORD timeReq = 0;
    DWORD timeValid;
    LSA_STRING srcName;
    OID_SET desiredMech;
    OID_SET *gotMechs = NULL;
    OID *mechUsed = NULL;
    SEC_BUFFER input;
    SEC_BUFFER output;
    DWORD flags;

    input.length = input.maxLength = MAX_TOKEN;
    input.buffer = NTLMAllocateMemory(MAX_TOKEN);
    if (!input.buffer)
    {
        status = LW_ERROR_OUT_OF_MEMORY;
        goto out;
    }

    /* note overflow below */
    sockStatus = ReceiveMsg(
                s, 
                (PBYTE)input.buffer, 
                input.length, 
                (DWORD*) &input.length
                );

    if (sockStatus)
    {
        printf("recv failure - 0x%x\n", sockStatus);
        goto out;
    }


    status = NTLMGssAcquireSuppliedCred(
                &dwMinor,
                NULL,
                timeReq,
                &desiredMech,
                NTLM_CREDENTIAL_BOTH,
                &hCred,
                &gotMechs,
                &timeValid);

    NTLMDumpCredential(D_ERROR, (PNTLM_CREDENTIAL) hCred);

    do {

        DBGDumpSecBuffer(D_ERROR, "ASC input: ", &input);

        status = NTLMGssAcceptSecContext(
                    &dwMinor, 
                    hCred,
                    &hAcceptCtxt, 
                    &input, 
                    &srcName, 
                    &mechUsed, 
                    &output,
                    &flags, 
                    &timeValid
                    ); 

        if (status != LW_ERROR_SUCCESS &&
            status != LW_WARNING_CONTINUE_NEEDED)
        {
            /* failure */
            printf("ASC failed - 0x%x\n", status);
            goto out;
        }

        firstPass = FALSE;

        /* all done - no more to send */
        if (output.length == 0)
            break;

        DBGDumpSecBuffer(D_ERROR, "ASC output: ", &output);

        /* send it */
        sockStatus = SendMsg(
                        s,
                        (PBYTE)output.buffer, 
                        output.length
                        );

        NTLM_SAFE_FREE(output.buffer);
        ZERO_STRUCT(output);

        if (sockStatus) 
        {
            printf("send failure - 0x%x\n", sockStatus);
            goto out;
        }

        /* final blob sent - nothing expected in return */
        if (status == LW_ERROR_SUCCESS)
            break;

        /* get response from client - this is input for next pass */
        input.length = MAX_TOKEN;

        /* note overflow below */
        sockStatus = ReceiveMsg(
                        s, 
                        (PBYTE)input.buffer, 
                        input.length, 
                        (DWORD*) &input.length
                        );

        if (sockStatus)
        {
            printf("recv failure - 0x%x\n", sockStatus);
            goto out;
        }

    } while (status);

    printf("authenticated!!!\n");


out:

    NTLM_SAFE_FREE(input.buffer);
    NTLM_SAFE_FREE(output.buffer);
    NTLMGssReleaseCred(&dwMinor, &hCred);
    NTLMGssDeleteSecContext(&dwMinor, hAcceptCtxt);

    if (sockStatus) 
        status = LW_ERROR_INTERNAL;

    return(status);
}
#endif

int
RunGSSClientTests(
    char *target,
    USHORT port,
    char *user,
    char *domain,
    char *password
)
{
    OM_uint32 status;
    int s;

    status = ConnectToServer(target, port, &s);
    if (status) 
    {
        printf("connect failed 0x%x\n", status);
        return status;
    }

    status = GSSClientAuthenticate(
                    s,
                    user,
                    domain,
                    password
                    );

    if (status)
        printf("ClientAuthenticate() failed - 0x%x\n", status);

    close(s);

    return status;
}
#if 0
int
RunClientTests(
    char *target,
    USHORT port,
    SEC_BUFFER *creds
)
{
    DWORD status;
    int s;

    status = ConnectToServer(target, port, &s);
    if (status) 
    {
        printf("connect failed 0x%x\n", status);
        return status;
    }

    status = ClientAuthenticate(
                    s,
                    creds
                    );

    if (status)
        printf("ClientAuthenticate() failed - 0x%x\n", status);

    close(s);

    return status;
}

#endif

int
RunServerTests(USHORT port)
{
    DWORD status;
    int s;

    do {
        status = AcceptClientConnection(port, &s);
        if (status)
            return status;

        status = GSSServerAuthenticate(
                    s,
                    0
                    );

        if (status)
            printf("ServerAuthenticate() failed - 0x%x\n", status);

        close(s);

    }
    while (status);

    return status;
}

enum testAction {
    SHOW_USAGE = 0,
    RUN_AS_SERVER,
    RUN_AS_CLIENT,
    RUN_AS_GSS_SERVER,
    RUN_AS_GSS_CLIENT,
    RUN_SESSION_KEY_TEST,
    RUN_V2_CHALLENGE_RESPONSE_TEST
};

#define SIGN_SEAL_TESTS             "-signseal"

#define SERVER_ARG          "-s"
#define CLIENT_ARG          "-c"
#define TARGET_ARG          "-target"
#define PORT_ARG            "-port"
#define USER_ARG            "-u"
#define DOMAIN_ARG          "-d"
#define PASSWORD_ARG        "-p"
#define TEST_ARG            "-t"
#define TEST_DBG_ARG        "-tdbg"
#define SESSION_KEY_ARG     "-skey"
#define V2_CRTEST_ARG       "-cr"
#define GLOBAL_DBG_ARG      "-dbg"
#define HELP_ARG            "-?"


#define GET_NEXT_PARAM(_i_)	if (++i >= argc){Usage();return 0;}

void
Usage()
{
    printf("Usage: gss\n");
    printf("Test Variants (pick one):\n");
    printf("\t%s act as server\n", SERVER_ARG);
    printf("\t%s act as client\n", CLIENT_ARG);
    printf("\n\tArgs for client tests\n");
    printf("\t%s <user> (optional)\n", USER_ARG);
    printf("\t%s <domain> (optional)\n",DOMAIN_ARG);
    printf("\t%s <password> (optional)\n",PASSWORD_ARG);
    printf("\t%s <targethost>\n",TARGET_ARG);
    printf("\n\tArgs for all GSS tests\n");
    printf("\t%s <port num> (optional, default = 2000)\n",PORT_ARG);
    printf("\n\tTest control args\n");
    printf("\t%s <test number>\n",TEST_ARG);
    printf("\t%s <test debug level>\n",TEST_DBG_ARG);
    printf("\t%s <server library debug level>\n",GLOBAL_DBG_ARG);
}



int
main(int argc, char* argv[])
{
    DWORD dwError = 0;
    DWORD test = RUN_ALL;
    int i, port = 2000;
    char* user = NULL;
    char* domain = NULL;
    char* password = NULL;
    char* target = NULL;
    enum testAction action = SHOW_USAGE;

    db_level |= D_BYTES;

    /* parse arguments */
    for (i = 1; i < argc; i++)  {

        if (!strcasecmp(argv[i], SERVER_ARG)) {
            action = RUN_AS_SERVER;
        } else if (!strcasecmp(argv[i], CLIENT_ARG)) {
            action = RUN_AS_CLIENT;
        } else if (!strcasecmp(argv[i], V2_CRTEST_ARG)) {
            action = RUN_V2_CHALLENGE_RESPONSE_TEST;
        } else if (!strcasecmp(argv[i], SESSION_KEY_ARG)) {
            action = RUN_SESSION_KEY_TEST;
        } else if (!strcasecmp(argv[i], TEST_ARG)) {
            GET_NEXT_PARAM(i);
            test = strtol(argv[i],(char **)NULL, 10);
        } else if (!strcasecmp(argv[i], USER_ARG)) {
            GET_NEXT_PARAM(i);
            user = (char *)argv[i];
        } else if (!strcasecmp(argv[i], DOMAIN_ARG)) {
            GET_NEXT_PARAM(i);
            domain = (char *)argv[i];
        } else if (!strcasecmp(argv[i], PASSWORD_ARG)) {
            GET_NEXT_PARAM(i);
            password = (char *)argv[i];
        } else if (!strcasecmp(argv[i], TEST_DBG_ARG)) {
            GET_NEXT_PARAM(i);
            g_verbosity = strtol(argv[i],(char **)NULL, 10);
        } else if (!strcasecmp(argv[i], TARGET_ARG)) {
            GET_NEXT_PARAM(i);
            target = (char *)argv[i];
        } else if (!strcasecmp(argv[i], PORT_ARG)) {
            GET_NEXT_PARAM(i);
            port = strtol(argv[i],(char **)NULL, 10);
        } else if (!strcasecmp(argv[i], GLOBAL_DBG_ARG)) {
            GET_NEXT_PARAM(i);
            db_level = strtol(argv[i],(char **)NULL, 10);
        } else {
            Usage();
            return -13;
        }
    }

    switch (action)
    {
        case RUN_SESSION_KEY_TEST:
            RunSessionKeyTest();
            break;

        case RUN_AS_SERVER:
        CHK_ERR(RunServerTests(port));
        
        break;

        case RUN_AS_CLIENT:

        /* eventually, this should use logged on creds */
        if (!user || !domain || !password)
            return -1;

        if (!target)
        {
            printf("missing target\n");
            Usage();
            return -1;
        }


        CHK_ERR(RunGSSClientTests(target, port, user, domain, password)); 
        break;

        case RUN_V2_CHALLENGE_RESPONSE_TEST:
        CHK_ERR(RunV2ChallengeTest());
        break;

        default:
        Usage();
    }

error:

    printf("%s - 0x%x\n", (dwError ? "FAIL!" : "success"), dwError);
    return dwError;
}

