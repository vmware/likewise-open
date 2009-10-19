

DWORD
FindGroupObjectByDN(
	HANDLE hDirectory,
	LPWSTR pszGroupDN,
	PLSA_GROUP_OBJECT *ppGroupObject
	)
{
	dwError = CacheFindGroupObjectByDN(
					hDirectory,
					pszGroupDN,
					ppGroupObject
					);
	if (dwError == ERROR_NOT_FOUND) {

		dwError = RemoteFindGroupObjectByName(
						hDirectory,
						pszGroupDN,
						ppGroupObject
						);
		BAIL_ON_ERROR(dwError);

		dwError = CacheWriteObject(
						hDirectory,
						pszGroupDN,
						pGroupObject
						);
		BAIL_ON_ERROR(dwError);
	}

	return(dwError);

}

DWORD
FindGroupObjectBySid(
	HANDLE hDirectory,
	LPWSTR pszGroupSid,
	PLSA_GROUP_OBJECT *ppGroupObject
	)
{
	DWORD dwError = 0;

	dwError = CacheFindGroupObjectBySid(
					hDirectory,
					pszGroupSid,
					ppGroupObject
					);
	if (dwError == ERROR_NOT_FOUND) {

		dwError = RemoteFindGroupObjectBySid(
						hDirectory,
						pszGroupSid,
						ppGroupObject
						);
		if (dwError == 0) {
			dwError = CacheAddGroupObject,
							hDirectory,
							pGroupObject
							);
			BAIL_ON_ERROR(dwError);
		}
	}

	return (dwError);

}


DWORD
FindGroupBySamName(
	HANDLE hDirectory,
