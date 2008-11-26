/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        batch_gather.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#include "adprovider.h"
#include "batch_gather.h"

static
DWORD
LsaAdBatchGatherObjectType(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType
    )
{
    DWORD dwError = 0;

    // Check that the object type is not changing under us.
    if ((ObjectType != LSA_AD_BATCH_OBJECT_TYPE_USER) &&
        (ObjectType != LSA_AD_BATCH_OBJECT_TYPE_GROUP))
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!pItem->ObjectType)
    {
        pItem->ObjectType = ObjectType;
    }
    else if (pItem->ObjectType != ObjectType)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaAdBatchGatherRpcObject(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN OUT PSTR* ppszSid,
    IN OUT PSTR* ppszSamAccountName
    )
{
    DWORD dwError = 0;

    SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_REAL);

    dwError = LsaAdBatchGatherObjectType(pItem, ObjectType);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_XFER_STRING(*ppszSid, pItem->pszSid);
    LSA_XFER_STRING(*ppszSamAccountName, pItem->pszSamAccountName);

    if (LSA_AD_BATCH_OBJECT_TYPE_USER == ObjectType)
    {
        pItem->UserInfo.dwPrimaryGroupRid = WELLKNOWN_SID_DOMAIN_USER_GROUP_RID;
        XXX; // verify that we do not need an LsaBatchGatherRpcUser()...
    }

cleanup:
    return dwError;

error:
    SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_ERROR);
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherSchemaModeUser(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    DWORD dwValue = 0;

    dwError = LsaLdapGetUInt32(
                    hDirectory,
                    pMessage,
                    AD_LDAP_UID_TAG,
                    &dwValue);
    if (LSA_ERROR_INVALID_LDAP_ATTR_VALUE == dwError)
    {
        SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED);
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED))
    {
        goto cleanup;
    }

    if (!dwValue)
    {
        LSA_LOG_DEBUG("uid must be non-zero for SID '%s'", pItem->pszSid);
        // SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_SKIP);
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pItem->UserInfo.uid = (uid_t)dwValue;

    dwError = LsaLdapGetUInt32(
                    hDirectory,
                    pMessage,
                    AD_LDAP_GID_TAG,
                    &dwValue);
    BAIL_ON_LSA_ERROR(dwError);

    if (!dwValue)
    {
        LSA_LOG_DEBUG("gid must be non-zero for SID '%s'", pItem->pszSid);
        // SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_SKIP);
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pItem->UserInfo.gid = (gid_t)dwValue;

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_ALIAS_TAG,
                    &pItem->UserInfo.pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_PASSWD_TAG,
                    &pItem->UserInfo.pszPasswd);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_GECOS_TAG,
                    &pItem->UserInfo.pszGecos);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_HOMEDIR_TAG,
                    &pItem->UserInfo.pszHomeDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_SHELL_TAG,
                    &pItem->UserInfo.pszShell);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherSchemaModeGroup(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    DWORD dwValue = 0;

    dwError = LsaLdapGetUInt32(
                    hDirectory,
                    pMessage,
                    AD_LDAP_GID_TAG,
                    &dwValue);
    if (LSA_ERROR_INVALID_LDAP_ATTR_VALUE == dwError)
    {
        SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED);
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED))
    {
        goto cleanup;
    }

    if (!dwValue)
    {
        LSA_LOG_DEBUG("gid must be non-zero for SID '%s'", pItem->pszSid);
        // SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_SKIP);
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pItem->GroupInfo.gid = (gid_t)dwValue;

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_ALIAS_TAG,
                    &pItem->GroupInfo.pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_PASSWD_TAG,
                    &pItem->GroupInfo.pszPasswd);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaAdBatchGatherSchemaMode(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;

    switch (pItem->ObjectType)
    {
        case LSA_AD_BATCH_OBJECT_TYPE_USER:
            dwError = LsaAdBatchGatherSchemaModeUser(
                            pItem,
                            hDirectory,
                            pMessage);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
            dwError = LsaAdBatchGatherSchemaModeGroup(
                            pItem,
                            hDirectory,
                            pMessage);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherNonSchemaModeUser(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN DWORD dwKeywordValuesCount,
    IN PSTR* ppszKeywordValues
    )
{
    DWORD dwError = 0;
    DWORD dwValue = 0;

    dwError = ADNonSchemaKeywordGetUInt32(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_UID_TAG,
                    &dwValue);
    if (LSA_ERROR_INVALID_LDAP_ATTR_VALUE == dwError)
    {
        SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED);
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED))
    {
        goto cleanup;
    }

    if (!dwValue)
    {
        LSA_LOG_DEBUG("uid must be non-zero for SID '%s'", pItem->pszSid);
        // SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_SKIP);
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pItem->UserInfo.uid = (uid_t)dwValue;

    dwError = ADNonSchemaKeywordGetUInt32(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_GID_TAG,
                    &dwValue);
    BAIL_ON_LSA_ERROR(dwError);

    if (!dwValue)
    {
        LSA_LOG_DEBUG("gid must be non-zero for SID '%s'", pItem->pszSid);
        // SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_SKIP);
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pItem->UserInfo.gid = (gid_t)dwValue;

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_ALIAS_TAG,
                    &pItem->UserInfo.pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_PASSWD_TAG,
                    &pItem->UserInfo.pszPasswd);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_GECOS_TAG,
                    &pItem->UserInfo.pszGecos);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_HOMEDIR_TAG,
                    &pItem->UserInfo.pszHomeDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_SHELL_TAG,
                    &pItem->UserInfo.pszShell);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherNonSchemaModeGroup(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN DWORD dwKeywordValuesCount,
    IN PSTR* ppszKeywordValues
    )
{
    DWORD dwError = 0;
    DWORD dwValue = 0;

    dwError = ADNonSchemaKeywordGetUInt32(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_GID_TAG,
                    &dwValue);
    if (LSA_ERROR_INVALID_LDAP_ATTR_VALUE == dwError)
    {
        SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED);
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED))
    {
        goto cleanup;
    }

    if (!dwValue)
    {
        LSA_LOG_DEBUG("gid must be non-zero for SID '%s'", pItem->pszSid);
        // SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_SKIP);
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pItem->GroupInfo.gid = (gid_t)dwValue;

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_DISPLAY_NAME_TAG,
                    &pItem->GroupInfo.pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_PASSWD_TAG,
                    &pItem->GroupInfo.pszPasswd);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaAdBatchGatherNonSchemaMode(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN DWORD dwKeywordValuesCount,
    IN PSTR* ppszKeywordValues
    )
{
    DWORD dwError = 0;

    switch (pItem->ObjectType)
    {
        case LSA_AD_BATCH_OBJECT_TYPE_USER:
            dwError = LsaAdBatchGatherNonSchemaModeUser(
                            pItem,
                            dwKeywordValuesCount,
                            ppszKeywordValues);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
            dwError = LsaAdBatchGatherNonSchemaModeGroup(
                            pItem,
                            dwKeywordValuesCount,
                            ppszKeywordValues);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherUnprovisionedModeUser(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;

    // Need the primary group RID to marshal gid later.
    dwError = LsaLdapGetUInt32(
                hDirectory,
                pMessage,
                AD_LDAP_PRIMEGID_TAG,
                &pItem->UserInfo.dwPrimaryGroupRid);
    BAIL_ON_LSA_ERROR(dwError);
#if 0
    if (dwError != 0)
    {
        LSA_LOG_ERROR("Failed to get primary group ID for SID '%s'",
                      pItem->pszSid);
        pItem->UserInfo.dwPrimaryGroupRid = WELLKNOWN_SID_DOMAIN_USER_GROUP_RID;
        dwError = 0;
    }
#endif

    // Use display name for user gecos.
    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_DISPLAY_NAME_TAG,
                    &pItem->UserInfo.pszGecos);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherUnprovisionedMode(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;

    switch (pItem->ObjectType)
    {
        case LSA_AD_BATCH_OBJECT_TYPE_USER:
            dwError = LsaAdBatchGatherUnprovisionedModeUser(
                            pItem,
                            hDirectory,
                            pMessage);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
            // Nothing special for groups.
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherRealUser(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;

    LSA_ASSERT(!pItem->UserInfo.pszUserPrincipalName);

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_UPN_TAG,
                    &pItem->UserInfo.pszUserPrincipalName);
    BAIL_ON_LSA_ERROR(dwError);

    XXX; // Get rid of ADGetLDAPUPNString or refactor it and use refactor...
    if (pItem->UserInfo.pszUserPrincipalName)
    {
        if (!index(pItem->UserInfo.pszUserPrincipalName, '@'))
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        // Do not touch the non-realm part, just the realm part
        // to make sure the realm conforms to spec.
        LsaPrincipalRealmToUpper(pItem->UserInfo.pszUserPrincipalName);
    }

    dwError = LsaLdapGetUInt32(
                    hDirectory,
                    pMessage,
                    AD_LDAP_USER_CTRL_TAG,
                    &pItem->UserInfo.UserAccountControl);
    if (dwError == LSA_ERROR_INVALID_LDAP_ATTR_VALUE)
    {
        LSA_LOG_ERROR(
                "User %s has an invalid value for the userAccountControl"
                " attribute. Please check that it is set and that the "
                "machine account has permission to read it.",
                pItem->pszSid);
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetUInt64(
                    hDirectory,
                    pMessage,
                    AD_LDAP_ACCOUT_EXP_TAG,
                    &pItem->UserInfo.AccountExpires);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetUInt64(
                    hDirectory,
                    pMessage,
                    AD_LDAP_PWD_LASTSET_TAG,
                    &pItem->UserInfo.PasswordLastSet);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaAdBatchGatherRealObject(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN OUT OPTIONAL PSTR* ppszSid,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;

    SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_REAL);

    dwError = LsaAdBatchGatherObjectType(pItem, ObjectType);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pItem->pszSid)
    {
        if (ppszSid)
        {
            LSA_XFER_STRING(*ppszSid, pItem->pszSid);
        }
        else
        {
            dwError = ADLdap_GetObjectSid(hDirectory, pMessage, &pItem->pszSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (IsNullOrEmptyString(pItem->pszSid))
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    LSA_ASSERT(!pItem->pszSamAccountName);

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_SAM_NAME_TAG,
                    &pItem->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);
    if (IsNullOrEmptyString(pItem->pszSamAccountName))
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    LSA_ASSERT(!pItem->pszDn);

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_DN_TAG,
                    &pItem->pszDn);
    BAIL_ON_LSA_ERROR(dwError);
    if (IsNullOrEmptyString(pItem->pszDn))
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Handle cases where real contains pseudo.
    if (LsaAdBatchIsDefaultSchemaMode())
    {
        // But only if we are not being called by a pseudo
        // lookup for default schema mode.
        if (!IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO))
        {
            SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO);
            dwError = LsaAdBatchGatherSchemaMode(
                            pItem,
                            hDirectory,
                            pMessage);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else if (LsaAdBatchIsUnprovisionedMode())
    {
        dwError = LsaAdBatchGatherUnprovisionedMode(
                        pItem,
                        hDirectory,
                        pMessage);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // User object has some AD-specific fields.
    if (LSA_AD_BATCH_OBJECT_TYPE_USER == pItem->ObjectType)
    {
        dwError = LsaAdBatchGatherRealUser(
                        pItem,
                        hDirectory,
                        pMessage);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_ERROR);
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherPseudoSid(
    IN OUT PSTR* ppszSid,
    IN OPTIONAL PCSTR pszSid,
    IN OPTIONAL DWORD dwKeywordValuesCount,
    IN OPTIONAL PSTR* ppszKeywordValues,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    PSTR pszResult = *ppszSid;

    if (!pszResult)
    {
        if (pszSid)
        {
            dwError = LsaAllocateString(pszSid, &pszResult);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (LsaAdBatchIsDefaultSchemaMode())
        {
            dwError = ADLdap_GetObjectSid(hDirectory, pMessage, &pszResult);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            PCSTR pszSidFromKeywords = NULL;

            LSA_ASSERT(ppszKeywordValues);

            pszSidFromKeywords = LsaAdBatchFindKeywordAttributeStatic(
                                        dwKeywordValuesCount,
                                        ppszKeywordValues,
                                        AD_LDAP_BACKLINK_PSEUDO_TAG);
            if (IsNullOrEmptyString(pszSidFromKeywords))
            {
                dwError = LSA_ERROR_INVALID_SID;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaAllocateString(pszSidFromKeywords, &pszResult);
            BAIL_ON_LSA_ERROR(dwError);
        }
        *ppszSid = pszResult;
    }
    else if (pszSid && strcasecmp(pszSid, pszResult))
    {
        LSA_ASSERT(FALSE);
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaAdBatchGatherPseudoObject(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN OPTIONAL PCSTR pszSid,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN BOOLEAN bIsSchemaMode,
    IN OPTIONAL DWORD dwKeywordValuesCount,
    IN OPTIONAL PSTR* ppszKeywordValues,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;

    LSA_ASSERT(LsaAdBatchIsDefaultSchemaMode() || ppszKeywordValues);

    SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO);

    dwError = LsaAdBatchGatherObjectType(pItem, ObjectType);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAdBatchGatherPseudoSid(
                    &pItem->pszSid,
                    pszSid,
                    dwKeywordValuesCount,
                    ppszKeywordValues,
                    hDirectory,
                    pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    if (bIsSchemaMode)
    {
        dwError = LsaAdBatchGatherSchemaMode(
                        pItem,
                        hDirectory,
                        pMessage);
        BAIL_ON_LSA_ERROR(dwError);
        if (LsaAdBatchIsDefaultSchemaMode() &&
            !IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_REAL))
        {
            dwError = LsaAdBatchGatherRealObject(
                            pItem,
                            ObjectType,
                            NULL,
                            hDirectory,
                            pMessage);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        dwError = LsaAdBatchGatherNonSchemaMode(
                        pItem,
                        dwKeywordValuesCount,
                        ppszKeywordValues);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_ERROR);
    goto cleanup;
}
