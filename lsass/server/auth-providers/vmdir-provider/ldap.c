/*
 * Copyright (C) VMware. All rights reserved.
 */

#include "includes.h"

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

static
DWORD
VmDirLdapGetBinaryValue(
    LDAP*        pLd,
    LDAPMessage* pMessage,
    PCSTR        pszAttrName,
    PVMDIR_DATA  *ppucValue
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

    switch (gVmDirAuthProviderGlobals.bindProtocol)
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
    BAIL_ON_VMDIR_ERROR(dwError);

    dwNumObjects = ldap_count_entries(pLd, pMessage);

    if (dwNumObjects == 0)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
    }
    else if (dwNumObjects != 1)
    {
        dwError = ERROR_INVALID_DATA;
    }
    BAIL_ON_VMDIR_ERROR(dwError);

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
                    BAIL_ON_VMDIR_ERROR(dwError);
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
                    BAIL_ON_VMDIR_ERROR(dwError);
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
                    BAIL_ON_VMDIR_ERROR(dwError);
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
                    BAIL_ON_VMDIR_ERROR(dwError);
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
                BAIL_ON_VMDIR_ERROR(dwError);

                break;

            case VMDIR_ATTR_TYPE_BINARY:
                dwError = VmDirLdapGetBinaryValue(
                                pLd,
                                pMessage,
                                pAttr->pszName,
                                pAttr->dataRef.ppData);
                BAIL_ON_VMDIR_ERROR(dwError);
                break;

            default:

                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_VMDIR_ERROR(dwError);

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
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwMapLdapErrorToLwError(
                    ldap_set_option(pLd, LDAP_OPT_PROTOCOL_VERSION, &ldapVer));
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwMapLdapErrorToLwError(
                    ldap_set_option(
                                  pLd,
                                  LDAP_OPT_X_SASL_NOCANON,
                                  LDAP_OPT_ON));
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwKrb5SetThreadDefaultCachePath(
                    pszCachePath,
                    &pszOldCachePath);
    BAIL_ON_VMDIR_ERROR(dwError);

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
    BAIL_ON_VMDIR_ERROR(dwError);

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
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwMapLdapErrorToLwError(
                  ldap_set_option(pLd, LDAP_OPT_PROTOCOL_VERSION, &ldapVer));
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwMapLdapErrorToLwError(
                  ldap_set_option(
                                  pLd,
                                  LDAP_OPT_X_SASL_NOCANON,
                                  LDAP_OPT_ON));
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateString(pszUPN, &pszUPN_local);
    BAIL_ON_VMDIR_ERROR(dwError);

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
    BAIL_ON_VMDIR_ERROR(dwError);

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
            BAIL_ON_VMDIR_ERROR(dwError);
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
    BAIL_ON_VMDIR_ERROR(dwError);

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
        BAIL_ON_VMDIR_ERROR(dwError);

        pszValueRef = !pszValue ? "0" : pszValue;
    }
    else
    {
        dwError = VmDirLdapGetStringValue(
                        pLd,
                        pMessage,
                        pszAttrName,
                        &pszValue);
        BAIL_ON_VMDIR_ERROR(dwError);

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
        BAIL_ON_VMDIR_ERROR(dwError);

        pszValueRef = !pszValue ? "0" : pszValue;
    }
    else
    {
        dwError = VmDirLdapGetStringValue(
                            pLd,
                            pMessage,
                            pszAttrName,
                            &pszValue);
        BAIL_ON_VMDIR_ERROR(dwError);

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
        BAIL_ON_VMDIR_ERROR(dwError);

        pszValueRef = !pszValue ? "0" : pszValue;
    }
    else
    {
        dwError = VmDirLdapGetStringValue(
                        pLd,
                        pMessage,
                        pszAttrName,
                        &pszValue);
        BAIL_ON_VMDIR_ERROR(dwError);

        pszValueRef = pszValue;
    }

    val = strtoll(pszValueRef, &pszEnd, 10);

    if (!pszEnd || (pszEnd == pszValueRef) || (*pszEnd != '\0'))
    {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_VMDIR_ERROR(dwError);
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
        BAIL_ON_VMDIR_ERROR(dwError);

        pszValueRef = !pszValue ? "0" : pszValue;
    }
    else
    {
        dwError = VmDirLdapGetStringValue(
                        pLd,
                        pMessage,
                        pszAttrName,
                        &pszValue);
        BAIL_ON_VMDIR_ERROR(dwError);

        pszValueRef = pszValue;
    }

    val = strtoull(pszValueRef, &pszEnd, 10);

    if (!pszEnd || (pszEnd == pszValueRef) || (*pszEnd != '\0'))
    {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_VMDIR_ERROR(dwError);
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
    BAIL_ON_VMDIR_ERROR(dwError);

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
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = LwAllocateString(*ppszValues, &pszValue);
    BAIL_ON_VMDIR_ERROR(dwError);

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
            BAIL_ON_VMDIR_ERROR(dwError);
        }
    }
    else
    {
        DWORD iValue = 0;

        dwCount = ldap_count_values(ppszLDAPValues);

        dwError = LwAllocateMemory(
                        sizeof(PSTR) * dwCount,
                        (PVOID*)&ppszStrArray);
        BAIL_ON_VMDIR_ERROR(dwError);

        for (; iValue < dwCount; iValue++)
        {
            PSTR pszValue = ppszLDAPValues[iValue];

            dwError = LwAllocateString(pszValue, &ppszStrArray[iValue]);
            BAIL_ON_VMDIR_ERROR(dwError);
        }
    }

    *pppszStrArray = ppszStrArray;
    *pdwCount      = dwCount;

cleanup:

    if (ppszLDAPValues)
    {
        ldap_value_free(ppszLDAPValues);
    }

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


static
DWORD
VmDirLdapGetBinaryValue(
    LDAP*        pLd,
    LDAPMessage* pMessage,
    PCSTR        pszAttrName,
    PVMDIR_DATA  *ppData
    )
{
    DWORD dwError = 0;
    struct berval **ppbv_val = NULL;
    PVMDIR_DATA pData = NULL;

    ppbv_val = ldap_get_values_len(pLd, pMessage, pszAttrName);
    if (!ppbv_val)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sizeof(*pData) + ppbv_val[0]->bv_len,
                               (PVOID*)&pData);
    BAIL_ON_VMDIR_ERROR(dwError);

    pData->pData = ((PBYTE) &pData->pData) + sizeof(pData->pData);
    memcpy(pData->pData, (PVOID) ppbv_val[0]->bv_val, ppbv_val[0]->bv_len);
    pData->dwDataLen = ppbv_val[0]->bv_len;

    *ppData = pData;

cleanup:

    if (ppbv_val)
    {
        ldap_value_free_len(ppbv_val);
    }

    return dwError;

error:

    LW_SAFE_FREE_MEMORY(pData);
    goto cleanup;
}
