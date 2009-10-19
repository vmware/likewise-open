

DWORD
ActiveDirectoryFindGroupObjectByName(
	HANDLE hProvider,
	LPWSTR pszGroupDN
	)
{
	DWORD dwError = 0;

	dwError = DirectoryCanonicalizeName(
					pszGroupDN,
					NAME_TYPE_GROUP_DN,
					&pNameObject
					);
	BAIL_ON_ERROR(dwError);

	dwError = DirectoryOpenHandle(
					pszDirectoryName,
					&hDirectory
					);
	BAIL_ON_ERROR(dwError);

	dwError = FindGroupObjectByName(
					hDirectory,
					pszGroupDN,
					ppLsaSecurityObject
					);
	BAIL_ON_ERROR(dwError);

	DirectoryCloseHandle(
			hDirectory
			);

error:

	return dwError
}


