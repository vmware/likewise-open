/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        netlogon.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        netlogon rpc server stub functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *          Adam Bernstein (abernstein@vmware.com)
 */

#include "includes.h"

typedef struct _listnode{
    struct _listnode *next;
    char *computer;
    char *domain;
    unsigned char cli_challenge[8];
    unsigned char srv_challenge[8];
    unsigned char cli_credential[8];
    unsigned char srv_credential[8];
    unsigned char session_key[16];
    time_t t_expired;
} listnode;


/* context for rpc_server_register_auth_info */
typedef struct _list_find_entry {
    unsigned char *(*find_key)(char *computer, void *list);
    void *list;
} list_find_entry;

extern void rpc_set_schannel_seskey(
    unsigned char seskey[16],
    char *netbios_host,
    char *dns_domain,
    unsigned32 *st);

/* Use list methods to save/retrieve client session key info */
static listhead *g_schn_list;

#if 0
#define _NETLOGON_TEST_VECTORS
#endif

#ifdef _NETLOGON_TEST_VECTORS
static CHAR unicodePwdTest[] = {
 0x73, 0x00, 0x4f, 0x00, 0x62, 0x00, 0x54, 0x00, 0x6c, 0x00, 0x6e, 0x00, 0x2c, 0x00
, 0x6e, 0x00, 0x55, 0x00, 0x38, 0x00, 0x75, 0x00, 0x47, 0x00, 0x49, 0x00, 0x56, 0x00, 0x24, 0x00
, 0x20, 0x00, 0x78, 0x00, 0x69, 0x00, 0x5c, 0x00, 0x6c, 0x00, 0x75, 0x00, 0x25, 0x00, 0x57, 0x00
, 0x23, 0x00, 0x3c, 0x00, 0x27, 0x00, 0x5c, 0x00, 0x65, 0x00, 0x37, 0x00, 0x5c, 0x00, 0x6a, 0x00
, 0x52, 0x00, 0x40, 0x00, 0x45, 0x00, 0x6f, 0x00, 0x37, 0x00, 0x62, 0x00, 0x6d, 0x00, 0x77, 0x00
, 0x3c, 0x00, 0x42, 0x00, 0x4e, 0x00, 0x35, 0x00, 0x55, 0x00, 0x4f, 0x00, 0x6c, 0x00, 0x65, 0x00
, 0x4a, 0x00, 0x31, 0x00, 0x6c, 0x00, 0x6e, 0x00, 0x67, 0x00, 0x20, 0x00, 0x46, 0x00, 0x53, 0x00
, 0x2d, 0x00, 0x27, 0x00, 0x22, 0x00, 0x3d, 0x00, 0x4d, 0x00, 0x4b, 0x00, 0x74, 0x00, 0x28, 0x00
, 0x2e, 0x00, 0x6a, 0x00, 0x54, 0x00, 0x5a, 0x00, 0x64, 0x00, 0x74, 0x00, 0x60, 0x00, 0x76, 0x00
, 0x38, 0x00, 0x68, 0x00, 0x50, 0x00, 0x75, 0x00, 0x2e, 0x00, 0x37, 0x00, 0x52, 0x00, 0x4b, 0x00
, 0x28, 0x00, 0x5d, 0x00, 0x76, 0x00, 0x4d, 0x00, 0x5b, 0x00, 0x33, 0x00, 0x6e, 0x00, 0x6b, 0x00
, 0x49, 0x00, 0x47, 0x00, 0x58, 0x00, 0x60, 0x00, 0x7a, 0x00, 0x77, 0x00, 0x5c, 0x00, 0x26, 0x00
, 0x48, 0x00, 0x6c, 0x00, 0x29, 0x00, 0x64, 0x00, 0x23, 0x00, 0x25, 0x00, 0x4e, 0x00, 0x54, 0x00
, 0x60, 0x00, 0x69, 0x00, 0x73, 0x00, 0x67, 0x00, 0x6c, 0x00, 0x4c, 0x00, 0x29, 0x00, 0x46, 0x00
, 0x75, 0x00, 0x6d, 0x00, 0x72, 0x00, 0x24, 0x00, 0x5d, 0x00, 0x2b, 0x00, 0x70, 0x00, 0x4c, 0x00
, 0x64, 0x00 };
/* Canned client nonce */
static BYTE cliNonceTest[] = { 0x4e, 0x00, 0x47, 0xa6, 0x75, 0x8b, 0xeb, 0x79, };
/* Canned server nonce */
static BYTE srvNonceTest[] = { 0xa7, 0x4c, 0x75, 0x78, 0x56, 0xa3, 0xe0, 0x78, };
static    BOOLEAN bUseTestVector = FALSE;
#endif

#ifndef _NETLOGON_UNIT_TEST
static
#endif
NTSTATUS
ComputeNetlogonCredentialAes(
    PBYTE challengeData,
    DWORD challengeDataLen,
    PBYTE key,
    DWORD keyLen,
    PBYTE credentialData,
    DWORD credentialDataLen);

static
NTSTATUS
NetrLogonGetDomainParameters(
    PSTR *ppszDnsDomainName,
    BOOLEAN bDnsDomainNameComplete,
    PSID *ppDomainSid,
    uuid_t retDomainGuid);

static
NTSTATUS
RpcUnicodeStringAllocateFromCString(
    RPC_PUNICODE_STRING pString,
    PCSTR pszString
    );

static
VOID
RpcUnicodeStringFree(
    RPC_PUNICODE_STRING pString);

static
NTSTATUS
NetLogonGetNetBiosNameFromDnsName(
    PSTR pszDnsName,
    PSTR *ppszNetBiosName);

static
NTSTATUS
NetlogonWC16StringDuplicate(
    PWSTR *ppwszString,
    PCWSTR pwszString
    );

static
NTSTATUS
NetlogonCStringAllocateFromWC16String(
    PSTR *ppszString,
    PWSTR pwszString);

/*
 * Add 64-bit authenticator with 32-bit timestamp without
 * carrying any overflow from the 32-bit addition.
 * TBD: Note: This function assumes LE architecture.
 */
static
void
uint32AddUint64NoCarry(uint64_t *v64, uint32_t v32)
{
    uint32_t temp32 = 0;

    /* Add low 4 bytes of v64 with v32, no carry */
    memcpy(&temp32, v64, sizeof(uint32_t));
    temp32 += v32;

    /* Copy result of addition back into 64-bit value */
    memcpy(v64, &temp32, sizeof(uint32_t));
}

static
int cmp_computer_name(listnode *find_node, listnode *entry)
{
    int found = 0;

    if (strcmp(entry->computer, find_node->computer) == 0)
    {
        found = 1;
    }

    return found;
}

unsigned char *find_computer_key(char *computer, void *list)
{
    NTSTATUS ntStatus = 0;
    listnode *found_node = NULL;
    listnode find_node = {0};
    unsigned char *ret_key = NULL;

    find_node.computer = computer;

    ntStatus = list_iterate_func(
                   list,
                   cmp_computer_name,
                   &find_node,
                   &found_node);
    if (ntStatus == 0)
    {
        ret_key = (unsigned char *) malloc(sizeof(found_node->session_key));
        if (ret_key)
        {
            memcpy(ret_key, found_node->session_key, sizeof(found_node->session_key));
        }
    }

    return ret_key;
}

list_find_entry *alloc_find_entry_ctx(void)
{
    int err = 0;
    list_find_entry *ret_entry = NULL;

    ret_entry = malloc(sizeof(list_find_entry));
    if (!ret_entry)
    {
        err = errno;
        goto error;
    }

    ret_entry->list = g_schn_list;
    ret_entry->find_key = find_computer_key;

cleanup:
    return ret_entry;

error:
    if (err)
    {
        if (ret_entry)
        {
            free(ret_entry);
            ret_entry = NULL;
        }
    }
    goto cleanup;
}

static
void delete_node(
    listnode *node)
{
    if (!node)
    {
        return;
    }
    if (node->domain)
    {
        free(node->domain);
    }
    if (node->computer)
    {
        free(node->computer);
    }
    free(node);
}

static
int cmp_node_names(listnode *find_node, listnode *entry)
{
    int found = 0;

    if (!find_node || !entry || !entry->computer || !entry->domain ||
        !find_node->computer || !find_node->domain)
    {
        goto error;
    }

    if (strcmp(entry->computer, find_node->computer) == 0 &&
        strcmp(entry->domain,   find_node->domain)   == 0)
    {
        found = 1;
    }

error:
    return found;
}

static
int netlogon_list_alloc_node(
    char *computer,
    char *domain,
    listnode **ret_node)
{
    int sts = 0;
    listnode *new_node = NULL;
    char *computer_alloc = NULL;
    char *domain_alloc = NULL;
    time_t t_cur = 0;

    new_node = calloc(1, sizeof(listnode));
    if (!new_node)
    {
        sts = errno;
        goto cleanup;
    }

    computer_alloc = strdup(computer);
    if (!computer_alloc)
    {
        sts = errno;
        goto cleanup;
    }

    domain_alloc = strdup(domain);
    if (!domain_alloc)
    {
        sts = errno;
        goto cleanup;
    }

    new_node->t_expired = time(&t_cur) + LIST_ENTRY_TIMEOUT;

    /* Assemble the return value */
    new_node->computer = computer_alloc;
    new_node->domain = domain_alloc;
    *ret_node = new_node;

cleanup:
    if (sts)
    {
        if (computer_alloc)
        {
            free(computer_alloc);
        }
        if (domain_alloc)
        {
            free(domain_alloc);
        }
        if (new_node)
        {
            free(new_node);
        }
    }

    return sts;
}

/* Replace existing list entry with new node */
static
NTSTATUS
netlogon_list_add_node(listhead *list, listnode *new_list_node)
{
    NTSTATUS ntStatus = 0;
    listnode *found_node = {0};

    ntStatus = list_iterate_func(
                   list,
                   cmp_node_names,
                   new_list_node,
                   &found_node);
    if (ntStatus == 0)
    {
        delete_node(found_node);
    }
    else
    {
        ntStatus = list_insert_node(list, new_list_node);
    }
    return ntStatus;
}

static
NTSTATUS
GetSchannelMachineEntry(
    wchar16_t *server_name,
    wchar16_t *computer_name,
    listnode **list_node_ret)
{
    NTSTATUS ntStatus = 0;
    PSTR pszComputerName = NULL;
    PSTR pszServerName = NULL;
    listnode *find_node = NULL;
    listnode *list_node = NULL;

    ntStatus = LwRtlCStringAllocateFromWC16String(
                 &pszComputerName,
                 computer_name);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LwRtlCStringAllocateFromWC16String(
                 &pszServerName,
                 server_name);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = netlogon_list_alloc_node(
                  pszComputerName,
                  pszServerName,
                  &find_node);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = list_iterate_func(
                   g_schn_list,
                   cmp_node_names,
                   find_node,
                   &list_node);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *list_node_ret = list_node;

cleanup:
    LW_SAFE_FREE_MEMORY(pszComputerName);
    LW_SAFE_FREE_MEMORY(pszServerName);
    delete_node(find_node);

    return ntStatus;

error:
    goto cleanup;
}

ULONG
SchannelBuildResponseCredential(
    wchar16_t * server_name,
    wchar16_t *computer_name,
    NetrAuth *authenticator,
    NetrAuth *return_authenticator)
{
    NTSTATUS status = STATUS_SUCCESS;
    unsigned char sessionKey[16] = {0};
    unsigned char tempCredential[8] = {0};
    unsigned char serverRetCredential[8] = {0};
    uint64_t clientCredential64 = 0;
    uint32_t t_auth = 0;
    listnode *list_node = NULL;

    status = GetSchannelMachineEntry(
                 server_name,
                 computer_name,
                 &list_node);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* Get saved credential and session key for this host */
    memcpy(&clientCredential64, list_node->cli_credential, sizeof(list_node->cli_credential));
    memcpy(sessionKey, list_node->session_key, sizeof(list_node->session_key));

    t_auth = (uint32_t) authenticator->timestamp;
    uint32AddUint64NoCarry(&clientCredential64, t_auth);

    status = ComputeNetlogonCredentialAes(
                 (PBYTE) &clientCredential64,
                 sizeof(clientCredential64),
                 sessionKey,
                 sizeof(sessionKey),
                 tempCredential,
                 sizeof(tempCredential));
    if (status)
    {
        goto error;
    }

    if (memcmp(tempCredential, authenticator->cred.data, sizeof(tempCredential)) != 0)
    {
       status = STATUS_ACCESS_DENIED;
       goto error;
    }

    /*
     * If the Netlogon credentials match, the server increments the
     * Netlogon credential in the Netlogon authenticator by one, performs
     * the computation described in section 3.1.4.5, paragraph 2., Netlogon
     * Credential Computation, and stores the new Netlogon credential.
     */
    uint32AddUint64NoCarry(&clientCredential64, 1);

    /*
     * 7. If AES is negotiated, then a signature MUST be computed using the
     * following algorithm:
     */
    status = ComputeNetlogonCredentialAes(
                 (PBYTE) &clientCredential64,
                 sizeof(clientCredential64),
                 sessionKey,
                 sizeof(sessionKey),
                 serverRetCredential,
                 sizeof(serverRetCredential));
    if (status)
    {
        goto error;
    }

    /* The result of these operations is a new Client Credential */
    memcpy(list_node->cli_credential, &clientCredential64, sizeof(clientCredential64));

    /* Set the return Schannel authenticator values */
    memcpy(return_authenticator->cred.data, serverRetCredential, sizeof(serverRetCredential));
    return_authenticator->timestamp = 0;

error:
    return status;
}

#ifndef _NETLOGON_UNIT_TEST
static
#endif
int
compute_md4_digest(
   PUCHAR puValue,
   DWORD dwValueLen,
   PUCHAR pMd4Digest)
{
    MD4_CTX ctx = {0};

    /* Returns 1 success, 0 failure */
    if (MD4_Init(&ctx) == 0)
    {
        return 0;
    }
    if (MD4_Update(&ctx, puValue, dwValueLen) == 0)
    {
        return 0;
    }
    if (MD4_Final(pMd4Digest, &ctx) == 0)
    {
        return 0;
    }

    return 1;
}

static
NTSTATUS
netlogon_ldap_get_unicode_password(
    wchar16_t *computer_name,
    wchar16_t **ppwszUnicodePwd,
    DWORD *pdwUnicodePwdLen)
{
    DWORD dwError = 0;
    HANDLE hDirectory = NULL;
    PNETLOGON_AUTH_PROVIDER_CONTEXT pContext = NULL;
    LDAP *pLd = NULL;
    PSTR pszLdapSearchBase = NULL;
    PSTR ppszAttributes[] = { "unicodePwd", NULL }; /* TBD:Adam-Store md4 hash?? */
    PSTR pszDnsDomainName = NULL;
    PSTR pszDomainDn = NULL;
    PSTR pszComputerName = NULL;
    LDAPMessage *pLdapObject = NULL;
    struct berval **bv_objectValue = NULL;
    PWSTR pwszUnicodePwd = NULL;
    PWSTR pwszUnicodePwdRet = NULL;
    PWSTR pwszUnicodePwdAlloc = NULL;
    DWORD dwUnicodePwdLen = 0;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    WCHAR wcQuote = { '"' };

    dwError = NetlogonLdapOpen(&hDirectory);
    BAIL_ON_NTSTATUS_ERROR(dwError);

    pContext = (PNETLOGON_AUTH_PROVIDER_CONTEXT) hDirectory;
    pLd = pContext->dirContext.pLd;

    /* Obtain the domain name for this DC */
    ntStatus = LwRtlCStringDuplicate(&pszDnsDomainName,
                                     pContext->dirContext.pBindInfo->pszDomainFqdn);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwLdapConvertDomainToDN(pszDnsDomainName,
                                      &pszDomainDn);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    ntStatus = LwRtlCStringAllocateFromWC16String(
                 &pszComputerName,
                 computer_name);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwAllocateStringPrintf(
                  &pszLdapSearchBase,
                  "cn=%s,cn=Computers,%s",
                  pszComputerName,
                  pszDomainDn);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    /* Get the computer account password */
    dwError = NetlogonLdapQueryObjects(
                  pLd,
                  pszLdapSearchBase,
                  LDAP_SCOPE_SUBTREE,
                  NULL,
                  ppszAttributes,
                  -1,
                  &pLdapObject);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    bv_objectValue = ldap_get_values_len(pLd, pLdapObject, ppszAttributes[0]);
    if (bv_objectValue && bv_objectValue[0])
    {
         pwszUnicodePwd = (PWSTR) bv_objectValue[0]->bv_val;
         dwUnicodePwdLen = (DWORD) bv_objectValue[0]->bv_len;
    }
    else
    {
        ntStatus = STATUS_OBJECT_NAME_NOT_FOUND;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /* Allocate a space for a NULL terminated PWSTR string */
    pwszUnicodePwdAlloc = LwRtlMemoryAllocate(dwUnicodePwdLen + sizeof(WCHAR), 1);
    if (!pwszUnicodePwdAlloc)
    {
        ntStatus = STATUS_NO_MEMORY;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }
    memcpy(pwszUnicodePwdAlloc, pwszUnicodePwd, dwUnicodePwdLen);


#ifdef _NETLOGON_TEST_VECTORS
    /* Respond with using previously captured data */
    if (memcmp(cli_challenge cliNonceTest, sizeof(cliNonceTest) == 0)
    {
        bUseTestVector = TRUE;
        memcpy(pwszUnicodePwdAlloc, unicodePwdTest, sizeof(unicodePwdTest));
        pwszUnicodePwdAlloc[sizeof(unicodePwdTest) / sizeof(WCHAR)] = (WCHAR) 0;
    }
#endif

    dwUnicodePwdLen = (DWORD) LwRtlWC16StringNumChars(pwszUnicodePwdAlloc);

    /* Must prune off the " " around the password */
    if (pwszUnicodePwdAlloc[0] == wcQuote &&
        pwszUnicodePwdAlloc[dwUnicodePwdLen-1] == wcQuote)
    {
        pwszUnicodePwd = pwszUnicodePwdAlloc + 1;

        /* -2, as the strlen is one shorter after removing the leading " */
        pwszUnicodePwd[dwUnicodePwdLen - 2] = (WCHAR) '\0';
        dwUnicodePwdLen = (DWORD) LwRtlWC16StringNumChars(pwszUnicodePwd);
    }
    else
    {
        pwszUnicodePwd = pwszUnicodePwdAlloc;
    }

    /* Allocate new pwd, as previous is pointing to aliased memory */
    ntStatus = NetlogonSrvAllocateMemory(
                 (VOID *) &pwszUnicodePwdRet,
                 dwUnicodePwdLen * sizeof(WCHAR) + sizeof(WCHAR)); /* null terminated */
    BAIL_ON_NTSTATUS_ERROR(ntStatus);
    memcpy(pwszUnicodePwdRet, pwszUnicodePwd, dwUnicodePwdLen*sizeof(WCHAR));

    *ppwszUnicodePwd = pwszUnicodePwdRet;
    *pdwUnicodePwdLen = dwUnicodePwdLen;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszUnicodePwdAlloc);
    LW_SAFE_FREE_MEMORY(pszLdapSearchBase);

    return ntStatus;

error:
    LW_SAFE_FREE_MEMORY(pwszUnicodePwdRet);
    goto cleanup;
}


#ifndef _NETLOGON_UNIT_TEST
static
#endif
NTSTATUS
ComputeSessionKeyAes(
    PBYTE sharedSecret,
    DWORD sharedSecretLen,
    PBYTE cliNonce,
    DWORD cliNonceLen,
    PBYTE srvNonce,
    DWORD srvNonceLen,
    PBYTE sessionKey,
    DWORD sessionKeyLen)
{
    NTSTATUS ntStatus = 0;
    UCHAR md4DigestBuf[MD4_DIGEST_LENGTH] = {0};
    HMAC_CTX hctx = {0};
    unsigned char hmac_buf[SHA512_DIGEST_LENGTH] = {0};
    unsigned int hmac_len = 0;
    unsigned int i = 0;
    BOOLEAN bHaveSharedSecret = FALSE;

    if (strncmp((char *) sharedSecret, "owfdigest:",  10) == 0)
    {
        /* Test vector with MD4 digest of unicodePassword already computed */
        bHaveSharedSecret = TRUE;
        sharedSecretLen -= 10;
        sharedSecret += 10;
        memcpy(md4DigestBuf, sharedSecret, sharedSecretLen);
    }

    if (!bHaveSharedSecret &&
        compute_md4_digest((unsigned char *) sharedSecret,
                           sharedSecretLen,
                           md4DigestBuf) == 0)
    {
        ntStatus = ERROR_INVALID_PASSWORD;
        goto cleanup;
    }

    HMAC_CTX_init(&hctx);

    if (HMAC_Init(&hctx,
        md4DigestBuf,
        sizeof(md4DigestBuf),
        EVP_sha256()) == 0)
    {
         ntStatus = ERROR_INVALID_HANDLE;
         goto cleanup;
    }

    if (HMAC_Update(&hctx, cliNonce, cliNonceLen) != 1)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }
    if (HMAC_Update(&hctx, srvNonce, srvNonceLen) != 1)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }
    if (HMAC_Final(&hctx, hmac_buf, &hmac_len) == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    for (i=0; i<16 && i<hmac_len && i<sessionKeyLen; i++)
    {
        sessionKey[i] = hmac_buf[i];
    }

cleanup:
    HMAC_cleanup(&hctx);

    return ntStatus;
}

NTSTATUS
ComputeHmacMD5(
    unsigned char *key,
    unsigned int keyLen,
    unsigned char *data1,
    unsigned int data1Len,
    unsigned char *data2,
    unsigned int data2Len,
    unsigned char *mac,
    unsigned int macLen)
{
    unsigned int ntStatus = 0;
    HMAC_CTX hctx = {0};
    unsigned char hmac_buf[SHA512_DIGEST_LENGTH] = {0};
    unsigned int hmac_len = 0;


    HMAC_CTX_init(&hctx);

    if (HMAC_Init(&hctx,
        key,
        keyLen,
        EVP_md5()) == 0)
    {
         ntStatus = 5;
         goto cleanup;
    }

    if (data1)
    {
        if (HMAC_Update(&hctx, data1, data1Len) != 1)
        {
            ntStatus = 5;
            goto cleanup;
        }
    }
    if (data2)
    {
        if (HMAC_Update(&hctx, data2, data2Len) != 1)
        {
            ntStatus = 5;
            goto cleanup;
        }
    }
    if (HMAC_Final(&hctx, hmac_buf, &hmac_len) == 0)
    {
        ntStatus = 5;
        goto cleanup;
    }

    ntStatus = 0;
    memcpy(mac, hmac_buf, hmac_len);

cleanup:
    HMAC_cleanup(&hctx);

    return ntStatus;
}

#ifndef _NETLOGON_UNIT_TEST
static
#endif
NTSTATUS
ComputeSessionKeyMD5(
    PBYTE sharedSecret,
    DWORD sharedSecretLen,
    PBYTE cliNonce,
    DWORD cliNonceLen,
    PBYTE srvNonce,
    DWORD srvNonceLen,
    PBYTE sessionKey,
    DWORD sessionKeyLen)
{
    NTSTATUS ntStatus = 0;
    MD5_CTX md5_ctx;
    UCHAR md4DigestBuf[MD4_DIGEST_LENGTH] = {0};
    BYTE zeros4[4] = {0};
    BYTE hmac_md5_buf[16] = {0};
    DWORD hmac_md5_len = sizeof(hmac_md5_buf);
    BYTE md5_buf[MD5_DIGEST_LENGTH] = {0};
    DWORD md5_len = sizeof(md5_buf);
    BOOLEAN bHaveSharedSecret = FALSE;

    if (strncmp((char *) sharedSecret, "owfdigest:",  10) == 0)
    {
        /* Test vector with MD4 digest of unicodePassword already computed */
        bHaveSharedSecret = TRUE;
        sharedSecretLen -= 10;
        sharedSecret += 10;
        memcpy(md4DigestBuf, sharedSecret, sharedSecretLen);
    }

    if (!bHaveSharedSecret &&
        compute_md4_digest((unsigned char *) sharedSecret,
                           sharedSecretLen,
                           md4DigestBuf) == 0)
    {
        ntStatus = ERROR_INVALID_PASSWORD;
        goto cleanup;
    }

    ntStatus = MD5_Init(&md5_ctx);
    if (ntStatus == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    ntStatus = MD5_Update(&md5_ctx, zeros4, sizeof(zeros4));
    if (ntStatus == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    ntStatus = MD5_Update(&md5_ctx, cliNonce, cliNonceLen);
    if (ntStatus == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    ntStatus = MD5_Update(&md5_ctx, srvNonce, srvNonceLen);
    if (ntStatus == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    ntStatus = MD5_Final(md5_buf, &md5_ctx);
    if (ntStatus == 0)
    {
        ntStatus = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    ntStatus = ComputeHmacMD5(
                   md4DigestBuf, sizeof(md4DigestBuf),
                   md5_buf, md5_len,
                   NULL, 0,
                   hmac_md5_buf, hmac_md5_len);
    if (ntStatus)
    {
        goto cleanup;
    }

    ntStatus = 0;
    memcpy(sessionKey, hmac_md5_buf, sessionKeyLen);

cleanup:
    return ntStatus;
}

#ifndef _NETLOGON_UNIT_TEST
static
#endif
NTSTATUS
ComputeNetlogonCredentialAes(
    PBYTE challengeData,
    DWORD challengeDataLen,
    PBYTE key,
    DWORD keyLen,
    PBYTE credentialData,
    DWORD credentialDataLen)
{
    BYTE IV[16] = {0}; /* Assuming IV is same length as key */
    DWORD IV_len = 0;
    NTSTATUS ntStatus = 0;
    EVP_CIPHER_CTX enc_ctx = {0};
    BYTE encryptedData[16] = {0};
    int encryptedDataLen = sizeof(encryptedData);
    int encryptedDataFinalLen = 0;

    ntStatus = EVP_EncryptInit_ex(
                   &enc_ctx,
                   EVP_aes_128_cfb8(),
                   NULL, /* Engine implementation */
                   key,
                   IV);
    if (ntStatus == 0)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    /* Double check IV length is correct */
    IV_len = EVP_CIPHER_iv_length(EVP_aes_128_cfb8());
    if (IV_len != sizeof(IV))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    ntStatus = EVP_EncryptUpdate(
                   &enc_ctx,
                   encryptedData,
                   &encryptedDataLen,
                   challengeData,
                   challengeDataLen);
    if (ntStatus == 0)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    ntStatus = EVP_EncryptFinal_ex(&enc_ctx,
                                   &encryptedData[encryptedDataLen],
                                   &encryptedDataFinalLen);
    if (ntStatus == 0)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    /* encryptedDataFinalLen == credentialDataLen I think */
    memcpy(credentialData, encryptedData, credentialDataLen);

    /* EVP_xxx() return 1 for success. Being here means success */
    ntStatus = 0;

cleanup:
    return ntStatus;
}

static
NTSTATUS
netlogon_compute_credentials(
    wchar16_t *server_name,
    wchar16_t *computer_name,
    UINT32 negotiate_flags,
    listnode *list_node,
    PBYTE bzClientCredential,
    PBYTE bzServerCredential)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszUnicodePwd = NULL;
    DWORD dwUnicodePwdLen = 0;
    BYTE bzClientCredentialRet[8] = {0};
    BYTE bzServerCredentialRet[8] = {0};
    DWORD dwClientChallengeLen = 0;
    DWORD dwServerChallengeLen = 0;
    PBYTE pClientChallenge = NULL;
    PBYTE pServerChallenge = NULL;
    listnode *find_node = {0};
    BYTE sessionKey[16] = {0};

#ifdef _NETLOGON_TEST_VECTORS
    if (bUseTestVector)
    {
        /* Use canned server nonce in response to canned client nonce */
        /* The computed credential should match the canned server cred */
        memcpy(server_challenge->data, srvNonceTest, ulRandBytesLen);
    }
#endif

    if (negotiate_flags & NETLOGON_NEG_AES_SUPPORT)
    {
        pClientChallenge = list_node->cli_challenge;
        dwClientChallengeLen = sizeof(list_node->cli_challenge);

        pServerChallenge = list_node->srv_challenge;
        dwServerChallengeLen = sizeof(list_node->srv_challenge);

        ntStatus = netlogon_ldap_get_unicode_password(
                       computer_name,
                       &pwszUnicodePwd,
                       &dwUnicodePwdLen);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = ComputeSessionKeyAes(
                       (PBYTE) pwszUnicodePwd,
                       dwUnicodePwdLen * sizeof (WCHAR),
                       pClientChallenge,
                       dwClientChallengeLen, /* Same length a server challenge */
                       pServerChallenge,
                       dwServerChallengeLen,
                       sessionKey,
                       sizeof(sessionKey));

        /* Compute client credential */
        ntStatus = ComputeNetlogonCredentialAes(
                       pClientChallenge,
                       dwClientChallengeLen,
                       sessionKey,
                       sizeof(sessionKey),
                       bzClientCredentialRet,
                       sizeof(bzClientCredentialRet));
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        /* Compute server credential */
        ntStatus = ComputeNetlogonCredentialAes(
                       pServerChallenge,
                       dwServerChallengeLen,
                       sessionKey,
                       sizeof(sessionKey),
                       bzServerCredentialRet,
                       sizeof(bzServerCredentialRet));
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }
    else
    {
        ntStatus = ComputeSessionKeyMD5(
                       (PBYTE) pwszUnicodePwd,
                       dwUnicodePwdLen * sizeof (WCHAR),
                       pClientChallenge,
                       dwClientChallengeLen, /* Same length a server challenge */
                       pServerChallenge,
                       dwServerChallengeLen,
                       sessionKey,
                       sizeof(sessionKey));
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* Save computed session key to list entry for this computer */
    memcpy(list_node->session_key, sessionKey, sizeof(sessionKey));

    /* Return credential arguments */
    memcpy(bzClientCredential, bzClientCredentialRet, sizeof(bzClientCredentialRet));
    memcpy(bzServerCredential, bzServerCredentialRet, sizeof(bzServerCredentialRet));

cleanup:
    return ntStatus;

error:
    delete_node(find_node);
    goto cleanup;
}



NTSTATUS srv_netr_Function00(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function01(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


    /* function 0x02 */
NTSTATUS srv_NetrLogonSamLogon(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ NetrAuth *creds,
        /* [in] */ NetrAuth *ret_creds,
        /* [in] */ UINT16 logon_level,
        /* [in] */ NetrLogonInfo *logon,
        /* [in] */ UINT16 validation_level,
        /* [out] */ NetrValidationInfo *validation,
        /* [out] */ UINT8 *authoritative
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}

	/* function 0x03 */
NTSTATUS srv_NetrLogonSamLogoff(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ NetrAuth *creds,
        /* [in] */ NetrAuth *ret_creds,
        /* [in] */ UINT16 logon_level,
        /* [in] */ NetrLogonInfo *logon
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}


	/* function 0x04 */
NTSTATUS srv_NetrServerReqChallenge(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ NetrCred *client_challenge,
        /* [out] */ NetrCred *server_challenge
        )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    UCHAR randBytes[8] = {0};;
    ULONG ulRandBytesLen = sizeof(randBytes);
    PSTR pszComputerName = NULL;
    PSTR pszServerName = NULL;
    listnode *new_list_node = NULL;

    ntStatus = list_new(&g_schn_list, delete_node);
    BAIL_ON_NTSTATUS_ERROR(dwError);

    ntStatus = LwRtlCStringAllocateFromWC16String(
                 &pszComputerName,
                 computer_name);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LwRtlCStringAllocateFromWC16String(
                 &pszServerName,
                 server_name);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = netlogon_list_alloc_node(
                  pszComputerName,
                  pszServerName,
                  &new_list_node);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* Generate server nonce (challenge) */
    RAND_pseudo_bytes(randBytes, ulRandBytesLen);

    /* Return argument for this RPC call */
    memcpy(server_challenge->data, randBytes, ulRandBytesLen);

    /* Add client/server challenge to list for later use by Authenticate3 */
    memcpy(new_list_node->cli_challenge, client_challenge->data, sizeof(client_challenge->data));
    memcpy(new_list_node->srv_challenge, randBytes, ulRandBytesLen);

    /* TBD:Adam-Replace existing node if already in list */
    ntStatus = netlogon_list_add_node(g_schn_list, new_list_node);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    LW_SAFE_FREE_MEMORY(pszServerName);
    LW_SAFE_FREE_MEMORY(pszComputerName);

    return ntStatus ? ntStatus : dwError;

error:
    goto cleanup;
}



	/* function 0x05 */
NTSTATUS srv_NetrServerAuthenticate(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t account_name[],
        /* [in] */ UINT16 secure_channel_type,
        /* [in] */ wchar16_t computer_name[],
        /* [in] */ NetrCred *cli_credentials,
        /* [out] */ NetrCred *srv_credentials
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}



NTSTATUS srv_netr_Function06(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function07(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function08(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function09(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0c(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0d(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0e(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


	/* function 0x0f */
NTSTATUS srv_NetrServerAuthenticate2(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t account_name[],
        /* [in] */ UINT16 secure_channel_type,
        /* [in] */ wchar16_t computer_name[],
        /* [in] */ NetrCred *cli_credentials,
        /* [out] */ NetrCred *srv_credentials,
        /* [in] */ UINT32 *negotiate_flags
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}



NTSTATUS srv_netr_Function10(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function11(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function12(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function13(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


    /* function 0x14 */
WINERROR srv_DsrGetDcName(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *domain_name,
        /* [in] */ GUID *domain_guid,
        /* [in] */ GUID *site_guid,
        /* [in] */ UINT32 get_dc_flags,
        /* [out] */ DsrDcNameInfo **info
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}


    /* function 0x15 */
NTSTATUS srv_NetrLogonGetCapabilities(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t * server_name,
    /* [in] */ wchar16_t *computer_name,
    /* [in] */ NetrAuth *authenticator,
    /* [in, out] */ NetrAuth *return_authenticator,
    /* [in] */ DWORD QueryLevel,
    /* [out] */ NetrCapabilities *server_capabilities
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SchannelBuildResponseCredential(
                 server_name,
                 computer_name,
                 authenticator,
                 return_authenticator);
    if (status)
    {
        goto error;
    }

    server_capabilities->server_capabilities = 0x612fffff; /* TBD:Adam-Negotiated earlier?  */

    /* No mention of the timestamp field in 3.1.4.5; set to zero */
    return_authenticator->timestamp = 0;

error:
    return status;
}

    /* function 0x16 */
NTSTATUS srv_NetrLogonSetServiceBits(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t * server_name,
    /* [in] */ UINT32 service_bits_of_interest,
    /* [in] */ UINT32 service_bits)
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}

NTSTATUS srv_netr_Function17(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function18(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function19(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

static
void
rpc_auth_key_retrieval_callback
(
    void                    *arg,
    unsigned_char_p_t       server_princ_name,
    unsigned32              key_type,
    unsigned32              key_ver,
    void                    **key,
    unsigned32              *st)
{
    unsigned char *key_ret = NULL;
    unsigned32 status = 0;
    list_find_entry *auth_info_ctx = (list_find_entry *) arg;

    key_ret = auth_info_ctx->find_key((char *) server_princ_name, auth_info_ctx->list);
    if (!key_ret)
    {
        status = rpc_s_entry_not_found;
        goto error;
    }

    *key = key_ret;
    *st = status;

error:
    return;
}



	/* function 0x1a */
NTSTATUS srv_NetrServerAuthenticate3(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t account_name[],
        /* [in] */ UINT16 secure_channel_type,
        /* [in] */ wchar16_t computer_name[],
        /* [in] */ NetrCred *credentials_in,
        /* [out] */ NetrCred *credentials_out,
        /* [in] */ UINT32 *negotiate_flags,
        /* [out] */ UINT32 *rid
        )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG retRid = 0;
    PNETLOGON_AUTH_PROVIDER_CONTEXT pContext = NULL;
    HANDLE hDirectory = NULL;
    LDAP *pLd = NULL;
    PSTR pszDnsDomainName = NULL;
    PSTR pszDomainDn = NULL;
    PSTR pszAccountName = NULL;
    PSTR pszLdapSearchBase = NULL;
    PSTR ppszAttributes[] = { "objectSid", NULL };
    LDAPMessage *pObjectSid = NULL;
    struct berval **bv_objectValue = NULL;
    PSID pDomainSid = NULL;
    BYTE bzClientCredential[8] = {0};
    BYTE bzServerCredential[8] = {0};
    listnode *list_node = NULL;
    list_find_entry *auth_info_ctx = alloc_find_entry_ctx();

    BAIL_ON_INVALID_PTR(credentials_out);

    /* Save computed credential in saved machine account entry */
    ntStatus = GetSchannelMachineEntry(
                 server_name,
                 computer_name,
                 &list_node);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* Compute credentials using client/server challenge */
    ntStatus = netlogon_compute_credentials(
                   server_name,
                   computer_name,
                   *negotiate_flags,
                   list_node,
                   bzClientCredential,
                   bzServerCredential);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    rpc_server_register_auth_info(
        (unsigned char *) computer_name,
        rpc_c_authn_schannel,
        rpc_auth_key_retrieval_callback,
        auth_info_ctx,
        &dwError);
    ntStatus = dwError;
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* Validate locally computed client credential with client's value */
    if (memcmp(credentials_in->data,
               bzClientCredential,
               sizeof(bzClientCredential)) != 0)
    {
        ntStatus = ERROR_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    memcpy(list_node->cli_credential, bzClientCredential, sizeof(bzClientCredential));
    memcpy(list_node->srv_credential, bzServerCredential, sizeof(bzServerCredential));

    dwError = NetlogonLdapOpen(&hDirectory);
    BAIL_ON_NTSTATUS_ERROR(dwError);

    pContext = (PNETLOGON_AUTH_PROVIDER_CONTEXT) hDirectory;
    pLd = pContext->dirContext.pLd;

    /* Obtain the domain name for this DC */
    ntStatus = LwRtlCStringDuplicate(
                   &pszDnsDomainName,
                   pContext->dirContext.pBindInfo->pszDomainFqdn);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwLdapConvertDomainToDN(pszDnsDomainName,
                                      &pszDomainDn);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    ntStatus = LwRtlCStringAllocateFromWC16String(
                  &pszAccountName,
                 account_name);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwAllocateStringPrintf(
                  &pszLdapSearchBase,
                  "(sAMAccountName=%s)",
                  pszAccountName);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    /* Get the domain objectSid */
    dwError = NetlogonLdapQueryObjects(
                  pLd,
                  pszDomainDn,
                  LDAP_SCOPE_SUBTREE,
                  pszLdapSearchBase,
                  ppszAttributes,
                  -1,
                  &pObjectSid);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    bv_objectValue = ldap_get_values_len(pLd, pObjectSid, ppszAttributes[0]);
    if (bv_objectValue && bv_objectValue[0])
    {
         pDomainSid = (PSID) bv_objectValue[0]->bv_val;
    }

    ntStatus = RtlGetRidSid(
                   &retRid,
                   pDomainSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

#ifdef _NETLOGON_TEST_VECTORS
    if (bUseTestVector)
    {
        memcpy(credentials_out->data,
               srvCredentialTest,
               sizeof(srvCredentialTest));
        *rid = 1103;
    }
    else
    {
        /* Return computed server credential */
        memcpy(credentials_out->data,
               bzServerCredential,
               sizeof(bzServerCredential));

        *rid = retRid;
    }
#else
    /* Return computed server credential */
    memcpy(credentials_out->data,
           bzServerCredential,
           sizeof(bzServerCredential));

    *rid = retRid;
#endif

cleanup:
    LW_SAFE_FREE_MEMORY(pszAccountName);
    NetlogonLdapRelease(hDirectory);
#ifdef _NETLOGON_TEST_VECTORS
    bUseTestVector = FALSE;
#endif
    return ntStatus;

error:
    goto cleanup;
}



NTSTATUS srv_netr_Function1b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function1c(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}



typedef struct netr_unknown6_query_1
{
    DWORD dwOsVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;

    wchar16_t *version_string_wc16;
    char *version_string;
} NetrUnknown6Query1, *PNetrUnknown6Query1;

static
VOID
NetrLogonQuery1FreeVersion(
    PNetrUnknown6Query1 pRetQuery1)
{
    if (pRetQuery1)
    {
        if (pRetQuery1->version_string_wc16)
        {
            NetlogonSrvFreeMemory(pRetQuery1->version_string_wc16);
        }
        if (pRetQuery1->version_string)
        {
            NetlogonSrvFreeMemory(pRetQuery1->version_string);
        }
        NetlogonSrvFreeMemory(pRetQuery1);
    }
}

/*
 * MS-PRPN 2.2.3.10.1 OSVERSIONINFO
 * This is a structured type with the following DWORD fields
 * dwOsVersionInfoSize
 * dwMajorVersion
 * dwMinorVersion
 * dwBuildNumber
 * dwPlatformId
 * szCSDVersion
 */
static
NTSTATUS
NetrLogonQuery1ParseVersion(
    NetrDomainQuery *query,
    PNetrUnknown6Query1 *ppRetQuery1)
{
    NTSTATUS status = 0;
    unsigned char *ptr = NULL;
    PNetrUnknown6Query1 pRetQuery1 = NULL;
    DWORD dwOsVersionInfoSize = 0;
    DWORD dwMajorVersion = 0;
    DWORD dwMinorVersion = 0;
    DWORD dwBuildNumber = 0;
    DWORD dwPlatformId = 0;
    PWSTR version_string_wc16 = NULL;
    PSTR version_string = NULL;

    if (!query || !ppRetQuery1)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    ptr = (unsigned char *) query->query1->unknown6.Buffer;
    status = NetlogonSrvAllocateMemory(
                 (VOID **) &pRetQuery1,
                 sizeof(*pRetQuery1));
    BAIL_ON_NTSTATUS_ERROR(status);

    memcpy(&dwOsVersionInfoSize, ptr, sizeof(DWORD));
    ptr += sizeof(DWORD);

    memcpy(&dwMajorVersion, ptr, sizeof(DWORD));
    ptr += sizeof(DWORD);

    memcpy(&dwMinorVersion, ptr, sizeof(DWORD));
    ptr += sizeof(DWORD);

    memcpy(&dwBuildNumber, ptr, sizeof(DWORD));
    ptr += sizeof(DWORD);

    memcpy(&dwPlatformId, ptr, sizeof(DWORD));
    ptr += sizeof(DWORD);

    /* ptr should be pointing at the WC16 version string, which is NULL terminated */
    status = NetlogonWC16StringDuplicate(
                 &version_string_wc16,
                 (PCWSTR) ptr);
    BAIL_ON_NTSTATUS_ERROR(status);
    ptr += LwRtlWC16StringNumChars(version_string_wc16) * sizeof(WCHAR) + sizeof(WCHAR);

    status = NetlogonCStringAllocateFromWC16String(
                 &version_string,
                 version_string_wc16);
    BAIL_ON_NTSTATUS_ERROR(status);

    pRetQuery1->dwOsVersionInfoSize = dwOsVersionInfoSize;
    pRetQuery1->dwMajorVersion = dwMajorVersion;
    pRetQuery1->dwMinorVersion = dwMinorVersion;
    pRetQuery1->dwBuildNumber = dwBuildNumber;
    pRetQuery1->dwPlatformId = dwPlatformId;
    pRetQuery1->version_string_wc16 = version_string_wc16;
    pRetQuery1->version_string = version_string;

    *ppRetQuery1 = pRetQuery1;

cleanup:
    return status;

error:
    if (version_string_wc16)
    {
        NetlogonSrvFreeMemory(version_string_wc16);
    }
    if (version_string)
    {
        NetlogonSrvFreeMemory(version_string);
    }
    if (pRetQuery1)
    {
        NetlogonSrvFreeMemory(pRetQuery1);
    }
    goto cleanup;
}


    /* function 0x1d */
NTSTATUS srv_NetrLogonGetDomainInfo(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ NetrAuth *creds,
        /* [in] */ NetrAuth *ret_creds,
        /* [in] */ UINT32 level,
        /* [in] */ NetrDomainQuery *query,
        /* [out] */ NetrDomainInfo *info
        )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNetrUnknown6Query1 pQuery1 = NULL;
    NetrDomainInfo1 *pDomainInfo1 = NULL;
    NetrDomainTrustInfo *pDomainTrustInfo = NULL;
    PSTR pszNetBiosName = NULL;
    PSTR pszDnsDomainName = NULL;
    PSTR pszTrustDnsDomainName = NULL;
    PWSTR pwszComputerName = NULL;
    PWSTR pwszRpcComputerName = NULL;
    PSID pDomainSid = NULL;
    PSID pDomainSid2 = NULL;
    uuid_t domainGuid;
    RPC_UNICODE_STRING *pTrustExtension = NULL;
    DWORD trustFlags = 0; // NETR_TRUST_FLAG_PRIMARY;
    DWORD trustParentIndex = 0;
    DWORD trustType = 2; // NETR_TRUST_TYPE_UPLEVEL; 2=Domain controller
    /* 0x1d */ 
    DWORD trustAttributes = NETR_TRUST_FLAG_NATIVE  | NETR_TRUST_FLAG_PRIMARY |
                            NETR_TRUST_FLAG_TREEROOT | NETR_TRUST_FLAG_IN_FOREST;
    DWORD workstationFlags = 3; /* TBD:Adam-Where to get the correct value or this field ? */
    DWORD supportedEncryptionTypes = 0xffffffff ; /* TBD:Adam-Where to get the correct value or this field ? */
    DWORD numTrusts = 1;
    PBYTE trustPtr = NULL;
    DWORD dwDomainSidLen = 0;
    status = SchannelBuildResponseCredential(
                 server_name,
                 computer_name,
                 creds,
                 ret_creds);
    if (status)
    {
        goto error;
    }


    if (level == 1)
    {
        status = NetrLogonQuery1ParseVersion(query, &pQuery1);
        BAIL_ON_NTSTATUS_ERROR(status);

        /* Memory region returned is initialized to zero */
        status = NetlogonSrvAllocateMemory(
                     (VOID **) &pDomainInfo1,
                     sizeof(*pDomainInfo1));
        BAIL_ON_NTSTATUS_ERROR(status);

        status = NetlogonSrvAllocateMemory(
                     (VOID **) &pDomainTrustInfo,
                     sizeof(*pDomainTrustInfo) * numTrusts);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = NetrLogonGetDomainParameters(
                     &pszDnsDomainName,
                     TRUE,
                     &pDomainSid,
                     domainGuid);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = NetLogonGetNetBiosNameFromDnsName(
                     pszDnsDomainName,
                     &pszNetBiosName);
        BAIL_ON_NTSTATUS_ERROR(status);

        /* These two fields must be populated */
        status = RpcUnicodeStringAllocateFromCString(
                     &pDomainInfo1->domain_info.domain_name,
                     pszNetBiosName);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = RpcUnicodeStringAllocateFromCString(
                     &pDomainInfo1->domain_info.full_domain_name,
                     pszDnsDomainName);
        BAIL_ON_NTSTATUS_ERROR(status);

        /* TBD:Adam-This may be wrong */
        status = RpcUnicodeStringAllocateFromCString(
                     &pDomainInfo1->domain_info.forest,
                     pszDnsDomainName);
        BAIL_ON_NTSTATUS_ERROR(status);

        

        memcpy(&pDomainInfo1->domain_info.guid, domainGuid, sizeof(domainGuid));  /* Domain GUID */
        pDomainInfo1->domain_info.sid = pDomainSid; /* Domain SID */


        /* Fill in the domain trust "trusts" array */
        status = RpcUnicodeStringAllocateFromCString(
                     &pDomainTrustInfo->domain_name,
                     pszNetBiosName);
        BAIL_ON_NTSTATUS_ERROR(status);

        /* Note: Similar to DnsDomainName, but without the trailing '.' */
        status = NetrLogonGetDomainParameters(
                     &pszTrustDnsDomainName,
                     FALSE,
                     NULL, 
                     NULL);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = RpcUnicodeStringAllocateFromCString(
                     &pDomainTrustInfo->full_domain_name,
                     pszTrustDnsDomainName);
        BAIL_ON_NTSTATUS_ERROR(status);


        /* Fill in the trust domain GUID value */
        memcpy(&pDomainTrustInfo->guid, domainGuid, sizeof(domainGuid));  /* Domain GUID */

        /* Fill in the trust domain SID value */
        dwDomainSidLen = RtlLengthSid(pDomainSid);
        status = NetlogonSrvAllocateMemory(
                 (VOID **) &pDomainSid2,
                 dwDomainSidLen);
        BAIL_ON_NETLOGON_LDAP_ERROR(status);

        status = RtlCopySid(dwDomainSidLen,
                            pDomainSid2,
                            pDomainSid);
        BAIL_ON_NETLOGON_LDAP_ERROR(status);

        pDomainTrustInfo->sid = pDomainSid2; /* Domain Trust SID */

        /* Build joining computer FQDN */
        status = LwRtlWC16StringAllocatePrintfW(
                     &pwszComputerName,
                     L"%ws.%ws",
                     computer_name,
                     pDomainTrustInfo->full_domain_name.Buffer);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = NetlogonWC16StringDuplicate(
                     &pwszRpcComputerName,
                     (PCWSTR) pwszComputerName);
        BAIL_ON_NTSTATUS_ERROR(status);

        pDomainInfo1->dns_hostname_in_ds.Length =
            LwRtlWC16StringNumChars(pwszComputerName) * sizeof(WCHAR);
        pDomainInfo1->dns_hostname_in_ds.MaximumLength =
            pDomainInfo1->dns_hostname_in_ds.Length + sizeof(WCHAR);
        /* Replace malloc buffer with rpc_ss_allocate buffer */
        LW_SAFE_FREE_MEMORY(pDomainInfo1->dns_hostname_in_ds.Buffer);
        pDomainInfo1->dns_hostname_in_ds.Buffer = pwszRpcComputerName;

        /*
         * 2.2.1.3.10 NETLOGON_ONE_DOMAIN_INFO
         * TrustExtension: An RPC_UNICODE_STRING structure, as specified in
         * [MS-DTYP] section 2.3.10, which does not point to a Unicode string,
         * but in fact points to a buffer of size 16, in bytes, in the
         * following format.
         */
        pTrustExtension = &pDomainTrustInfo->TrustExtension;
        pTrustExtension->Length = 16;
        pTrustExtension->MaximumLength = 16;
        status = NetlogonSrvAllocateMemory(
                     (VOID **) &pTrustExtension->Buffer,
                     pTrustExtension->MaximumLength);
        BAIL_ON_NTSTATUS_ERROR(status);
        trustPtr = (PBYTE) pTrustExtension->Buffer;

        /* Buffer is a structure with the following 4 DWORD fields:
         * Flags
         * ParentIndex
         * TrustType
         * TrustAttributes
         * 2.2.1.6.2 DS_DOMAIN_TRUSTSW defines the semantics of these fields
         */
        memcpy(trustPtr, &trustAttributes, sizeof(trustAttributes));
        trustPtr += sizeof(trustAttributes);

        memcpy(trustPtr, &trustParentIndex, sizeof(trustParentIndex));
        trustPtr += sizeof(trustParentIndex);

        memcpy(trustPtr, &trustType, sizeof(trustType));
        trustPtr += sizeof(trustType);

        memcpy(trustPtr, &trustFlags, sizeof(trustFlags));
        trustPtr += sizeof(trustFlags);
    }
    else
    {
        status = LW_RPC_NT_INVALID_TAG;
        goto error;
    }

    pDomainInfo1->workstation_flags = workstationFlags;
    pDomainInfo1->supported_types = supportedEncryptionTypes;

    /* zzz  fill in the trusted domains structure */
    pDomainInfo1->num_trusts = numTrusts;
    pDomainInfo1->trusts = pDomainTrustInfo;
    info->info1 = pDomainInfo1;

cleanup:
    LW_SAFE_FREE_MEMORY(pszNetBiosName);
    NetrLogonQuery1FreeVersion(pQuery1);
    return status;

error:
    RpcUnicodeStringFree(&pDomainInfo1->domain_info.domain_name);
    RpcUnicodeStringFree(&pDomainInfo1->domain_info.full_domain_name);
    RpcUnicodeStringFree(&pDomainInfo1->domain_info.forest);
    LW_SAFE_FREE_MEMORY(pwszRpcComputerName);
    if (pTrustExtension)
    {
        RpcUnicodeStringFree(pTrustExtension);
    }
    if (pDomainInfo1)
    {
        NetlogonSrvFreeMemory(pDomainInfo1);
    }
    goto cleanup;
}


NTSTATUS srv_netr_Function1e(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function1f(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function20(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function21(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function22(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function23(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


	/* function 0x24 */
NTSTATUS srv_NetrEnumerateTrustedDomainsEx(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [out] */ NetrDomainTrustList *domain_trusts
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}



NTSTATUS srv_netr_Function25(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function26(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


	/* function 0x27 */
NTSTATUS srv_NetrLogonSamLogonEx(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ UINT16 logon_level,
        /* [in] */ NetrLogonInfo *logon,
        /* [in] */ UINT16 validation_level,
        /* [out] */ NetrValidationInfo *validation,
        /* [out] */ UINT8 *authoritative,
        /* [in] */ UINT32 *flags
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}

static
NTSTATUS
NetLogonGetNetBiosNameFromDnsName(
    PSTR pszDnsName,
    PSTR *ppszNetBiosName)
{
    PSTR pszNetBiosName = NULL;
    NTSTATUS status = 0;
    DWORD i = 0;

    status = LwRtlCStringDuplicate(&pszNetBiosName,
                                   pszDnsName);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i=0; pszNetBiosName[i] && i < 16 && pszNetBiosName[i] != '.'; i++)
    {
        pszNetBiosName[i] = (char) toupper((int) pszNetBiosName[i]);
    }
    pszNetBiosName[i] = '\0';
    
    *ppszNetBiosName = pszNetBiosName;

cleanup:
    return status;

error:
    LW_SAFE_FREE_MEMORY(pszNetBiosName);
    goto cleanup;
}


static
NTSTATUS
NetrLogonGetDomainParameters(
    PSTR *ppszDnsDomainName,
    BOOLEAN bDnsDomainNameComplete,
    PSID *pRetDomainSid,
    uuid_t retDomainGuid)
{
    NTSTATUS status = 0;
    PSTR pszDnsDomainName = NULL;
    size_t dnsDomainNameLen = 0;
    PNETLOGON_AUTH_PROVIDER_CONTEXT pContext = NULL;
    LDAP *pLd = NULL;
    PSTR pszDomainDn = NULL;
    PSTR ppszAttributes[] = { "objectSid", "objectGUID", NULL };
    struct berval **bv_objectValue = NULL;
    PSID pDomainSid = NULL;
    PSID pDomainSidAlloc = NULL;
    DWORD dwDomainSidLen = 0;
    uuid_t domainGuid;
    DWORD dwDomainGuidLen = 0;
    HANDLE hDirectory = NULL;
    PVOID pDomainGuid = NULL;
    DWORD i = 0;

    status = NetlogonLdapOpen(&hDirectory);
    BAIL_ON_NTSTATUS_ERROR(status);

    pContext = (PNETLOGON_AUTH_PROVIDER_CONTEXT) hDirectory;
    pLd = pContext->dirContext.pLd;
    LDAPMessage *pObjects = NULL;

    /* Obtain the domain name for this DC */
    /* +1 for '.', +1 for \0 */
    dnsDomainNameLen =
        LwRtlCStringNumChars(pContext->dirContext.pBindInfo->pszDomainFqdn) + 1;
    status = LwAllocateMemory(dnsDomainNameLen + 1, (PVOID*)&pszDnsDomainName);
    BAIL_ON_NETLOGON_LDAP_ERROR(status);
    memcpy(pszDnsDomainName, pContext->dirContext.pBindInfo->pszDomainFqdn, dnsDomainNameLen);
    status = LwLdapConvertDomainToDN(pszDnsDomainName,
                                     &pszDomainDn);
    BAIL_ON_NETLOGON_LDAP_ERROR(status);


    /* Get the domain objectSid */
    status = NetlogonLdapQueryObjects(
                 pLd,
                 pszDomainDn,
                 LDAP_SCOPE_SUBTREE,
                 "(objectClass=*)",
                 ppszAttributes,
                 -1,
                 &pObjects);
    BAIL_ON_NETLOGON_LDAP_ERROR(status);

    bv_objectValue = ldap_get_values_len(pLd, pObjects, ppszAttributes[0]);
    if (bv_objectValue && bv_objectValue[0])
    {
        /*
         * Allocate rpc_ss_alloc() memory, then copy the domainSID
         */
        pDomainSid = (PSID) bv_objectValue[0]->bv_val;
        dwDomainSidLen = RtlLengthSid(pDomainSid);
        status = NetlogonSrvAllocateMemory(
                 (VOID **) &pDomainSidAlloc,
                 dwDomainSidLen);
        BAIL_ON_NETLOGON_LDAP_ERROR(status);

        status = RtlCopySid(dwDomainSidLen,
                            pDomainSidAlloc,
                            pDomainSid);
        BAIL_ON_NETLOGON_LDAP_ERROR(status);
    }

    /* Get the domain objectGUID */
    bv_objectValue = ldap_get_values_len(pLd, pObjects, ppszAttributes[1]);
    if (bv_objectValue && bv_objectValue[0])
    {
        pDomainGuid = bv_objectValue[0]->bv_val;
        dwDomainGuidLen = bv_objectValue[0]->bv_len;
    }

    /* Convert DomainGuid to binary form */
    if (dwDomainGuidLen == 16)
    {
        memcpy(domainGuid, pDomainGuid, dwDomainGuidLen);
    }
    else if (dwDomainGuidLen != 36 || uuid_parse(pDomainGuid, domainGuid) != 0)
    {
        status = STATUS_NO_GUID_TRANSLATION;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    /* lower case domain name */
    for (i=0; pszDnsDomainName[i]; i++)
    {
        pszDnsDomainName[i] = (char) tolower((int) pszDnsDomainName[i]);
    }

    /* Extra space was allocated for this terminating '.' above... */
    if (bDnsDomainNameComplete)
    {
        pszDnsDomainName[i] = '.';
    }

    *ppszDnsDomainName = pszDnsDomainName;
    if (pRetDomainSid)
    {
        *pRetDomainSid = pDomainSidAlloc;
        pDomainSidAlloc = NULL;
    }
    if (retDomainGuid)
    {
        memcpy(retDomainGuid, domainGuid, sizeof(domainGuid));
    }

cleanup:
    if (pDomainSidAlloc)
    {
        NetlogonSrvFreeMemory(pDomainSidAlloc);
    }
    NetlogonLdapRelease(hDirectory);
    return status;

error:
    LW_SAFE_FREE_MEMORY(pszDnsDomainName);

    goto cleanup;
}

static
VOID
RpcUnicodeStringFree(
    RPC_PUNICODE_STRING pString)
{
    if (pString && pString->Buffer)
    {
        NetlogonSrvFreeMemory(pString->Buffer);
    }
}

static
NTSTATUS
RpcUnicodeStringAllocateFromCString(
    RPC_PUNICODE_STRING pString,
    PCSTR pszString
    )
{
    NTSTATUS status = 0;
    USHORT allocLen = 0;
    PWCHAR allocBuf = NULL;
    UNICODE_STRING tmpUnicodeStr = {0};

    /* Convert C string to UNICODE */
    status = LwRtlUnicodeStringAllocateFromCString(
                 &tmpUnicodeStr,
                 pszString);
    BAIL_ON_NTSTATUS_ERROR(status);

    allocLen = tmpUnicodeStr.MaximumLength;
    status = NetlogonSrvAllocateMemory(
                 (VOID **) &allocBuf,
                 allocLen);
    BAIL_ON_NTSTATUS_ERROR(status);

    pString->Length = tmpUnicodeStr.Length;
    pString->MaximumLength = tmpUnicodeStr.MaximumLength;

    /* Return RPC_UNICODE_STRING string with rpc_ss_alloc() memory */
    pString->Buffer = allocBuf;
    memcpy(allocBuf, tmpUnicodeStr.Buffer, allocLen);

cleanup:
    LwRtlUnicodeStringFree(&tmpUnicodeStr);
    return status;

error:
    NetlogonSrvFreeMemory(allocBuf);
    if (allocBuf)
    {
        NetlogonSrvFreeMemory(allocBuf);
    }
    goto cleanup;
}

static
NTSTATUS
NetlogonWC16StringDuplicate(
    PWSTR *ppwszString,
    PCWSTR pwszString
    )
{
    NTSTATUS status = 0;
    PWSTR pwszRetString = NULL;
    size_t allocLen = 0;

    /* Length in bytes of WC16 string, including '\0' terminator */
    allocLen = (LwRtlWC16StringNumChars(pwszString) + 1) * sizeof(WCHAR);
    status = NetlogonSrvAllocateMemory(
                 (VOID **) &pwszRetString,
                 allocLen);
    BAIL_ON_NTSTATUS_ERROR(status);

    memcpy(pwszRetString, pwszString, allocLen);
    *ppwszString = pwszRetString;

cleanup:
    return status;

error:
    LW_SAFE_FREE_MEMORY(pwszRetString);
    goto cleanup;
   
}

static
NTSTATUS
NetlogonCStringAllocateFromWC16String(
    PSTR *ppszString,
    PWSTR pwszString)
{
    NTSTATUS status = 0;
    size_t allocLen = 0;
    PSTR pszRetString = NULL;
    PSTR pszTmpString = NULL;

    status = LwRtlCStringAllocateFromWC16String(
                  &pszTmpString,
                  pwszString);
    BAIL_ON_NTSTATUS_ERROR(status);

    allocLen = LwRtlWC16StringNumChars(pwszString);
    status = NetlogonSrvAllocateMemory(
                 (VOID **) &pszRetString,
                 allocLen + 1);
    BAIL_ON_NTSTATUS_ERROR(status);

    memcpy(pszRetString, pszTmpString, allocLen);
    *ppszString = pszRetString;

cleanup:
    LW_SAFE_FREE_MEMORY(pszTmpString);
    return status;

error:
    if (pszRetString)
    {
        NetlogonSrvFreeMemory(pszRetString);
    }
    goto cleanup;
}

/* function 0x28 */
WINERROR
srv_DsrEnumerateDomainTrusts(
        /* [in] */  handle_t IDL_handle,
        /* [in] */  wchar16_t *server_name,
        /* [in] */  UINT32 trust_flags,
        /* [out] */ NetrDomainTrustList *trusts
        )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszServerName = NULL;
    PSTR pszDnsDomainName = NULL;
    DWORD dwDomainTrustCount = 1;
    NetrDomainTrust *pDomainTrustArray = {0};
    PSTR pszNetBiosName = NULL;

#if 1 /* TBD:Adam-Perform ldap queries to get this data; hard code now */
    /* 0x1d */
    DWORD dwTrustFlags = NETR_TRUST_FLAG_NATIVE  | NETR_TRUST_FLAG_PRIMARY |
                         NETR_TRUST_FLAG_TREEROOT | NETR_TRUST_FLAG_IN_FOREST;
    DWORD dwParentIndex = 0x00;
    DWORD dwTrustType = 0x02;
    DWORD dwTrustAttrs = 0x00;
#endif

    PWSTR pwszNetBiosName = NULL;
    PWSTR pwszDnsDomainName = NULL;
    PSID pDomainSid = NULL;
    uuid_t domainGuid;

    status = NetrLogonGetDomainParameters(
                 &pszDnsDomainName,
                 FALSE,
                 &pDomainSid,
                 domainGuid);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetLogonGetNetBiosNameFromDnsName(
                 pszDnsDomainName,
                 &pszNetBiosName);
    BAIL_ON_NTSTATUS_ERROR(status);

    /*
     * I am the DC, so return data returned from calling
     *  NetrLogonGetDomainInfo()
     *
     * "If the server is a domain controller (section 3.1.4.8), it MUST perform
     *  behavior equivalent to locally invoking NetrLogonGetDomainInfo with
     *  the previously described parameters."
     */
    status = LwRtlCStringAllocateFromWC16String(
                 &pszServerName,
                 server_name);
    BAIL_ON_NTSTATUS_ERROR(status);

    LSA_LOG_ERROR("srv_NetrEnumerateTrustedDomainsEx: server_name=%s trusts=%x",
                  pszServerName, trust_flags);

    status = LwRtlWC16StringAllocateFromCString(
                 &pwszNetBiosName,
                 pszNetBiosName);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = LwRtlWC16StringAllocateFromCString(
                 &pwszDnsDomainName,
                 pszDnsDomainName);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* Populate this value! */
    status = NetlogonSrvAllocateMemory(
                 (VOID **) &pDomainTrustArray,
                 sizeof(NetrDomainTrust) * dwDomainTrustCount);
    BAIL_ON_NTSTATUS_ERROR(status);

    pDomainTrustArray[0].netbios_name = pwszNetBiosName;
    pDomainTrustArray[0].dns_name = pwszDnsDomainName;
    pDomainTrustArray[0].trust_flags = dwTrustFlags;
    pDomainTrustArray[0].parent_index = dwParentIndex;
    pDomainTrustArray[0].trust_type = dwTrustType;
    pDomainTrustArray[0].trust_attrs = dwTrustAttrs; /* TBD:Adam ??? */
    pDomainTrustArray[0].sid = pDomainSid;
    memcpy(&pDomainTrustArray[0].guid, &domainGuid, sizeof(domainGuid));

    trusts->count = dwDomainTrustCount;
    trusts->array = pDomainTrustArray;

cleanup:
    LW_SAFE_FREE_MEMORY(pszServerName);

    return status;

error:
    status = status ? status : dwError;
    LW_SAFE_FREE_MEMORY(pszNetBiosName);
    LW_SAFE_FREE_MEMORY(pszDnsDomainName);
    LW_SAFE_FREE_MEMORY(pwszNetBiosName);
    LW_SAFE_FREE_MEMORY(pwszDnsDomainName);
    LW_SAFE_FREE_MEMORY(pDomainSid);
    if (pDomainTrustArray)
    {
        NetlogonSrvFreeMemory(pDomainTrustArray);
    }

    goto cleanup;
}



NTSTATUS srv_netr_Function29(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2c(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2d(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2e(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2f(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function30(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function31(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

#if 0 /* MS-NRPC does not define any functions beyond 31 */
NTSTATUS srv_netr_Function32(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function33(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function34(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function35(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function36(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function37(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function38(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function39(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function3a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function3b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

#endif


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
