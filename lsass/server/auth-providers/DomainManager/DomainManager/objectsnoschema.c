
DWORD
ObjectsNoSchemaFindUserBySid(
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
ObjectsNoSchemaFindUserByDN(
	HANDLE hDirectory.
	PWSTR pszUserDN,
	PLSA_SECURITY_OBJECT * ppSecurityObject
	)
{


}

DWORD
ObjectsNoSchemaEnumUsers(
	HANDLE hDirectory,
	DWORD dwNumUsers,
	PLSA_SECURITY_OBJECT ** pppSecurityObjects
	)
{
	DWORD dwError = 0;

}


DWORD
ObjectsNoSchemaFindGroupBySid(
	HANDLE hDirectory,
	PWSTR pszGroupSid,
	PLSA_SECURITY_OBJECT *pSecurityObject
	)
{
	DWORD dwError = 0;


	return dwError;
}


DWORD
ObjectsNoSchemaFindGroupBySamAccountName(
	HANDLE hDirectory,
	PWSTR pszSamAccountName,



DWORD
ADFindObjectByNameTypeNoCache(
    IN HANDLE hProvider,
    IN PWSTR pszName,
    IN ADLogInNameType NameType,
    IN ADAccountType AccountType,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsUser = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;

    switch (AccountType)
    {
        case AccountType_User:
            bIsUser = TRUE;
            break;
        case AccountType_Group:
            bIsUser = FALSE;
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    switch (NameType)
    {
        case NameType_NT4:
            dwError = ADFindObjectByNT4NameNoCache(
                            hProvider,
                            pszName,
                            &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case NameType_UPN:
            dwError = ADFindObjectByUpnNoCache(
                            hProvider,
                            pszName,
                            &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case NameType_Alias:
            dwError = ADFindObjectByAliasNoCache(
                            hProvider,
                            pszName,
                            bIsUser,
                            &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    // Check whether the object we find is correct type or not
    if (AccountType != pObject->type)
    {
        dwError = bIsUser ? LW_ERROR_NO_SUCH_USER : LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *ppObject = pObject;

    return dwError;

error:
    if (LW_ERROR_NO_SUCH_OBJECT == dwError)
    {
        dwError = bIsUser ? LW_ERROR_NO_SUCH_USER : LW_ERROR_NO_SUCH_GROUP;
    }
    ADCacheSafeFreeObject(&pObject);
                                                              3516,24       8