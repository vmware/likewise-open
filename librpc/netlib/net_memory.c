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
 *        net_memory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetAPI memory allocation functions.
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
NetInitMemory(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    /* Init allocation of dependant rpc libraries first */
    status = LsaRpcInitMemory();
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrInitMemory();
    BAIL_ON_NTSTATUS_ERROR(status);

    if (!bNetApiInitialised) {
        status = MemPtrListInit((PtrList**)&netapi_ptr_list);
        BAIL_ON_NTSTATUS_ERROR(status);

        bNetApiInitialised = 1;
    }
cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    goto cleanup;
}


NTSTATUS
NetDestroyMemory(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (bNetApiInitialised && netapi_ptr_list) {
        status = MemPtrListDestroy((PtrList**)&netapi_ptr_list);
        BAIL_ON_NTSTATUS_ERROR(status);

        bNetApiInitialised = 0;
    }

    /* Destroy allocation of dependant rpc libraries */
    status = LsaRpcDestroyMemory();
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrDestroyMemory();
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    goto cleanup;
}


NTSTATUS
NetAllocateMemory(
    void **out,
    size_t size,
    void *dep
    )
{
    return MemPtrAllocate((PtrList*)netapi_ptr_list, out, size, dep);
}


NTSTATUS
NetFreeMemory(
    void *ptr
    )
{
    return MemPtrFree((PtrList*)netapi_ptr_list, ptr);
}


NTSTATUS
NetAddDepMemory(
    void *ptr,
    void *dep
    )
{
    return MemPtrAddDependant((PtrList*)netapi_ptr_list, ptr, dep);
}


DWORD
NetAllocBufferByte(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    BYTE    ubSource,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PBYTE pubDest = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    dwSize = sizeof(ubSource);

    if (pCursor)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WINERR_ERROR(err);
        }

        pubDest  = (PBYTE)pCursor;
        *pubDest = ubSource;

        pCursor      += dwSize;
        dwSpaceLeft  -= dwSize;
        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


DWORD
NetAllocBufferWord(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    WORD    wSource,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PWORD pwDest = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    dwSize = sizeof(wSource);

    if (pCursor)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WINERR_ERROR(err);
        }

        pwDest    = (PWORD)pCursor;
        *pwDest   = wSource;

        pCursor      += dwSize;
        dwSpaceLeft  -= dwSize;
        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


DWORD
NetAllocBufferDword(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    DWORD   dwSource,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PDWORD pdwDest = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    dwSize = sizeof(dwSource);

    if (pCursor)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WINERR_ERROR(err);
        }

        pdwDest   = (PDWORD)pCursor;
        *pdwDest  = dwSource;

        pCursor      += dwSize;
        dwSpaceLeft  -= dwSize;
        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


DWORD
NetAllocBufferUlong64(
    PVOID   *ppCursor,
    PDWORD   pdwSpaceLeft,
    ULONG64  ullSource,
    PDWORD   pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PULONG64 pullDest = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    dwSize = sizeof(ullSource);

    if (pCursor)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WINERR_ERROR(err);
        }

        pullDest      = (PULONG64)pCursor;
        *pullDest     = ullSource;

        pCursor      += dwSize;
        dwSpaceLeft  -= dwSize;
        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


DWORD
NetAllocBufferWinTimeFromNtTime(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    NtTime  Time,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD dwTime = LwNtTimeToWinTime((ULONG64)Time);

    err = NetAllocBufferDword(ppCursor,
                              pdwSpaceLeft,
                              dwTime,
                              pdwSize);
    return err;
}


DWORD
NetAllocBufferNtTimeFromWinTime(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    DWORD   dwTime,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    ULONG64 ullTime = LwWinTimeToNtTime(dwTime);

    err = NetAllocBufferUlong64(ppCursor,
                                pdwSpaceLeft,
                                ullTime,
                                pdwSize);
    return err;
}


#define SET_USER_FLAG(acb_flags, uf_flags)           \
    if (dwAcbFlags & (acb_flags))                    \
    {                                                \
        dwUserFlags |= (uf_flags);                   \
    }


DWORD
NetAllocBufferUserFlagsFromAcbFlags(
    PVOID *ppCursor,
    PDWORD pdwSpaceLeft,
    DWORD  dwAcbFlags,
    PDWORD pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD dwUserFlags = 0;

    /* ACB flags not covered:
        - ACB_NO_AUTH_DATA_REQD
        - ACB_MNS
        - ACB_AUTOLOCK

       UF flags not covered:
        - UF_SCRIPT
        - UF_LOCKOUT
        - UF_PASSWD_CANT_CHANGE,
        - UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION
    */

    SET_USER_FLAG(ACB_DISABLED, UF_ACCOUNTDISABLE);
    SET_USER_FLAG(ACB_HOMDIRREQ, UF_HOMEDIR_REQUIRED);
    SET_USER_FLAG(ACB_PWNOTREQ, UF_PASSWD_NOTREQD);
    SET_USER_FLAG(ACB_TEMPDUP, UF_TEMP_DUPLICATE_ACCOUNT);
    SET_USER_FLAG(ACB_NORMAL, UF_NORMAL_ACCOUNT);
    SET_USER_FLAG(ACB_DOMTRUST, UF_INTERDOMAIN_TRUST_ACCOUNT);
    SET_USER_FLAG(ACB_WSTRUST, UF_WORKSTATION_TRUST_ACCOUNT);
    SET_USER_FLAG(ACB_SVRTRUST, UF_SERVER_TRUST_ACCOUNT);
    SET_USER_FLAG(ACB_PWNOEXP, UF_DONT_EXPIRE_PASSWD);
    SET_USER_FLAG(ACB_ENC_TXT_PWD_ALLOWED, UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED);
    SET_USER_FLAG(ACB_SMARTCARD_REQUIRED, UF_SMARTCARD_REQUIRED);
    SET_USER_FLAG(ACB_TRUSTED_FOR_DELEGATION, UF_TRUSTED_FOR_DELEGATION);
    SET_USER_FLAG(ACB_NOT_DELEGATED, UF_NOT_DELEGATED);
    SET_USER_FLAG(ACB_USE_DES_KEY_ONLY, UF_USE_DES_KEY_ONLY);
    SET_USER_FLAG(ACB_DONT_REQUIRE_PREAUTH, UF_DONT_REQUIRE_PREAUTH);
    SET_USER_FLAG(ACB_PW_EXPIRED, UF_PASSWORD_EXPIRED);

    err = NetAllocBufferDword(ppCursor,
                              pdwSpaceLeft,
                              dwUserFlags,
                              pdwSize);
    return err;
}


#define SET_ACB_FLAG(uf_flags, acb_flags)            \
    if (dwUserFlags & (uf_flags))                    \
    {                                                \
        dwAcbFlags |= (acb_flags);                   \
    }


DWORD
NetAllocBufferAcbFlagsFromUserFlags(
    PVOID *ppCursor,
    PDWORD pdwSpaceLeft,
    DWORD  dwUserFlags,
    PDWORD pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD dwAcbFlags = 0;

    /* ACB flags not covered:
        - ACB_NO_AUTH_DATA_REQD
        - ACB_MNS
        - ACB_AUTOLOCK

       UF flags not covered:
        - UF_SCRIPT
        - UF_LOCKOUT
        - UF_PASSWD_CANT_CHANGE,
        - UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION
    */

    SET_ACB_FLAG(UF_ACCOUNTDISABLE, ACB_DISABLED);
    SET_ACB_FLAG(UF_HOMEDIR_REQUIRED, ACB_HOMDIRREQ);
    SET_ACB_FLAG(UF_PASSWD_NOTREQD, ACB_PWNOTREQ);
    SET_ACB_FLAG(UF_TEMP_DUPLICATE_ACCOUNT, ACB_TEMPDUP);
    SET_ACB_FLAG(UF_NORMAL_ACCOUNT, ACB_NORMAL);
    SET_ACB_FLAG(UF_INTERDOMAIN_TRUST_ACCOUNT, ACB_DOMTRUST);
    SET_ACB_FLAG(UF_WORKSTATION_TRUST_ACCOUNT, ACB_WSTRUST);
    SET_ACB_FLAG(UF_SERVER_TRUST_ACCOUNT, ACB_SVRTRUST);
    SET_ACB_FLAG(UF_DONT_EXPIRE_PASSWD, ACB_PWNOEXP);
    SET_ACB_FLAG(UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED, ACB_ENC_TXT_PWD_ALLOWED);
    SET_ACB_FLAG(UF_SMARTCARD_REQUIRED, ACB_SMARTCARD_REQUIRED);
    SET_ACB_FLAG(UF_TRUSTED_FOR_DELEGATION, ACB_TRUSTED_FOR_DELEGATION);
    SET_ACB_FLAG(UF_NOT_DELEGATED, ACB_NOT_DELEGATED);
    SET_ACB_FLAG(UF_USE_DES_KEY_ONLY, ACB_USE_DES_KEY_ONLY);
    SET_ACB_FLAG(UF_DONT_REQUIRE_PREAUTH, ACB_DONT_REQUIRE_PREAUTH);
    SET_ACB_FLAG(UF_PASSWORD_EXPIRED, ACB_PW_EXPIRED);

    err = NetAllocBufferDword(ppCursor,
                              pdwSpaceLeft,
                              dwAcbFlags,
                              pdwSize);
    return err;
}


DWORD
NetAllocBufferWC16String(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    PCWSTR  pwszSource,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PWSTR *ppwszDest = NULL;
    PVOID pStr = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pwszSource)
    {
        err = LwWc16sLen(pwszSource, (size_t*)&dwSize);
        BAIL_ON_WINERR_ERROR(err);

        /* it's a 2-byte unicode string */
        dwSize *= 2;

        /* string termination */
        dwSize += sizeof(WCHAR);
    }

    if (pCursor && pwszSource)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WINERR_ERROR(err);
        }

        pStr = (pCursor + dwSpaceLeft) - dwSize;

        /* sanity check - the string and current buffer cursor
           must not overlap */
        if ((pCursor + sizeof(PWSTR)) > pStr)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WINERR_ERROR(err);
        }

        err = LwWc16snCpy((PWSTR)pStr, pwszSource, (dwSize / 2) - 1);
        BAIL_ON_WINERR_ERROR(err);

        /* recalculate size and space after copying the string */
        ppwszDest     = (PWSTR*)pCursor;
        *ppwszDest    = (PWSTR)pStr;
        dwSpaceLeft  -= dwSize;

        /* recalculate size and space after setting the string pointer */
        pCursor      += sizeof(PWSTR);
        dwSpaceLeft  -= sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }
    else if (pCursor)
    {
        pCursor      += sizeof(PWSTR);
        dwSpaceLeft  -= sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    /* include size of the pointer */
    dwSize += sizeof(PWSTR);

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


DWORD
NetAllocBufferWC16StringFromUnicodeString(
    PVOID         *ppCursor,
    PDWORD         pdwSpaceLeft,
    UnicodeString *pSource,
    PDWORD         pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PWSTR *ppwszDest = NULL;
    PVOID pStr = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pSource)
    {
        dwSize += pSource->len + sizeof(WCHAR);
    }

    if (pCursor && pSource)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WINERR_ERROR(err);
        }

        pStr = (pCursor + dwSpaceLeft) - dwSize;

        /* sanity check - the string and current buffer cursor
           must not overlap */
        if ((pCursor + sizeof(PWSTR)) > pStr)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WINERR_ERROR(err);
        }

        err = LwWc16snCpy((PWSTR)pStr,
                          pSource->string,
                          pSource->len / 2);
        BAIL_ON_WINERR_ERROR(err);

        /* recalculate size and space after copying the string */
        ppwszDest     = (PWSTR*)pCursor;
        *ppwszDest    = (PWSTR)pStr;
        dwSpaceLeft  -= dwSize;

        /* recalculate size and space after setting the string pointer */
        pCursor      += sizeof(PWSTR);
        dwSpaceLeft  -= sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }
    else if (pCursor)
    {
        pCursor      += sizeof(PWSTR);
        dwSpaceLeft  -= sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    /* include size of the pointer */
    dwSize  += sizeof(PWSTR);

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


DWORD
NetAllocBufferUnicodeStringFromWC16String(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    PCWSTR  pwszSource,
    PDWORD  pdwSize
    )
{
    const WCHAR wszNullStr[] = { '\0' };
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwStrSize = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (!pwszSource)
    {
        pwszSource = &wszNullStr[0];
    }

    err = LwWc16sLen(pwszSource, (size_t*)&dwStrSize);
    BAIL_ON_WINERR_ERROR(err);

    /* it's a 2-byte unicode string */
    dwStrSize *= 2;

    /* string termination */
    dwStrSize += sizeof(WCHAR);

    if (pCursor)
    {
        /* string length field */
        err = NetAllocBufferWord(&pCursor,
                                 &dwSpaceLeft,
                                 (WORD)(dwStrSize - sizeof(WCHAR)),
                                 &dwSize);
        BAIL_ON_WINERR_ERROR(err);

        /* string size field */
        err = NetAllocBufferWord(&pCursor,
                                 &dwSpaceLeft,
                                 (WORD)dwStrSize,
                                 &dwSize);
        BAIL_ON_WINERR_ERROR(err);

        /* the string itself */
        err = NetAllocBufferWC16String(&pCursor,
                                       &dwSpaceLeft,
                                       pwszSource,
                                       &dwSize);
        BAIL_ON_WINERR_ERROR(err);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }
    else
    {
        /* size of length and size fields */
        dwSize += 2 * sizeof(USHORT);

        /* size of the string */
        dwSize += dwStrSize;

        /* size of the string pointer */
        dwSize += sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


DWORD
NetAllocBufferLogonHours(
    PVOID      *ppCursor,
    PDWORD      pdwSpaceLeft,
    LogonHours *pHours,
    PDWORD      pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PBYTE *ppbDest = NULL;
    PBYTE pbBytes = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    /*
     * The actual value of pHours is ignored at the moment
     */

    /* Logon hours is a 21-byte bit string */
    dwSize += sizeof(UINT8) * 21;

    if (pCursor)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WINERR_ERROR(err);
        }

        pbBytes = (pCursor + dwSpaceLeft) - dwSize;

        /* sanity check - the string and current buffer cursor
           must not overlap */
        if ((pCursor + sizeof(PWSTR)) > (PVOID)pbBytes)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WINERR_ERROR(err);
        }

        /* Allow all logon hours */
        memset(pbBytes, 1, dwSize);

        /* recalculate size and space after copying the string */
        ppbDest       = (PBYTE*)pCursor;
        *ppbDest      = (PBYTE)pbBytes;
        dwSpaceLeft  -= dwSize;

        /* recalculate size and space after setting the string pointer */
        pCursor      += sizeof(PWSTR);
        dwSpaceLeft  -= sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }
    else if (pCursor)
    {
        pCursor      += sizeof(PWSTR);
        dwSpaceLeft  -= sizeof(PWSTR);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    /* include size of the pointer */
    dwSize += sizeof(PBYTE);

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


/*
 * See the definition of LogonHours in include/lwrpc/samrdefs.h
 */
DWORD
NetAllocBufferSamrLogonHoursFromNetLogonHours(
    PVOID      *ppCursor,
    PDWORD      pdwSpaceLeft,
    PDWORD      pdwHours,
    PDWORD      pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    WORD wUnitsPerWeek = 0;
    PBYTE pbUnits = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    /*
     * The actual value of pHours is ignored at the moment
     */

    if (pCursor)
    {
        /* units_per_week */
        err = NetAllocBufferWord(&pCursor,
                                 &dwSpaceLeft,
                                 wUnitsPerWeek,
                                 &dwSize);
        BAIL_ON_WINERR_ERROR(err);

        err = NetAllocBufferByteBlob(&pCursor,
                                     &dwSpaceLeft,
                                     pbUnits,
                                     (DWORD)wUnitsPerWeek,
                                     &dwSize);
        BAIL_ON_WINERR_ERROR(err);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }
    else
    {
        /* size of units_per_week */
        dwSize += sizeof(WORD);

        /* size of the blob */
        dwSize += wUnitsPerWeek / 8;

        /* size of the blob pointer */
        dwSize += sizeof(PBYTE);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


DWORD
NetAllocBufferSid(
    PVOID      *ppCursor,
    PDWORD      pdwSpaceLeft,
    PSID        pSourceSid,
    DWORD       dwSourceSidLength,
    PDWORD      pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PVOID pSid = NULL;
    PSID *ppDest = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pSourceSid)
    {
        dwSize = RtlLengthRequiredSid(pSourceSid->SubAuthorityCount);

    }
    else if (dwSourceSidLength)
    {
        dwSize = dwSourceSidLength;
    }
    else
    {
        /* reserve max space if there's no clue about the size */
        dwSize = RtlLengthRequiredSid(SID_MAX_SUB_AUTHORITIES);
    }

    if (pCursor)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WINERR_ERROR(err);
        }

        pSid = (pCursor + dwSpaceLeft) - dwSize;

        /* sanity check - the string and current buffer cursor
           must not overlap */
        if ((pCursor + sizeof(PSID)) > pSid)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WINERR_ERROR(err);
        }

        if (pSourceSid)
        {
            status = RtlCopySid(dwSize,
                                (PSID)pSid,
                                pSourceSid);
            BAIL_ON_NTSTATUS_ERROR(status);
        }

        /* recalculate size and space after copying the SID */
        ppDest        = (PSID*)pCursor;
        *ppDest       = (PSID)pSid;
        dwSpaceLeft  -= dwSize;

        /* recalculate size and space after setting the SID pointer */
        pCursor      += sizeof(PSID);
        dwSpaceLeft  -= sizeof(PSID);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    /* include size of the pointer */
    dwSize += sizeof(PSID);

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(status);
    }

    return err;

error:
    goto cleanup;
}


DWORD
NetAllocBufferByteBlob(
    PVOID      *ppCursor,
    PDWORD      pdwSpaceLeft,
    PBYTE       pbBlob,
    DWORD       dwBlobSize,
    PDWORD      pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PBYTE *ppbDest = NULL;
    PBYTE pbBytes = NULL;

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    /*
     * The actual value of pHours is ignored at the moment
     */

    dwSize += dwBlobSize;

    if (pCursor && pbBlob)
    {
        if (dwSize > dwSpaceLeft)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WINERR_ERROR(err);
        }

        pbBytes = (pCursor + dwSpaceLeft) - dwSize;

        /* sanity check - the string and current buffer cursor
           must not overlap */
        if ((pCursor + sizeof(PWSTR)) > (PVOID)pbBytes)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            BAIL_ON_WINERR_ERROR(err);
        }

        /* copy the blob */
        memcpy(pbBytes, pbBlob, dwSize);

        /* recalculate size and space after copying the blob */
        ppbDest       = (PBYTE*)pCursor;
        *ppbDest      = (PBYTE)pbBytes;
        dwSpaceLeft  -= dwSize;

        /* recalculate cursor and space after setting the blob pointer */
        pCursor      += sizeof(PBYTE);
        dwSpaceLeft  -= sizeof(PBYTE);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }
    else if (pCursor)
    {
        pCursor      += sizeof(PBYTE);
        dwSpaceLeft  -= sizeof(PBYTE);

        *ppCursor     = pCursor;
        *pdwSpaceLeft = dwSpaceLeft;
    }

    /* include size of the pointer */
    dwSize += sizeof(PBYTE);

    if (pdwSize)
    {
        *pdwSize += dwSize;
    }

cleanup:
    return err;

error:
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
