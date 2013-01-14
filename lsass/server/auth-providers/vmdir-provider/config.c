/*
 * Copyright (C) VMware. All rights reserved.
 */

#include "includes.h"

static
DWORD
VmDirRegReadString(
	HANDLE hConnection,
	HKEY   hKey,
	PCSTR  pszKey,
	PSTR*  ppszValue
	);

static
VOID
VmDirFreeBindInfo(
	PVMDIR_BIND_INFO pBindInfo
	);

DWORD
VmDirCreateBindInfo(
	PVMDIR_BIND_INFO* ppBindInfo
	)
{
	DWORD dwError = 0;
	PVMDIR_BIND_INFO pBindInfo = NULL;
	HANDLE hConnection = NULL;
	HKEY   hKeyRoot = NULL;
	HKEY   hKeyVmDir = NULL;
	HKEY   hKeyCreds = NULL;
	PCSTR  pszKey_uri      = VMDIR_REG_KEY_BIND_INFO_URI;
	PCSTR  pszKey_binddn   = VMDIR_REG_KEY_BIND_INFO_BIND_DN;
	PCSTR  pszKey_password = VMDIR_REG_KEY_BIND_INFO_PASSWORD;

	dwError = RegOpenServer(&hConnection);
	BAIL_ON_VMDIR_ERROR(dwError);

	dwError = RegOpenKeyExA(
					hConnection,
					NULL,
					HKEY_THIS_MACHINE,
					0,
					KEY_READ,
					&hKeyRoot);
	BAIL_ON_VMDIR_ERROR(dwError);

	dwError = RegOpenKeyExA(
					hConnection,
					hKeyRoot,
					VMDIR_REG_KEY,
					0,
					KEY_READ,
					&hKeyVmDir);
	BAIL_ON_VMDIR_ERROR(dwError);

	dwError = RegOpenKeyExA(
					hConnection,
					hKeyRoot,
					VMDIR_REG_KEY_CREDS,
					0,
					KEY_READ,
					&hKeyCreds);
	BAIL_ON_VMDIR_ERROR(dwError);

	dwError = LwAllocateMemory(sizeof(VMDIR_BIND_INFO), (PVOID*)&pBindInfo);
	BAIL_ON_VMDIR_ERROR(dwError);

	pBindInfo->refCount = 1;

	dwError = VmDirRegReadString(
					hConnection,
					hKeyCreds,
					pszKey_uri,
					&pBindInfo->pszURI);
	BAIL_ON_VMDIR_ERROR(dwError);

	dwError = VmDirRegReadString(
					hConnection,
					hKeyCreds,
					pszKey_binddn,
					&pBindInfo->pszBindDN);
	BAIL_ON_VMDIR_ERROR(dwError);

	dwError = VmDirRegReadString(
					hConnection,
					hKeyCreds,
					pszKey_password,
					&pBindInfo->pszPassword);
	BAIL_ON_VMDIR_ERROR(dwError);

	dwError = VmDirGetDomainFromDN(
					pBindInfo->pszBindDN,
					&pBindInfo->pszDomainFqdn);
	BAIL_ON_VMDIR_ERROR(dwError);

	dwError = LwAllocateString(
					pBindInfo->pszDomainFqdn,
					&pBindInfo->pszDomainShort);
	BAIL_ON_VMDIR_ERROR(dwError);

	dwError = VmDirGetDefaultSearchBase(
					pBindInfo->pszBindDN,
					&pBindInfo->pszSearchBase);
	BAIL_ON_VMDIR_ERROR(dwError);

	*ppBindInfo = pBindInfo;

cleanup:

	if (hKeyCreds)
	{
		RegCloseKey(hConnection, hKeyCreds);
	}

	if (hKeyVmDir)
	{
		RegCloseKey(hConnection, hKeyVmDir);
	}

	if (hKeyRoot)
	{
		RegCloseKey(hConnection, hKeyRoot);
	}

	if (hConnection)
	{
		RegCloseServer(hConnection);
	}

	return dwError;

error:

	*ppBindInfo = NULL;

	if (pBindInfo)
	{
		VmDirFreeBindInfo(pBindInfo);
	}

	goto cleanup;
}

PVMDIR_BIND_INFO
VmDirAcquireBindInfo(
	PVMDIR_BIND_INFO pBindInfo
	)
{
	if (pBindInfo)
	{
		LwInterlockedIncrement(&pBindInfo->refCount);
	}

	return pBindInfo;
}

VOID
VmDirReleaseBindInfo(
	PVMDIR_BIND_INFO pBindInfo
	)
{
	if (pBindInfo && (0 == LwInterlockedDecrement(&pBindInfo->refCount)))
	{
		VmDirFreeBindInfo(pBindInfo);
	}
}

static
DWORD
VmDirRegReadString(
	HANDLE hConnection,
	HKEY   hKey,
	PCSTR  pszName,
	PSTR*  ppszValue
	)
{
	DWORD dwError = LW_ERROR_SUCCESS;
	DWORD dwType = 0;
	CHAR  szValue[MAX_VALUE_LENGTH];
	DWORD dwSize = sizeof(szValue);
	PSTR  pszValue = NULL;

	memset(&szValue[0], 0, dwSize);

	dwError = RegGetValueA(
				hConnection,
				hKey,
				NULL,
				pszName,
				RRF_RT_REG_SZ,
				&dwType,
				szValue,
				&dwSize);
	BAIL_ON_VMDIR_ERROR(dwError);

	dwError = LwAllocateString((dwSize > 0 ? szValue : ""), &pszValue);
	BAIL_ON_VMDIR_ERROR(dwError);

	*ppszValue = pszValue;

cleanup:

	return dwError;

error:

	if (ppszValue)
	{
		*ppszValue = NULL;
	}

	LW_SAFE_FREE_STRING(pszValue);

	goto cleanup;
}

static
VOID
VmDirFreeBindInfo(
	PVMDIR_BIND_INFO pBindInfo
	)
{
	if (pBindInfo)
	{
		LW_SECURE_FREE_STRING(pBindInfo->pszURI);
		LW_SECURE_FREE_STRING(pBindInfo->pszBindDN);
		LW_SECURE_FREE_STRING(pBindInfo->pszPassword);
		LW_SAFE_FREE_STRING(pBindInfo->pszDomainFqdn);
		LW_SAFE_FREE_STRING(pBindInfo->pszDomainShort);
		LW_SAFE_FREE_STRING(pBindInfo->pszSearchBase);

		LwFreeMemory(pBindInfo);
	}
}
