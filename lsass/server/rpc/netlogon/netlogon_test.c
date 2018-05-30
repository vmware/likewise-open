#include "includes.h"

int
compute_md4_digest(
   PUCHAR puValue,
   DWORD dwValueLen,
   PUCHAR pMd4Digest);

NTSTATUS
ComputeSessionKeyMD5(
    PBYTE sharedSecret,
    DWORD sharedSecretLen,
    PBYTE cliNonce,
    DWORD cliNonceLen,
    PBYTE srvNonce,
    DWORD srvNonceLen,
    PBYTE sessionKey,
    DWORD sessionKeyLen);

NTSTATUS
ComputeSessionKeyAes(
    PBYTE sharedSecret,
    DWORD sharedSecretLen,
    PBYTE cliNonce,
    DWORD cliNonceLen,
    PBYTE srvNonce,
    DWORD srvNonceLen,
    PBYTE sessionKey,
    DWORD sessionKeyLen);

NTSTATUS
ComputeNetlogonCredentialAes(
    PBYTE challengeData,
    DWORD challengeDataLen,
    PBYTE key,
    DWORD keyLen,
    PBYTE credentialData,
    DWORD credentialDataLen);


NTSTATUS
ComputeSessionKeyDes(
    PBYTE sharedSecret,
    DWORD sharedSecretLen,
    PBYTE cliNonce,
    DWORD cliNonceLen,
    PBYTE srvNonce,
    DWORD srvNonceLen,
    PBYTE sessionKey,
    DWORD sessionKeyLen);

NTSTATUS
ComputeNetlogonCredentialDes(
    PBYTE sessionKey,
    DWORD sessionKeyLen,
    PBYTE inputChallenge,
    int inputChallengeLen,
    PBYTE retCredential,
    DWORD *retCredentialLen);


void printhex(unsigned char *hex, int hexlen)
{
    int i = 0;
    for (i=0; i<hexlen; i++)
    {
        printf("%02x ", hex[i]);
    }
}

int main(int argc, char *argv[])
{
    unsigned char md4Digest[16] = {0};
    unsigned char md4input_test[] = {
        0x74, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00 };

    unsigned char unicodePwd[] = {
 0x2e, 0x00, 0x2f, 0x00, 0x2c, 0x00, 0x6e, 0x00, 0x4c, 0x00, 0x3e, 0x00, 0x4f, 0x00, 0x4c, 0x00,
 0x5a, 0x00, 0x36, 0x00, 0x73, 0x00, 0x74, 0x00, 0x5e, 0x00, 0x58, 0x00, 0x4b, 0x00, 0x65, 0x00,
 0x4d, 0x00, 0x25, 0x00, 0x2e, 0x00, 0x49, 0x00, 0x2d, 0x00, 0x74, 0x00, 0x45, 0x00, 0x60, 0x00,
 0x57, 0x00, 0x56, 0x00, 0x6a, 0x00, 0x43, 0x00, 0x5b, 0x00, 0x30, 0x00, 0x36, 0x00, 0x3f, 0x00,
 0x5d, 0x00, 0x3a, 0x00, 0x51, 0x00, 0x76, 0x00, 0x5f, 0x00, 0x54, 0x00, 0x6e, 0x00, 0x55, 0x00,
 0x6f, 0x00, 0x3a, 0x00, 0x3a, 0x00, 0x42, 0x00, 0x77, 0x00, 0x2c, 0x00, 0x67, 0x00, 0x60, 0x00,
 0x76, 0x00, 0x23, 0x00, 0x4a, 0x00, 0x4d, 0x00, 0x36, 0x00, 0x4d, 0x00, 0x71, 0x00, 0x53, 0x00,
 0x50, 0x00, 0x75, 0x00, 0x55, 0x00, 0x28, 0x00, 0x6e, 0x00, 0x71, 0x00, 0x34, 0x00, 0x3e, 0x00,
 0x79, 0x00, 0x6a, 0x00, 0x5b, 0x00, 0x64, 0x00, 0x5c, 0x00, 0x2b, 0x00, 0x56, 0x00, 0x70, 0x00,
 0x52, 0x00, 0x5f, 0x00, 0x79, 0x00, 0x78, 0x00, 0x75, 0x00, 0x63, 0x00, 0x21, 0x00, 0x67, 0x00,
 0x30, 0x00, 0x54, 0x00, 0x36, 0x00, 0x35, 0x00, 0x76, 0x00, 0x7a, 0x00, 0x57, 0x00, 0x41, 0x00,
 0x42, 0x00, 0x5f, 0x00, 0x42, 0x00, 0x22, 0x00, 0x69, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x53, 0x00,
 0x2b, 0x00, 0x34, 0x00, 0x27, 0x00, 0x5e, 0x00, 0x3a, 0x00, 0x21, 0x00, 0x2c, 0x00, 0x3b, 0x00,
 0x25, 0x00, 0x47, 0x00, 0x73, 0x00, 0x2d, 0x00, 0x28, 0x00, 0x22, 0x00, 0x3a, 0x00, 0x20, 0x00,
 0x6d, 0x00, 0x3e, 0x00, 0x21, 0x00, 0x43, 0x00, 0x4c, 0x00, 0x66, 0x00, 0x6e, 0x00, 0x4e, 0x00, };

    unsigned char unicodePwdDes[] = {
 0x45, 0x00, 0x52, 0x00, 0x33, 0x00, 0x5d, 0x00, 0x31, 0x00, 0x26, 0x00, 0x4d, 0x00, 0x44, 0x00,
 0x22, 0x00, 0x59, 0x00, 0x76, 0x00, 0x7a, 0x00, 0x38, 0x00, 0x35, 0x00, 0x3b, 0x00, 0x35, 0x00,
 0x6a, 0x00, 0x5f, 0x00, 0x3c, 0x00, 0x3f, 0x00, 0x51, 0x00, 0x6c, 0x00, 0x34, 0x00, 0x4d, 0x00,
 0x36, 0x00, 0x67, 0x00, 0x78, 0x00, 0x7a, 0x00, 0x3c, 0x00, 0x50, 0x00, 0x3a, 0x00, 0x58, 0x00,
 0x5e, 0x00, 0x67, 0x00, 0x61, 0x00, 0x64, 0x00, 0x67, 0x00, 0x20, 0x00, 0x45, 0x00, 0x54, 0x00,
 0x44, 0x00, 0x32, 0x00, 0x38, 0x00, 0x28, 0x00, 0x38, 0x00, 0x59, 0x00, 0x62, 0x00, 0x5b, 0x00,
 0x26, 0x00, 0x52, 0x00, 0x71, 0x00, 0x23, 0x00, 0x25, 0x00, 0x46, 0x00, 0x5c, 0x00, 0x36, 0x00,
 0x3f, 0x00, 0x69, 0x00, 0x68, 0x00, 0x27, 0x00, 0x48, 0x00, 0x5c, 0x00, 0x4b, 0x00, 0x5e, 0x00,
 0x2b, 0x00, 0x29, 0x00, 0x75, 0x00, 0x5b, 0x00, 0x3a, 0x00, 0x34, 0x00, 0x33, 0x00, 0x5c, 0x00,
 0x46, 0x00, 0x3d, 0x00, 0x4d, 0x00, 0x51, 0x00, 0x57, 0x00, 0x4d, 0x00, 0x35, 0x00, 0x43, 0x00,
 0x34, 0x00, 0x54, 0x00, 0x32, 0x00, 0x38, 0x00, 0x3c, 0x00, 0x6b, 0x00, 0x29, 0x00, 0x3f, 0x00,
 0x49, 0x00, 0x48, 0x00, 0x4c, 0x00, 0x68, 0x00, 0x69, 0x00, 0x62, 0x00, 0x21, 0x00, 0x45, 0x00,
 0x27, 0x00, 0x51, 0x00, 0x66, 0x00, 0x26, 0x00, 0x64, 0x00, 0x5e, 0x00, 0x72, 0x00, 0x69, 0x00,
 0x60, 0x00, 0x3e, 0x00, 0x4c, 0x00, 0x36, 0x00, 0x4e, 0x00, 0x5f, 0x00, 0x31, 0x00, 0x5a, 0x00,
 0x2b, 0x00, 0x54, 0x00, 0x25, 0x00, 0x5d, 0x00, 0x33, 0x00, 0x31, 0x00, 0x3c, 0x00, 0x59, 0x00, };

    unsigned char clientChallengeDes[] = {
 0xa8, 0x55, 0xcb, 0x26, 0x01, 0x87, 0xa8, 0x6a };

    unsigned char serverChallengeDes[] = {
 0xef, 0x98, 0xd3, 0x6c, 0x90, 0x4b, 0x40, 0x82 };

    unsigned char clientChallenge[] = {
 0x3a, 0x03, 0x90, 0xa4, 0x6d, 0x0c, 0x3d, 0x4f };

    unsigned char serverChallenge[] = {
 0x0c, 0x4c, 0x13, 0xd1, 0x60, 0x41, 0xc8, 0x60 };

    unsigned char sessionKeyAnswer[] = {
 0xee, 0xfe, 0x8f, 0x40, 0x00, 0x7a, 0x2e, 0xeb, 0x68, 0x43, 0xd0, 0xd3, 0x0a, 0x5b, 0xe2, 0xe3 };


    unsigned char sample_owf_password[] = {
 'o','w','f','d','i','g','e','s','t',':',
 0x13, 0xc0, 0xb0, 0x4b, 0x66, 0x25, 0x0d, 0x08, 0xb8, 0xa3, 0x90, 0x4d, 0xcc, 0x8b, 0x34, 0xe3 };

    unsigned char sample_cli_challenge[] = {
 0x25, 0x63, 0xe3, 0x5f, 0x69, 0xe1, 0x5a, 0x24 };

    unsigned char sample_srv_challenge[] = {
 0x9C, 0x66, 0x5F, 0x90, 0xD9, 0x83, 0xDF, 0x43 };

    unsigned char sample_session_key[] = {
 0xc9, 0xc7, 0xf7, 0x2f, 0xc6, 0xb9, 0x13, 0xe3, 0x67, 0xae, 0xa9, 0x1d, 0x0a, 0xe3, 0xa7, 0x70 };


    unsigned char sample_client_credential[] = {
 0x58, 0x6a, 0xdf, 0x53, 0xef, 0x72, 0x78, 0xd9 };
    unsigned char sample_server_credential[] = {
 0xE1, 0x41, 0x62, 0x09, 0xB2, 0x3E, 0x57, 0x51 };

    unsigned char sample_computed_cli_credential[8] = {0};
    unsigned char sample_computed_srv_credential[8] = {0};

    unsigned char computed_cli_credential_des[8] = {0};
    unsigned char computed_srv_credential_des[8] = {0};
    unsigned int retCredentialLen = 0;

#if 0
/* http://samba.2283325.n4.nabble.com/MS-NRPC-AES-Schannel-problems-td2516943.html test data */
        OWF Password :
            13 c0 b0 4b 66 25 0d 08-b8 a3 90 4d cc 8b 34 e3

        Client Challenge:
           25 63 e3 5f 69 e1 5a 24

        Server Challenge:
           9C 66 5F 90 D9 83 DF 43

        Session Key calculated:
            c9 c7 f7 2f c6 b9 13 e3-67 ae a9 1d 0a e3 a7 70

        Client Credential:
           58 6a df 53 ef 72 78 d9

        Server Credential
           E1 41 62 09 B2 3E 57 51 
#endif

    unsigned char sessionKeyComputed[16] = {0};
    NTSTATUS ntStatus = 0;

    unsigned char sessionKeySampleAes[16] = {0};

    unsigned char sessionKeyDes[8] = {0};

    compute_md4_digest(md4input_test,
                       sizeof(md4input_test),
                       md4Digest);
    printf("md4Digest 'test':\n");
    printhex(md4Digest, 16);
    printf("\n");

    printf("\n");
    printf("md4Digest 'unicodePwd' test vector:\n");
    compute_md4_digest(unicodePwd,
                       sizeof(unicodePwd),
                       md4Digest);
    printhex(md4Digest, 16);
    printf("\n");

    compute_md4_digest(sessionKeyAnswer, sizeof(sessionKeyAnswer), md4Digest);
    
    printf("\n");
    printf("md4Digest 'sessionKeyAnswer' test vector:\n");
    printhex(sessionKeyAnswer, sizeof(sessionKeyAnswer));
    printf("\n");

    ntStatus = ComputeSessionKeyMD5(
                   unicodePwd,
                   sizeof(unicodePwd),
                   clientChallenge,
                   sizeof(clientChallenge),
                   serverChallenge,
                   sizeof(serverChallenge),
                   sessionKeyComputed,
                   sizeof(sessionKeyComputed));
    if (ntStatus)
    {
        printf("ComputeSessionKeyMD5() failed\n");
    }
    else
    {
        printf("ComputeSessionKeyMD5 'sessionKeyComputed':\n");
        printhex(sessionKeyComputed, sizeof(sessionKeyComputed));
        printf("\n");
    }

    ntStatus = ComputeSessionKeyAes(
                   sample_owf_password,
                   sizeof(sample_owf_password),
                   sample_cli_challenge,
                   sizeof(sample_cli_challenge),
                   sample_srv_challenge,
                   sizeof(sample_srv_challenge),
                   sessionKeySampleAes,
                   sizeof(sessionKeySampleAes));
    if (ntStatus)
    {
        printf("ComputeSessionKeyAes() failed\n");
    }
    else
    {
        printf("\n");
        printf("sample AES session key answer:\n");
        printhex(sample_session_key, sizeof(sample_session_key));
        printf("\n");
        printf("sample AES session key computed:\n");
        printhex(sessionKeySampleAes, sizeof(sessionKeySampleAes));
        printf("\n");
    }

    ntStatus = ComputeNetlogonCredentialAes(
                   sample_cli_challenge,
                   sizeof(sample_cli_challenge),
                   sessionKeySampleAes,
                   sizeof(sessionKeySampleAes),
                   sample_computed_cli_credential,
                   sizeof(sample_computed_cli_credential));
    if (ntStatus)
    {
        printf("ComputeNetlogonCredentialAes() failed\n");
    }
    else
    {
        printf("\n");
        printf("sample AES client credential test vector:\n");
        printhex(sample_client_credential, sizeof(sample_client_credential));
        printf("\n");
        printf("sample AES client credential computed:\n");
        printhex(sample_computed_cli_credential, sizeof(sample_computed_cli_credential));
        printf("\n");
    }

    ntStatus = ComputeNetlogonCredentialAes(
                   sample_srv_challenge,
                   sizeof(sample_srv_challenge),
                   sessionKeySampleAes,
                   sizeof(sessionKeySampleAes),
                   sample_computed_srv_credential,
                   sizeof(sample_computed_srv_credential));
    if (ntStatus)
    {
        printf("ComputeNetlogonCredentialAes() failed\n");
    }
    else
    {
        printf("\n");
        printf("sample AES server credential test vector:\n");
        printhex(sample_server_credential, sizeof(sample_server_credential));
        printf("\n");
        printf("sample AES server credential computed:\n");
        printhex(sample_computed_srv_credential, sizeof(sample_computed_srv_credential));
        printf("\n");
    }

    ntStatus = ComputeSessionKeyDes(
                   unicodePwdDes,
                   sizeof(unicodePwdDes),
                   clientChallengeDes,
                   sizeof(clientChallengeDes),
                   serverChallengeDes,
                   sizeof(serverChallengeDes),
                   sessionKeyDes,
                   sizeof(sessionKeyDes));
    if (ntStatus)
    {
        printf("ComputeSessionKeyDes() failed\n");
    }
    else
    {
        printf("\n");
        printf("DES sessionkey computed:\n");
        printhex(sessionKeyDes, sizeof(sessionKeyDes));
        printf("\n");
    }

    ntStatus = ComputeNetlogonCredentialDes(
                   sessionKeyDes,
                   sizeof(sessionKeyDes),
                   clientChallengeDes,
                   sizeof(clientChallengeDes),
                   computed_cli_credential_des,
                   &retCredentialLen);
    if (ntStatus)
    {
        printf("ComputeNetlogonCredentialDes() failed\n");
    }
    else
    {
        printf("\n");
        printf("DES client credential:\n");
        printhex(computed_cli_credential_des, retCredentialLen);
        printf("\n");
    }

    ntStatus = ComputeNetlogonCredentialDes(
                   sessionKeyDes,
                   sizeof(sessionKeyDes),
                   serverChallengeDes,
                   sizeof(serverChallengeDes),
                   computed_srv_credential_des,
                   &retCredentialLen);
    if (ntStatus)
    {
        printf("ComputeNetlogonCredentialDes() failed\n");
    }
    else
    {
        printf("\n");
        printf("DES server credential:\n");
        printhex(computed_srv_credential_des, retCredentialLen);
        printf("\n");
    }
                   
    return 0;
}
