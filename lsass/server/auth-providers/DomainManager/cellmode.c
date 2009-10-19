
DWORD
CellModeFindUserBySid(
	HANDLE hDirectory,
	PWSTR pszUserSid,
	PLSA_SECURITY_OBJECT * ppSecurityObject
	)
{
	PDIRECTORY_HANDLE pDirectoryHandle = NULL;

	pDirectoryHandle = (PDIRECTORY_HANDLE)pDirectoryHandle;

	pTrustedDomain = pDirectoryHandle->pTrustedDomain;

	if (pTrustedDomain->dwReachableState == DOMAIN_NOT_REACHABLE) {

		dwError = LW_LSA_ERROR_DOMAIN_NOT_REACHABLE;
		BAIL_ON_ERROR(dwError);
	}

	//
	//  Now go find the data
	//  If you can't reach the dc - refresh the data and put on cache
	//

	return dwError;
}


DWORD
CellModeFindUserByDN(
	HANDLE hDirectory.
	PWSTR pszUserDN,
	PLSA_SECURITY_OBJECT * ppSecurityObject
	)
{


}

DWORD
CellModeEnumUsers(
	HANDLE hDirectory,
	DWORD dwNumUsers,
	PLSA_SECURITY_OBJECT ** pppSecurityObjects
	)
{
	DWORD dwError = 0;

}


DWORD
RemoteFindGroupBySid(
	HANDLE hDirectory,
	PWSTR pszGroupSid,
	PLSA_SECURITY_OBJECT *pSecurityObject
	)
{
	DWORD dwError = 0;


	return dwError;
}


DWORD
RemoteFindGroupBySamAccountName(
	HANDLE hDirectory,
	PWSTR pszSamAccountName,
	)

}