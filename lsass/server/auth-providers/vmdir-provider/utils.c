/*
 * Copyright (C) VMware. All rights reserved.
 */

#include "includes.h"

DWORD
VmDirCrackLoginId(
    PCSTR pszLoginId,
    PSTR* ppszDomain,
    PSTR* ppszAccount
    )
{
    DWORD dwError    = 0;
    PCSTR pszCursor  = pszLoginId;
    PSTR  pszDomain  = NULL;
    PSTR  pszAccount = NULL;

    while (*pszCursor && (*pszCursor != '\\') && (*pszCursor != '@'))
    {
        pszCursor++;
    }

    if (*pszCursor == '\\')
    {
        // NT4 Style => Domain\\Account

        size_t len = pszCursor - pszLoginId;

        dwError = LwAllocateMemory(len, (PVOID*)&pszDomain);
        BAIL_ON_VMDIR_ERROR(dwError);

        memcpy(pszDomain, pszLoginId, len);

        if (++pszCursor && *pszCursor)
        {
            dwError = LwAllocateString(pszCursor, &pszAccount);
            BAIL_ON_VMDIR_ERROR(dwError);
        }
    }
    else if (*pszCursor == '@')
    {
        // UPN => Account@Domain

        size_t len = pszCursor - pszLoginId;

        dwError = LwAllocateMemory(len, (PVOID*)&pszAccount);
        BAIL_ON_VMDIR_ERROR(dwError);

        memcpy(pszAccount, pszLoginId, len);

        if (++pszCursor && *pszCursor)
        {
            dwError = LwAllocateString(pszCursor, &pszDomain);
            BAIL_ON_VMDIR_ERROR(dwError);
        }
    }
    else
    {
        // Account

        dwError = LwAllocateString(pszLoginId, &pszAccount);
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    *ppszDomain = pszDomain;
    *ppszAccount = pszAccount;

cleanup:

    return dwError;

error:

    *ppszDomain = NULL;
    *ppszAccount = NULL;

    LW_SAFE_FREE_MEMORY(pszDomain);
    LW_SAFE_FREE_MEMORY(pszAccount);

    goto cleanup;
}

DWORD
VmDirGetBindInfo(
    PVMDIR_BIND_INFO* ppBindInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PVMDIR_BIND_INFO pBindInfo = NULL;

    dwError = VMDIR_ACQUIRE_RWLOCK_SHARED(
                    &gVmDirAuthProviderGlobals.mutex_rw,
                    bInLock);
    BAIL_ON_VMDIR_ERROR(dwError);

    if (gVmDirAuthProviderGlobals.pBindInfo)
    {
        pBindInfo = VmDirAcquireBindInfo(gVmDirAuthProviderGlobals.pBindInfo);
    }
    else
    {
        VMDIR_RELEASE_RWLOCK(&gVmDirAuthProviderGlobals.mutex_rw, bInLock);

        dwError = VmDirCreateBindInfo(&pBindInfo);
        BAIL_ON_VMDIR_ERROR(dwError);

        dwError = VMDIR_ACQUIRE_RWLOCK_EXCLUSIVE(
                        &gVmDirAuthProviderGlobals.mutex_rw,
                        bInLock);
        BAIL_ON_VMDIR_ERROR(dwError);

        if (gVmDirAuthProviderGlobals.pBindInfo)
        {
            VmDirReleaseBindInfo(gVmDirAuthProviderGlobals.pBindInfo);
        }

        gVmDirAuthProviderGlobals.pBindInfo = VmDirAcquireBindInfo(pBindInfo);
    }

    *ppBindInfo = pBindInfo;

cleanup:

    VMDIR_RELEASE_RWLOCK(&gVmDirAuthProviderGlobals.mutex_rw, bInLock);

    return dwError;

error:

    *ppBindInfo = NULL;

    if (pBindInfo)
    {
        VmDirReleaseBindInfo(pBindInfo);
    }

    goto cleanup;
}

DWORD
VmDirGetDomainFromDN(
    PCSTR pszDN,
    PSTR* ppszDomain
    )
{
    DWORD  dwError = 0;
    PSTR   pszDN_local = NULL;
    PSTR   pszReadCursor = NULL;
    PSTR   pszDC = NULL;
    PCSTR  pszDCPrefix = "DC=";
    size_t sLenDCPrefix = sizeof("DC=") - 1;
    size_t sLenDomain = 0;
    PSTR   pszDomain = NULL;
    PSTR   pszWriteCursor = NULL;

    dwError = LwAllocateString(pszDN, &pszDN_local);
    BAIL_ON_VMDIR_ERROR(dwError);

    LwStrToUpper(pszDN_local);

    pszDC = strstr(pszDN_local, pszDCPrefix);
    if (!pszDC)
    {
        dwError = ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    pszReadCursor = pszDC;

    // Find the length of the domain
    while (!IsNullOrEmptyString(pszReadCursor))
    {
        size_t sLenPart = 0;

        if (0 != strncmp(pszReadCursor, pszDCPrefix, sLenDCPrefix))
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_VMDIR_ERROR(dwError);
        }

        pszReadCursor  += sLenDCPrefix;

        sLenPart = strcspn(pszReadCursor, ",");

        if (sLenPart == 0)
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_VMDIR_ERROR(dwError);
        }

        if (sLenDomain > 0)
        {
            sLenDomain++;
        }

        sLenDomain += sLenPart;

        pszReadCursor  += sLenPart;

        sLenPart = strspn(pszReadCursor, ",");

        pszReadCursor  += sLenPart;
    }

    sLenDomain++;

    dwError = LwAllocateMemory(sLenDomain, (PVOID*)&pszDomain);
    BAIL_ON_VMDIR_ERROR(dwError);

    pszReadCursor = pszDC;
    pszWriteCursor = pszDomain;

    while (!IsNullOrEmptyString(pszReadCursor))
    {
        size_t sLenPart = 0;

        pszReadCursor += sLenDCPrefix;

        sLenPart = strcspn(pszReadCursor, ",");

        if (pszWriteCursor != pszDomain)
        {
            *pszWriteCursor++ = '.';
        }

        memcpy(pszWriteCursor, pszReadCursor, sLenPart);

        pszWriteCursor += sLenPart;
        pszReadCursor += sLenPart;

        sLenPart = strspn(pszReadCursor, ",");

        pszReadCursor += sLenPart;
    }

    *ppszDomain = pszDomain;

cleanup:

    LW_SAFE_FREE_STRING(pszDN_local);

    return dwError;

error:

    *ppszDomain = NULL;

    LW_SAFE_FREE_STRING(pszDomain);

    goto cleanup;
}

DWORD
VmDirGetDefaultSearchBase(
    PCSTR pszBindDN,
    PSTR* ppszSearchBase
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszBindDN_local = NULL;
    PCSTR pszDCPrefix   = "DC=";
    PCSTR pszDC = NULL;
    PSTR  pszSearchBase = NULL;

    dwError = LwAllocateString(pszBindDN, &pszBindDN_local);
    BAIL_ON_VMDIR_ERROR(dwError);

    LwStrToUpper(pszBindDN_local);

    pszDC = strstr(pszBindDN_local, pszDCPrefix);
    if (!pszDC)
    {
        dwError = ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    if (IsNullOrEmptyString(pszDC))
    {
        dwError = ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = LwAllocateString(pszDC, &pszSearchBase);
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppszSearchBase = pszSearchBase;

cleanup:

    LW_SAFE_FREE_MEMORY(pszBindDN_local);

    return dwError;

error:

    *ppszSearchBase = NULL;

    LW_SAFE_FREE_MEMORY(pszSearchBase);

    goto cleanup;
}

DWORD
VmDirGetRID(
    PCSTR  pszObjectSid,
    PDWORD pdwRID
    )
{
    DWORD dwError = 0;
    PCSTR pszDash = NULL;
    PCSTR  p = pszObjectSid;
    PSTR pszEnd = NULL;
    DWORD dwDashes = 0;
    DWORD dwLastDash = 0;
    DWORD dwMachId = 0;
    DWORD i = 0;
    DWORD dwRID = 0;
    DWORD dwMachID = 0;
    DWORD dwRetUID = 0;

    /*
     * Old Format, 8 dashes:
     *   S-1-7-21-1604805504-317578674-977968053-259541063-16778241
     *   16778241 is the "RID" which is |-8bits-|-24-bits-|
     *                                   MachId   RID
     * New Format, 9 dashes:
     *   S-1-7-21-3080227618-571495328-3360055706-2260875924-1-1025
     *      .... -1-1025
     *      1:    machine ID
     *      1025: RID
     *
     * Active Directory Format, 7 dashes:
     *   S-1-5-21-3560509133-2113184660-2722472689-545
     *
     * Problem: Old code assumed the 8/24 "RID" format was the UNIX uid.
     * This implementation takes the ...-MachID-RID format, and converts this
     * to the old RID format.
     */

    for (i=0; pszObjectSid[i]; i++)
    {
        /* Scan SID for dashes, to determine if new or old format */
        if (pszObjectSid[i] == '-')
        {
            dwDashes++;
            dwLastDash = i + 1;
            if (dwDashes == 8)
            {
                dwMachId = i + 1;
            }
        }
    }

    pszDash = &pszObjectSid[dwMachId]; 
    if (dwDashes == 8)
    {
        /* Old format */
        dwRetUID = strtoul(pszDash, &pszEnd, 0);
        if (!pszEnd || pszEnd == pszDash || *pszEnd)
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_VMDIR_ERROR(dwError);
        }
    }
    else if (dwDashes == 9)
    {
        /* pszDash should be pointing at "M-RID" string */
        dwMachID = strtoul(pszDash, &pszEnd, 0);
        if (!pszEnd || pszEnd == pszDash || *pszEnd != '-')
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_VMDIR_ERROR(dwError);
        }

        pszDash = pszEnd+1;
        dwRID = strtoul(pszDash, &pszEnd, 0);
        if (!pszEnd || pszEnd == pszDash || *pszEnd)
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_VMDIR_ERROR(dwError);
        }

        /* Test for overflow of these two values */
        if (dwRID > 0x00FFFFFF || dwMachID > 0xFF)
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_VMDIR_ERROR(dwError);
        }

        dwRetUID = dwMachID << 24 | (dwRID & 0x00FFFFFF);
    }
    else if (p[0] == 'S' && p[1] == '-' && p[2] == '1' &&
             p[3] == '-' && p[4] == '5' && p[5] == '-' && 
             p[6] == '2' && p[7] == '1' && p[8] == '-' &&
             dwDashes == 7)
    {
        /* S-1-5-21 SID format. Field after last '-' is the RID */
        pszDash = &pszObjectSid[dwLastDash]; 
        dwRetUID = strtoul(pszDash, &pszEnd, 0);
        if (!pszEnd || pszEnd == pszDash || *pszEnd)
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_VMDIR_ERROR(dwError);
        }
    }
    *pdwRID = dwRetUID;

error:
    return dwError;
}

DWORD
VmDirGetRIDFromUID(
    DWORD uid,
    PSTR *pszRid
    )
{
    DWORD dwError = 0;
    PSTR pszRetRid = NULL;

    /*
     * This is the inverse operation of VmDirGetRID()
     * Scheme: |-8bits-|-24bits-|
     * Convert UID with upper 8 bits set to M-RID format, otherwise return
     * just the UID as the RID.
     */
    if (uid & 0xFF000000)
    {
        /* Construct M-RID formatted string */
        dwError = LwAllocateStringPrintf(
                      &pszRetRid,
                      "%u-%u",
                      (uid & 0xFF000000) >> 24,
                      uid & 0x00FFFFFF);
        BAIL_ON_VMDIR_ERROR(dwError);
    }
    else
    {
        /* Construct RID formatted string */
        dwError = LwAllocateStringPrintf(
                      &pszRetRid,
                      "%u",
                      uid);
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    *pszRid = pszRetRid;
    pszRetRid = NULL;

error:
    if (dwError)
    {
        LW_SAFE_FREE_MEMORY(pszRetRid);
    }
    return dwError;
}

DWORD
VmDirInitializeUserLoginCredentials(
    IN PCSTR pszUPN,
    IN PCSTR pszPassword,
    IN uid_t uid,
    IN gid_t gid,
    OUT PDWORD pdwGoodUntilTime)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwGoodUntilTime = 0;
    PSTR pszCachePath = NULL;
    PCSTR pszTempCachePath = NULL;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;

    if (!pszUPN || !pszPassword)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_new_unique(
            ctx,
            "FILE",
            "hint",
            &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    pszTempCachePath = krb5_cc_get_name(ctx, cc);

    dwError = LwKrb5GetTgt(
                    pszUPN,
                    pszPassword,
                    pszTempCachePath,
                    &dwGoodUntilTime);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwChangeOwner(
                    pszTempCachePath,
                    uid,
                    gid);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwKrb5GetUserCachePath(
                    uid,
                    KRB5_File_Cache,
                    &pszCachePath);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwMoveFile(
                    pszTempCachePath,
                    pszCachePath + sizeof("FILE:") - 1);
    BAIL_ON_VMDIR_ERROR(dwError);

    pszTempCachePath = NULL;

    if (pdwGoodUntilTime)
    {
        *pdwGoodUntilTime = dwGoodUntilTime;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszCachePath);

    if (ctx)
    {
        if (cc)
        {
            krb5_cc_destroy(ctx, cc);
        }
        krb5_free_context(ctx);
    }

    return dwError;

error:

    if (pszTempCachePath)
    {
        LwRemoveFile(pszTempCachePath);
    }

    goto cleanup;
}
