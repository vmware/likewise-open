/*
 * Copyright (C) VMware. All rights reserved.
 */

#include "includes.h"

#if 0
#define wszVMDIR_DB_DIR_ATTR_EOL NULL
#endif
static
DWORD
VmDirLdapInitializeWithKerberos(
    PCSTR            pszURI,
    PCSTR            pszUPN,
    PCSTR            pszPassword,
    PCSTR            pszCachePath,
    LDAP**           ppLd
    );

static
DWORD
VmDirLdapInitializeWithSRP(
    PCSTR            pszURI,
    PCSTR            pszUPN,
    PCSTR            pszPassword,
    PCSTR            pszCachePath,
    LDAP**           ppLd
    );

static
int
VmDirSASLInteractionKerberos(
    LDAP*    pLd,
    unsigned flags,
    PVOID    pDefaults,
    PVOID    pIn
    );

static
int
VmDirSASLInteractionSRP(
     LDAP*    pLd,
     unsigned flags,
     PVOID    pDefaults,
     PVOID    pIn
     );

static
DWORD
VmDirLdapGetDN(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	PSTR*        ppszDN,
	BOOLEAN      bOptional
	);

static
DWORD
VmDirLdapGetInt32Value(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	PINT32       pValue,
	BOOLEAN      bOptional
	);

static
DWORD
VmDirLdapGetUint32Value(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	PUINT32      pValue,
	BOOLEAN      bOptional
	);

static
DWORD
VmDirLdapGetInt64Value(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	PINT64       pValue,
	BOOLEAN      bOptional
	);

static
DWORD
VmDirLdapGetUint64Value(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	PUINT64      pValue,
	BOOLEAN      bOptional
	);

static
DWORD
VmDirLdapGetOptionalStringValue(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	PSTR*        ppszValue
	);

static
DWORD
VmDirLdapGetStringValue(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	PSTR*        ppszValue
	);

static
DWORD
VmDirLdapGetStringArray(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	BOOLEAN      bOptional,
	PSTR**       pppszStrArray,
	PDWORD       pdwCount
	);

DWORD
VmDirLdapInitialize(
	PCSTR            pszURI,
	PCSTR            pszUPN,
	PCSTR            pszPassword,
    PCSTR            pszCachePath,
	LDAP**           ppLd
	)
{
    DWORD dwError = 0;
    
    switch (gVmdirGlobals.bindProtocol)
    {
        case VMDIR_BIND_PROTOCOL_KERBEROS:
            
            dwError = VmDirLdapInitializeWithKerberos(
                            pszURI,
                            pszUPN,
                            pszPassword,
                            pszCachePath,
                            ppLd);
            
            break;
            
        case VMDIR_BIND_PROTOCOL_SRP:
            
            dwError = VmDirLdapInitializeWithSRP(
                            pszURI,
                            pszUPN,
                            pszPassword,
                            pszCachePath,
                            ppLd);
            
            break;
            
        default:
            
            dwError = ERROR_INVALID_STATE;
            
            break;
    }
    
    return dwError;
}

DWORD
VmDirLdapQuerySingleObject(
	LDAP*         pLd,
	PCSTR         pszBaseDN,
	int           scope,
	PCSTR         pszFilter,
	char**        attrs,
	LDAPMessage** ppMessage
	)
{
	DWORD dwError = 0;
	DWORD dwNumObjects = 0;
	LDAPMessage* pMessage = NULL;

	dwError = VmDirLdapQueryObjects(
					pLd,
					pszBaseDN,
					scope,
					pszFilter,
					attrs,
					-1,
					&pMessage);
	BAIL_ON_VMDIRDB_ERROR(dwError);

	dwNumObjects = ldap_count_entries(pLd, pMessage);

	if (dwNumObjects == 0)
	{
		dwError = LW_ERROR_NO_SUCH_OBJECT;
	}
	else if (dwNumObjects != 1)
	{
		dwError = ERROR_INVALID_DATA;
	}
	BAIL_ON_VMDIRDB_ERROR(dwError);

	*ppMessage = pMessage;

cleanup:

	return dwError;

error:

	*ppMessage = NULL;

	if (pMessage)
	{
		VmDirLdapFreeMessage(pMessage);
	}

	goto cleanup;
}

DWORD
VmDirLdapQueryObjects(
	LDAP*         pLd,
	PCSTR         pszBaseDN,
	int           scope,
	PCSTR         pszFilter,
	char**        attrs,
	int           sizeLimit,
	LDAPMessage** ppMessage
	)
{
	DWORD dwError = 0;

	struct timeval waitTime = {0};

	waitTime.tv_sec  = DEFAULT_LDAP_QUERY_TIMEOUT_SECS;
	waitTime.tv_usec = 0;

	dwError = LwMapLdapErrorToLwError(
				ldap_search_ext_s(
					pLd,
					pszBaseDN,
					scope,
					pszFilter,
					attrs,
					FALSE,     /* Attrs only      */
					NULL,      /* Server controls */
					NULL,      /* Client controls */
					&waitTime,
					sizeLimit, /* size limit      */
					ppMessage));

	return dwError;
}

DWORD
VmDirLdapGetValues(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PVMDIR_ATTR  pValueArray,
	DWORD        dwNumValues
	)
{
	DWORD dwError = 0;
	DWORD iValue = 0;

	for (; iValue < dwNumValues; iValue++)
	{
		PVMDIR_ATTR pAttr = &pValueArray[iValue];

		switch (pAttr->type)
		{
			case VMDIR_ATTR_TYPE_DN:

				dwError = VmDirLdapGetDN(
								pLd,
								pMessage,
								pAttr->pszName,
								pAttr->dataRef.ppszData,
								pAttr->bOptional);

				break;

			case VMDIR_ATTR_TYPE_STRING:

				if (pAttr->bOptional)
				{
					dwError = VmDirLdapGetOptionalStringValue(
									pLd,
									pMessage,
									pAttr->pszName,
									pAttr->dataRef.ppszData);
				}
				else
				{
					dwError = VmDirLdapGetStringValue(
								pLd,
								pMessage,
								pAttr->pszName,
								pAttr->dataRef.ppszData);
				}

				break;

			case VMDIR_ATTR_TYPE_INT32:

				if (pAttr->size < sizeof(INT32))
				{
					dwError = ERROR_INVALID_PARAMETER;
					BAIL_ON_VMDIRDB_ERROR(dwError);
				}

				dwError = VmDirLdapGetInt32Value(
								pLd,
								pMessage,
								pAttr->pszName,
								pAttr->dataRef.pData_int32,
								pAttr->bOptional);

				break;

			case VMDIR_ATTR_TYPE_UINT32:

				if (pAttr->size < sizeof(UINT32))
				{
					dwError = ERROR_INVALID_PARAMETER;
					BAIL_ON_VMDIRDB_ERROR(dwError);
				}

				dwError = VmDirLdapGetUint32Value(
								pLd,
								pMessage,
								pAttr->pszName,
								pAttr->dataRef.pData_uint32,
								pAttr->bOptional);

				break;

			case VMDIR_ATTR_TYPE_INT64:

				if (pAttr->size < sizeof(INT64))
				{
					dwError = ERROR_INVALID_PARAMETER;
					BAIL_ON_VMDIRDB_ERROR(dwError);
				}

				dwError = VmDirLdapGetInt64Value(
								pLd,
								pMessage,
								pAttr->pszName,
								pAttr->dataRef.pData_int64,
								pAttr->bOptional);

				break;

			case VMDIR_ATTR_TYPE_UINT64:

				if (pAttr->size < sizeof(UINT64))
				{
					dwError = ERROR_INVALID_PARAMETER;
					BAIL_ON_VMDIRDB_ERROR(dwError);
				}

				dwError = VmDirLdapGetUint64Value(
								pLd,
								pMessage,
								pAttr->pszName,
								pAttr->dataRef.pData_uint64,
								pAttr->bOptional);

				break;

			case VMDIR_ATTR_TYPE_MULTI_STRING:

				dwError = VmDirLdapGetStringArray(
								pLd,
								pMessage,
								pAttr->pszName,
								pAttr->bOptional,
								pAttr->dataRef.pppszStrArray,
								pAttr->pdwCount);
				BAIL_ON_VMDIRDB_ERROR(dwError);

				break;

			default:

				dwError = ERROR_INVALID_PARAMETER;
				BAIL_ON_VMDIRDB_ERROR(dwError);

				break;
		}
	}

error:

	return dwError;
}

VOID
VmDirLdapFreeMessage(
	LDAPMessage* pMessage
	)
{
	ldap_msgfree(pMessage);
}

VOID
VmDirLdapClose(
	LDAP* pLd
	)
{
	ldap_unbind_ext(pLd, NULL, NULL);
}

static
DWORD
VmDirLdapInitializeWithKerberos(
    PCSTR            pszURI,
    PCSTR            pszUPN,
    PCSTR            pszPassword,
    PCSTR            pszCachePath,
    LDAP**           ppLd
    )
{
    DWORD dwError = 0;
    const int ldapVer = LDAP_VERSION3;
    PSTR  pszUPN_local = NULL;
    LDAP* pLd = NULL;
    PSTR pszOldCachePath = NULL;
    
    dwError = LwMapLdapErrorToLwError(
                    ldap_initialize(&pLd, pszURI));
    BAIL_ON_VMDIRDB_ERROR(dwError);
    
    dwError = LwMapLdapErrorToLwError(
                    ldap_set_option(pLd, LDAP_OPT_PROTOCOL_VERSION, &ldapVer));
    BAIL_ON_VMDIRDB_ERROR(dwError);
    
    dwError = LwMapLdapErrorToLwError(
                    ldap_set_option(
                                  pLd,
                                  LDAP_OPT_X_SASL_NOCANON,
                                  LDAP_OPT_ON));
    BAIL_ON_VMDIRDB_ERROR(dwError);
    
    dwError = LwKrb5SetThreadDefaultCachePath(
                    pszCachePath,
                    &pszOldCachePath);
    BAIL_ON_VMDIRDB_ERROR(dwError);
    
    dwError = LwMapLdapErrorToLwError(
                    ldap_sasl_interactive_bind_s(
                                               pLd,
                                               NULL,
                                               "GSSAPI",
                                               NULL,
                                               NULL,
                                               LDAP_SASL_QUIET,
                                               &VmDirSASLInteractionKerberos,
                                               NULL));
    BAIL_ON_VMDIRDB_ERROR(dwError);
    
    *ppLd = pLd;
    
cleanup:
    
    if (pszOldCachePath)
    {
        LwKrb5SetThreadDefaultCachePath(
                pszOldCachePath,
                NULL);
        LwFreeString(pszOldCachePath);
    }
    
    LW_SAFE_FREE_STRING(pszUPN_local);
    
    return dwError;
    
error:
    
    *ppLd = NULL;
    
    if (pLd)
    {
        VmDirLdapClose(pLd);
    }
    
    goto cleanup;
}

static
DWORD
VmDirLdapInitializeWithSRP(
   PCSTR            pszURI,
   PCSTR            pszUPN,
   PCSTR            pszPassword,
   PCSTR            pszCachePath,
   LDAP**           ppLd
   )
{
    DWORD dwError = 0;
    const int ldapVer = LDAP_VERSION3;
    VMDIR_SASL_INFO srpDefault = {0};
    PSTR  pszUPN_local = NULL;
    LDAP* pLd = NULL;
    
    dwError = LwMapLdapErrorToLwError(
                  ldap_initialize(&pLd, pszURI));
    BAIL_ON_VMDIRDB_ERROR(dwError);
    
    dwError = LwMapLdapErrorToLwError(
                  ldap_set_option(pLd, LDAP_OPT_PROTOCOL_VERSION, &ldapVer));
    BAIL_ON_VMDIRDB_ERROR(dwError);
    
    dwError = LwMapLdapErrorToLwError(
                  ldap_set_option(
                                  pLd,
                                  LDAP_OPT_X_SASL_NOCANON,
                                  LDAP_OPT_ON));
    BAIL_ON_VMDIRDB_ERROR(dwError);
    
    dwError = LwAllocateString(pszUPN, &pszUPN_local);
    BAIL_ON_VMDIRDB_ERROR(dwError);
    
    LwStrToLower(pszUPN_local);
    
    srpDefault.pszAuthName = pszUPN_local;
    
    srpDefault.pszPassword = pszPassword;
    
    dwError = LwMapLdapErrorToLwError(
                  ldap_sasl_interactive_bind_s(
                                               pLd,
                                               NULL,
                                               "SRP",
                                               NULL,
                                               NULL,
                                               LDAP_SASL_QUIET,
                                               &VmDirSASLInteractionSRP,
                                               &srpDefault));
    BAIL_ON_VMDIRDB_ERROR(dwError);
    
    *ppLd = pLd;
    
cleanup:
    
    LW_SAFE_FREE_STRING(pszUPN_local);
    
    return dwError;
    
error:
    
    *ppLd = NULL;
    
    if (pLd)
    {
        VmDirLdapClose(pLd);
    }
    
    goto cleanup;
}

static
int
VmDirSASLInteractionKerberos(
    LDAP *      pLd,
    unsigned    flags,
    void *      pDefaults,
    void *      pIn
    )
{
    // dummy function to satisfy ldap_sasl_interactive_bind call
    return LDAP_SUCCESS;
}

static
int
VmDirSASLInteractionSRP(
    LDAP *      pLd,
    unsigned    flags,
    void *      pDefaults,
    void *      pIn
    )
{
    sasl_interact_t* pInteract = pIn;
    PVMDIR_SASL_INFO pDef = pDefaults;
    
    while( (pDef != NULL) && (pInteract->id != SASL_CB_LIST_END) )
    {
        switch( pInteract->id )
        {
            case SASL_CB_GETREALM:
                pInteract->defresult = pDef->pszRealm;
                break;
            case SASL_CB_AUTHNAME:
                pInteract->defresult = pDef->pszAuthName;
                break;
            case SASL_CB_PASS:
                pInteract->defresult = pDef->pszPassword;
                break;
            case SASL_CB_USER:
                pInteract->defresult = pDef->pszUser;
                break;
            default:
                break;
        }
        
        pInteract->result = (pInteract->defresult) ? pInteract->defresult : "";
        pInteract->len    = strlen( pInteract->result );
        
        pInteract++;
    }
    
    return LDAP_SUCCESS;
}

static
DWORD
VmDirLdapGetDN(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	PSTR*        ppszDN,
	BOOLEAN      bOptional
	)
{
	DWORD dwError = 0;
	PSTR  pszDN_ldap = NULL;
	PSTR  pszDN = NULL;
	PSTR  pszDNRef = NULL;

	pszDN_ldap = ldap_get_dn(pLd, pMessage);
	if (IsNullOrEmptyString(pszDN_ldap))
	{
		if (!bOptional)
		{
			dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
			BAIL_ON_VMDIRDB_ERROR(dwError);
		}
		else
		{
			pszDNRef = "";
		}
	}
	else
	{
		pszDNRef = pszDN_ldap;
	}

	dwError = LwAllocateString(pszDNRef, &pszDN);
	BAIL_ON_VMDIRDB_ERROR(dwError);

	*ppszDN = pszDN;

cleanup:

	if (pszDN_ldap)
	{
		ldap_memfree(pszDN_ldap);
	}

	return dwError;

error:

	*ppszDN = NULL;

	goto cleanup;
}

static
DWORD
VmDirLdapGetInt32Value(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	PINT32       pValue,
	BOOLEAN      bOptional
	)
{
	DWORD dwError = 0;
	PSTR  pszValue = NULL;
	PSTR  pszValueRef = NULL;

	if (bOptional)
	{
		dwError = VmDirLdapGetOptionalStringValue(
						pLd,
						pMessage,
						pszAttrName,
						&pszValue);
		BAIL_ON_VMDIRDB_ERROR(dwError);

		pszValueRef = !pszValue ? "0" : pszValue;
	}
	else
	{
		dwError = VmDirLdapGetStringValue(
						pLd,
						pMessage,
						pszAttrName,
						&pszValue);
		BAIL_ON_VMDIRDB_ERROR(dwError);

		pszValueRef = pszValue;
	}

	*pValue = atoi(pszValueRef);

cleanup:

	LW_SAFE_FREE_MEMORY(pszValue);

	return dwError;

error:

	*pValue = 0;

	goto cleanup;
}

static
DWORD
VmDirLdapGetUint32Value(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	PUINT32      pValue,
	BOOLEAN      bOptional
	)
{
	DWORD dwError = 0;
	PSTR  pszValue = NULL;
	PSTR  pszValueRef = NULL;

	if (bOptional)
	{
		dwError = VmDirLdapGetOptionalStringValue(
							pLd,
							pMessage,
							pszAttrName,
							&pszValue);
		BAIL_ON_VMDIRDB_ERROR(dwError);

		pszValueRef = !pszValue ? "0" : pszValue;
	}
	else
	{
		dwError = VmDirLdapGetStringValue(
							pLd,
							pMessage,
							pszAttrName,
							&pszValue);
		BAIL_ON_VMDIRDB_ERROR(dwError);

		pszValueRef = pszValue;
	}

	*pValue = atoi(pszValueRef);

cleanup:

	LW_SAFE_FREE_MEMORY(pszValue);

	return dwError;

error:

	*pValue = 0;

	goto cleanup;
}

static
DWORD
VmDirLdapGetInt64Value(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	PINT64       pValue,
	BOOLEAN      bOptional
	)
{
	DWORD dwError = 0;
	PSTR  pszValue = NULL;
	PSTR  pszValueRef = NULL;
	PSTR  pszEnd = NULL;
	INT64 val = 0;

	if (bOptional)
	{
		dwError = VmDirLdapGetOptionalStringValue(
						pLd,
						pMessage,
						pszAttrName,
						&pszValue);
		BAIL_ON_VMDIRDB_ERROR(dwError);

		pszValueRef = !pszValue ? "0" : pszValue;
	}
	else
	{
		dwError = VmDirLdapGetStringValue(
						pLd,
						pMessage,
						pszAttrName,
						&pszValue);
		BAIL_ON_VMDIRDB_ERROR(dwError);

		pszValueRef = pszValue;
	}

	val = strtoll(pszValueRef, &pszEnd, 10);

	if (!pszEnd || (pszEnd == pszValueRef) || (*pszEnd != '\0'))
	{
		dwError = ERROR_INVALID_DATA;
		BAIL_ON_VMDIRDB_ERROR(dwError);
	}

	*pValue = val;

cleanup:

	LW_SAFE_FREE_MEMORY(pszValue);

	return dwError;

error:

	*pValue = 0;

	goto cleanup;
}

static
DWORD
VmDirLdapGetUint64Value(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	PUINT64      pValue,
	BOOLEAN      bOptional
	)
{
	DWORD  dwError = 0;
	PSTR   pszValue = NULL;
	PSTR   pszValueRef = NULL;
	PSTR   pszEnd = NULL;
	UINT64 val = 0;

	if (bOptional)
	{
		dwError = VmDirLdapGetOptionalStringValue(
						pLd,
						pMessage,
						pszAttrName,
						&pszValue);
		BAIL_ON_VMDIRDB_ERROR(dwError);

		pszValueRef = !pszValue ? "0" : pszValue;
	}
	else
	{
		dwError = VmDirLdapGetStringValue(
						pLd,
						pMessage,
						pszAttrName,
						&pszValue);
		BAIL_ON_VMDIRDB_ERROR(dwError);

		pszValueRef = pszValue;
	}

	val = strtoull(pszValueRef, &pszEnd, 10);

	if (!pszEnd || (pszEnd == pszValueRef) || (*pszEnd != '\0'))
	{
		dwError = ERROR_INVALID_DATA;
		BAIL_ON_VMDIRDB_ERROR(dwError);
	}

	*pValue = val;

cleanup:

	LW_SAFE_FREE_MEMORY(pszValue);

	return dwError;

error:

	*pValue = 0;

	goto cleanup;
}

static
DWORD
VmDirLdapGetOptionalStringValue(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	PSTR*        ppszValue
	)
{
	DWORD dwError = 0;
	PSTR  pszValue = NULL;

	dwError = VmDirLdapGetStringValue(
					pLd,
					pMessage,
					pszAttrName,
					&pszValue);
	BAIL_ON_VMDIRDB_ERROR(dwError);

	*ppszValue = pszValue;

cleanup:

	return dwError;

error:

	*ppszValue = NULL;

	if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
	{
		dwError = LW_ERROR_SUCCESS;
	}

	goto cleanup;
}

static
DWORD
VmDirLdapGetStringValue(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	PSTR*        ppszValue
	)
{
	DWORD dwError = 0;
	PSTR* ppszValues = NULL;
	PSTR  pszValue = NULL;

	ppszValues = (PSTR*)ldap_get_values(pLd, pMessage, pszAttrName);
	if (!ppszValues || !*ppszValues)
	{
		dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
		BAIL_ON_VMDIRDB_ERROR(dwError);
	}

	dwError = LwAllocateString(*ppszValues, &pszValue);
	BAIL_ON_VMDIRDB_ERROR(dwError);

	*ppszValue = pszValue;

cleanup:

	if (ppszValues)
	{
		ldap_value_free(ppszValues);
	}

	return dwError;

error:

	*ppszValue = NULL;

	goto cleanup;
}

static
DWORD
VmDirLdapGetStringArray(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PCSTR        pszAttrName,
	BOOLEAN      bOptional,
	PSTR**       pppszStrArray,
	PDWORD       pdwCount
	)
{
	DWORD dwError = 0;
	PSTR* ppszLDAPValues = NULL;
	PSTR* ppszStrArray = NULL;
	DWORD dwCount = 0;

	ppszLDAPValues = (PSTR*)ldap_get_values(pLd, pMessage, pszAttrName);
	if (!ppszLDAPValues || !*ppszLDAPValues)
	{
		if (!bOptional)
		{
			dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
			BAIL_ON_VMDIRDB_ERROR(dwError);
		}
	}
	else
	{
		DWORD iValue = 0;

		dwCount = ldap_count_values(ppszLDAPValues);

		dwError = LwAllocateMemory(
                              sizeof(PSTR) * dwCount,
                              (PVOID*)&ppszStrArray);
		BAIL_ON_VMDIRDB_ERROR(dwError);

		for (; iValue < dwCount; iValue++)
		{
			PSTR pszValue = ppszLDAPValues[iValue];

			dwError = LwAllocateString(pszValue, &ppszStrArray[iValue]);
			BAIL_ON_VMDIRDB_ERROR(dwError);
		}
	}

	*pppszStrArray = ppszStrArray;
	*pdwCount      = dwCount;

cleanup:

	return dwError;

error:

	*pppszStrArray = NULL;
	*pdwCount = 0;

	if (ppszStrArray)
	{
		LwFreeStringArray(ppszStrArray, dwCount);
	}

	goto cleanup;
}

static DWORD
VmDirAllocLdapQueryMapEntry(
    PSTR pszSql,
    PSTR pszLdapBasePrefix,
    PSTR pszLdapBase,
    PSTR pszLdapFilter,
    ULONG uScope,
    PSTR *ppszLdapAttributes, /* optional */
    PDWORD pdwLdapAttributesType, /* optional */
    VMDIR_LDAPQUERY_FILTER_FORMAT_FUNC pfnLdapFilterPrintf,  /* optional */
    VMDIRDB_LDAPQUERY_MAP_ENTRY_TRANSFORM_FUNC pfnTransform, /* optional */
    DWORD dwIndex,
    PVMDIRDB_LDAPQUERY_MAP pLdapMap)
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR pszBaseDn = NULL;
    PSTR pszBaseDnAlloc = NULL;
    PSTR *ppszTmpLdapAttributes = NULL;
    PDWORD pdwTmpLdapAttributesType = NULL;

    if (!pLdapMap || dwIndex > pLdapMap->dwMaxEntries)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    if (pszLdapBasePrefix)
    {
        dwError = LwAllocateStringPrintf(
                    &pszBaseDnAlloc,
                    "%s,%s",
                    pszLdapBasePrefix,
                    pszLdapBase);
        BAIL_ON_VMDIRDB_ERROR(dwError);
        pszBaseDn = pszBaseDnAlloc;
    }
    else
    {
        pszBaseDn = pszLdapBase;
    }

    pLdapMap->queryMap[dwIndex].uScope = uScope;
    dwError = LwAllocateString(
                  pszSql,
                  (VOID *) &pLdapMap->queryMap[dwIndex].pszSqlQuery);
    BAIL_ON_VMDIRDB_ERROR(dwError);
    dwError = LwAllocateString(
                  pszLdapFilter,
                  (VOID *) &pLdapMap->queryMap[dwIndex].pszLdapQuery);
    BAIL_ON_VMDIRDB_ERROR(dwError);
    dwError = LwAllocateString(
                  pszBaseDn,
                  (VOID *) &pLdapMap->queryMap[dwIndex].pszLdapBase);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    pLdapMap->queryMap[dwIndex].pfnLdapFilterPrintf = pfnLdapFilterPrintf;
    pLdapMap->queryMap[dwIndex].pfnTransform = pfnTransform;
    if (ppszLdapAttributes)
    {
        for (i=0; ppszLdapAttributes[i]; i++)
            ;
        dwError = LwAllocateMemory(sizeof(PSTR) * (i + 1),
                      (VOID *) &ppszTmpLdapAttributes);
        BAIL_ON_VMDIRDB_ERROR(dwError);
        dwError = LwAllocateMemory(sizeof(DWORD) * (i + 1),
                      (VOID *) &pdwTmpLdapAttributesType);
        BAIL_ON_VMDIRDB_ERROR(dwError);

        for (i=0; ppszLdapAttributes[i]; i++)
        {
            dwError = LwAllocateString(ppszLdapAttributes[i],
                          (VOID *) &ppszTmpLdapAttributes[i]);
            BAIL_ON_VMDIRDB_ERROR(dwError);
            pdwTmpLdapAttributesType[i] = pdwLdapAttributesType[i];
        }
        pLdapMap->queryMap[dwIndex].ppszLdapAttributes = ppszTmpLdapAttributes;
        pLdapMap->queryMap[dwIndex].pdwLdapAttributesType = pdwTmpLdapAttributesType;
    }

cleanup:
    LW_SAFE_FREE_STRING(pszBaseDnAlloc);
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pLdapMap->queryMap[dwIndex].pszSqlQuery);
    LW_SAFE_FREE_MEMORY(pLdapMap->queryMap[dwIndex].pszLdapQuery);
    LW_SAFE_FREE_MEMORY(pLdapMap->queryMap[dwIndex].pszLdapBase);
    if (ppszTmpLdapAttributes)
    {
        for (i=0; ppszTmpLdapAttributes[i]; i++)
        {
            LW_SAFE_FREE_STRING(ppszTmpLdapAttributes[i]);
        } 
        LW_SAFE_FREE_MEMORY(ppszTmpLdapAttributes);
    }
    goto cleanup;
}

DWORD
VmDirAttributeCopyEntry(
    PDIRECTORY_ATTRIBUTE out,
    PDIRECTORY_ATTRIBUTE in)
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    PWSTR pwszValue = NULL;
    POCTET_STRING pBinaryData = NULL;


    /* Copy attribute name */
    ntStatus = LwRtlWC16StringDuplicate(&out->pwszName, in->pwszName);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));


    /* Copy attribute value, according to its tagged data type */
    if (in->pValues[0].Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING)
    {
        ntStatus = LwRtlWC16StringDuplicate(
                       &pwszValue,
                       in->pValues[0].data.pwszStringValue);
        BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

        out->pValues[0].Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        out->pValues[0].data.pwszStringValue = pwszValue;
    }
    else if (in->pValues[0].Type == DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR)
    {
        /* Deal with binary data types */
        dwError = LwAllocateMemory(sizeof(OCTET_STRING),
                                   (VOID*) &pBinaryData);
        BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

        pBinaryData->ulNumBytes = in->pValues[0].data.pOctetString->ulNumBytes,
        dwError = LwAllocateMemory(
                      sizeof(UCHAR) * pBinaryData->ulNumBytes,
                      (VOID*) &pBinaryData->pBytes);
        BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

        memcpy(pBinaryData->pBytes,
               in->pValues[0].data.pOctetString->pBytes,
               pBinaryData->ulNumBytes);

        out->pValues[0].data.pOctetString = pBinaryData;
        out->pValues[0].Type = DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR;
        out->pValues[0].data.pOctetString = pBinaryData;
    }

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszValue);
    if (pBinaryData)
    {
        LW_SAFE_FREE_MEMORY(pBinaryData->pBytes);
        LW_SAFE_FREE_MEMORY(pBinaryData);
    }
    goto cleanup;
}

/*
 * Search attribute list for specified entry. If not present, create a new entry at the
 * end if the current list.
 */
DWORD
VmDirAttributeCreateFromData(
    PDIRECTORY_ATTRIBUTE pAttributes,
    DWORD dwNumAttributes,
    PSTR pszAttributeName,
    DWORD dwAttributeType,
    VOID *pAttributeValue)
{
    DWORD dwError = 0;
    DWORD i = 0;
    NTSTATUS ntStatus = 0;
    PWSTR pwszAttributeName = NULL;
    PWSTR pwszValue = NULL;
    POCTET_STRING pOctetValue = NULL;
    POCTET_STRING pBinaryData = NULL;
    ULONG uLongValue = 0;
    LONG64 lLongValue64 = 0;
    PDIRECTORY_ATTRIBUTE pAttrib = NULL;

    ntStatus = LwRtlWC16StringAllocateFromCString(&pwszAttributeName, pszAttributeName);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

    /*
     * Determine if attribute name already exists. Fail if it does. Otherwise
     * retain pointer to first empty slot in attribute table
     */
    for (i=0; i<dwNumAttributes; i++)
    {
        if (!pAttributes[i].pwszName)
        {
            /* The last attribute, and retain a pointer to that array slot */
            pAttrib = &pAttributes[i];
            break;
        }
        if (wc16scasecmp(pAttributes[i].pwszName, pwszAttributeName) == 0)
        {
            /* Bail if presented attribute already exists in array */
            dwError = LW_STATUS_OBJECTID_EXISTS;
            BAIL_ON_VMDIRDB_ERROR(dwError);
        }
    }

    /* Set data value for provided attribute */
    if (dwAttributeType == DIRECTORY_ATTR_TYPE_UNICODE_STRING)
    {
        ntStatus = LwRtlWC16StringDuplicate(
                       &pwszValue,
                       (PWSTR) pAttributeValue);
        BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));
        pAttrib->pValues[0].data.pwszStringValue = pwszValue;
        pAttrib->pValues[0].Type = dwAttributeType;
    }
    else if (dwAttributeType == DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR)
    {
        pOctetValue = (POCTET_STRING) pAttributeValue;

        /* Deal with binary data types */
        dwError = LwAllocateMemory(sizeof(OCTET_STRING),
                                   (VOID*) &pBinaryData);
        BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

        pBinaryData->ulNumBytes = pOctetValue->ulNumBytes,
        dwError = LwAllocateMemory(
                      sizeof(UCHAR) * pBinaryData->ulNumBytes,
                      (VOID*) &pBinaryData->pBytes);
        BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

        memcpy(pBinaryData->pBytes,
               pOctetValue->pBytes,
               pBinaryData->ulNumBytes);

        pAttrib->pValues[0].Type = dwAttributeType;
        pAttrib->pValues[0].data.pOctetString = pBinaryData;
    }
    else if (dwAttributeType == DIRECTORY_ATTR_TYPE_INTEGER)
    {
        memcpy(&uLongValue, pAttributeValue, sizeof(uLongValue));
        pAttrib->pValues[0].Type = dwAttributeType;
        pAttrib->pValues[0].data.ulValue = uLongValue;
    }
    else if (dwAttributeType == DIRECTORY_ATTR_TYPE_LARGE_INTEGER)
    {
        memcpy(&lLongValue64, pAttributeValue, sizeof(lLongValue64));
        pAttrib->pValues[0].Type = dwAttributeType;
        pAttrib->pValues[0].data.llValue = lLongValue64;
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }
    pAttrib->pwszName = pwszAttributeName;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszAttributeName);
    if (pBinaryData)
    {
        LW_SAFE_FREE_MEMORY(pBinaryData->pBytes);
        LW_SAFE_FREE_MEMORY(pBinaryData);
    }
    goto cleanup;
}

DWORD
VmDirConstructMachineDN(
    PSTR pszSqlDomainName,
    PSTR *ppszMachineAcctDN)
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    PSTR pszDnPtr = NULL;
    PSTR pszPtr = NULL;
    DWORD i = 0;
    
    if (!pszSqlDomainName || !ppszMachineAcctDN)
    {
        dwError =  LwNtStatusToWin32Error(STATUS_NOT_FOUND);
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    ntStatus = LwRtlCStringAllocateAppendPrintf(
                   &pszDnPtr,
                   "%s",
                   pszSqlDomainName);
    dwError =  LwNtStatusToWin32Error(ntStatus);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    for (i=0; pszDnPtr[i]; i++)
        ;

    if (i < 3 ||
        (char) tolower((int) pszDnPtr[0]) != 'c' ||
        (char) tolower((int) pszDnPtr[1]) != 'n' ||
        pszDnPtr[2] != '=')
    {
        dwError = LwNtStatusToWin32Error(STATUS_NOT_FOUND);
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    pszPtr = strchr(pszDnPtr, ',');
    if (!pszPtr || pszPtr[0] != ',' || !pszPtr[1])
    {
        dwError = LwNtStatusToWin32Error(STATUS_NOT_FOUND);
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }
    pszPtr[0] = '\0';

    /* Build the actual dn: value cn=xyz,cn=users, name component */
    ntStatus = LwRtlCStringAllocateAppendPrintf(
                   &pszDnPtr,
                   ",%s",
                   gVmdirGlobals.pszDomainDn);
    dwError =  LwNtStatusToWin32Error(ntStatus);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    *ppszMachineAcctDN = pszDnPtr;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszDnPtr);
    goto cleanup;
}

DWORD
VmDirConstructMachineUPN(
    PSTR pszSqlDomainName,
    PSTR *ppszMachineAcctUpn)
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = 0;
    PSTR pszObjectDn = NULL;
    PSTR pszAcctUpn = NULL;
    PSTR pszAcctDomain = NULL;
    PSTR pszPtr = NULL;

    dwError = VmDirConstructMachineDN(
                  pszSqlDomainName,
                  &pszObjectDn);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = LwLdapConvertDNToDomain(pszObjectDn, &pszAcctDomain);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    for (pszPtr = pszAcctDomain; *pszPtr; pszPtr++)
    {
        *pszPtr = (char) toupper((int) *pszPtr);
    }

    pszPtr = strstr(pszObjectDn+3, ",dc");
    if (pszPtr)
    {
        *pszPtr = '\0';
    }

    /* + 3 to skip "cn=" prefix */
    ntStatus = LwRtlCStringDuplicate(&pszAcctUpn, pszObjectDn+3);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

    ntStatus = LwRtlCStringAllocateAppendPrintf(
                   &pszAcctUpn,
                   "@%s",
                   pszAcctDomain);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

    *ppszMachineAcctUpn = pszAcctUpn;

cleanup:
    LW_SAFE_FREE_STRING(pszObjectDn);
    LW_SAFE_FREE_STRING(pszAcctDomain);
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszAcctUpn);
    goto cleanup;
}

DWORD
VmDirConstructMachineDNFromName(
    PSTR pszName,
    PSTR *ppszMachineAcctDN)
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    PSTR pszDnPtr = NULL;
    
    if (!pszName || !ppszMachineAcctDN)
    {
        dwError =  LwNtStatusToWin32Error(STATUS_NOT_FOUND);
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    ntStatus = LwRtlCStringAllocateAppendPrintf(
                   &pszDnPtr,
                   "%s",
                   pszName);
    dwError =  LwNtStatusToWin32Error(ntStatus);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    /* Build the actual dn: value cn=xyz,cn=users, name component */
    ntStatus = LwRtlCStringAllocateAppendPrintf(
                   &pszDnPtr,
                   ",%s",
                   gVmdirGlobals.pszDomainDn);
    dwError =  LwNtStatusToWin32Error(ntStatus);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    *ppszMachineAcctDN = pszDnPtr;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszDnPtr);
    goto cleanup;
}

/* ================== SQL to LDAP translation callback functions =============*/

/* 
 * "ObjectClass=1 AND Domain='lightwave.local'"
 *
 * "(&(objectclass=dcObject)(entryDn=%s))"
 */
static PSTR
pfnSqlToLdapEntryDnXform(
    PSTR pszLdapFilter,
    ...)
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = 0;
    PSTR *ppszAttributes = NULL;
    PSTR pszModifiedFilter = NULL;
    PSTR pszDomainDn = NULL;
    va_list ap;

    va_start(ap, pszLdapFilter);

    /* Function type passes list to attribute string array */
    ppszAttributes = (PSTR *) va_arg(ap, char **);
    if (ppszAttributes && ppszAttributes[0])
    {
        dwError = LwLdapConvertDomainToDN(ppszAttributes[0],
                                          &pszDomainDn);
        BAIL_ON_VMDIRDB_ERROR(dwError);

        ntStatus = LwRtlCStringAllocateAppendPrintf(
                       &pszModifiedFilter,
                       pszLdapFilter,
                       pszDomainDn);
        BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));
    }

    /* TBD: Implement logic to patch %s field with ppszAttribute value */


cleanup:
    va_end(ap);
    return pszModifiedFilter;

error:
    LW_SAFE_FREE_MEMORY(pszModifiedFilter);
    goto cleanup;
}
    

/*
 * Handle filter of this form:
 * "(ObjectClass=1 OR ObjectClass=2) AND ObjectSID='S-1-5-21-100314066-221396614-742840509'"
 *
 * "(&(objectclass=dcObject)(objectSid=%s))",
 */
static PSTR
pfnSqlToLdapObjectSidXform(
    PSTR pszLdapFilter,
    ...)
{
    NTSTATUS ntStatus = 0;
    PSTR pszModifiedFilter = NULL;
    PSTR *ppszAttributes = NULL;

    va_list ap;

    va_start(ap, pszLdapFilter);
    ppszAttributes = (PSTR *) va_arg(ap, char **);

    if (ppszAttributes && ppszAttributes[0])
    {
        ntStatus = LwRtlCStringAllocateAppendPrintf(
                       &pszModifiedFilter,
                       pszLdapFilter,
                       ppszAttributes[0]);
        BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));
    }

cleanup:
    va_end(ap);

    return pszModifiedFilter;

error:
    LW_SAFE_FREE_MEMORY(pszModifiedFilter);
    goto cleanup;
}


/*
 * "(ObjectClass=5 AND SamAccountName='PHOTON--59U15NB$' AND Domain='photon-102-test') OR
 *  (ObjectClass=4 AND SamAccountName='PHOTON--59U15NB$' AND Domain='photon-102-test')"
 *
 *  SAMDB_OBJECT_CLASS_USER            = 5
 *  SAMDB_OBJECT_CLASS_LOCAL_GROUP     = 4
 *
 *xxx  "(|(&(objectClass=user)(samAccountName=%s)(domain=%s))(&(objectClass=local)(samAccountName=%s)(domain=%s)))"
 *
 *  "(|(&(objectClass=user)(samAccountName=%s))(&(objectClass=computer)(samAccountName=%s)))"
 */

static PSTR
    pfnSqlToLdapsamAcctAndDomainXform(
    PSTR pszLdapFilter,
    ...)
{
    NTSTATUS ntStatus = 0;
    PSTR pszModifiedFilter = NULL;
    PSTR *ppszAttributes = NULL;
    va_list ap;

    va_start(ap, pszLdapFilter);
    ppszAttributes = (PSTR *) va_arg(ap, char **);

    if (ppszAttributes && ppszAttributes[0])
    {
        ntStatus = LwRtlCStringAllocateAppendPrintf(
                       &pszModifiedFilter,
                       pszLdapFilter,
                       ppszAttributes[0],
                       ppszAttributes[0]);
        BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));
    }

cleanup:
    va_end(ap);

    return pszModifiedFilter;

error:
    LW_SAFE_FREE_MEMORY(pszModifiedFilter);
    goto cleanup;
}

/*
 * "DistinguishedName='CN=PHOTON--59U15NB$,dc=photon-102-test'"
 * 
 * "(dn=CN=PHOTON--59U15NB$)"
 */
static PSTR
pfnSqlToLdapDistinguishedNameXform(
    PSTR pszLdapFilter,
    ...)
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    PSTR pszMachineAcctDn = NULL;
    PSTR *ppszAttributes = NULL;
    PSTR pszModifiedFilter = NULL;
    PSTR pszDomainNameIn = NULL;

    va_list ap;

    va_start(ap, pszLdapFilter);
    ppszAttributes = (PSTR *) va_arg(ap, char **);

    pszDomainNameIn = ppszAttributes[0];
    if (!pszDomainNameIn)
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));
    }

    dwError = VmDirConstructMachineDN(
                  pszDomainNameIn,
                  &pszMachineAcctDn);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    ntStatus = LwRtlCStringAllocateAppendPrintf(
                   &pszModifiedFilter,
                   pszLdapFilter,
                   pszMachineAcctDn);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

cleanup:
    va_end(ap);

    return pszModifiedFilter;

error:
#if 1
    LW_SAFE_FREE_STRING(pszMachineAcctDn);
    LW_SAFE_FREE_MEMORY(pszModifiedFilter);
#else
    LW_SAFE_FREE_STRING(pszCommonName);
#endif
    goto cleanup;
}



/* ====== LDAP response to DIRECTORY_ENTRY translation callback functions ====*/


/*
 * SQL Filter: "ObjectClass=1 OR ObjectClass=2"
 * SQL Attributes: "entryDn" -> "CommonName"
 *
 * LDAP Filter: "(objectclass=dcObject)",
 */
static DWORD
pfnLdap2DirectoryEntryDnToCn(
    DWORD dwNumEntries,
    PDIRECTORY_ENTRY in,
    PDIRECTORY_ENTRY *ppOut)
{
    DWORD dwError = 0;
    DWORD dwEntries = 0;
    PDWORD pdwAttributesCount = NULL;
    PWSTR pwszCommonName = NULL;
    PSTR pszDn = NULL;
    PSTR pszDomainName = NULL;
    PWSTR pwszDomainName = NULL;
    NTSTATUS ntStatus = 0; 
    WCHAR wszEntryDn[] = {'e','n','t','r','y','D','n',0};
    PDIRECTORY_ENTRY pOut = NULL;

    /* Memory has been allocated for this transform by the caller */
    if (!in || dwNumEntries > 1 || 
        in[0].ulNumAttributes != 1)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    /* Allocate attributes count array, then allocate return DIRECTORY_ENTRY */
    dwError = LwAllocateMemory(
                  sizeof(DWORD) * (dwNumEntries + 1),
                  (VOID *) &pdwAttributesCount);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    for (dwEntries=0; dwEntries < dwNumEntries; dwEntries++)
    {
        pdwAttributesCount[dwEntries] = in[dwEntries].ulNumAttributes;
    }
    dwError = VmdirDbAllocateEntriesAndAttributes(
                  dwNumEntries,
                  pdwAttributesCount,
                  &pOut);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    if (!LwRtlWC16StringIsEqual(in[0].pAttributes[0].pwszName,
                                wszEntryDn,
                                FALSE))
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    /* Replace "entryDn" with "CommonName" */
    if (!LwRtlWC16StringIsEqual(in[0].pAttributes[0].pwszName, wszEntryDn, FALSE))
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    /* Replace DN value with FQDN format */
    ntStatus = LwRtlCStringAllocateFromWC16String(
                   &pszDn, 
                   in[0].pAttributes[0].pValues[0].data.pwszStringValue);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

    dwError = LwLdapConvertDNToDomain(pszDn, &pszDomainName);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    ntStatus = LwRtlWC16StringAllocateFromCString(&pwszCommonName, "CommonName");
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));
   
    ntStatus = LwRtlWC16StringAllocateFromCString(&pwszDomainName, pszDomainName);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

    /* Replace output DIRECTORY_ENTRY values with the converted form */
    pOut[0].pAttributes[0].pwszName = pwszCommonName;

    pOut[0].pAttributes[0].pValues[0].data.pwszStringValue = pwszDomainName;
    pOut[0].pAttributes[0].pValues[0].Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;

    /* Save domain name for later use */
    if (!gVmdirGlobals.pszDomainDn)
    {
        gVmdirGlobals.pszDomainDn = pszDn;
    }

    *ppOut = pOut;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszCommonName);
    LW_SAFE_FREE_MEMORY(pOut);
    LW_SAFE_FREE_STRING(pszDn);
    goto cleanup;
}



#if 1

/*
 * Translate entryDN to CommonName Response for this filter:
 * "(ObjectClass=1 OR ObjectClass=2) AND ObjectSID='%s'"
 */
static DWORD
pfnLdap2DirectoryEntryObjectSIDDnToCn(
    DWORD dwNumEntries,
    PDIRECTORY_ENTRY in,
    PDIRECTORY_ENTRY *ppOut)
{
    NTSTATUS ntStatus = 0; 
    DWORD dwError = 0;
    DWORD iEntry = 0; /* loop over dwNumEntries */
    DWORD dwEntries = 0;
    ULONG ulNumAttributes = 0;
    PDWORD pdwAttributesCount = NULL;
#if 0 /*TBD: Adam */
    DWORD i = 0;
    DWORD iEntryDn = 0;
    PDIRECTORY_ATTRIBUTE pAttrib = NULL;
    WCHAR wszEntryDn[] = {'e','n','t','r','y','D','n',0};
#endif
    PWSTR pwszCommonName = NULL;
    PWSTR pwszDomainName = NULL;
    PSTR pszDn = NULL;
    PSTR pszDomainName = NULL;
    PDIRECTORY_ENTRY pOut = NULL;
    CHAR szHostName[128] = {0};
    PSTR pszDcHostName = NULL;
    PWSTR pwszStr = NULL;
    ULONG ulValue = 0;
    ULONG64 ulValue64 = 0;

    /* Memory has been allocated for this transform by the caller */
    if (!in || dwNumEntries > 1)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    /* Allocate attributes count array, then allocate return DIRECTORY_ENTRY */
    dwError = LwAllocateMemory(
                  sizeof(DWORD) * (dwNumEntries + 1),
                  (VOID *) &pdwAttributesCount);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    /* TBD: Make this a for loop */
    ulNumAttributes = in[dwEntries].ulNumAttributes;

    /*
     * Increase the total attribute count, as 8 "fake" values are added.
     */
    pdwAttributesCount[0] = ulNumAttributes + 8;

    dwError = VmdirDbAllocateEntriesAndAttributes(
                  dwNumEntries,
                  pdwAttributesCount,
                  &pOut);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    /* The "CommonName" */
    gethostname(szHostName, sizeof(szHostName)-1);

    /* The "DistinguishedName"; just dc= + hostname */
    ntStatus = LwRtlCStringAllocateAppendPrintf(
                   &pszDcHostName,
                   "dc=%s",
                   szHostName);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

    for (dwEntries=0; dwEntries < ulNumAttributes; dwEntries++)
    {
        dwError = VmDirAttributeCopyEntry(
                      &pOut[iEntry].pAttributes[dwEntries],
                      &in[iEntry].pAttributes[dwEntries]);
        BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));
    }

    /* Add "stubbed in" data values needed for this lookup to work properly */

    /* "CommonName, i.e. hostname */
    ntStatus = LwRtlWC16StringAllocateFromCString(&pwszStr, szHostName);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

    dwError = VmDirAttributeCreateFromData(
                  pOut[iEntry].pAttributes,
                  pdwAttributesCount[0],
                  "CommonName",
                  DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                  (VOID *) pwszStr);
    BAIL_ON_VMDIRDB_ERROR(dwError);
    LW_SAFE_FREE_MEMORY(pwszStr);

    /* "DistinguishedName, i.e. DC="hostname" */
    ntStatus = LwRtlWC16StringAllocateFromCString(&pwszStr, pszDcHostName);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

    dwError = VmDirAttributeCreateFromData(
                  pOut[iEntry].pAttributes,
                  pdwAttributesCount[0],
                  "DistinguishedName",
                  DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                  (VOID *) pwszStr);
    BAIL_ON_VMDIRDB_ERROR(dwError);
    LW_SAFE_FREE_MEMORY(pwszStr);

    /* MinPwdAge = 0 in samdb */
    ulValue64 = 0;
    dwError = VmDirAttributeCreateFromData(
                  pOut[iEntry].pAttributes,
                  pdwAttributesCount[0],
                  "MinPwdAge",
                  DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                  (VOID *) &ulValue64);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    /* MaxPwdAge = 24192000000000 in samdb */
    ulValue64 = 24192000000000LL;
    dwError = VmDirAttributeCreateFromData(
                  pOut[iEntry].pAttributes,
                  pdwAttributesCount[0],
                  "MaxPwdAge",
                  DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                  (VOID *) &ulValue64);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    /* MinPwdLength = 0 in samdb */
    ulValue = 0;
    dwError = VmDirAttributeCreateFromData(
                  pOut[iEntry].pAttributes,
                  pdwAttributesCount[0],
                  "MinPwdLength",
                  DIRECTORY_ATTR_TYPE_INTEGER,
                  (VOID *) &ulValue);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    /* PwdPromptTime = 12096000000000 in samdb */
    ulValue64 = 12096000000000LL;
    dwError = VmDirAttributeCreateFromData(
                  pOut[iEntry].pAttributes,
                  pdwAttributesCount[0],
                  "PwdPromptTime",
                  DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                  (VOID *) &ulValue64);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    /* PwdProperties = 0 in samdb */
    ulValue = 0;
    dwError = VmDirAttributeCreateFromData(
                  pOut[iEntry].pAttributes,
                  pdwAttributesCount[0],
                  "PwdProperties",
                  DIRECTORY_ATTR_TYPE_INTEGER,
                  (VOID *) &ulValue);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    /* SequenceNumber = 31 in samdb */
    ulValue64 = 0;
    dwError = VmDirAttributeCreateFromData(
                  pOut[iEntry].pAttributes,
                  pdwAttributesCount[0],
                  "SequenceNumber",
                  DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                  (VOID *) &ulValue64);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    *ppOut = pOut;

cleanup:
    LW_SAFE_FREE_STRING(pszDn);
    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszDcHostName);

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszCommonName);
    LW_SAFE_FREE_MEMORY(pwszDomainName);
    LW_SAFE_FREE_MEMORY(pwszStr);
    goto cleanup;
}


/*
 * Translate entryDN to DistinguishedName response for this filter:
 * "(ObjectClass=5 AND SamAccountName='%s' AND Domain='%s') OR (ObjectClass=4 AND SamAccountName='%s' AND Domain='%s')",
 */
static DWORD
pfnLdap2DirectoryEntryDomainNameXform(
    DWORD dwNumEntries,
    PDIRECTORY_ENTRY in,
    PDIRECTORY_ENTRY *ppOut)
{
    DWORD dwError = 0;
    DWORD dwEntries = 0;
    ULONG ulValue = 0;
    PDWORD pdwAttributesCount = NULL;
    PWSTR pwszName = NULL;
    PSTR pszName = NULL;
    PSTR pszValue = NULL;
    NTSTATUS ntStatus = 0; 
    WCHAR wszEntryDn[] = {'e','n','t','r','y','D','n',0};
    WCHAR wszAccountFlags[] = {'u','s','e','r','A','c','c','o','u','n','t','C','o','n','t','r','o','l',0};

    PDIRECTORY_ENTRY pOut = NULL;

    /* Memory has been allocated for this transform by the caller */
    if (!in)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }
    else if (dwNumEntries > 1 || in[0].ulNumAttributes != 1)
    {
        dwError = ERROR_INVALID_EVENT_COUNT;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    /* Allocate attributes count array, then allocate return DIRECTORY_ENTRY */
    dwError = LwAllocateMemory(
                  sizeof(DWORD) * (dwNumEntries + 1),
                  (VOID *) &pdwAttributesCount);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    for (dwEntries=0; dwEntries < dwNumEntries; dwEntries++)
    {
        pdwAttributesCount[dwEntries] = in[dwEntries].ulNumAttributes;
    }
    dwError = VmdirDbAllocateEntriesAndAttributes(
                  dwNumEntries,
                  pdwAttributesCount,
                  &pOut);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    if (LwRtlWC16StringIsEqual(in[0].pAttributes[0].pwszName,
                                wszEntryDn,
                                FALSE))
    {
        pszName = "DistinguishedName";
    }
    else if (LwRtlWC16StringIsEqual(in[0].pAttributes[0].pwszName,
                                wszAccountFlags,
                                FALSE))
    {
        pszName = "AccountFlags";
    }
    else
    {
        dwError = ERROR_INVALID_NAME;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }
    ntStatus = LwRtlWC16StringAllocateFromCString(&pwszName, pszName);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

    /* Change value type to INTEGER */
    ntStatus = LwRtlCStringAllocateFromWC16String(
                   &pszValue,
                   in[0].pAttributes[0].pValues[0].data.pwszStringValue);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));


    /* Replace output DIRECTORY_ENTRY values with the converted form */
    pOut[0].pAttributes[0].pwszName = pwszName;
    ulValue = strtoul(pszValue, NULL, 10);
    pOut[0].pAttributes[0].pValues[0].data.ulValue = ulValue;
    pOut[0].pAttributes[0].pValues[0].Type = DIRECTORY_ATTR_TYPE_INTEGER;

    *ppOut = pOut;

cleanup:
    LW_SAFE_FREE_MEMORY(pdwAttributesCount);
    LW_SAFE_FREE_STRING(pszValue);
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszName);
    goto cleanup;
}


#endif

/* ===================  End of pfn translation layer functions ================== */


DWORD
VmDirAllocLdapQueryMap(
    PSTR pszSearchBase,
    PVMDIRDB_LDAPQUERY_MAP *ppLdapMap)
{
    DWORD dwError = 0;
    DWORD dwMaxEntries = 64; /* TBD, could be much larger */
    DWORD i = 0;
    PVMDIRDB_LDAPQUERY_MAP pLdapMap = NULL;

    dwError = LwAllocateMemory(
                  sizeof(VMDIRDB_LDAPQUERY_MAP) +
                      sizeof(VMDIRDB_LDAPQUERY_MAP_ENTRY) * dwMaxEntries,
                  (VOID *) &pLdapMap);
    BAIL_ON_VMDIRDB_ERROR(dwError);
    pLdapMap->dwMaxEntries = dwMaxEntries;

#if 0 /* TBD: Don't need to store pszSearchBase in map context */
    dwError = LwAllocateString(
                  pszSearchBase,
                  (VOID *) &pLdapMap->pszSearchBase);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    SAMDB_OBJECT_CLASS_DOMAIN          = 1
    SAMDB_OBJECT_CLASS_BUILTIN_DOMAIN  = 2
    SAMDB_OBJECT_CLASS_CONTAINER       = 3
    SAMDB_OBJECT_CLASS_LOCAL_GROUP     = 4
    SAMDB_OBJECT_CLASS_USER            = 5
    SAMDB_OBJECT_CLASS_LOCALGRP_MEMBER = 6
#endif

    /* Initialize map entries */
    dwError = VmDirAllocLdapQueryMapEntry(
                  "ObjectClass = 5",
                  NULL,                    /* SearchBasePrefix (optional) */
                  pszSearchBase,
                  "(cn=*)",
                  LDAP_SCOPE_SUBTREE,
                  NULL,
                  NULL,
                  NULL,
                  NULL,
                  i++,
                  pLdapMap);
    BAIL_ON_VMDIRDB_ERROR(dwError);
   
    /* Initialize filters */

    /* LSAR filters */

    /* SAMR filters */
    /* Domain Object: cn or objectSid query */
{
    PSTR pszOptionalAttributes[] = {"cn", "entryDn", 0};
    DWORD dwOptionalAttributesType[] = { 6, 6, 0 };
    dwError = VmDirAllocLdapQueryMapEntry(
                  "ObjectClass = 1",
                  NULL,                    /* SearchBasePrefix (optional) */
                  pszSearchBase,
                  "(objectclass=dcObject)",
                  LDAP_SCOPE_SUBTREE,
                  pszOptionalAttributes,
                  dwOptionalAttributesType,
                  NULL,
                  NULL,
                  i++,
                  pLdapMap);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = VmDirAllocLdapQueryMapEntry(
                  "ObjectClass=1 OR ObjectClass=2",
                  NULL,                    /* SearchBasePrefix (optional) */
                  pszSearchBase,
                  "(objectclass=dcObject)",
                  LDAP_SCOPE_SUBTREE,
                  pszOptionalAttributes,
                  dwOptionalAttributesType,
                  NULL, /* filter transform callback */
                  pfnLdap2DirectoryEntryDnToCn,
                  i++,
                  pLdapMap);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = VmDirAllocLdapQueryMapEntry(
                  "ObjectClass=1 AND Domain='%s'", /* "ObjectClass=1 AND Domain='lightwave.local'" */
                  NULL,                    /* SearchBasePrefix (optional) */
                  pszSearchBase,
                  "(&(objectclass=dcObject)(entryDn=%s))",
                  LDAP_SCOPE_SUBTREE,
                  NULL,  /* attributes */
                  NULL, /* Attribute types */
                  pfnSqlToLdapEntryDnXform,
                  NULL,/* DIRECTORY_ENTRY transform callback */
                  i++,
                  pLdapMap);
    BAIL_ON_VMDIRDB_ERROR(dwError);
}

    dwError = VmDirAllocLdapQueryMapEntry(
                  "ObjectClass=2 AND Domain='Builtin'",
                  "cn=builtin",            /* SearchBasePrefix (optional) */
                  pszSearchBase,
                  "(cn=*)",
                  LDAP_SCOPE_SUBTREE,
                  NULL,
                  NULL, /* Attribute types */
                  NULL,
                  NULL,
                  i++,
                  pLdapMap);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = VmDirAllocLdapQueryMapEntry(
                  "(ObjectClass=1 OR ObjectClass=2) AND ObjectSID='%s'",
                  NULL,                    /* SearchBasePrefix (optional) */
                  pszSearchBase,
                  "(&(objectclass=dcObject)(objectSid=%s))",
                  LDAP_SCOPE_SUBTREE,
                  NULL, /* override attributes */
                  NULL, /* Attribute types */
                  pfnSqlToLdapObjectSidXform, /* ldap transform callback */
                  pfnLdap2DirectoryEntryObjectSIDDnToCn,
                  i++,
                  pLdapMap);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = VmDirAllocLdapQueryMapEntry(
                  "(ObjectClass=5 AND SamAccountName='%s' AND Domain='%s') OR (ObjectClass=4 AND SamAccountName='%s' AND Domain='%s')",
                  NULL,                    /* SearchBasePrefix (optional) */
                  pszSearchBase,
                  "(|(&(objectClass=user)(samAccountName=%s))(&(objectClass=computer)(samAccountName=%s)))",
                  LDAP_SCOPE_SUBTREE,
                  NULL, /* override attributes */
                  NULL, /* Attribute types */
                  pfnSqlToLdapsamAcctAndDomainXform, /* ldap transform callback */
                  NULL,/* DIRECTORY_ENTRY transform callback */
                  i++,
                  pLdapMap);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = VmDirAllocLdapQueryMapEntry(
                  "DistinguishedName='%s'",
                  NULL,                    /* SearchBasePrefix (optional) */
                  pszSearchBase,
                  "(entrydn=%s)",   /* must search entrydn vs dn */
                  LDAP_SCOPE_SUBTREE,
                  NULL, /* override attributes */
                  NULL, /* Attribute types */
                  pfnSqlToLdapDistinguishedNameXform, /* ldap transform callback */
                  pfnLdap2DirectoryEntryDomainNameXform, /* DE transform callback */
                  i++,
                  pLdapMap);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    pLdapMap->dwNumEntries = i;
    *ppLdapMap = pLdapMap;

cleanup:
    return dwError;

error:
    VmDirFreeLdapQueryMap(&pLdapMap);
    goto cleanup;
}

DWORD
VmDirFreeLdapQueryMap(
    PVMDIRDB_LDAPQUERY_MAP *ppLdapMap)
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    PVMDIRDB_LDAPQUERY_MAP pLdapMap = NULL;

    if (!ppLdapMap)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }
    pLdapMap = *ppLdapMap;

    for (dwIndex=0; dwIndex < pLdapMap->dwNumEntries; dwIndex++)
    {
        LW_SAFE_FREE_MEMORY(pLdapMap->queryMap[dwIndex].pszSqlQuery);
        LW_SAFE_FREE_MEMORY(pLdapMap->queryMap[dwIndex].pszLdapQuery);
        LW_SAFE_FREE_MEMORY(pLdapMap->queryMap[dwIndex].pszLdapBase);
    }
    
#if 0
    LW_SAFE_FREE_MEMORY(pLdapMap->pszSearchBase);
#endif
    LW_SAFE_FREE_MEMORY(pLdapMap);
    *ppLdapMap = NULL;

cleanup:
    return dwError;

error:
    goto cleanup;
}

   

DWORD
VmDirAllocLdapAttributeMap(
    PVMDIRDB_LDAPATTR_MAP *ppAttrMap)
{
    DWORD dwError = 0;
    DWORD dwMaxEntries = 0;
    DWORD i = 0;
    PVMDIRDB_LDAPATTR_MAP pAttrMap = NULL;

    WCHAR wszVMDIR_DB_DIR_ATTR_COMMON_NAME[] = VMDIR_DB_DIR_ATTR_COMMON_NAME;
    WCHAR wszVMDIR_DB_DIR_ATTR_DISTINGUISHED_NAME[] = VMDIR_DB_DIR_ATTR_DISTINGUISHED_NAME;
    WCHAR wszVMDIR_DB_DIR_ATTR_SAM_ACCOUNT_NAME[] = VMDIR_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszVMDIR_DB_DIR_ATTR_SECURITY_DESCRIPTOR[] = VMDIR_DB_DIR_ATTR_SECURITY_DESCRIPTOR;
    WCHAR wszVMDIR_DB_DIR_ATTR_OBJECT_CLASS[] = VMDIR_DB_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszVMDIR_DB_DIR_ATTR_CREATED_TIME[] = VMDIR_DB_DIR_ATTR_CREATED_TIME;
    WCHAR wszVMDIR_DB_DIR_ATTR_OBJECT_SID[] = VMDIR_DB_DIR_ATTR_OBJECT_SID;
    WCHAR wszVMDIR_DB_DIR_ATTR_UID[] = VMDIR_DB_DIR_ATTR_UID;
    WCHAR wszVMDIR_DB_DIR_ATTR_MEMBERS[] = VMDIR_DB_DIR_ATTR_MEMBERS;

//    WCHAR wszVMDIR_DB_DIR_ATTR_RECORD_ID[] = VMDIR_DB_DIR_ATTR_RECORD_ID;
    WCHAR wszVMDIR_DB_DIR_ATTR_PARENT_DN[] = VMDIR_DB_DIR_ATTR_PARENT_DN;
//    WCHAR wszVMDIR_DB_DIR_ATTR_DOMAIN[] = VMDIR_DB_DIR_ATTR_DOMAIN;
//    WCHAR wszVMDIR_DB_DIR_ATTR_NETBIOS_NAME[] = VMDIR_DB_DIR_ATTR_NETBIOS_NAME;
    WCHAR wszVMDIR_DB_DIR_ATTR_USER_PRINCIPAL_NAME[] = VMDIR_DB_DIR_ATTR_USER_PRINCIPAL_NAME;
    WCHAR wszVMDIR_DB_DIR_ATTR_DESCRIPTION[] = VMDIR_DB_DIR_ATTR_DESCRIPTION;
    WCHAR wszVMDIR_DB_DIR_ATTR_COMMENT[] = VMDIR_DB_DIR_ATTR_COMMENT;
    WCHAR wszVMDIR_DB_DIR_ATTR_PASSWORD[] = VMDIR_DB_DIR_ATTR_PASSWORD;
    WCHAR wszVMDIR_DB_DIR_ATTR_ACCOUNT_FLAGS[] = VMDIR_DB_DIR_ATTR_ACCOUNT_FLAGS;
//    WCHAR wszVMDIR_DB_DIR_ATTR_GECOS[] = VMDIR_DB_DIR_ATTR_GECOS;
//    WCHAR wszVMDIR_DB_DIR_ATTR_HOME_DIR[] = VMDIR_DB_DIR_ATTR_HOME_DIR;
//    WCHAR wszVMDIR_DB_DIR_ATTR_HOME_DRIVE[] = VMDIR_DB_DIR_ATTR_HOME_DRIVE;
//    WCHAR wszVMDIR_DB_DIR_ATTR_LOGON_SCRIPT[] = VMDIR_DB_DIR_ATTR_LOGON_SCRIPT;
//    WCHAR wszVMDIR_DB_DIR_ATTR_PROFILE_PATH[] = VMDIR_DB_DIR_ATTR_PROFILE_PATH;
//    WCHAR wszVMDIR_DB_DIR_ATTR_WORKSTATIONS[] = VMDIR_DB_DIR_ATTR_WORKSTATIONS;
//    WCHAR wszVMDIR_DB_DIR_ATTR_PARAMETERS[] = VMDIR_DB_DIR_ATTR_PARAMETERS;
//    WCHAR wszVMDIR_DB_DIR_ATTR_SHELL[] = VMDIR_DB_DIR_ATTR_SHELL;
    WCHAR wszVMDIR_DB_DIR_ATTR_PASSWORD_LAST_SET[] = VMDIR_DB_DIR_ATTR_PASSWORD_LAST_SET;
//    WCHAR wszVMDIR_DB_DIR_ATTR_ALLOW_PASSWORD_CHANGE[] = VMDIR_DB_DIR_ATTR_ALLOW_PASSWORD_CHANGE;
//    WCHAR wszVMDIR_DB_DIR_ATTR_FORCE_PASSWORD_CHANGE[] = VMDIR_DB_DIR_ATTR_FORCE_PASSWORD_CHANGE;
//    WCHAR wszVMDIR_DB_DIR_ATTR_FULL_NAME[] = VMDIR_DB_DIR_ATTR_FULL_NAME;
    WCHAR wszVMDIR_DB_DIR_ATTR_ACCOUNT_EXPIRY[] = VMDIR_DB_DIR_ATTR_ACCOUNT_EXPIRY;
//    WCHAR wszVMDIR_DB_DIR_ATTR_LM_HASH[] = VMDIR_DB_DIR_ATTR_LM_HASH;
//    WCHAR wszVMDIR_DB_DIR_ATTR_NT_HASH[] = VMDIR_DB_DIR_ATTR_NT_HASH;
//    WCHAR wszVMDIR_DB_DIR_ATTR_PRIMARY_GROUP[] = VMDIR_DB_DIR_ATTR_PRIMARY_GROUP;
//    WCHAR wszVMDIR_DB_DIR_ATTR_GID[] = VMDIR_DB_DIR_ATTR_GID;
//    WCHAR wszVMDIR_DB_DIR_ATTR_COUNTRY_CODE[] = VMDIR_DB_DIR_ATTR_COUNTRY_CODE;
//    WCHAR wszVMDIR_DB_DIR_ATTR_CODE_PAGE[] = VMDIR_DB_DIR_ATTR_CODE_PAGE;
      WCHAR wszVMDIR_DB_DIR_ATTR_MAX_PWD_AGE[] = VMDIR_DB_DIR_ATTR_MAX_PWD_AGE;
//    WCHAR wszVMDIR_DB_DIR_ATTR_MIN_PWD_AGE[] = VMDIR_DB_DIR_ATTR_MIN_PWD_AGE;
//    WCHAR wszVMDIR_DB_DIR_ATTR_PWD_PROMPT_TIME[] = VMDIR_DB_DIR_ATTR_PWD_PROMPT_TIME;
    WCHAR wszVMDIR_DB_DIR_ATTR_LAST_LOGON[] = VMDIR_DB_DIR_ATTR_LAST_LOGON;
//    WCHAR wszVMDIR_DB_DIR_ATTR_LAST_LOGOFF[] = VMDIR_DB_DIR_ATTR_LAST_LOGOFF;
//    WCHAR wszVMDIR_DB_DIR_ATTR_LOCKOUT_TIME[] = VMDIR_DB_DIR_ATTR_LOCKOUT_TIME;
//    WCHAR wszVMDIR_DB_DIR_ATTR_LOGON_COUNT[] = VMDIR_DB_DIR_ATTR_LOGON_COUNT;
//    WCHAR wszVMDIR_DB_DIR_ATTR_BAD_PASSWORD_COUNT[] = VMDIR_DB_DIR_ATTR_BAD_PASSWORD_COUNT;
//    WCHAR wszVMDIR_DB_DIR_ATTR_LOGON_HOURS[] = VMDIR_DB_DIR_ATTR_LOGON_HOURS;
//    WCHAR wszVMDIR_DB_DIR_ATTR_ROLE[] = VMDIR_DB_DIR_ATTR_ROLE;
    WCHAR wszVMDIR_DB_DIR_ATTR_MIN_PWD_LENGTH[] = VMDIR_DB_DIR_ATTR_MIN_PWD_LENGTH;
    WCHAR wszVMDIR_DB_DIR_ATTR_PWD_HISTORY_LENGTH[] = VMDIR_DB_DIR_ATTR_PWD_HISTORY_LENGTH;
    WCHAR wszVMDIR_DB_DIR_ATTR_PWD_PROPERTIES[] = VMDIR_DB_DIR_ATTR_PWD_PROPERTIES;
//    WCHAR wszVMDIR_DB_DIR_ATTR_FORCE_LOGOFF_TIME[] = VMDIR_DB_DIR_ATTR_FORCE_LOGOFF_TIME;
//    WCHAR wszVMDIR_DB_DIR_ATTR_PRIMARY_DOMAIN[] = VMDIR_DB_DIR_ATTR_PRIMARY_DOMAIN;
    WCHAR wszVMDIR_DB_DIR_ATTR_SEQUENCE_NUMBER[] = VMDIR_DB_DIR_ATTR_SEQUENCE_NUMBER;
//    WCHAR wszVMDIR_DB_DIR_ATTR_LOCKOUT_DURATION[] = VMDIR_DB_DIR_ATTR_LOCKOUT_DURATION;
//    WCHAR wszVMDIR_DB_DIR_ATTR_LOCKOUT_WINDOW[] = VMDIR_DB_DIR_ATTR_LOCKOUT_WINDOW;
//    WCHAR wszVMDIR_DB_DIR_ATTR_LOCKOUT_THRESHOLD[] = VMDIR_DB_DIR_ATTR_LOCKOUT_THRESHOLD;


#if 0
#define DIRECTORY_ATTR_TYPE_BOOLEAN                 1
#define DIRECTORY_ATTR_TYPE_INTEGER                 2
#define DIRECTORY_ATTR_TYPE_LARGE_INTEGER           3
#define DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR  4
#define DIRECTORY_ATTR_TYPE_OCTET_STREAM            5
#define DIRECTORY_ATTR_TYPE_UNICODE_STRING          6
#define DIRECTORY_ATTR_TYPE_ANSI_STRING             7

#endif

    /* The order of dwAttributeTypes, szAttributes and wszAttributes MUST be the same!!! */
    DWORD dwAttributeTypes[] = {
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING, /* 10 */
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING, /* 20 */
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
    };


    /* The order of dwAttributeTypes, szAttributes and wszAttributes MUST be the same!!! */
    CHAR *szAttributes[] = {
        "cn",
        "entryDn", /* TBD:Adam-Kyoung says entryDn is real; dn is abstract */
        "sAMAccountName",
        "nTSecurityDescriptor",
        "objectClass",
        "createTimeStamp",
        "objectSid",
        "uid",
        "member",
        "parentid",                         /* 10 */
        "userPrincipalName",
        "description",
        "comment",
        "userPassword",
        "pwdLastSet",
        "vmwPasswordLifetimeDays",
        "pwdLastSet",
        "lastLogonTimestamp",
        "vmwPasswordMinLength",
        "vmwPasswordProhibitedPreviousCount", /* 20 */
        "vmwPasswordSpecialChars",
        "vmwRidSequenceNumber",
        "userAccountControl",
    };

    /* The order of dwAttributeTypes, szAttributes and wszAttributes MUST be the same!!! */
    WCHAR *wszAttributes[] = {
        wszVMDIR_DB_DIR_ATTR_COMMON_NAME,
        wszVMDIR_DB_DIR_ATTR_DISTINGUISHED_NAME,
        wszVMDIR_DB_DIR_ATTR_SAM_ACCOUNT_NAME,
        wszVMDIR_DB_DIR_ATTR_SECURITY_DESCRIPTOR,
        wszVMDIR_DB_DIR_ATTR_OBJECT_CLASS,
        wszVMDIR_DB_DIR_ATTR_CREATED_TIME,
        wszVMDIR_DB_DIR_ATTR_OBJECT_SID,
        wszVMDIR_DB_DIR_ATTR_UID,
        wszVMDIR_DB_DIR_ATTR_MEMBERS,
        wszVMDIR_DB_DIR_ATTR_PARENT_DN,           /* 10 */
        wszVMDIR_DB_DIR_ATTR_USER_PRINCIPAL_NAME,
        wszVMDIR_DB_DIR_ATTR_DESCRIPTION,
        wszVMDIR_DB_DIR_ATTR_COMMENT,
        wszVMDIR_DB_DIR_ATTR_PASSWORD,
        wszVMDIR_DB_DIR_ATTR_PASSWORD_LAST_SET,
        wszVMDIR_DB_DIR_ATTR_ACCOUNT_EXPIRY,
        wszVMDIR_DB_DIR_ATTR_MAX_PWD_AGE,
        wszVMDIR_DB_DIR_ATTR_LAST_LOGON,
        wszVMDIR_DB_DIR_ATTR_MIN_PWD_LENGTH,
        wszVMDIR_DB_DIR_ATTR_PWD_HISTORY_LENGTH,  /* 20 */
        wszVMDIR_DB_DIR_ATTR_PWD_PROPERTIES,
        wszVMDIR_DB_DIR_ATTR_SEQUENCE_NUMBER,
        wszVMDIR_DB_DIR_ATTR_ACCOUNT_FLAGS,

        wszVMDIR_DB_DIR_ATTR_EOL, /* This must be the last entry: End Of Line */

#if 0 /* Maybe need to map these attributes. Use above pattern */
        { VMDIR_DB_DIR_ATTR_RECORD_ID },
        { VMDIR_DB_DIR_ATTR_PARENT_DN },
        { VMDIR_DB_DIR_ATTR_DOMAIN },
        { VMDIR_DB_DIR_ATTR_NETBIOS_NAME },
        { VMDIR_DB_DIR_ATTR_USER_PRINCIPAL_NAME },
        { VMDIR_DB_DIR_ATTR_DESCRIPTION },
        { VMDIR_DB_DIR_ATTR_COMMENT },
        { VMDIR_DB_DIR_ATTR_PASSWORD },
        { VMDIR_DB_DIR_ATTR_ACCOUNT_FLAGS },
        { VMDIR_DB_DIR_ATTR_GECOS },
        { VMDIR_DB_DIR_ATTR_HOME_DIR },
        { VMDIR_DB_DIR_ATTR_HOME_DRIVE },
        { VMDIR_DB_DIR_ATTR_LOGON_SCRIPT },
        { VMDIR_DB_DIR_ATTR_PROFILE_PATH },
        { VMDIR_DB_DIR_ATTR_WORKSTATIONS },
        { VMDIR_DB_DIR_ATTR_PARAMETERS },
        { VMDIR_DB_DIR_ATTR_SHELL },
        { VMDIR_DB_DIR_ATTR_PASSWORD_LAST_SET },
        { VMDIR_DB_DIR_ATTR_ALLOW_PASSWORD_CHANGE },
        { VMDIR_DB_DIR_ATTR_FORCE_PASSWORD_CHANGE },
        { VMDIR_DB_DIR_ATTR_FULL_NAME },
        { VMDIR_DB_DIR_ATTR_ACCOUNT_EXPIRY },
        { VMDIR_DB_DIR_ATTR_LM_HASH },
        { VMDIR_DB_DIR_ATTR_NT_HASH },
        { VMDIR_DB_DIR_ATTR_PRIMARY_GROUP },
        { VMDIR_DB_DIR_ATTR_GID },
        { VMDIR_DB_DIR_ATTR_COUNTRY_CODE },
        { VMDIR_DB_DIR_ATTR_CODE_PAGE },
        { VMDIR_DB_DIR_ATTR_MAX_PWD_AGE },
        { VMDIR_DB_DIR_ATTR_MIN_PWD_AGE },
        { VMDIR_DB_DIR_ATTR_PWD_PROMPT_TIME },
        { VMDIR_DB_DIR_ATTR_LAST_LOGON },
        { VMDIR_DB_DIR_ATTR_LAST_LOGOFF },
        { VMDIR_DB_DIR_ATTR_LOCKOUT_TIME },
        { VMDIR_DB_DIR_ATTR_LOGON_COUNT },
        { VMDIR_DB_DIR_ATTR_BAD_PASSWORD_COUNT },
        { VMDIR_DB_DIR_ATTR_LOGON_HOURS },
        { VMDIR_DB_DIR_ATTR_ROLE },
        { VMDIR_DB_DIR_ATTR_MIN_PWD_LENGTH },
        { VMDIR_DB_DIR_ATTR_PWD_HISTORY_LENGTH },
        { VMDIR_DB_DIR_ATTR_PWD_PROPERTIES },
        { VMDIR_DB_DIR_ATTR_FORCE_LOGOFF_TIME },
        { VMDIR_DB_DIR_ATTR_PRIMARY_DOMAIN },
        { VMDIR_DB_DIR_ATTR_SEQUENCE_NUMBER },
        { VMDIR_DB_DIR_ATTR_LOCKOUT_DURATION },
        { VMDIR_DB_DIR_ATTR_LOCKOUT_WINDOW },
        { VMDIR_DB_DIR_ATTR_LOCKOUT_THRESHOLD },
#endif
    };


    for (i=0; wszAttributes[i]; i++)
        ;

    dwMaxEntries = i;

    dwError = LwAllocateMemory(
                  sizeof(VMDIRDB_LDAPATTR_MAP) +
                      sizeof(VMDIRDB_LDAPATTR_MAP_ENTRY) * dwMaxEntries,
                               (VOID *) &pAttrMap);
    BAIL_ON_VMDIRDB_ERROR(dwError);
    
    for (i=0; wszAttributes[i]; i++)
    {
        /* Wide character "SQL" attribute name */
        dwError = LwRtlWC16StringDuplicate(
                      &pAttrMap->attrMap[i].pwszAttribute,
                      wszAttributes[i]);

        /* C String "LDAP" attribute name */
        dwError = LwAllocateString(szAttributes[i],
                                   (VOID *) &pAttrMap->attrMap[i].pszAttribute);
        BAIL_ON_VMDIRDB_ERROR(dwError);
        pAttrMap->attrMap[i].dwType = dwAttributeTypes[i];
    }
    pAttrMap->dwNumEntries = i;
    pAttrMap->dwMaxEntries = i;
    *ppAttrMap = pAttrMap;

cleanup:
    return dwError;

error:
    VmdirFreeLdapAttributeMap(&pAttrMap);
    goto cleanup;
}

DWORD
VmdirFreeLdapAttributeMap(
    PVMDIRDB_LDAPATTR_MAP *ppAttrMap)
{
    DWORD dwError = 0;
    DWORD i = 0;
    PVMDIRDB_LDAPATTR_MAP pAttrMap = NULL;

    if (!ppAttrMap)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    pAttrMap = *ppAttrMap;
    if (pAttrMap)
    {
        for (i=0; pAttrMap->dwNumEntries; i++)
        {
            LW_SAFE_FREE_MEMORY(pAttrMap->attrMap[i].pwszAttribute);
            LW_SAFE_FREE_MEMORY(pAttrMap->attrMap[i].pszAttribute);
        }
        LW_SAFE_FREE_MEMORY(pAttrMap);
        *ppAttrMap = NULL;
    }
    
cleanup:
    return dwError;

error:
    goto cleanup;
}


/* Assume the sequence '%s' is an argument in the SQL filter */
static DWORD
VmDirSqlFilterMatchWithArgs(
    PSTR pszSqlStatement,
    PSTR pszSqlFilter,
    DWORD *pbFound,
    PSTR **pppszArgs)
{
    DWORD dwError = 0;
    DWORD ai = 0;
    DWORD aj = 0;
    DWORD ak = 0;
    DWORD argIdx = 0;
    int lc1 = 0;
    int lc2 = 0;
    int quotePos = 0;
    PSTR *ppszArgs = NULL;
    PSTR pszArg = NULL;
    DWORD dwNumArgs = 10;
    DWORD dwLenArg = 0;
    DWORD bFound = 0;

    /* Allocate ppszArgs */
    dwError = LwAllocateMemory(sizeof(PSTR) * dwNumArgs,
                               (VOID *) &ppszArgs);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    /* Pattern search, looking for '%s' value(s) */
    for (ai=0, aj=0; pszSqlStatement[ai] && pszSqlFilter[aj]; ai++)
    {
        lc1 = tolower((int) pszSqlStatement[ai]);
        lc2 = tolower((int) pszSqlFilter[aj]);
        if (lc1 == lc2)
        {
            if (lc1 == '\'')
            {
                quotePos = aj;
            }
            aj++;
        }

        /*
         * Found %; Looking for '%s'. Verify previous char is ', 
         * and next is 's'.
         */
        else if (pszSqlFilter[aj] == '%' && aj == (quotePos+1) &&
                 pszSqlFilter[aj+1] && pszSqlFilter[aj+1] == 's' &&
                 pszSqlFilter[aj+2] && pszSqlFilter[aj+2] == '\'')
        {
            aj += 3; /* Skip over %s' in sqlFilter */

            /* Allocate space for argument from sqlStatement */
            dwLenArg = strlen(&pszSqlStatement[ai]);
            dwError = LwAllocateMemory(sizeof(CHAR) * dwLenArg,
                                   (VOID *) &pszArg);
            BAIL_ON_VMDIRDB_ERROR(dwError);

            for (ak=0;
                 pszSqlStatement[ak + ai] && pszSqlStatement[ak + ai] != '\'';
                 ak++)
            {
                pszArg[ak] = pszSqlStatement[ak+ai];
            }

            /* Found start of an argument. All stuff until next ' is the argument */
            ppszArgs[argIdx++] = pszArg;
            ai += ak;

            if (argIdx >= dwNumArgs)
            {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_VMDIRDB_ERROR(dwError);
            }
        }
        else
        {
            /* no match; fail */
            break;
        }
    }

    if (!pszSqlStatement[ai] && !pszSqlFilter[aj])
    {
        bFound = 1;
        *pbFound = bFound;
        *pppszArgs = ppszArgs;
    }


cleanup:
    return dwError;

error:
    if (ppszArgs)
    {
        for (ai=0; ai<argIdx; ai++)
        {
            LW_SAFE_FREE_MEMORY(ppszArgs[ai]);
        }
        LW_SAFE_FREE_MEMORY(ppszArgs);
    }
    goto cleanup;

}

DWORD
VmDirFindLdapQueryMapEntry(
    PSTR pszSqlQuery,
    PVMDIRDB_LDAPQUERY_MAP_ENTRY *ppQueryMapEntry)
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD bFound = 0;
    PSTR *ppszSqlArgs = NULL;
    PSTR pszPatchLdapFilter = NULL;
    PVMDIRDB_LDAPQUERY_MAP pLdapMap = NULL;
    PVMDIRDB_LDAPQUERY_MAP_ENTRY pQueryMapEntry = NULL;

    if (!pszSqlQuery || !gVmdirGlobals.pLdapMap)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }
    pLdapMap = gVmdirGlobals.pLdapMap;

    for (i=0; i < pLdapMap->dwNumEntries; i++)
    {
        /* 
         * More complex match. Take into account pattern match terms which 
         * are returned data. Example: "ObjectClass=1 AND Domain='%s'
         */
        dwError = VmDirSqlFilterMatchWithArgs(
                       pszSqlQuery,
                       pLdapMap->queryMap[i].pszSqlQuery,
                       &bFound,
                       &ppszSqlArgs);
        BAIL_ON_VMDIRDB_ERROR(dwError);
        if (bFound)
        {
            pQueryMapEntry = &pLdapMap->queryMap[i];

            /* Patch LDAP filter with variable argument data from SQL search */
            if (pLdapMap->queryMap[i].pfnLdapFilterPrintf)
            {
                pszPatchLdapFilter = pLdapMap->queryMap[i].pfnLdapFilterPrintf(
                                         pLdapMap->queryMap[i].pszLdapQuery,
                                         ppszSqlArgs);
                if (pszPatchLdapFilter)
                {
                    LW_SAFE_FREE_MEMORY(pQueryMapEntry->pszLdapQuery);
                    pQueryMapEntry->pszLdapQuery = pszPatchLdapFilter;
                }
            }
            break;
        }
    }

    if (!pQueryMapEntry)
    {
        /* No filter translation found; error out */

        dwError = LW_ERROR_LDAP_FILTER_ERROR;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    *ppQueryMapEntry = pQueryMapEntry;
    return dwError;

cleanup:
    return dwError;

error:
    goto cleanup;
}

/*
 * Map PWSTR array of "SQL" attributes to PSTR array of "LDAP" 
 * attribute equivalents.
 */
DWORD
VmdirFindLdapAttributeList(
    PSTR *ppszAttributes,
    PSTR **pppszLdapAttributes,
    PDWORD *ppdwLdapAttributeTypes)
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD j = 0;
    DWORD iFound = 0;
    PSTR *ppszLdapAttributes = NULL;
    PDWORD pdwLdapAttributeTypes = NULL;
    
    PVMDIRDB_LDAPATTR_MAP pAttrMap = NULL;

    if (!ppszAttributes || !pppszLdapAttributes || !gVmdirGlobals.pLdapAttrMap)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }
    pAttrMap = gVmdirGlobals.pLdapAttrMap;

    for (i=0; ppszAttributes[i]; i++)
        ;

    dwError = LwAllocateMemory((i+1) * sizeof(PSTR),
                               (VOID *) &ppszLdapAttributes);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = LwAllocateMemory((i+1) * sizeof(DWORD),
                               (VOID *) &pdwLdapAttributeTypes);
    BAIL_ON_VMDIRDB_ERROR(dwError);
    
    for (i=0; ppszAttributes[i]; i++)
    {
        for (j=0; j < pAttrMap->dwNumEntries; j++)
        {
            if (LwRtlCStringIsEqual(ppszAttributes[i],
                                       (pAttrMap->attrMap[j].pszAttribute),
                                        FALSE))
            {
                dwError = LwAllocateString(
                              pAttrMap->attrMap[j].pszAttribute,
                              (VOID *) &ppszLdapAttributes[iFound]);
                BAIL_ON_VMDIRDB_ERROR(dwError);
                pdwLdapAttributeTypes[iFound] = pAttrMap->attrMap[j].dwType;
                iFound++;
                break;
            }
        }
    }
    
    *pppszLdapAttributes = ppszLdapAttributes;
    *ppdwLdapAttributeTypes = pdwLdapAttributeTypes;
    return dwError;

cleanup:
    return dwError;

error:
#if 0 /* TBD: Write cleanup function for pRetAttrMapEntries */
    VmdirFreeLdapAttributeList(&ppszLdapAttributes);
#endif
    goto cleanup;
}

DWORD
VmdirFindLdapPwszAttributeList(
    PWSTR *ppwszAttributes,
    PSTR **pppszLdapAttributes,
    PDWORD *ppdwLdapAttributeTypes)
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD j = 0;
    DWORD iFound = 0;
    PSTR *ppszLdapAttributes = NULL;
    PDWORD pdwLdapAttributeTypes = NULL;
    
    PVMDIRDB_LDAPATTR_MAP pAttrMap = NULL;

    if (!ppwszAttributes || !pppszLdapAttributes || !gVmdirGlobals.pLdapAttrMap)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }
    pAttrMap = gVmdirGlobals.pLdapAttrMap;

    for (i=0; ppwszAttributes[i]; i++)
        ;

    dwError = LwAllocateMemory((i+1) * sizeof(PSTR),
                               (VOID *) &ppszLdapAttributes);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = LwAllocateMemory((i+1) * sizeof(DWORD),
                               (VOID *) &pdwLdapAttributeTypes);
    BAIL_ON_VMDIRDB_ERROR(dwError);
    
    for (i=0; ppwszAttributes[i]; i++)
    {
        for (j=0; j < pAttrMap->dwNumEntries; j++)
        {
            if (LwRtlWC16StringIsEqual(ppwszAttributes[i],
                                       (pAttrMap->attrMap[j].pwszAttribute),
                                        FALSE))
            {
                dwError = LwAllocateString(
                              pAttrMap->attrMap[j].pszAttribute,
                              (VOID *) &ppszLdapAttributes[iFound]);
                BAIL_ON_VMDIRDB_ERROR(dwError);
                pdwLdapAttributeTypes[iFound] = pAttrMap->attrMap[j].dwType;
                iFound++;
                break;
            }
        }
    }
    
    *pppszLdapAttributes = ppszLdapAttributes;
    *ppdwLdapAttributeTypes = pdwLdapAttributeTypes;
    return dwError;

cleanup:
    return dwError;

error:
#if 0 /* TBD: Write cleanup function for pRetAttrMapEntries */
    VmdirFreeLdapAttributeList(&ppszLdapAttributes);
#endif
    goto cleanup;
}


DWORD
VmdirFreeLdapAttributeList(
    PSTR **pppszLdapAttributes)
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR *ppszLdapAttributes = NULL;

    if (!pppszLdapAttributes)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    ppszLdapAttributes = *pppszLdapAttributes;

    for (i=0; ppszLdapAttributes[i]; i++)
    {
        LW_SAFE_FREE_MEMORY(ppszLdapAttributes[i]);
    }
    LW_SAFE_FREE_MEMORY(ppszLdapAttributes);
    *ppszLdapAttributes = NULL;
    

cleanup:
    return dwError;

error:
    goto cleanup;
}
