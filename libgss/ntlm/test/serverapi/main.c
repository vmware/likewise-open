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
 *        main.c
 *
 * Abstract:
 *
 *        Server API tests for GSSNTLM
 *
 * Authors: Todd Stecher (v-todds@likewise.com)
 *
 */

#include <server.h>
#include "../include/framework.h"

#define USERNAME "user"
#define DOMAINNAME "!test!" 
#define PASSWORD "password"
#define NULLPASSWORD ""

DWORD g_tn = 0xFFFF;
DWORD cur_tn;
DWORD g_verbosity = TRACE_2;

extern DWORD db_level;

typedef struct _NTLM_RC_TEST_SET {
    BYTE challenge[8];
    BYTE clientChallenge[8];
    SEC_BUFFER_S response;
    SEC_BUFFER_S sessionKey;
} NTLM_RC_TEST_SET, *PNTLM_RC_TEST_SET;


NTLM_RC_TEST_SET responseChallengeTests[] = {
    /* v1, ntlm2 key, no key exchange */
    {{0xe8,0x27,0x7a,0xb0,0x48,0x0a,0xc2,0x81}, 
     {0x70,0xdc,0x15,0x8a,0x0f,0x59,0x87,0xb0},
     {24,24, {0xe8,0x79,0x29,0x92,0x3a,0x78,0x95,0x84,
              0xdd,0x35,0xe8,0xf1,0x27,0xaf,0x42,0x80,
              0x8e,0xf0,0x55,0xa1,0xdc,0xd6,0x74,0x0b}},
     {0,0,{0}}},
    /* v1, ntlm2 key, no key exchange */
    {{0xdc,0xc8,0x3c,0xdd,0xf3,0x87,0xe7,0x86},
     {0xc8,0x41,0x2c,0xb1,0xe8,0x35,0xbe,0xe9},
     {24, 24, {0x33,0x60,0xd6,0x3a,0xbe,0x80,0x50,0x49,
               0x36,0xfa,0x5c,0x4b,0x24,0x1e,0x40,0x9d,
               0xe6,0x24,0x97,0x68,0xa2,0xb2,0xe0,0xb8}},
     {0,0,{0}}},
    /* v1, ntlm2 key, no key exchange */
    /* v1, ntlm2 key, key exchange */
    {{0x52,0x54,0x03,0xf6,0xa1,0xa9,0x25,0xb5},
     {0xb6,0x9f,0x19,0x2e,0x7d,0x98,0xfc,0x8e},
     {24, 24, {0x18,0x27,0x63,0xcf,0xfd,0xbf,0xcc,0xbf,
               0x6f,0xd9,0xa3,0xf0,0xa3,0xbf,0x09,0x5e,
               0x66,0x7c,0x4c,0x63,0x62,0x98,0x09,0xd2}},
     {16, 16, {0xde,0x35,0x8e,0xdc,0xad,0x62,0x84,0xe9,
               0xaf,0x01,0x7a,0x3d,0xd2,0x3c,0x39,0xa9}}}


};

        

ULONG responseChallengeTestCount = sizeof(responseChallengeTests) /
    sizeof(NTLM_RC_TEST_SET);





/* test 1: Fundamental acquire cred handle test */
DWORD
AcquireCredentialsHandleQuickTest(
    PSEC_BUFFER pCreds
    )
{

    DWORD dwError, dwMinor;
    PVOID h1 = NULL;
    PNTLM_CREDENTIAL hc1;
    PVOID h2 = NULL;
    PNTLM_CREDENTIAL hc2;
    DWORD timeReq;
    DWORD timeValid;
    OID_SET desiredMech;
    OID_SET *gotMechs = NULL;
    SEC_BUFFER creds;
    PSEC_BUFFER cred = (pCreds ? &creds : NULL);

    DWORD tn = 1;

    /* @todo - compose desired mech */

    TRACE(("*** %d.0 Testing acquire cred %s ***\n", cur_tn, 
            (pCreds ?  "(supplied)" : "(default)")));

    /*
     * test 1.1 make sure we can get a cred handle
     */
    TRACE(("*  Test %i - simply get a cred handle\n", tn));

    /* 
     * since these are internal APIs, we're ok w/ no consting - but
     * that means we have to keep making copies of our supp. creds 
     * in this test...  This assumption may have to change for "standalone"
     * mode.
     */ 
    NTLMAllocCopySecBuffer(&creds, pCreds);

    CHK_ERR( NTLMGssAcquireSuppliedCred(
                &dwMinor,
                cred,
                timeReq,
                &desiredMech,
                NTLM_CREDENTIAL_BOTH,
                &h1,
                &gotMechs,
                &timeValid));

    hc1 = (PNTLM_CREDENTIAL) h1;
    NTLMDumpCredential(D_ERROR, h1);

    TEST_END(tn);

    if (pCreds)
        SEC_BUFFER_COPY(&creds, pCreds);

   /*
    * test 1.2 make sure next matching call returns same cred handle
    */
    TRACE(("*  Test %i - get a matching cred handle\n", tn));
    CHK_ERR(NTLMGssAcquireSuppliedCred(
                &dwMinor,
                cred,
                timeReq,
                &desiredMech,
                NTLM_CREDENTIAL_BOTH,
                &h2,
                &gotMechs,
                &timeValid));

    hc2 = (PNTLM_CREDENTIAL) h2;
    NTLMDumpCredential(D_ERROR, h2);
    if (h1 != h2) {
        TRACE(("Credential handles are *NOT* equal - h1:0x%p h2:0x%p\n", h1,
                h2));
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_CREDENTIAL);
    }

    TEST_END(tn);

    if (pCreds)
        SEC_BUFFER_COPY(&creds, pCreds);

    /*
     * test 1.3 make sure 2 calls to free are required before
     * cred handle is invalid.
     */ 
    TRACE(("*  Test %i - test cred handle release\n", tn));
    CHK_ERR(NTLMGssReleaseCred(h1));
    CHK_ERR(NTLMGssReleaseCred(h2));
    CHK_EXPECTED(NTLMGssReleaseCred(h1), LSA_ERROR_INVALID_CREDENTIAL);
    CHK_EXPECTED(NTLMGssReleaseCred(h2), LSA_ERROR_INVALID_CREDENTIAL);
    
    TEST_END(tn);

error:

    TRACE(("\n\n*** Test %d.0 - error: 0x%x minor: 0x%x\n",  g_tn, dwError, dwMinor));

    return dwError;
}

/* test challenge response inputs */
BOOLEAN
ChallengeResponseTests(
    char* szUser,
    char* szPassword,
    char* szDomain,
    PSEC_BUFFER_S challenge,
    PSEC_BUFFER_S clientChallenge,
    PSEC_BUFFER_S sessionKey,
    PSEC_BUFFER NTResponse,
    ULONG addedFlags
    )
{

    DWORD dwError;

    LSA_STRING user;
    LSA_STRING password;
    LSA_STRING domain;
    NTLM_CONTEXT ctxt;
    PAUTH_PROVIDER provider;
    AUTH_USER authUser;
    LSA_STRING providerName;
    SEC_BUFFER generatedResponse;
    SEC_BUFFER_S generatedSessionKey;

    ZERO_STRUCT(authUser);
    ZERO_STRUCT(user);
    ZERO_STRUCT(domain);
    ZERO_STRUCT(password);
    ZERO_STRUCT(generatedResponse);
    ZERO_STRUCT(ctxt);

    addedFlags |= (sessionKey->length ? NEGOTIATE_KEY_EXCH : 0); 

    LsaInitializeLsaStringA("testProvider", &providerName);
    LsaInitializeLsaStringA(szUser, &user);
    LsaInitializeLsaStringA(szDomain, &domain);
    LsaInitializeLsaStringA(szPassword, &password);

    /* build user */
    CHK_BOOL(NTLMInitializeAuthUser(
                &user,
                &domain,
                (szPassword ? &password : NULL),
                0,
                &authUser
                ));


    /* select auth provider - use test here */
    /* @todo - test uses test pwd, not supplied one */
   
    provider = NTLMSelectNamedAuthProvider(
                    &providerName,
                    &authUser
                    );
                    
    CHK_BOOL(NTLMCreateDummyContext(
                &ctxt,
                &authUser,
                (NEGOTIATE_NTLM2 | addedFlags)
                ));

    /* get, and print, the NTLMv1 response */
    CHK_ERR(NTLMComputeNTLMv1Response(
                &ctxt,
                (PUCHAR)challenge->buffer,
                &generatedResponse
                ));

    CHK_ERR(NTLMComputeSessionKey(
                &ctxt,
                challenge->buffer,
                clientChallenge,
                &generatedSessionKey,
                false /* server */ 
                ));

    DBGDumpSecBuffer(D_ERROR,"challenge", (PSEC_BUFFER) challenge);
    DBGDumpSecBuffer(D_ERROR,"client challenge", (PSEC_BUFFER) clientChallenge);
    DBGDumpSecBuffer(D_ERROR,"generated session key", (PSEC_BUFFER) &generatedSessionKey);
    DBGDumpSecBuffer(D_ERROR,"generated response", (PSEC_BUFFER) &generatedResponse);

    
    /* if the session key arg is present, verify it */
    if (sessionKey->length)
    {
        if (sessionKey->length != generatedSessionKey.length || 
            sessionKey->length != 16)
        {
            ERROR(("invalid session key length - %u vs %u\n",
                    sessionKey->length, generatedSessionKey.length));
            goto error;
        }

        if (memcmp(sessionKey->buffer, &generatedSessionKey.buffer, 16))
        {
            ERROR(("session keys don't match\n"));

            DBGDumpSecBuffer(
                D_ERROR,
                "expected session key",
                (PSEC_BUFFER) sessionKey
                );

            goto error;
        }

    }

    if (NTResponse->length)
    {

        if (NTResponse->length != generatedResponse.length || 
            NTResponse->length != 16)
        {
            ERROR(("invalid response length - %u vs %u\n",
                    NTResponse->length, generatedResponse.length));
            goto error;
        }

        if (memcmp(
                NTResponse->buffer,
                &generatedResponse.buffer, 
                generatedResponse.length
                ))
        {
            ERROR(("nt responses don't match\n"));

            DBGDumpSecBuffer(
                D_ERROR,
                "expected response",
                (PSEC_BUFFER)NTResponse
                );

            goto error;
        }

    }


    /* get, and print, NTLM2 response */


error:

    NTLM_SAFE_FREE(&user.buffer);
    NTLM_SAFE_FREE(&domain.buffer);
    NTLM_SAFE_FREE(&password.buffer);
    

    TRACE(("\n\n*** C%d.0 - error: 0x%x \n",  g_tn, dwError));

    return dwError;
}










DWORD
SimpleGssApi(
    PSEC_BUFFER pCreds
    )
{

    DWORD dwError, dwMinor;
    PVOID hCred = NULL;
    PVOID hInitCtxt = NULL;
    PVOID hAcceptCtxt = NULL;
    PNTLM_CREDENTIAL hc1;
    DWORD timeReq = 0;
    DWORD timeValid;
    OID_SET desiredMech;
    OID_SET *gotMechs = NULL;
    OID targetMech;
    OID *mechUsed = NULL;
    SEC_BUFFER creds;
    PSEC_BUFFER cred = (pCreds ? &creds : NULL);
    PSEC_BUFFER negToken = NULL;
    PSEC_BUFFER challengeToken = NULL;
    PSEC_BUFFER authToken = NULL;
    PSEC_BUFFER dummyToken = NULL;
    PLSA_STRING srcName = NULL;
    DWORD flags;


    DWORD tn = 1;

    TRACE(("*** %d.0 Testing simple gssapi ***\n", tn));

    /*
     * Setup credential 
     */

    NTLMAllocCopySecBuffer(&creds, pCreds);

    CHK_ERR( NTLMGssAcquireSuppliedCred(
                &dwMinor,
                cred,
                timeReq,
                &desiredMech,
                NTLM_CREDENTIAL_BOTH,
                &hCred,
                &gotMechs,
                &timeValid));

    hc1 = (PNTLM_CREDENTIAL) hCred;
    NTLMDumpCredential(D_ERROR, hc1);

   /*
    * test that isc generates a negotiate message
    */
    TRACE(("*  Test %i - first call to isc\n", tn));
    CHK_EXPECTED(NTLMGssInitSecContext(
                 &dwMinor,
                 hCred,
                 &hInitCtxt,
                 NULL, /* @todo - fill this in */
                 &targetMech,
                 0, /* @todo - flags? */
                 timeReq,
                 NULL, /* channel bindings are NOT supported */
                 NULL, /* empty input token */ 
                 &mechUsed,
                 &negToken,
                 &flags,
                 &timeValid), 
        LSA_WARNING_CONTINUE_NEEDED);


    NTLMDumpNegotiateMessage(D_ERROR, (PNTLM_CONTEXT) hInitCtxt,
        (PNEGOTIATE_MESSAGE) negToken->buffer);

    TEST_END(tn);

    /*               
     * test that asc consume negotiate, and returns challenge 
     */
    TRACE(("*  Test %i - first call to asc\n", tn));
    CHK_EXPECTED(NTLMGssAcceptSecContext(
                 &dwMinor,
                 hCred,
                 &hAcceptCtxt,
                 negToken,
                 NULL, /* channel bindings are NOT supported */
                 &srcName,
                 &mechUsed,
                 &challengeToken,
                 &flags,
                 &timeValid),
        LSA_WARNING_CONTINUE_NEEDED);

    /* @todo - validate src name */
    /* @todo - validate mech */

    TEST_END(tn);

   /*
    * test next isc call
    */
    TRACE(("*  Test %i - second (authenticate) call to isc\n", tn));
    CHK_ERR(NTLMGssInitSecContext(
                 &dwMinor,
                 hCred,
                 &hInitCtxt,
                 NULL, /* @todo - fill this in */
                 &targetMech,
                 0, /* @todo - flags? */
                 timeReq,
                 NULL, /* channel bindings are NOT supported */
                 challengeToken,
                 &mechUsed,
                 &authToken,
                 &flags,
                 &timeValid));

    /* for RFC gssapi, should this return continue, or ok w/ data? */
    
    CHK_BOOL(authToken->length); 

    TEST_END(tn);

    /*               
     * final call to ASC - should be S_OK
     */
    TRACE(("*  Test %i - second call to asc\n", tn));
    CHK_ERR(NTLMGssAcceptSecContext(
                 &dwMinor,
                 hCred,
                 &hAcceptCtxt,
                 authToken,
                 NULL, /* channel bindings are NOT supported */
                 &srcName,
                 &mechUsed,
                 &dummyToken,
                 &flags,
                 &timeValid));

    /* CHK_BOOL(!dummyToken) empty or NULL? */
    /* @todo - validate src name */
    /* @todo - validate mech */

    TEST_END(tn);
error:

    TRACE(("\n\n*** Test %d.0 - error: 0x%x minor: 0x%x\n",  g_tn, dwError, dwMinor));

    return dwError;
}


DWORD
RunQuickTests( 
    DWORD test,
    PSEC_BUFFER suppliedCreds
    )
{
    DWORD dwError;
    /* test 1 - basic ach test */
    RUN_TEST(AcquireCredentialsHandleQuickTest(suppliedCreds));
    
    /* test 2 - ach - no creds */
    RUN_TEST(AcquireCredentialsHandleQuickTest(NULL));

    /* test 3 - simple gssapi calls */
    RUN_TEST(SimpleGssApi(suppliedCreds));

error:

    return dwError;
}

enum testAction {
    SHOW_USAGE = 0,
    RUN_QUICK_TESTS = 1,
    RUN_CHALLENGE_RESPONSE_TESTS,
    RUN_SIGN_SEAL_TESTS
};

#define QUICK_TESTS                 "-regress"
#define CHALLENGE_RESPONSE_TESTS    "-crtest"
#define SIGN_SEAL_TESTS             "-signseal"

#define USER_ARG            "-u"
#define DOMAIN_ARG          "-d"
#define PASSWORD_ARG        "-p"
#define SRV_CHALL_ARG       "-chall"
#define CLI_CHALL_ARG       "-clichall"
#define RESPONSE_ARG        "-response"
#define SESKEY_ARG          "-sesskey"
#define TEST_ARG            "-t"
#define TEST_DBG_ARG        "-tdbg"
#define GLOBAL_DBG_ARG      "-dbg"
#define HELP_ARG            "-?"


#define GET_NEXT_PARAM(_i_)	if (++i >= argc){Usage();return 0;}


int
ParseByteString(char* s, PSEC_BUFFER sb)
{

    int cur = 0;
    BYTE b = 0x00;
    int cc;

    if (s[cur] == '0')
        cur++;

    if (s[cur] == 'x' || s[cur] == 'X')
        cur++;

    /* note - we don't do a lot of validation here */
    for (;s[cur] != '\0'; cur++)
    {
        if ((s[cur] >= '0') && 
            (s[cur] <= '9'))
            cc = 0x30;
        else if ((toupper(s[cur]) >= 'A') &&
                 (toupper(s[cur]) <= 'F'))
            cc = 0x41;
        else
            return -1;

        if (b != 0x00) 
        {
            b = toupper(s[cur]) - cc;
            b = b << 4;
        }
        else 
        {
            b |= ((toupper(s[cur]) - cc) & 0x0f);
            sb->buffer[sb->length++] = b;
            b = 0x00;
        }
    }

    return 0;
}

void
Usage()
{
    printf("Usage: serverapi\n");
    printf("Test Variants:\n");
    printf("\t%s regressions (default)\n", QUICK_TESTS);
    printf("\t%s challenge response unit test\n", CHALLENGE_RESPONSE_TESTS);
    printf("\t%s <user> (optional)\n", USER_ARG);
    printf("\t%s <domain> (optional)\n",DOMAIN_ARG);
    printf("\t%s <password> (optional)\n",PASSWORD_ARG);
    printf("\n\tArgs for challenge response unit tests\n");
    printf("\t%s challenge in response msg (hex string)\n", SRV_CHALL_ARG);
    printf("\t%s client challenge in lmresponse (hex string)\n", CLI_CHALL_ARG);
    printf("\t%s nt response (hex string)\n", RESPONSE_ARG);
    printf("\t%s encrypted session key (hex string)\n", SESKEY_ARG);
    printf("\n\tTest control args\n");
    printf("\t%s <test number>\n",TEST_ARG);
    printf("\t%s <test debug level>\n",TEST_DBG_ARG);
    printf("\t%s <server library debug level>\n",GLOBAL_DBG_ARG);
}

int
main(int argc, char* argv[])
{
    DWORD dwError;
    DWORD test = RUN_ALL;
    int i;
    char* user = NULL;
    char* domain = NULL;
    char* password = NULL;
    SEC_BUFFER suppliedCredentials;
    SEC_BUFFER_S challenge;
    SEC_BUFFER_S cliChallenge;
    SEC_BUFFER_S sessionKey;
    SEC_BUFFER ntResponse;
    enum testAction action = RUN_QUICK_TESTS;

    ZERO_STRUCT(challenge);
    ZERO_STRUCT(cliChallenge);
    ZERO_STRUCT(sessionKey);
    ZERO_STRUCT(ntResponse);
    ZERO_STRUCT(user);
    ZERO_STRUCT(domain);
    ZERO_STRUCT(password);

    /* parse arguments */
    for (i = 1; i < argc; i++)  {

        if (!strcasecmp(argv[i], QUICK_TESTS)) {
            action = RUN_QUICK_TESTS;	
        } else if (!strcasecmp(argv[i], CHALLENGE_RESPONSE_TESTS)) {
            action = RUN_CHALLENGE_RESPONSE_TESTS;	
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
        } else if (!strcasecmp(argv[i], GLOBAL_DBG_ARG)) {
            GET_NEXT_PARAM(i);
            db_level = strtol(argv[i],(char **)NULL, 10);
        } else if (!strcasecmp(argv[i], SRV_CHALL_ARG)) {
            GET_NEXT_PARAM(i);
            if (ParseByteString(argv[i], (PSEC_BUFFER) &challenge)) { 
                printf("Bad HEX input - challenge\n");
                return -13;
            }
        } else if (!strcasecmp(argv[i], CLI_CHALL_ARG)) {
            GET_NEXT_PARAM(i);
            if (ParseByteString(argv[i], (PSEC_BUFFER) &cliChallenge)) { 
                printf("Bad HEX input - cli challenge\n");
                return -13;
            }
        } else if (!strcasecmp(argv[i], SESKEY_ARG)) {
            GET_NEXT_PARAM(i);
            if (ParseByteString(argv[i], (PSEC_BUFFER) &sessionKey)) { 
                printf("Bad HEX input - sessionKey \n");
                return -13;
            }
        } else if (!strcasecmp(argv[i], RESPONSE_ARG)) {
            GET_NEXT_PARAM(i);
            ntResponse.buffer = NTLMAllocateMemory(strlen(argv[i]) * 3);
            if (ParseByteString(argv[i], &ntResponse)) { 
                printf("Bad HEX input - response \n");
                return -13;
            }
        } else {
            Usage();
            return -13;
        }
    }

    if (user || domain || password) {

        CHK_ERR(NTLMBuildSupplementalCredentials(
                        user,
                        domain,
                        password,
                        &suppliedCredentials
                        )); 

    } else {

        CHK_ERR(NTLMBuildSupplementalCredentials(
                        USERNAME,
                        DOMAINNAME,
                        PASSWORD,
                        &suppliedCredentials
                        )); 
    }

    /* initialize server */
    CHK_ERR(InitializeGSSNTLMServer(NTLM_MODE_STANDALONE));

    switch (action)
    {
        case RUN_QUICK_TESTS:
        
        CHK_ERR(RunQuickTests(test, &suppliedCredentials ));

        break;

        case RUN_CHALLENGE_RESPONSE_TESTS:

        CHK_ERR(ChallengeResponseTests(
                user,
                password,
                domain,
                &challenge,
                &cliChallenge,
                &sessionKey,
                &ntResponse,
                0));
        break;

        default:
        Usage();
    }

error:

    printf("%s - 0x%x\n", (dwError ? "FAIL!" : "success"), dwError);
    return dwError;
}

