#include "includes.h"

#define _MACHACCT_UNICODE_PWD_LENGTH 240

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

typedef struct _netlogon_authdata
{
    char hex_unicode_pwd[512];
    char hex_client_challenge[48];
    char hex_server_challenge[48];
    char hex_client_credential[48];
    char hex_server_credential[48];
    char label[128];

    unsigned char unicode_pwd[120 * 2];
    unsigned char client_challenge[8];
    unsigned char server_challenge[8];
    unsigned char client_credential[8];
    unsigned char server_credential[8];

    unsigned char sessionKeyAes[16];
    unsigned char cliCredential[8];
    unsigned char srvCredential[8];
} netlogon_authdata;

void texthex_to_binary(char *in, int inlen, unsigned char *binary)
{
    int i = 0;
    int j = 0;
    char hexpair[3];

    for (i=0; i<inlen; i+= 2)
    {
        hexpair[0] = in[i];
        hexpair[1] = in[i+1];
        hexpair[2] = '\0';
        binary[j++] = strtoul(hexpair, NULL, 16);
    }
}

int read_authdata_file(
        char *filename,
        netlogon_authdata *authdata)
{
    FILE *fp = NULL;
    int sts = 0;
    char *ptr = NULL;

    fp = fopen(filename, "r");
    if (!fp)
    {
        sts = errno;
        goto cleanup;
    }

    ptr = fgets(authdata->hex_unicode_pwd, 
                sizeof(authdata->hex_unicode_pwd), 
                fp);
    if (!ptr)
    {
        sts = ENOSPC;
        goto cleanup;
    }
    ptr = fgets(authdata->hex_client_challenge,
                sizeof(authdata->hex_client_challenge),
                fp);
    if (!ptr)
    {
        sts = ENOSPC;
        goto cleanup;
    }
    ptr = fgets(authdata->hex_server_challenge,
                sizeof(authdata->hex_server_challenge),
                fp);
    if (!ptr)
    {
        sts = ENOSPC;
        goto cleanup;
    }
    ptr = fgets(authdata->hex_client_credential,
                sizeof(authdata->hex_client_credential),
                fp);
    if (!ptr)
    {
        sts = ENOSPC;
        goto cleanup;
    }
    ptr = fgets(authdata->hex_server_credential,
                sizeof(authdata->hex_server_credential),
                fp);
    if (!ptr)
    {
        sts = ENOSPC;
        goto cleanup;
    }

    /* Label is optional, and just used for display */
    authdata->label[0] = '\0';
    fgets(authdata->label,
          sizeof(authdata->label),
          fp);
    ptr = strstr(authdata->label, "label:");
    if (ptr)
    {
        ptr += sizeof("label:") - 1;
        memmove(authdata->label, ptr, strlen(ptr) + 1);
    }

    /* Convert all text hex data to binary values */
    ptr = strstr(authdata->hex_unicode_pwd, "unicode_pwd:");
    if (!ptr)
    {
        sts = EINVAL;
        goto cleanup;
    }
    ptr += sizeof("unicode_pwd:") - 1;
    ptr += 4; /* skip WC16 " character */

    /* Multiply by 2, as this is text hex representation of WC16 string */
    texthex_to_binary(ptr, _MACHACCT_UNICODE_PWD_LENGTH * 2, authdata->unicode_pwd);

    ptr = strstr(authdata->hex_client_challenge, "client_challenge:");
    if (!ptr)
    {
        sts = EINVAL;
        goto cleanup;
    }
    ptr += sizeof("client_challenge:") - 1;
    texthex_to_binary(ptr, 16, authdata->client_challenge);

    ptr = strstr(authdata->hex_server_challenge, "server_challenge:");
    if (!ptr)
    {
        sts = EINVAL;
        goto cleanup;
    }
    ptr += sizeof("server_challenge:") - 1;
    texthex_to_binary(ptr, 16, authdata->server_challenge);

    ptr = strstr(authdata->hex_client_credential, "client_credential:");
    if (!ptr)
    {
        sts = EINVAL;
        goto cleanup;
    }
    ptr += sizeof("client_credential:") - 1;
    texthex_to_binary(ptr, 16, authdata->client_credential);

    ptr = strstr(authdata->hex_server_credential, "server_credential:");
    if (!ptr)
    {
        sts = EINVAL;
        goto cleanup;
    }
    ptr += sizeof("server_credential:") - 1;
    texthex_to_binary(ptr, 16, authdata->server_credential);

cleanup:
    fclose(fp);
    return sts;
}

NTSTATUS 
ComputeCredentialsAes(
    netlogon_authdata *authdata)
{

    NTSTATUS ntStatus = 0;

    ntStatus = ComputeSessionKeyAes(
                   authdata->unicode_pwd,
                   sizeof(authdata->unicode_pwd),
                   authdata->client_challenge,
                   sizeof(authdata->client_challenge),
                   authdata->server_challenge,
                   sizeof(authdata->server_challenge),
                   authdata->sessionKeyAes,
                   sizeof(authdata->sessionKeyAes));
    if (ntStatus)
    {
        goto cleanup;
    }

    ntStatus = ComputeNetlogonCredentialAes(
                   authdata->client_challenge,
                   sizeof(authdata->client_challenge),
                   authdata->sessionKeyAes,
                   sizeof(authdata->sessionKeyAes),
                   authdata->cliCredential,
                   sizeof(authdata->cliCredential));
    if (ntStatus)
    {
        goto cleanup;
    }
    if (memcmp(authdata->cliCredential, authdata->client_credential, sizeof(authdata->client_credential) != 0))
    {
        ntStatus = STATUS_NETWORK_CREDENTIAL_CONFLICT;
        goto cleanup;
    }

    ntStatus = ComputeNetlogonCredentialAes(
                   authdata->server_challenge,
                   sizeof(authdata->server_challenge),
                   authdata->sessionKeyAes,
                   sizeof(authdata->sessionKeyAes),
                   authdata->srvCredential,
                   sizeof(authdata->srvCredential));
    if (ntStatus)
    {
        goto cleanup;
    }

    if (memcmp(authdata->srvCredential, authdata->server_credential, sizeof(authdata->server_credential) != 0))
    {
        ntStatus = STATUS_NETWORK_CREDENTIAL_CONFLICT;
        goto cleanup;
    }

cleanup:
    return ntStatus;
}

void PrintCredentialsAes(netlogon_authdata *authdata)
{
    if (authdata->label[0])
    {
        printf("%s\n", authdata->label);
    }
    printf("AES session key computed:\n");
    printhex(authdata->sessionKeyAes, sizeof(authdata->sessionKeyAes));
    printf("\n");
    printf("AES client credential computed:\n");
    printhex(authdata->cliCredential, sizeof(authdata->cliCredential));
    printf("\n");
    printf("AES server credential computed:\n");
    printhex(authdata->srvCredential, sizeof(authdata->srvCredential));
    printf("\n-----------------------------------------------------------------------\n\n");
}

int main(int argc, char *argv[])
{
    netlogon_authdata authdata = {0};
    char *authdata_file = NULL;
    int i = 0;

    NTSTATUS ntStatus = 0;

    for (i=1; i<argc; i++)
    {
        authdata_file = argv[i];
        ntStatus = read_authdata_file(authdata_file, &authdata);
        if (ntStatus)
        {
            printf("read_auth_data: failed %d\n", ntStatus);
            exit(1);
        }
    
        ntStatus = ComputeCredentialsAes(&authdata);
        if (ntStatus)
        {
            printf("ComputeCredentialsAes(netlogon-data.txt) failed %d\n", ntStatus);
        }
        else
        {
            PrintCredentialsAes(&authdata);
        }
    }

    return 0;
}
