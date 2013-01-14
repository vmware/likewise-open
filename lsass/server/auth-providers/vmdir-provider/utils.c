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
	DWORD dwRID = 0;
	PLSA_SECURITY_IDENTIFIER pSID = NULL;

	dwError = LsaAllocSecurityIdentifierFromString(pszObjectSid, &pSID);
	BAIL_ON_VMDIR_ERROR(dwError);

	dwError = LsaGetSecurityIdentifierRid(pSID, &dwRID);
	BAIL_ON_VMDIR_ERROR(dwError);

	*pdwRID = dwRID;

cleanup:

	if (pSID)
	{
		LsaFreeSecurityIdentifier(pSID);
	}

	return dwError;

error:

	*pdwRID = 0;

	goto cleanup;
}
