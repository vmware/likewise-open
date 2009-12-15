/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        samr_crypto.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Encrypted password blob handling functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
SamrSrvDecodePasswordBuffer(
    IN  PBYTE   pBlob,
    IN  DWORD   dwBlobSize,
    OUT PWSTR  *ppwszPassword,
    OUT PDWORD  pdwPasswordLen
    );


static
NTSTATUS
SamrSrvEncodePasswordBuffer(
    IN  PCWSTR  pwszPassword,
    IN  PBYTE   pBlobInit,
    OUT PBYTE  *ppBlob,
    OUT PDWORD  pdwBlobSize
    );


NTSTATUS
SamrSrvDecryptPasswordBlobEx(
    IN  PCONNECT_CONTEXT  pConnCtx,
    IN  CryptPasswordEx  *pPassBlob,
    IN  UINT8             PassLen,
    OUT PWSTR            *ppwszPassword
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PBYTE pPlainTextBlob = NULL;
    DWORD dwPlainTextBlobSize = 0;
    DWORD dwPasswordSize = 0;
    DWORD dwPassBlobSize = 0;
    BYTE KeyInit[16] = {0};
    BYTE DigestedSessionKey[16] = {0};
    MD5_CTX ctx;
    RC4_KEY key;
    PWSTR pwszPassword = NULL;
    DWORD dwPasswordLen = 0;
    CryptPasswordEx PassBlobVerifier;

    BAIL_ON_INVALID_PTR(pConnCtx);
    BAIL_ON_INVALID_PTR(pPassBlob);
    BAIL_ON_INVALID_PTR(ppwszPassword);

    dwPasswordSize      = (PassLen + 1) * sizeof(WCHAR);
    dwPassBlobSize      = sizeof(pPassBlob->data);
    dwPlainTextBlobSize = dwPassBlobSize - sizeof(KeyInit);
    memset(&ctx, 0, sizeof(ctx));
    memset(&key, 0, sizeof(key));
    memset(&PassBlobVerifier, 0, sizeof(PassBlobVerifier));

    /*
     * Allocate memory for plain text password
     */
    dwError = LwAllocateMemory(dwPlainTextBlobSize,
                               OUT_PPVOID(&pPlainTextBlob));
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Copy crypto key initialisator
     */
    memcpy(KeyInit,
           &(pPassBlob->data[dwPassBlobSize - sizeof(KeyInit)]),
           sizeof(KeyInit));

    /*
     * Prepare the session key digested with key initialisator
     */
    MD5_Init(&ctx);
    MD5_Update(&ctx, KeyInit, sizeof(KeyInit));
    MD5_Update(&ctx, pConnCtx->pSessionKey, pConnCtx->dwSessionKeyLen);
    MD5_Final(DigestedSessionKey, &ctx);

    /*
     * Set the key and decrypt the plain text password buffer
     */
    RC4_set_key(&key,
                sizeof(DigestedSessionKey),
                (unsigned char*)DigestedSessionKey);
    RC4(&key,
        dwPlainTextBlobSize,
        (const unsigned char*)pPassBlob->data,
        (unsigned char*)pPlainTextBlob);

    /*
     * Get the unicode password from plain text blob
     */
    ntStatus = SamrSrvDecodePasswordBuffer(pPlainTextBlob,
                                           dwPlainTextBlobSize,
                                           &pwszPassword,
                                           &dwPasswordLen);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Basic check if the password has been decrypted correctly
     */
    if (PassLen != dwPasswordLen)
    {
        ntStatus = STATUS_WRONG_PASSWORD;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * More careful check - password, key init and blob init should
     * yield the same encrypted blob as the function input
     */
    ntStatus = SamrSrvEncryptPasswordBlobEx(pConnCtx,
                                            pwszPassword,
                                            KeyInit,
                                            sizeof(KeyInit),
                                            pPlainTextBlob,
                                            &PassBlobVerifier);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (memcmp(pPassBlob->data, PassBlobVerifier.data,
               sizeof(pPassBlob->data)))
    {
        ntStatus = STATUS_WRONG_PASSWORD;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    *ppwszPassword = pwszPassword;

cleanup:
    if (pPlainTextBlob)
    {
        memset(pPlainTextBlob, 0, dwPlainTextBlobSize);
        LW_SAFE_FREE_MEMORY(pPlainTextBlob);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    *ppwszPassword = NULL;

    goto cleanup;
}


static
NTSTATUS
SamrSrvDecodePasswordBuffer(
    IN  PBYTE   pBlob,
    IN  DWORD   dwBlobSize,
    OUT PWSTR  *ppwszPassword,
    OUT PDWORD  pdwPasswordLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwPasswordLen = 0;
    DWORD iByte = dwBlobSize;
    PWSTR pwszPasswordLE = NULL;
    PWSTR pwszPassword = NULL;

    /*
     * Decode the password length (in bytes) - the last 4 bytes
     */
    dwPasswordLen |= pBlob[--iByte] << 24;
    dwPasswordLen |= pBlob[--iByte] << 16;
    dwPasswordLen |= pBlob[--iByte] << 8;
    dwPasswordLen |= pBlob[--iByte];

    dwError = LwAllocateMemory(dwPasswordLen + sizeof(pwszPasswordLE[0]),
                               OUT_PPVOID(&pwszPasswordLE));
    BAIL_ON_LSA_ERROR(dwError);

    if (dwPasswordLen > iByte)
    {
        ntStatus = STATUS_WRONG_PASSWORD;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * Copy the password - it's right before the length bytes
     */
    iByte -= dwPasswordLen;
    memcpy(pwszPasswordLE, &(pBlob[iByte]), dwPasswordLen);

    dwError = LwAllocateMemory(dwPasswordLen + sizeof(pwszPassword[0]),
                               OUT_PPVOID(&pwszPassword));
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Copied password is a 2-byte little-endian string. Make
     * sure we return a string in native endianness
     */
    wc16lestowc16s(pwszPassword, pwszPasswordLE, dwPasswordLen);

    *ppwszPassword  = pwszPassword;
    *pdwPasswordLen = dwPasswordLen / 2;

cleanup:
    if (pwszPasswordLE)
    {
        memset(pwszPasswordLE, 0, dwPasswordLen);
        LW_SAFE_FREE_MEMORY(pwszPasswordLE);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pwszPassword)
    {
        memset(pwszPassword, 0, dwPasswordLen);
        LW_SAFE_FREE_MEMORY(pwszPassword);
    }

    *ppwszPassword = NULL;

    goto cleanup;
}


NTSTATUS
SamrSrvEncryptPasswordBlobEx(
    IN  PCONNECT_CONTEXT  pConnCtx,
    IN  PCWSTR            pwszPassword,
    IN  PBYTE             pKeyInit,
    IN  DWORD             dwKeyInitLen,
    IN  PBYTE             pBlobInit,
    OUT CryptPasswordEx  *pEncryptedPassBlob
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PBYTE pPassBlob = NULL;
    DWORD dwPassBlobLen = 0;
    BYTE DigestedSessionKey[16] = {0};
    MD5_CTX ctx;
    RC4_KEY key;
    PBYTE pEncryptedBlob = NULL;

    memset(&ctx, 0, sizeof(ctx));
    memset(&key, 0, sizeof(key));

    ntStatus = SamrSrvEncodePasswordBuffer(pwszPassword,
                                           pBlobInit,
                                           &pPassBlob,
                                           &dwPassBlobLen);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwAllocateMemory(dwPassBlobLen,
                               OUT_PPVOID(&pEncryptedBlob));
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Prepare the session key digested with key initialisator
     */
    MD5_Init(&ctx);
    MD5_Update(&ctx, pKeyInit, dwKeyInitLen);
    MD5_Update(&ctx, pConnCtx->pSessionKey, pConnCtx->dwSessionKeyLen);
    MD5_Final(DigestedSessionKey, &ctx);

    /*
     * Set the key and encrypt the plain text password buffer
     */
    RC4_set_key(&key,
                sizeof(DigestedSessionKey),
                (unsigned char*)DigestedSessionKey);

    RC4(&key,
        dwPassBlobLen,
        (const unsigned char*)pPassBlob,
        (unsigned char*)pEncryptedBlob);

    if (dwPassBlobLen + dwKeyInitLen > sizeof(pEncryptedPassBlob->data))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    memcpy(pEncryptedPassBlob->data,
           pEncryptedBlob,
           dwPassBlobLen);
    memcpy(&(pEncryptedPassBlob->data[dwPassBlobLen]),
           pKeyInit,
           dwKeyInitLen);

cleanup:
    if (pPassBlob)
    {
        memset(pPassBlob, 0, dwPassBlobLen);
        LW_SAFE_FREE_MEMORY(pPassBlob);
    }

    if (pEncryptedBlob)
    {
        memset(pEncryptedBlob, 0, dwPassBlobLen);
        LW_SAFE_FREE_MEMORY(pEncryptedBlob);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    memset(pEncryptedBlob, 0, sizeof(*pEncryptedBlob));

    goto cleanup;
}


static
NTSTATUS
SamrSrvEncodePasswordBuffer(
    IN  PCWSTR  pwszPassword,
    IN  PBYTE   pBlobInit,
    OUT PBYTE  *ppBlob,
    OUT PDWORD  pdwBlobSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    size_t sPasswordLen = 0;
    size_t sPasswordSize = 0;
    PWSTR pwszPasswordLE = NULL;
    BYTE PasswordBlob[516] = {0};
    DWORD iByte = 0;
    PBYTE pBlob = NULL;
    DWORD dwBlobSize = 0;

    dwError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    /* size doesn't include terminating zero here */
    sPasswordSize = sPasswordLen * sizeof(pwszPassword[0]);

    /*
     * Make sure encoded password is 2-byte little-endian
     */
    dwError = LwAllocateMemory(sPasswordSize + sizeof(pwszPassword[0]),
                               OUT_PPVOID(&pwszPasswordLE));
    BAIL_ON_LSA_ERROR(dwError);

    wc16stowc16les(pwszPasswordLE, pwszPassword, sPasswordLen);

    /*
     * Encode the password length (in bytes) - the last 4 bytes
     */
    iByte = sizeof(PasswordBlob);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 24) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 16) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 8) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize) & 0xff);

    /*
     * Copy the password and the initial random bytes
     */
    iByte -= sPasswordSize;
    memcpy(&(PasswordBlob[iByte]),
           pwszPasswordLE,
           sPasswordSize);

    memcpy(PasswordBlob,
           pBlobInit,
           iByte);

    dwBlobSize = sizeof(PasswordBlob);
    dwError = LwAllocateMemory(dwBlobSize,
                               OUT_PPVOID(&pBlob));
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pBlob, PasswordBlob, dwBlobSize);

    *ppBlob      = pBlob;
    *pdwBlobSize = dwBlobSize;

cleanup:
    if (pBlob)
    {
        memset(PasswordBlob, 0, sizeof(PasswordBlob));
    }

    if (pwszPasswordLE)
    {
        memset(pwszPasswordLE, 0, sPasswordSize);
        LW_SAFE_FREE_MEMORY(pwszPasswordLE);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pBlob)
    {
        memset(pBlob, 0, dwBlobSize);
        LW_SAFE_FREE_MEMORY(pBlob);
    }

    *ppBlob      = NULL;
    *pdwBlobSize = 0;

    goto cleanup;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
