/*
 * Copyright (C) VMware. All rights reserved.
 */

#include "includes.h"

static
DWORD
VmDirRepositoryEnumUsers(
    PVMDIR_ENUM_HANDLE     pEnumHandle,
    DWORD                  dwMaxCount,
    PLSA_SECURITY_OBJECT** pppObjects,
    PDWORD                 pdwCount
    );

static
DWORD
VmDirRepositoryEnumGroups(
    PVMDIR_ENUM_HANDLE     pEnumHandle,
    DWORD                  dwMaxCount,
    PLSA_SECURITY_OBJECT** pppObjects,
    PDWORD                 pdwCount
    );

static
DWORD
VmDirFindUserObject(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszFilter,
    PLSA_SECURITY_OBJECT* ppObject
    );

static
DWORD
VmDirFindGroupObject(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszFilter,
    PLSA_SECURITY_OBJECT* ppObject
    );

static
DWORD
VmDirBuildUserObject(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszSamAcctName,
    PCSTR                 pszDN,
    PCSTR                 pszObjectSid,
    PCSTR                 pszUPN,
    PCSTR                 pszFirstname,
    PCSTR                 pszLastname,
    DWORD                 dwUserAccountControl,
    PLSA_SECURITY_OBJECT* ppObject
    );

static
DWORD
VmDirBuildGroupObject(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszSamAcctName,
    PCSTR                 pszDN,
    PCSTR                 pszObjectSid,
    PLSA_SECURITY_OBJECT* ppObject
    );

BOOLEAN
VmDirMatchesDomain(
    PCSTR pszDomain
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bIsMatch = FALSE;
    PVMDIR_BIND_INFO pBindInfo = NULL;

    dwError = VmDirGetBindInfo(&pBindInfo);
    BAIL_ON_VMDIR_ERROR(dwError);

    bIsMatch = (!strcasecmp(pszDomain, pBindInfo->pszDomainFqdn) ||
                   (pBindInfo->pszDomainShort &&
                       !strcasecmp(pszDomain, pBindInfo->pszDomainShort)));

cleanup:

    if (pBindInfo)
    {
        VmDirReleaseBindInfo(pBindInfo);
    }

    return bIsMatch;

error:

    // bIsMatch = FALSE;

    goto cleanup;
}

DWORD
VmDirFindUserByName(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszLoginId,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszDomain  = NULL;
    PSTR  pszAccount = NULL;
    PSTR  pszFilter = NULL;
    PLSA_SECURITY_OBJECT pObject  = NULL;

    dwError = VmDirCrackLoginId(pszLoginId, &pszDomain, &pszAccount);
    BAIL_ON_VMDIR_ERROR(dwError);

    if (pszDomain && !VmDirMatchesDomain(pszDomain))
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(
                      &pszFilter,
                      "(&(objectclass=%s)(!(objectclass=%s))(%s=%s))",
                      VMDIR_OBJ_CLASS_USER,
                      VMDIR_OBJ_CLASS_COMPUTER,
                      VMDIR_ATTR_NAME_ACCOUNT,
                      pszAccount);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirFindUserObject(
                      pDirContext,
                      pszFilter,
                      &pObject);
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppObject = pObject;

cleanup:

    LW_SAFE_FREE_MEMORY(pszDomain);
    LW_SAFE_FREE_MEMORY(pszAccount);
    LW_SAFE_FREE_MEMORY(pszFilter);

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    switch (dwError)
    {
        case LW_ERROR_LDAP_NO_SUCH_OBJECT:
        case LW_ERROR_NO_SUCH_OBJECT:
            dwError = LW_ERROR_NO_SUCH_USER;
            break;
    }

    goto cleanup;
}

DWORD
VmDirFindUserById(
    PVMDIR_DIR_CONTEXT    pDirContext,
    uid_t                 uid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszRid = NULL;
    PSTR  pszDomainSID = NULL;
    PSTR  pszUserSID = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;

    dwError = VmDirFindDomainSID(pDirContext, &pszDomainSID);
    BAIL_ON_VMDIR_ERROR(dwError);

    /* Map UID to new RID format */
    dwError = VmDirGetRIDFromUID(uid, &pszRid);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                  &pszUserSID,
                  "%s-%s",
                  pszDomainSID,
                  pszRid);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirFindUserBySID(pDirContext, pszUserSID, &pObject);
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppObject = pObject;

cleanup:

    LW_SAFE_FREE_STRING(pszDomainSID);
    LW_SAFE_FREE_STRING(pszUserSID);
    LW_SAFE_FREE_STRING(pszRid);

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    switch (dwError)
    {
        case LW_ERROR_LDAP_NO_SUCH_OBJECT:
        case LW_ERROR_NO_SUCH_OBJECT:
            dwError = LW_ERROR_NO_SUCH_USER;
            break;
    }

    goto cleanup;
}

DWORD
VmDirFindUserBySID(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszSID,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszFilter = NULL;
    PLSA_SECURITY_OBJECT pObject  = NULL;

    dwError = LwAllocateStringPrintf(
                      &pszFilter,
                      "(&(objectclass=%s)(!(objectclass=%s))(%s=%s))",
                      VMDIR_OBJ_CLASS_USER,
                      VMDIR_OBJ_CLASS_COMPUTER,
                      VMDIR_ATTR_NAME_OBJECTSID,
                      pszSID);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirFindUserObject(pDirContext, pszFilter, &pObject);
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppObject = pObject;

cleanup:

    LW_SAFE_FREE_MEMORY(pszFilter);

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    switch (dwError)
    {
        case LW_ERROR_LDAP_NO_SUCH_OBJECT:
        case LW_ERROR_NO_SUCH_OBJECT:
            dwError = LW_ERROR_NO_SUCH_USER;
            break;
    }

    goto cleanup;
}

DWORD
VmDirFindGroupByName(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszGroupName,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszDomain  = NULL;
    PSTR  pszAccount = NULL;
    PSTR pszFilter = NULL;
    PLSA_SECURITY_OBJECT pObject  = NULL;

    dwError = VmDirCrackLoginId(pszGroupName, &pszDomain, &pszAccount);
    BAIL_ON_VMDIR_ERROR(dwError);

    if (pszDomain && !VmDirMatchesDomain(pszDomain))
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(
                      &pszFilter,
                      "(&(objectclass=%s)(%s=%s))",
                      VMDIR_OBJ_CLASS_GROUP,
                      VMDIR_ATTR_NAME_ACCOUNT,
                      pszAccount);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirFindGroupObject(pDirContext, pszFilter, &pObject);
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppObject = pObject;

cleanup:

    LW_SAFE_FREE_MEMORY(pszDomain);
    LW_SAFE_FREE_MEMORY(pszAccount);
    LW_SAFE_FREE_MEMORY(pszFilter);

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    switch (dwError)
    {
        case LW_ERROR_LDAP_NO_SUCH_OBJECT:
        case LW_ERROR_NO_SUCH_OBJECT:
            dwError = LW_ERROR_NO_SUCH_GROUP;
            break;
    }

    goto cleanup;
}

DWORD
VmDirFindGroupById(
    PVMDIR_DIR_CONTEXT    pDirContext,
    gid_t                 gid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszDomainSID = NULL;
    PSTR  pszGroupSID = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;

    dwError = VmDirFindDomainSID(pDirContext, &pszDomainSID);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                      &pszGroupSID,
                      "%s-%u",
                      pszDomainSID,
                      gid);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirFindGroupBySID(pDirContext, pszGroupSID, &pObject);
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppObject = pObject;

cleanup:

    LW_SAFE_FREE_STRING(pszDomainSID);
    LW_SAFE_FREE_STRING(pszGroupSID);

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    switch (dwError)
    {
        case LW_ERROR_LDAP_NO_SUCH_OBJECT:
        case LW_ERROR_NO_SUCH_OBJECT:
            dwError = LW_ERROR_NO_SUCH_GROUP;
            break;
    }

    goto cleanup;
}

DWORD
VmDirFindGroupBySID(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszSID,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszFilter = NULL;
    PLSA_SECURITY_OBJECT pObject  = NULL;

    dwError = LwAllocateStringPrintf(
                      &pszFilter,
                      "(&(objectclass=%s)(%s=%s))",
                      VMDIR_OBJ_CLASS_GROUP,
                      VMDIR_ATTR_NAME_OBJECTSID,
                      pszSID);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirFindGroupObject(pDirContext, pszFilter, &pObject);
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppObject = pObject;

cleanup:

    LW_SAFE_FREE_MEMORY(pszFilter);

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    switch (dwError)
    {
        case LW_ERROR_LDAP_NO_SUCH_OBJECT:
        case LW_ERROR_NO_SUCH_OBJECT:
            dwError = LW_ERROR_NO_SUCH_GROUP;
            break;
    }

    goto cleanup;
}

DWORD
VmDirFindDomainSID(
    PVMDIR_DIR_CONTEXT pDirContext,
    PSTR*              ppszDomainSID
    )
{
    DWORD dwError = 0;
    PSTR  pszAttrName_objectsid    = VMDIR_ATTR_NAME_OBJECTSID;
    PSTR  attrs[] =
    {
        pszAttrName_objectsid,
        NULL
    };
    PSTR  pszDomainSID = NULL;
    VMDIR_ATTR values[]  =
    {
        {
            .pszName   = pszAttrName_objectsid,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = FALSE,
            .dataRef =
               {
                   .ppszData = &pszDomainSID
               },
               .size    = -1
        }
    };
    PCSTR pszFilter = "(objectclass=*)";
    LDAPMessage* pLdapResult = NULL;

    dwError = VmDirLdapQuerySingleObject(
                      pDirContext->pLd,
                      pDirContext->pBindInfo->pszSearchBase,
                      LDAP_SCOPE_BASE,
                      pszFilter,
                      &attrs[0],
                      &pLdapResult);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirLdapGetValues(
                      pDirContext->pLd,
                      pLdapResult,
                      &values[0],
                      sizeof(values)/sizeof(values[0]));
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppszDomainSID = pszDomainSID;

cleanup:

    if (pLdapResult)
    {
        VmDirLdapFreeMessage(pLdapResult);
    }

    return dwError;

error:

    *ppszDomainSID = NULL;

    LW_SAFE_FREE_MEMORY(pszDomainSID);

    goto cleanup;
}

DWORD
VmDirFindObjectBySID(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszSID,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_SECURITY_OBJECT pObject = NULL;

    dwError = VmDirFindUserBySID(pDirContext, pszSID, &pObject);
    if (dwError == LW_ERROR_NO_SUCH_USER)
    {
        dwError = VmDirFindGroupBySID(pDirContext, pszSID, &pObject);
    }
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppObject = pObject;

cleanup:

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    switch (dwError)
    {
        case LW_ERROR_LDAP_NO_SUCH_OBJECT:
            dwError = LW_ERROR_NO_SUCH_OBJECT;
            break;
    }

    goto cleanup;
}

DWORD
VmDirCreateUserEnumHandle(
    PVMDIR_DIR_CONTEXT  pDirContext,
    PVMDIR_ENUM_HANDLE* ppEnumHandle
    )
{
    DWORD dwError   = 0;
    PVMDIR_ENUM_HANDLE pEnumHandle = NULL;

    dwError = LwAllocateMemory(sizeof(*pEnumHandle), (PVOID*)&pEnumHandle);
    BAIL_ON_VMDIR_ERROR(dwError);

    pEnumHandle->type             = VMDIR_ENUM_HANDLE_TYPE_OBJECTS;
    pEnumHandle->objectType       = LSA_OBJECT_TYPE_USER;
    pEnumHandle->pDirContext      = pDirContext;
    pEnumHandle->sizeLimit        = 1024;
    pEnumHandle->llLastUSNChanged = 0;
    pEnumHandle->dwIndex          = 0;

    *ppEnumHandle = pEnumHandle;

cleanup:

    return dwError;

error:

    *ppEnumHandle = NULL;

    if (pEnumHandle)
    {
        VmDirCloseEnum(pEnumHandle);
    }

    goto cleanup;
}

DWORD
VmDirCreateGroupEnumHandle(
    PVMDIR_DIR_CONTEXT  pDirContext,
    PVMDIR_ENUM_HANDLE* ppEnumHandle
    )
{
    DWORD dwError = 0;
    PVMDIR_ENUM_HANDLE pEnumHandle = NULL;

    dwError = LwAllocateMemory(sizeof(*pEnumHandle), (PVOID*)&pEnumHandle);
    BAIL_ON_VMDIR_ERROR(dwError);

    pEnumHandle->type             = VMDIR_ENUM_HANDLE_TYPE_OBJECTS;
    pEnumHandle->objectType       = LSA_OBJECT_TYPE_GROUP;
    pEnumHandle->pDirContext      = pDirContext;
    pEnumHandle->sizeLimit        = 1024;
    pEnumHandle->llLastUSNChanged = 0;
    pEnumHandle->dwIndex          = 0;

    *ppEnumHandle = pEnumHandle;

cleanup:

    return dwError;

error:

    *ppEnumHandle = NULL;

    goto cleanup;
}

DWORD
VmDirRepositoryEnumObjects(
    PVMDIR_ENUM_HANDLE     pEnumHandle,
    DWORD                  dwMaxCount,
    PLSA_SECURITY_OBJECT** pppObjects,
    PDWORD                 pdwCount
    )
{
    DWORD dwError = 0;

    switch (pEnumHandle->objectType)
    {
        case LSA_OBJECT_TYPE_USER:

            dwError = VmDirRepositoryEnumUsers(
                          pEnumHandle,
                          dwMaxCount,
                          pppObjects,
                          pdwCount);

            break;

        case LSA_OBJECT_TYPE_GROUP:

            dwError = VmDirRepositoryEnumGroups(
                          pEnumHandle,
                          dwMaxCount,
                          pppObjects,
                          pdwCount);

            break;

        default:

            dwError = ERROR_INVALID_STATE;

            break;
    }

    return dwError;
}

static
DWORD
VmDirRepositoryEnumUsers(
    PVMDIR_ENUM_HANDLE     pEnumHandle,
    DWORD                  dwMaxCount,
    PLSA_SECURITY_OBJECT** pppObjects,
    PDWORD                 pdwCount
    )
{
    DWORD dwError = 0;
    PSTR  pszAttrName_dn           = VMDIR_ATTR_NAME_DN;
    PSTR  pszAttrName_account      = VMDIR_ATTR_NAME_ACCOUNT;
    PSTR  pszAttrName_objectsid    = VMDIR_ATTR_NAME_OBJECTSID;
    PSTR  pszAttrName_uac          = VMDIR_ATTR_NAME_UAC;
    PSTR  pszAttrName_upn          = VMDIR_ATTR_NAME_UPN;
    PSTR  pszAttrName_first_name   = VMDIR_ATTR_NAME_FIRST_NAME;
    PSTR  pszAttrName_last_name    = VMDIR_ATTR_NAME_LAST_NAME;
    PSTR  pszAttrName_usn_changed  = VMDIR_ATTR_NAME_USN_CHANGED;
    PSTR  attrs[] =
    {
        pszAttrName_account,
        pszAttrName_objectsid,
        pszAttrName_uac,
        pszAttrName_upn,
        pszAttrName_first_name,
        pszAttrName_last_name,
        pszAttrName_usn_changed,
        NULL
    };
    PSTR   pszSamAcctName = NULL;
    PSTR   pszDN          = NULL;
    PSTR   pszObjectSid   = NULL;
    PSTR   pszUPN         = NULL;
    PSTR   pszFirstname   = NULL;
    PSTR   pszLastname    = NULL;
    DWORD  dwUserAccountControl = 0;
    LONG64 llUSNChanged = 0;
    VMDIR_ATTR values[]  =
    {
        {
            .pszName   = pszAttrName_account,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = FALSE,
            .dataRef =
            {
                .ppszData = &pszSamAcctName
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_dn,
            .type      = VMDIR_ATTR_TYPE_DN,
            .bOptional = FALSE,
            .dataRef =
            {
                .ppszData = &pszDN
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_objectsid,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = FALSE,
            .dataRef =
            {
                .ppszData = &pszObjectSid
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_first_name,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = TRUE,
            .dataRef =
            {
                .ppszData = &pszFirstname
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_last_name,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = TRUE,
            .dataRef =
            {
                .ppszData = &pszLastname
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_upn,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = TRUE,
            .dataRef =
            {
                .ppszData = &pszUPN
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_uac,
            .type      = VMDIR_ATTR_TYPE_UINT32,
            .bOptional = TRUE,
            .dataRef =
            {
                .pData_uint32 = &dwUserAccountControl
            },
            .size    = sizeof(dwUserAccountControl)
        },
        {
            .pszName   = pszAttrName_usn_changed,
            .type      = VMDIR_ATTR_TYPE_INT64,
            .bOptional = FALSE,
            .dataRef   =
            {
                .pData_int64 = &llUSNChanged
            },
            .size      = sizeof(llUSNChanged)
        }
    };
    DWORD  dwCount = 0;
    DWORD  iObject = 0;
    PSTR   pszFilter = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    if (pEnumHandle->pSearchResult && !pEnumHandle->dwRemaining)
    {
        VmDirLdapFreeMessage(pEnumHandle->pSearchResult);
        pEnumHandle->pSearchResult = NULL;
    }

    if (!pEnumHandle->pSearchResult)
    {
        dwError = LwAllocateStringPrintf(
                        &pszFilter,
                        "(&(objectclass=%s)(!(objectclass=%s))(%s>=%lld))",
                        VMDIR_OBJ_CLASS_USER,
                        VMDIR_OBJ_CLASS_COMPUTER,
                        VMDIR_ATTR_NAME_USN_CHANGED,
                        pEnumHandle->llLastUSNChanged+1);
        BAIL_ON_VMDIR_ERROR(dwError);

        dwError = VmDirLdapQueryObjects(
                        pEnumHandle->pDirContext->pLd,
                        pEnumHandle->pDirContext->pBindInfo->pszSearchBase,
                        LDAP_SCOPE_SUBTREE,
                        pszFilter,
                        attrs,
                        pEnumHandle->sizeLimit,
                        &pEnumHandle->pSearchResult);
        BAIL_ON_VMDIR_ERROR(dwError);

        pEnumHandle->dwIndex     = 0;
        pEnumHandle->dwRemaining = ldap_count_entries(
                                        pEnumHandle->pDirContext->pLd,
                                        pEnumHandle->pSearchResult);
    }

    if (!pEnumHandle->dwRemaining)
    {
        dwError = ERROR_NO_MORE_ITEMS;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    if (pEnumHandle->dwRemaining <= dwMaxCount)
    {
        dwCount = pEnumHandle->dwRemaining;
    }
    else
    {
        dwCount = dwMaxCount;
    }

    dwError = LwAllocateMemory(
                    sizeof(PLSA_SECURITY_OBJECT) * dwCount,
                    (PVOID*)&ppObjects);
    BAIL_ON_VMDIR_ERROR(dwError);

    for (iObject = 0; iObject < dwCount; iObject++)
    {
        LDAPMessage* pEntry = NULL;

        if (!pEnumHandle->dwIndex)
        {
            pEntry = ldap_first_entry(
                        pEnumHandle->pDirContext->pLd,
                        pEnumHandle->pSearchResult);
        }
        else
        {
            pEntry = ldap_next_entry(
                        pEnumHandle->pDirContext->pLd,
                        pEnumHandle->pCurrentEntry);
        }
        pEnumHandle->pCurrentEntry = pEntry;

        LW_SAFE_FREE_MEMORY(pszSamAcctName);
        LW_SAFE_FREE_MEMORY(pszDN);
        LW_SAFE_FREE_MEMORY(pszObjectSid);
        LW_SAFE_FREE_MEMORY(pszFirstname);
        LW_SAFE_FREE_MEMORY(pszLastname);
        LW_SAFE_FREE_MEMORY(pszUPN);

        dwError = VmDirLdapGetValues(
                        pEnumHandle->pDirContext->pLd,
                        pEntry,
                        &values[0],
                        sizeof(values)/sizeof(values[0]));
        BAIL_ON_VMDIR_ERROR(dwError);

        dwError = VmDirBuildUserObject(
                        pEnumHandle->pDirContext,
                        pszSamAcctName,
                        pszDN,
                        pszObjectSid,
                        pszUPN,
                        pszFirstname,
                        pszLastname,
                        dwUserAccountControl,
                        &ppObjects[iObject]);
        BAIL_ON_VMDIR_ERROR(dwError);

        pEnumHandle->dwIndex++;
        pEnumHandle->dwRemaining--;

        pEnumHandle->llLastUSNChanged = llUSNChanged;
    }

    *pppObjects = ppObjects;
    *pdwCount   = dwCount;

cleanup:

    LW_SAFE_FREE_MEMORY(pszSamAcctName);
    LW_SAFE_FREE_MEMORY(pszDN);
    LW_SAFE_FREE_MEMORY(pszObjectSid);
    LW_SAFE_FREE_MEMORY(pszFirstname);
    LW_SAFE_FREE_MEMORY(pszLastname);
    LW_SAFE_FREE_MEMORY(pszUPN);
    LW_SAFE_FREE_MEMORY(pszFilter);

    return dwError;

error:

    *pppObjects = NULL;
    *pdwCount   = 0;

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwCount, ppObjects);
    }

    goto cleanup;
}

static
DWORD
VmDirRepositoryEnumGroups(
    PVMDIR_ENUM_HANDLE     pEnumHandle,
    DWORD                  dwMaxCount,
    PLSA_SECURITY_OBJECT** pppObjects,
    PDWORD                 pdwCount
    )
{
    DWORD dwError = 0;
    PSTR  pszAttrName_dn           = VMDIR_ATTR_NAME_DN;
    PSTR  pszAttrName_account      = VMDIR_ATTR_NAME_ACCOUNT;
    PSTR  pszAttrName_objectsid    = VMDIR_ATTR_NAME_OBJECTSID;
    PSTR  pszAttrName_usn_changed  = VMDIR_ATTR_NAME_USN_CHANGED;
    PSTR  attrs[] =
    {
            pszAttrName_account,
            pszAttrName_objectsid,
            pszAttrName_usn_changed,
            NULL
    };
    PSTR   pszSamAcctName = NULL;
    PSTR   pszDN          = NULL;
    PSTR   pszObjectSid   = NULL;
    LONG64 llUSNChanged = 0;
    VMDIR_ATTR values[]  =
    {
        {
            .pszName   = pszAttrName_account,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = FALSE,
            .dataRef =
            {
                .ppszData = &pszSamAcctName
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_dn,
            .type      = VMDIR_ATTR_TYPE_DN,
            .bOptional = FALSE,
            .dataRef =
            {
                .ppszData = &pszDN
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_objectsid,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = FALSE,
            .dataRef =
            {
                .ppszData = &pszObjectSid
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_usn_changed,
            .type      = VMDIR_ATTR_TYPE_INT64,
            .bOptional = FALSE,
            .dataRef   =
            {
                .pData_int64 = &llUSNChanged
            },
            .size      = sizeof(llUSNChanged)
        }
    };
    DWORD  dwCount = 0;
    DWORD  iObject = 0;
    PSTR   pszFilter = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    if (pEnumHandle->pSearchResult && !pEnumHandle->dwRemaining)
    {
        VmDirLdapFreeMessage(pEnumHandle->pSearchResult);
        pEnumHandle->pSearchResult = NULL;
    }

    if (!pEnumHandle->pSearchResult)
    {
        dwError = LwAllocateStringPrintf(
                        &pszFilter,
                        "(&(objectclass=%s)(%s>=%lld))",
                        VMDIR_OBJ_CLASS_GROUP,
                        VMDIR_ATTR_NAME_USN_CHANGED,
                        pEnumHandle->llLastUSNChanged+1);
        BAIL_ON_VMDIR_ERROR(dwError);

        dwError = VmDirLdapQueryObjects(
                        pEnumHandle->pDirContext->pLd,
                        pEnumHandle->pDirContext->pBindInfo->pszSearchBase,
                        LDAP_SCOPE_SUBTREE,
                        pszFilter,
                        attrs,
                        pEnumHandle->sizeLimit,
                        &pEnumHandle->pSearchResult);
        BAIL_ON_VMDIR_ERROR(dwError);

        pEnumHandle->dwIndex     = 0;
        pEnumHandle->dwRemaining = ldap_count_entries(
                                        pEnumHandle->pDirContext->pLd,
                                        pEnumHandle->pSearchResult);
    }

    if (!pEnumHandle->dwRemaining)
    {
        dwError = ERROR_NO_MORE_ITEMS;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    if (pEnumHandle->dwRemaining <= dwMaxCount)
    {
        dwCount = pEnumHandle->dwRemaining;
    }
    else
    {
        dwCount = dwMaxCount;
    }

    dwError = LwAllocateMemory(
                    sizeof(PLSA_SECURITY_OBJECT) * dwCount,
                    (PVOID*)&ppObjects);
    BAIL_ON_VMDIR_ERROR(dwError);

    for (iObject = 0; iObject < dwCount; iObject++)
    {
        LDAPMessage* pEntry = NULL;

        if (!pEnumHandle->dwIndex)
        {
            pEntry = ldap_first_entry(
                        pEnumHandle->pDirContext->pLd,
                        pEnumHandle->pSearchResult);
        }
        else
        {
            pEntry = ldap_next_entry(
                        pEnumHandle->pDirContext->pLd,
                        pEnumHandle->pCurrentEntry);
        }
        pEnumHandle->pCurrentEntry = pEntry;

        LW_SAFE_FREE_MEMORY(pszSamAcctName);
        LW_SAFE_FREE_MEMORY(pszDN);
        LW_SAFE_FREE_MEMORY(pszObjectSid);

        dwError = VmDirLdapGetValues(
                        pEnumHandle->pDirContext->pLd,
                        pEntry,
                        &values[0],
                        sizeof(values)/sizeof(values[0]));
        BAIL_ON_VMDIR_ERROR(dwError);

        dwError = VmDirBuildGroupObject(
                        pEnumHandle->pDirContext,
                        pszSamAcctName,
                        pszDN,
                        pszObjectSid,
                        &ppObjects[iObject]);
        BAIL_ON_VMDIR_ERROR(dwError);

        pEnumHandle->dwIndex++;
        pEnumHandle->dwRemaining--;

        pEnumHandle->llLastUSNChanged = llUSNChanged;
    }

    *pppObjects = ppObjects;
    *pdwCount   = dwCount;

cleanup:

    LW_SAFE_FREE_MEMORY(pszSamAcctName);
    LW_SAFE_FREE_MEMORY(pszDN);
    LW_SAFE_FREE_MEMORY(pszObjectSid);
    LW_SAFE_FREE_MEMORY(pszFilter);

    return dwError;

error:

    *pppObjects = NULL;
    *pdwCount   = 0;

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwCount, ppObjects);
    }

    goto cleanup;
}

DWORD
VmDirInitEnumMembersHandle(
    PVMDIR_DIR_CONTEXT  pDirContext,
    PCSTR               pszSid,
    PVMDIR_ENUM_HANDLE* ppEnumHandle
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszAttrName_member = VMDIR_ATTR_NAME_MEMBER;
    PSTR  attrs[] =
    {
            pszAttrName_member,
            NULL
    };
    PSTR* ppszDNArray = NULL;
    DWORD dwDNCount = 0;
    VMDIR_ATTR values[]  =
    {
        {
            .pszName   = pszAttrName_member,
            .type      = VMDIR_ATTR_TYPE_MULTI_STRING,
            .bOptional = TRUE,
            .dataRef =
            {
                .pppszStrArray = &ppszDNArray
            },
            .size    = -1,
            .pdwCount = &dwDNCount
        }
    };
    PSTR               pszFilter = NULL;
    LDAPMessage*       pSearchResult = NULL;
    PVMDIR_ENUM_HANDLE pEnumHandle = NULL;

    dwError = LwAllocateStringPrintf(
                        &pszFilter,
                        "(&(objectclass=%s)(%s=%s))",
                        VMDIR_OBJ_CLASS_GROUP,
                        VMDIR_ATTR_NAME_OBJECTSID,
                        pszSid);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirLdapQuerySingleObject(
                    pDirContext->pLd,
                    pDirContext->pBindInfo->pszSearchBase,
                    LDAP_SCOPE_SUBTREE,
                    pszFilter,
                    &attrs[0],
                    &pSearchResult);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirLdapGetValues(
                    pDirContext->pLd,
                    pSearchResult,
                    &values[0],
                    sizeof(values)/sizeof(values[0]));
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pEnumHandle), (PVOID*)&pEnumHandle);
    BAIL_ON_VMDIR_ERROR(dwError);

    pEnumHandle->type             = VMDIR_ENUM_HANDLE_TYPE_MEMBERS;
    pEnumHandle->pDirContext      = pDirContext;
    pEnumHandle->sizeLimit        = 0;
    pEnumHandle->llLastUSNChanged = 0;
    pEnumHandle->dwIndex          = 0;
    pEnumHandle->ppszDNArray      = ppszDNArray;
    ppszDNArray = NULL;
    pEnumHandle->dwDNCount        = dwDNCount;
    pEnumHandle->dwRemaining      = dwDNCount;

    *ppEnumHandle = pEnumHandle;

cleanup:

    if (pSearchResult)
    {
        VmDirLdapFreeMessage(pSearchResult);
    }
    LW_SAFE_FREE_MEMORY(pszFilter);

    return dwError;

error:

    if (pEnumHandle)
    {
        VmDirCloseEnum(pEnumHandle);
    }

    if (ppszDNArray)
    {
        LwFreeStringArray(ppszDNArray, dwDNCount);
    }

    goto cleanup;
}

DWORD
VmDirRepositoryEnumMembers(
    PVMDIR_ENUM_HANDLE pEnumHandle,
    DWORD              dwMaxCount,
    PSTR**             pppszMemberSids,
    PDWORD             pdwCount
    )
{
    DWORD dwError = 0;
    PSTR* ppszMemberSids = NULL;
    DWORD dwCount = 0;
    DWORD iMember = 0;
    LDAPMessage* pSearchResult = NULL;

    if (!pEnumHandle->dwRemaining)
    {
        dwError = ERROR_NO_MORE_ITEMS;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    if (pEnumHandle->dwRemaining <= dwMaxCount)
    {
        dwCount = pEnumHandle->dwRemaining;
    }
    else
    {
        dwCount = dwMaxCount;
    }

    dwError = LwAllocateMemory(sizeof(PSTR*) * dwCount, (PVOID*)&ppszMemberSids);
    BAIL_ON_VMDIR_ERROR(dwError);

    for (; iMember < dwCount; iMember++)
    {
        dwError = VmDirFindSidForDN(
                        pEnumHandle->pDirContext,
                        pEnumHandle->ppszDNArray[pEnumHandle->dwIndex++],
                        &ppszMemberSids[iMember]);
        BAIL_ON_VMDIR_ERROR(dwError);

        pEnumHandle->dwRemaining--;
    }

    *pppszMemberSids = ppszMemberSids;
    *pdwCount = dwCount;

cleanup:

    if (pSearchResult)
    {
        VmDirLdapFreeMessage(pSearchResult);
    }

    return dwError;

error:

    *pppszMemberSids = NULL;
    *pdwCount = 0;

    if (ppszMemberSids)
    {
        LwFreeStringArray(ppszMemberSids, dwCount);
    }

    goto cleanup;
}

DWORD
VmDirFindMemberships(
    PVMDIR_DIR_CONTEXT pDirContext,
    PCSTR              pszSid,
    PLW_HASH_TABLE     pGroupSidTable
    )
{
    DWORD dwError = 0;
    PSTR  pszAttrName_member_of = VMDIR_ATTR_NAME_MEMBER_OF;
    PSTR  attrs[] =
    {
        pszAttrName_member_of,
        NULL
    };
    PSTR* ppszDNArray = NULL;
    DWORD dwDNCount = 0;
    VMDIR_ATTR values[]  =
    {
        {
            .pszName   = pszAttrName_member_of,
            .type      = VMDIR_ATTR_TYPE_MULTI_STRING,
            .bOptional = TRUE,
            .dataRef =
            {
                .pppszStrArray = &ppszDNArray
            },
            .size    = -1,
            .pdwCount = &dwDNCount
        }
    };
    PSTR         pszFilter = NULL;
    LDAPMessage* pSearchResult = NULL;
    PSTR  pszSid2 = NULL;
    DWORD iDN = 0;

    dwError = LwAllocateStringPrintf(
                    &pszFilter,
                    "(&(|(objectclass=%s)(objectclass=%s))(%s=%s))",
                    VMDIR_OBJ_CLASS_USER,
                    VMDIR_OBJ_CLASS_GROUP,
                    VMDIR_ATTR_NAME_OBJECTSID,
                    pszSid);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirLdapQuerySingleObject(
                    pDirContext->pLd,
                    pDirContext->pBindInfo->pszSearchBase,
                    LDAP_SCOPE_SUBTREE,
                    pszFilter,
                    &attrs[0],
                    &pSearchResult);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirLdapGetValues(
                    pDirContext->pLd,
                    pSearchResult,
                    &values[0],
                    sizeof(values)/sizeof(values[0]));
    BAIL_ON_VMDIR_ERROR(dwError);

    for (; iDN < dwDNCount; iDN++)
    {
        PSTR pszExistingSid = NULL;
        PSTR pszDN = ppszDNArray[iDN];

        LW_SAFE_FREE_STRING(pszSid2);

        dwError = VmDirFindSidForDN(pDirContext, pszDN, &pszSid2);
        BAIL_ON_VMDIR_ERROR(dwError);

        dwError = LwHashGetValue(
                        pGroupSidTable,
                        pszSid2,
                        (PVOID*)&pszExistingSid);

        if (dwError == ERROR_NOT_FOUND)
        {
            dwError = LwHashSetValue(
                        pGroupSidTable,
                        pszSid2,
                        pszSid2);
            BAIL_ON_VMDIR_ERROR(dwError);

            pszSid2 = NULL;
        }
    }

cleanup:

    if (pSearchResult)
    {
        VmDirLdapFreeMessage(pSearchResult);
    }

    LW_SAFE_FREE_STRING(pszFilter);
    LW_SAFE_FREE_STRING(pszSid2);

    if (ppszDNArray)
    {
        LwFreeStringArray(ppszDNArray, dwDNCount);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
VmDirFindSidForDN(
    PVMDIR_DIR_CONTEXT pDirContext,
    PCSTR              pszDN,
    PSTR*              ppszSid
    )
{
    DWORD dwError = 0;
    PSTR pszAttrName_object_sid = VMDIR_ATTR_NAME_OBJECTSID;
    PSTR pszSid = NULL;
    PSTR attrs[] =
    {
            pszAttrName_object_sid,
            NULL
    };
    VMDIR_ATTR values[] =
    {
        {
            .pszName   = pszAttrName_object_sid,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = FALSE,
            .dataRef =
            {
                .ppszData = &pszSid
            },
            .size    = -1,
            .pdwCount = NULL
        }
    };
    PSTR pszFilter = "(objectclass=*)";
    LDAPMessage* pSearchResult = NULL;

    dwError = VmDirLdapQuerySingleObject(
                    pDirContext->pLd,
                    pszDN,
                    LDAP_SCOPE_BASE,
                    pszFilter,
                    &attrs[0],
                    &pSearchResult);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirLdapGetValues(
                    pDirContext->pLd,
                    pSearchResult,
                    &values[0],
                    sizeof(values)/sizeof(values[0]));
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppszSid = pszSid;

cleanup:

    if (pSearchResult)
    {
        VmDirLdapFreeMessage(pSearchResult);
        pSearchResult = NULL;
    }

    return dwError;

error:

    *ppszSid = NULL;

    LW_SAFE_FREE_STRING(pszSid);

    goto cleanup;
}

DWORD
VmDirRepositoryVerifyPassword(
    PVMDIR_DIR_CONTEXT pDirContext,
    PCSTR              pszUPN,
    PCSTR              pszPassword
    )
{
    DWORD dwError = 0;
    LDAP* pLd = NULL;
    
    dwError = VmDirLdapInitialize(
                    pDirContext->pBindInfo->pszURI,
                    pszUPN,
                    pszPassword,
                    NULL, /* cache path */
                    &pLd);
    BAIL_ON_VMDIR_ERROR(dwError);
    
cleanup:
    
    if (pLd)
    {
        VmDirLdapClose(pLd);
    }
    
    return dwError;
    
error:
    
    // TODO : Differentiate error codes
    
    dwError = LW_ERROR_PASSWORD_MISMATCH;
    
    goto cleanup;
}

DWORD
VmDirRepositoryChangePassword(
    PVMDIR_DIR_CONTEXT pDirContext,
    PCSTR              pszUPN,
    PCSTR              pszNewPassword,
    PCSTR              pszOldPassword
    )
{
    DWORD    dwError = 0;
    LDAP*    pLd     = NULL;
    PSTR     vals_new[2] = {(PSTR)pszNewPassword, NULL};
    PSTR     vals_old[2] = {(PSTR)pszOldPassword, NULL};
    LDAPMod  mod[2]  = {{0}};
    LDAPMod* mods[3] = {&mod[0], &mod[1], NULL};
    PSTR     pszCachePath = NULL;
        PLSA_SECURITY_OBJECT pObject = NULL;

        dwError = VmDirFindUserByName(
                      pDirContext,
                          pszUPN,
                          &pObject);
        BAIL_ON_VMDIR_ERROR(dwError);

        dwError = LwKrb5GetUserCachePath(
                        pObject->userInfo.uid,
                        KRB5_File_Cache,
                        &pszCachePath);
        BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirLdapInitialize(
              pDirContext->pBindInfo->pszURI,
              pszUPN,
              pszOldPassword,
              pszCachePath,
              &pLd);
    BAIL_ON_VMDIR_ERROR(dwError);

    mod[0].mod_op = LDAP_MOD_ADD;
    mod[0].mod_type = VMDIR_ATTR_USER_PASSWORD;
    mod[0].mod_vals.modv_strvals = vals_new;

    mod[1].mod_op = LDAP_MOD_DELETE;
    mod[1].mod_type = VMDIR_ATTR_USER_PASSWORD;
    mod[1].mod_vals.modv_strvals = vals_old;

    dwError = ldap_modify_ext_s(
                            pLd,
                            pObject->pszDN,
                            mods,
                            NULL,
                            NULL);
    BAIL_ON_VMDIR_ERROR(dwError);

cleanup:

        if (pObject)
        {
            LsaUtilFreeSecurityObject(pObject);
        }
    if (pLd)
    {
        VmDirLdapClose(pLd);
    }
    LW_SAFE_FREE_STRING(pszCachePath);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
VmDirFindUserObject(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszFilter,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszDomain  = NULL;
    PSTR  pszAccount = NULL;
    PSTR  pszAttrName_dn           = VMDIR_ATTR_NAME_DN;
    PSTR  pszAttrName_account      = VMDIR_ATTR_NAME_ACCOUNT;
    PSTR  pszAttrName_objectsid    = VMDIR_ATTR_NAME_OBJECTSID;
    PSTR  pszAttrName_uac          = VMDIR_ATTR_NAME_UAC;
    PSTR  pszAttrName_upn          = VMDIR_ATTR_NAME_UPN;
    PSTR  pszAttrName_first_name   = VMDIR_ATTR_NAME_FIRST_NAME;
    PSTR  pszAttrName_last_name    = VMDIR_ATTR_NAME_LAST_NAME;
    PSTR  attrs[] =
    {
            pszAttrName_account,
            pszAttrName_objectsid,
            pszAttrName_uac,
            pszAttrName_upn,
            pszAttrName_first_name,
            pszAttrName_last_name,
            NULL
    };
    PSTR  pszSamAcctName = NULL;
    PSTR  pszDN          = NULL;
    PSTR  pszObjectSid   = NULL;
    PSTR  pszUPN         = NULL;
    PSTR  pszFirstname   = NULL;
    PSTR  pszLastname    = NULL;
    DWORD dwUserAccountControl = 0;
    VMDIR_ATTR values[]  =
    {
        {
            .pszName   = pszAttrName_account,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = FALSE,
            .dataRef =
            {
                .ppszData = &pszSamAcctName
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_dn,
            .type      = VMDIR_ATTR_TYPE_DN,
            .bOptional = FALSE,
            .dataRef =
            {
                .ppszData = &pszDN
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_objectsid,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = FALSE,
            .dataRef =
            {
                .ppszData = &pszObjectSid
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_first_name,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = TRUE,
            .dataRef =
            {
                .ppszData = &pszFirstname
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_last_name,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = TRUE,
            .dataRef =
            {
                .ppszData = &pszLastname
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_upn,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = TRUE,
            .dataRef =
            {
                .ppszData = &pszUPN
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_uac,
            .type      = VMDIR_ATTR_TYPE_UINT32,
            .bOptional = TRUE,
            .dataRef =
            {
                .pData_uint32 = &dwUserAccountControl
            },
            .size    = sizeof(dwUserAccountControl)
        }
    };
    LDAPMessage* pLdapResult = NULL;
    PLSA_SECURITY_OBJECT pObject  = NULL;

    dwError = VmDirLdapQuerySingleObject(
                    pDirContext->pLd,
                    pDirContext->pBindInfo->pszSearchBase,
                    LDAP_SCOPE_SUBTREE,
                    pszFilter,
                    &attrs[0],
                    &pLdapResult);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirLdapGetValues(
                    pDirContext->pLd,
                    pLdapResult,
                    &values[0],
                    sizeof(values)/sizeof(values[0]));
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirBuildUserObject(
                    pDirContext,
                    pszSamAcctName,
                    pszDN,
                    pszObjectSid,
                    pszUPN,
                    pszFirstname,
                    pszLastname,
                    dwUserAccountControl,
                    &pObject);
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppObject = pObject;

cleanup:

    if (pLdapResult)
    {
        VmDirLdapFreeMessage(pLdapResult);
    }

    LW_SAFE_FREE_MEMORY(pszDomain);
    LW_SAFE_FREE_MEMORY(pszAccount);
    LW_SAFE_FREE_MEMORY(pszSamAcctName);
    LW_SAFE_FREE_MEMORY(pszDN);
    LW_SAFE_FREE_MEMORY(pszObjectSid);
    LW_SAFE_FREE_MEMORY(pszFirstname);
    LW_SAFE_FREE_MEMORY(pszLastname);
    LW_SAFE_FREE_MEMORY(pszUPN);

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    goto cleanup;
}

static
DWORD
VmDirFindGroupObject(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszFilter,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszAttrName_account      = VMDIR_ATTR_NAME_ACCOUNT;
    PSTR  pszAttrName_dn           = VMDIR_ATTR_NAME_DN;
    PSTR  pszAttrName_objectsid    = VMDIR_ATTR_NAME_OBJECTSID;
    PSTR  attrs[] =
    {
            pszAttrName_account,
            pszAttrName_objectsid,
            NULL
    };
    PSTR  pszSamAcctName = NULL;
    PSTR  pszDN          = NULL;
    PSTR  pszObjectSid   = NULL;
    VMDIR_ATTR values[]  =
    {
        {
            .pszName   = pszAttrName_account,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = FALSE,
            .dataRef =
            {
                .ppszData = &pszSamAcctName
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_dn,
            .type      = VMDIR_ATTR_TYPE_DN,
            .bOptional = FALSE,
            .dataRef =
            {
                .ppszData = &pszDN
            },
            .size    = -1
        },
        {
            .pszName   = pszAttrName_objectsid,
            .type      = VMDIR_ATTR_TYPE_STRING,
            .bOptional = FALSE,
            .dataRef =
            {
                .ppszData = &pszObjectSid
            },
            .size    = -1
        }
    };
    LDAPMessage* pLdapResult = NULL;
    PLSA_SECURITY_OBJECT pObject  = NULL;

    dwError = VmDirLdapQuerySingleObject(
                    pDirContext->pLd,
                    pDirContext->pBindInfo->pszSearchBase,
                    LDAP_SCOPE_SUBTREE,
                    pszFilter,
                    &attrs[0],
                    &pLdapResult);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirLdapGetValues(
                    pDirContext->pLd,
                    pLdapResult,
                    &values[0],
                    sizeof(values)/sizeof(values[0]));
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirBuildGroupObject(
                    pDirContext,
                    pszSamAcctName,
                    pszDN,
                    pszObjectSid,
                    &pObject);
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppObject = pObject;

cleanup:

    if (pLdapResult)
    {
        VmDirLdapFreeMessage(pLdapResult);
    }

    LW_SAFE_FREE_MEMORY(pszSamAcctName);
    LW_SAFE_FREE_MEMORY(pszDN);
    LW_SAFE_FREE_MEMORY(pszObjectSid);

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    goto cleanup;
}

static
DWORD
VmDirBuildUserObject(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszSamAcctName,
    PCSTR                 pszDN,
    PCSTR                 pszObjectSid,
    PCSTR                 pszUPN,
    PCSTR                 pszFirstname,
    PCSTR                 pszLastname,
    DWORD                  dwUserAccountControl,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pGroupObject = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;

    dwError = LwAllocateMemory(sizeof(*pObject), (PVOID*)&pObject);
    BAIL_ON_VMDIR_ERROR(dwError);

    pObject->type = LSA_OBJECT_TYPE_USER;

    dwError = LwAllocateString(pszDN, &pObject->pszDN);
    BAIL_ON_VMDIR_ERROR(dwError);

    pObject->version.qwDbId = -1;
    pObject->version.fWeight = 0;
    pObject->version.dwObjectSize = 0;
    pObject->version.tLastUpdated = 0;

    dwError = LwAllocateString(
                    pszSamAcctName,
                    &pObject->pszSamAccountName);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pObject->userInfo.pszUnixName,
                    "%s\\%s",
                    pDirContext->pBindInfo->pszDomainShort,
                    pszSamAcctName);
    BAIL_ON_VMDIR_ERROR(dwError);

    pObject->userInfo.pszPasswd = NULL; // Never give out the password

    if (!IsNullOrEmptyString(pszUPN))
    {
        dwError = LwAllocateString(
                        pszUPN,
                        &pObject->userInfo.pszUPN);
        BAIL_ON_VMDIR_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateStringPrintf(
                        &pObject->userInfo.pszUPN,
                        "%s@%s",
                        pszSamAcctName,
                        pDirContext->pBindInfo->pszDomainFqdn);
        BAIL_ON_VMDIR_ERROR(dwError);

        pObject->userInfo.bIsGeneratedUPN = TRUE;
    }

    dwError = LwAllocateString(
                            VMDIR_USER_SHELL,
                            &pObject->userInfo.pszShell);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                            &pObject->userInfo.pszHomedir,
                            "/home/%s",
                            pszSamAcctName);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                            &pObject->userInfo.pszGecos,
                            "%s%s%s",
                            pszFirstname ? pszFirstname : "",
                            pszFirstname && pszLastname ? " " : "",
                            pszLastname ? pszLastname : "");
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirFindGroupByName(
                        pDirContext,
                        VMDIR_DEFAULT_PRIMARY_GROUP_NAME,
                        &pGroupObject);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateString(
                        pGroupObject->pszObjectSid,
                        &pObject->userInfo.pszPrimaryGroupSid);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateString(pszObjectSid, &pObject->pszObjectSid);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirGetRID(pszObjectSid, &pObject->userInfo.uid);
    BAIL_ON_VMDIR_ERROR(dwError);

    pObject->userInfo.gid = pGroupObject->groupInfo.gid;

#if 0
    pObject->userInfo.bPasswordNeverExpires =
            ((dwUserAccountControl & VMDIR_UAC_FLAG_PWD_NEVER_EXPIRES) != 0);
    if (!pObject->userInfo.bPasswordNeverExpires)
    {
        pObject->userInfo.bPasswordExpired =
                    ((dwUserAccountControl & VMDIR_UAC_FLAG_PWD_EXPIRED) != 0);
    }
    else
    {
        pObject->userInfo.bPasswordExpired = FALSE;
    }
    pObject->userInfo.bAccountDisabled =
            ((dwUserAccountControl & VMDIR_UAC_FLAG_ACCT_DISABLED) != 0);
    pObject->userInfo.bAccountExpired  = FALSE; // TODO
    pObject->userInfo.bAccountLocked   =
            ((dwUserAccountControl & VMDIR_UAC_FLAG_ACCT_LOCKED) != 0);
    pObject->userInfo.bUserCanChangePassword =
            ((dwUserAccountControl & VMDIR_UAC_FLAG_ALLOW_PWD_CHANGE) != 0);
#else
    pObject->userInfo.bPasswordNeverExpires  = TRUE;
    pObject->userInfo.bPasswordExpired       = FALSE;
    pObject->userInfo.bAccountDisabled       = FALSE;
    pObject->userInfo.bAccountExpired        = FALSE;
    pObject->userInfo.bAccountLocked         = FALSE;
    pObject->userInfo.bUserCanChangePassword = TRUE;
#endif

    pObject->enabled = TRUE;

    *ppObject = pObject;

cleanup:

    if (pGroupObject)
    {
        LsaUtilFreeSecurityObject(pGroupObject);
    }

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    goto cleanup;
}

static
DWORD
VmDirBuildGroupObject(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszSamAcctName,
    PCSTR                 pszDN,
    PCSTR                 pszObjectSid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pObject = NULL;

    dwError = LwAllocateMemory(sizeof(*pObject), (PVOID*)&pObject);
    BAIL_ON_VMDIR_ERROR(dwError);

    pObject->type = LSA_OBJECT_TYPE_GROUP;

    dwError = LwAllocateString(pszDN, &pObject->pszDN);
    BAIL_ON_VMDIR_ERROR(dwError);

    pObject->version.qwDbId = -1;
    pObject->version.fWeight = 0;
    pObject->version.dwObjectSize = 0;
    pObject->version.tLastUpdated = 0;

    dwError = LwAllocateString(
                    pszSamAcctName,
                    &pObject->pszSamAccountName);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pObject->groupInfo.pszUnixName,
                    "%s\\%s",
                    pDirContext->pBindInfo->pszDomainShort,
                    pszSamAcctName);
    BAIL_ON_VMDIR_ERROR(dwError);

    pObject->groupInfo.pszPasswd = NULL; // Never give out the password

    dwError = LwAllocateString(pszObjectSid, &pObject->pszObjectSid);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirGetRID(pszObjectSid, &pObject->groupInfo.gid);
    BAIL_ON_VMDIR_ERROR(dwError);

    pObject->enabled = TRUE;

    *ppObject = pObject;

cleanup:

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    goto cleanup;
}
